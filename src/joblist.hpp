/*
 * joblist.hpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */

#ifndef JOBLIST_HPP_
#define JOBLIST_HPP_

#include <set>
#include <string>
#include <mysql/mysql.h>
#include <vector>
using namespace std;

class joblist {
private:
	vector<set<int> > job_ids;
	int _size;
	//set<int> job_ids_blocked;
	int exp_id;
	time_t last_updated;
	time_t last_used;
	pthread_mutex_t jobs_mutex;
public:
	joblist(int);
	void update_jobs(MYSQL* con);
	int get_random_job();
	int size();
	time_t get_last_used();
	int get_expid();
};

#endif /* JOBLIST_HPP_ */
