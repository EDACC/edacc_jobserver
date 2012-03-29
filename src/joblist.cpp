/*
 * joblist.cpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */
#include <stdlib.h>
#include <pthread.h>

#include "joblist.hpp"
#include "database.hpp"
#include "log.hpp"


joblist::joblist(int expid) {
	this->exp_id = expid;
	this->last_updated = 0;
	this->last_used = time(NULL);
	_size = 0;
	pthread_mutex_init(& (this->jobs_mutex), NULL);
}

void joblist::update_jobs(MYSQL* con) {
	pthread_mutex_lock(&jobs_mutex);
	if (time(NULL) - last_updated >= 10 || (job_ids.size() == 0 && time(NULL) - last_updated >= 5)) {
		job_ids.clear();
		_size = 0;
		if (!get_job_ids(con, exp_id, job_ids)) {
			job_ids.clear();
		} else {
			vector<set<int> >::iterator it;
			for (it = job_ids.begin(); it != job_ids.end(); it++) {
				_size+= (*it).size();
			}
		//  set<int>::iterator it;
		//	for (it = job_ids_blocked.begin(); it != job_ids_blocked.end(); it++) {
		//		int job_id = *it;
		//		job_ids.erase(job_id);
		//	}
	//		job_ids_blocked.clear();
		}
		last_updated = time(NULL);
		log_message(LOG_DEBUG, "Updated job ids for experiment id %d on %s: %d jobs", exp_id, con->db, _size);
	}
	pthread_mutex_unlock(&jobs_mutex);
}

int joblist::size() {
	int res = 0;
	pthread_mutex_lock(&jobs_mutex);
	this->last_used = time(NULL);
	res = _size;
	pthread_mutex_unlock(&jobs_mutex);
	return res;
}

int joblist::get_random_job() {
	int res = 0;
	pthread_mutex_lock(&jobs_mutex);
	this->last_used = time(NULL);
	if (job_ids.empty()) {
		res = -1;
	} else {
		set<int>::iterator it = job_ids[0].begin();
		//advance(it, rand() % job_ids[0].size());
		res = *it;
		//job_ids_blocked.insert(res);
		job_ids[0].erase(it);
		_size--;
		if (job_ids[0].size() == 0) {
			job_ids.erase(job_ids.begin());
		}
	}
	pthread_mutex_unlock(&jobs_mutex);
	return res;
}

time_t joblist::get_last_used() {
	return this->last_used;
}

int joblist::get_expid() {
	return exp_id;
}
