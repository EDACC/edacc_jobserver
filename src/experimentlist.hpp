/*
 * experimentlist.hpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */

#ifndef EXPERIMENTLIST_HPP_
#define EXPERIMENTLIST_HPP_

#include <map>
#include <vector>
#include <mysql/mysql.h>
#include "joblist.hpp"

using namespace std;

class experimentlist {
private:
	MYSQL* connection;
	map<int, joblist> joblists;
	time_t last_used;
	pthread_mutex_t exp_mutex;
	joblist* get_job_list(int exp_id);
public:
	experimentlist(MYSQL* connection);
	int get_random_job(int exp_id,int solver_binary_id);
	time_t get_last_used();
	void clear_not_used_experiments();
	bool empty();
	string getDB();
	int get_job_count(int exp_id);
	int get_possible_exp_ids_for_grid_queue_id(int grid_queue_id, vector<int> &exp_ids);
};

#endif /* EXPERIMENTLIST_HPP_ */
