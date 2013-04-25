/*
 * experimentlist.cpp
 *
 *  Created on: 31.07.2011
 *      Author: simon
 */
#include <pthread.h>
#include "experimentlist.hpp"
#include "log.hpp"
#include "database.hpp"

experimentlist::experimentlist(MYSQL* connection) {
	this->connection = connection;
	pthread_mutex_init(& (this->exp_mutex), NULL);
	this->last_used = time(NULL);
}

joblist* experimentlist::get_job_list(int exp_id) {
	pthread_mutex_lock(&exp_mutex);
	this->last_used = time(NULL);
	map<int, joblist>::iterator it = joblists.find(exp_id);
	if (it == joblists.end()) {
		joblists.insert( pair<int, joblist>(exp_id, joblist(exp_id)));
		it = joblists.find(exp_id);
	}
	pthread_mutex_unlock(&exp_mutex);
	return &((*it).second);
}

int experimentlist::get_possible_exp_ids_for_grid_queue_id(int grid_queue_id, vector<int> &exp_ids) {
	vector<int> tmp_exp_ids;
	pthread_mutex_lock(&exp_mutex);
	if (get_exp_ids_for_grid_queue_id(connection, grid_queue_id, tmp_exp_ids) == 0) {
		pthread_mutex_unlock(&exp_mutex);
		return -1;
	}
	pthread_mutex_unlock(&exp_mutex);

	for (int i = 0; i < tmp_exp_ids.size(); i++) {
		joblist* jl = get_job_list(tmp_exp_ids[i]);
		pthread_mutex_lock(&exp_mutex);
		jl->update_jobs(connection);
		pthread_mutex_unlock(&exp_mutex);
		if (jl->size() > 0) {
			exp_ids.push_back(tmp_exp_ids[i]);
		}
	}
	return 0;
}

int experimentlist::get_job_count(int exp_id) {
	joblist* jl = get_job_list(exp_id);
	return jl->size();
}

int experimentlist::get_random_job(int exp_id, int solver_binary_id) {
	int res = -1;
	joblist* jl = get_job_list(exp_id);
	pthread_mutex_lock(&exp_mutex);
	jl->update_jobs(connection);
	pthread_mutex_unlock(&exp_mutex);
	res = jl->get_random_job(solver_binary_id);
	return res;
}

void experimentlist::clear_not_used_experiments() {
	pthread_mutex_lock(&exp_mutex);
	map<int, joblist>::iterator it = joblists.begin();
	while (it != joblists.end()) {
		if (time(NULL) - it->second.get_last_used() >= 3600) {
			log_message(LOG_IMPORTANT, "Removing experiment id %d of database %s from cache.", it->second.get_expid(), connection->db);
			map<int, joblist>::iterator tmp = it++;
			joblists.erase(tmp);
		} else {
			it++;
		}
	}
	pthread_mutex_unlock(&exp_mutex);
}

time_t experimentlist::get_last_used() {
	return this->last_used;
}

bool experimentlist::empty() {
	return joblists.empty();
}

string experimentlist::getDB() {
	return string(connection->db);
}
