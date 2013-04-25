/*
 * databaselist.cpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */
#include <pthread.h>
#include <sstream>
#include <map>

#include "datastructures.hpp"
#include "databaselist.hpp"
#include "database.hpp"
#include "log.hpp"
#include "md5sum.h"
#include "statistics.hpp"

using namespace std;

extern statistics stats;

databaselist::databaselist(string hostname, map<string, DatabaseInformation> database_informations, int port) {
	this->hostname = hostname;
	this->db_infos = database_informations;
	this->port = port;
	old_jps = 0.f;
	pthread_mutex_init(&db_mutex, NULL);
}

experimentlist* databaselist::get_experiment_list(string db) {
	pthread_mutex_lock(&db_mutex);
	map<string, experimentlist>::iterator it = experimentlists.find(db);

	if (it == experimentlists.end()) {
		map<string, DatabaseInformation>::iterator db_it = db_infos.find(db);
		if (db_it == db_infos.end()) {
			log_message(LOG_IMPORTANT, "No informations found for database %s", db.c_str());
			pthread_mutex_unlock(&db_mutex);
			return NULL;
		}
		MYSQL* con;
	    if ((con = database_connect(hostname.c_str(), db.c_str(), db_it->second.username.c_str(), db_it->second.password.c_str(), port)) == NULL) {
	    	log_message(LOG_IMPORTANT, "Could not connect to database.");
	    	pthread_mutex_unlock(&db_mutex);
	    	return NULL;
	    }
		experimentlists.insert( pair<string, experimentlist>(db, experimentlist(con)));
		it = experimentlists.find(db);
	}
	pthread_mutex_unlock(&db_mutex);
	return &((*it).second);
}

int databaselist::get_possible_exp_ids_for_grid_queue_id(string db, int grid_queue_id, vector<int> &exp_ids) {
	int res = 1;
	experimentlist* el = get_experiment_list(db);
	if (el == NULL) {
		return res;
	}
	return el->get_possible_exp_ids_for_grid_queue_id(grid_queue_id, exp_ids);
}

int databaselist::get_random_job(string db, int exp_id, int solver_binary_id) {
	int res = -1;
	experimentlist* el = get_experiment_list(db);
	if (el == NULL) {
		return -1;
	}
	res = el->get_random_job(exp_id, solver_binary_id);

	stats.increment_jobs_served();
	pthread_mutex_lock(&db_mutex);
	float jps = stats.get_jobs_per_second();
	if (jps > 0.f && jps != old_jps) {
		log_message(LOG_IMPORTANT, "Serving %.2f jobs / sec", jps);
		old_jps = jps;
	}
	pthread_mutex_unlock(&db_mutex);
	return res;
}

void databaselist::clear_not_used_databases() {
	pthread_mutex_lock(&db_mutex);
	map<string, experimentlist>::iterator it = experimentlists.begin();
	while (it != experimentlists.end()) {
		bool erase = false;
		if (time(NULL) - it->second.get_last_used() >= 3600) {
			erase = true;
		} else {
			it->second.clear_not_used_experiments();
			if (it->second.empty()) {
				erase = true;
			}
		}
		if (erase) {
			log_message(LOG_IMPORTANT, "Removing database %s from cache.", it->second.getDB().c_str());
			map<string, experimentlist>::iterator tmp = it++;
			experimentlists.erase(tmp);
		} else {
			it++;
		}
	}
	pthread_mutex_unlock(&db_mutex);
}

void databaselist::getDbMD5(string database, unsigned char* buf, int r) {
	stringstream ss;
	ss << r; // << username << password;
	map<string, DatabaseInformation>::iterator db_it = db_infos.find(database);
	if (db_it == db_infos.end()) {
		log_message(LOG_IMPORTANT, "No informations found for database %s", database.c_str());
	} else {
		ss << db_it->second.username << db_it->second.password;
	}
    md5_buffer(ss.str().c_str(), ss.str().length(), buf);
	char md5String[33];
	char* md5StringPtr;
	int i;
	for (i = 0, md5StringPtr = md5String; i < 16; ++i, md5StringPtr += 2)
		sprintf(md5StringPtr, "%02x", buf[i]);
	md5String[32] = '\0';
	log_message(LOG_IMPORTANT, "MD5SUM: %s", md5String);
}
