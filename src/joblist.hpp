/*
 * joblist.hpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */

#ifndef JOBLIST_HPP_
#define JOBLIST_HPP_

#include <string>
#include <mysql/mysql.h>
#include <vector>
using namespace std;

class job {
public:
	int job_id;
	int solver_binary_id;
	job(int _job_id,int _solver_binary_id) {
		job_id = _job_id;
		solver_binary_id = _solver_binary_id;
	}
};

class joblist {
private:
	vector<vector<job> > job_ids;
	int _size;
	//set<int> job_ids_blocked;
	int exp_id;
	time_t last_updated;
	time_t last_used;
	pthread_mutex_t jobs_mutex;
public:
	joblist(int);
	void update_jobs(MYSQL* con);
	int get_random_job(int solver_binary_id);
	int size();
	time_t get_last_used();
	int get_expid();
};

#endif /* JOBLIST_HPP_ */
