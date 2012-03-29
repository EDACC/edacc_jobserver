/*
 * database.hpp
 *
 *  Created on: 30.07.2011
 *      Author: simon
 */

#ifndef DATABASE_HPP_
#define DATABASE_HPP_

#include <string>
#include <set>
#include <vector>
using namespace std;

MYSQL* database_connect(const string& hostname, const string& database,
							const string& username, const string& password,
							unsigned int port);

const char QUERY_SELECT_JOB_IDS[] =
		"SELECT idJob, priority FROM ExperimentResults WHERE status = -1 AND priority >= 0 AND Experiment_idExperiment = %d ORDER BY priority DESC LIMIT 10000";
int get_job_ids(MYSQL* con, int exp_id, vector<set<int> > &job_ids);

const char QUERY_SELECT_EXPERIMENT_IDS[] =
		"SELECT Experiment_idExperiment FROM Experiment_has_gridQueue JOIN Experiment ON Experiment_idExperiment = idExperiment WHERE gridQueue_idgridQueue = %d AND active = TRUE";
int get_exp_ids_for_grid_queue_id(MYSQL* con, int grid_queue_id, vector<int> &exp_ids);

#endif /* DATABASE_HPP_ */
