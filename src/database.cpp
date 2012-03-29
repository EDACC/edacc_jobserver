/*
 * database.cpp
 *
 *  Created on: 30.07.2011
 *      Author: simon
 */
#include <string>
#include <vector>
#include <mysql/mysql.h>
#include <mysql/my_global.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>

using namespace std;

#include "database.hpp"
#include "log.hpp"

static time_t WAIT_BETWEEN_RECONNECTS = 5;

/**
 * Establishes a database connection with the specified connection details.
 *
 * @param hostname DB host IP/DNS
 * @param database DB name
 * @param username DB username
 * @param password DB password
 * @param port DB port
 * @return 0 on errors, 1 on success.
 */
MYSQL* database_connect(const string& hostname, const string& database,
							const string& username, const string& password,
							unsigned int port) {
    MYSQL* connection = mysql_init(NULL);
    if (connection == NULL) {
        return NULL;
    }

    if (mysql_real_connect(connection, hostname.c_str(), username.c_str(),
                           password.c_str(), database.c_str(), port,
                           NULL, 0) == NULL) {
        log_error(AT, "Database connection attempt failed: %s", mysql_error(connection));
        return NULL;
    }

    // enable auto-reconnect on SERVER_LOST and SERVER_GONE errors
    // e.g. due to connection time outs. Failed queries have to be
    // re-issued in any case.
    my_bool mysql_opt_reconnect = 1;
    mysql_options(connection, MYSQL_OPT_RECONNECT, &mysql_opt_reconnect);

    log_message(LOG_IMPORTANT, "Established database connection to %s:xxxx@%s:%u/%s",
					username.c_str(), hostname.c_str(),
					port, database.c_str());
    return connection;
}

int database_query_select(string query, MYSQL_RES*& res, MYSQL*& con) {
    int status = mysql_query(con, query.c_str());
    if (status != 0) {
        if (mysql_errno(con) == CR_SERVER_GONE_ERROR || mysql_errno(con) == CR_SERVER_LOST) {
            // server connection lost, try to re-issue query once
            for (int i = 0; i < 10; i++) {
                sleep(WAIT_BETWEEN_RECONNECTS);
                if (mysql_query(con, query.c_str()) != 0) {
                    // still doesn't work
                    log_error(AT, "Lost connection to server and couldn't \
                            reconnect when executing query: %s - %s", query.c_str(), mysql_error(con));
                } else {
                    // successfully re-issued query
                    log_message(LOG_INFO, "Lost connection but successfully re-established \
                                when executing query: %s", query.c_str());
                    return 1;
                }

            }
            return 0;
        }

        log_error(AT, "Query failed: %s, return code (status): %d errno: %d", query.c_str(), status, mysql_errno(con));
        return 0;
    }

    if ((res = mysql_store_result(con)) == NULL) {
        log_error(AT, "Couldn't fetch query result of query: %s - %s",
                    query.c_str(), mysql_error(con));
        return 0;
    }

    return 1;
}

int get_job_ids(MYSQL* con, int exp_id, vector<set<int> > &job_ids) {
    char* query = new char[4096];
    snprintf(query, 4096, QUERY_SELECT_JOB_IDS, exp_id);
    MYSQL_RES* result = 0;
    if (database_query_select(query, result, con) == 0) {
        log_error(AT, "Error querying for job ids: %s", mysql_error(con));
        delete[] query;
        mysql_free_result(result);
        return 0;
    }
    delete[] query;
    MYSQL_ROW row;
    int priority = -1;

    set<int> jobs;
    while ((row = mysql_fetch_row(result))) {
    	if (atoi(row[1]) != priority) {
    		if (jobs.size() > 0) {
    			job_ids.push_back(jobs);
    		}
    		jobs.clear();
    		priority = atoi(row[1]);
    	}
    	jobs.insert(atoi(row[0]));
    }
    if (jobs.size() > 0) {
    	job_ids.push_back(jobs);
    }
    mysql_free_result(result);
    return 1;
}

int get_exp_ids_for_grid_queue_id(MYSQL* con, int grid_queue_id, vector<int> &exp_ids) {
    char* query = new char[4096];
    snprintf(query, 4096, QUERY_SELECT_EXPERIMENT_IDS, grid_queue_id);
    MYSQL_RES* result = 0;
    if (database_query_select(query, result, con) == 0) {
        log_error(AT, "Error querying for experiment ids: %s", mysql_error(con));
        delete[] query;
        mysql_free_result(result);
        return 0;
    }
    delete[] query;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
    	exp_ids.push_back(atoi(row[0]));
    }
    mysql_free_result(result);
    return 1;
}
