/*
 * databaselist.hpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */

#ifndef DATABASELIST_HPP_
#define DATABASELIST_HPP_
#include <string>
#include <map>
#include <vector>
#include "experimentlist.hpp"
#include "datastructures.hpp"
using namespace std;

class databaselist {
private:
	map<string, DatabaseInformation> db_infos;
	string hostname;
	int port;
	map<string, experimentlist> experimentlists;
	pthread_mutex_t db_mutex;
	float old_jps;
	experimentlist* get_experiment_list(string db);
public:
	databaselist(string hostname, map<string, DatabaseInformation> database_informations, int port);
	int get_random_job(string db, int exp_id, int solver_binary_id);
	void clear_not_used_databases();
	int get_possible_exp_ids_for_grid_queue_id(string db, int grid_queue_id, vector<int> &exp_ids);
	void getDbMD5(string database, unsigned char* buf, int r);
};

#endif /* DATABASELIST_HPP_ */
