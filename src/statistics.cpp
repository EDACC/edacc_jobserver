/*
 * statistics.cpp
 *
 *  Created on: 19.11.2011
 *      Author: simon
 */
#include <sys/time.h>

#include "statistics.hpp"

long long int gettime() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec *1000 + t.tv_usec / 1000.0)+ 0.5;
}

statistics::statistics() {
	pthread_mutex_init(&mutex, 0);
	jobs_served = 0;
	jobs_per_second = 0.f;
	persecond_timestamp = gettime();
	persecond_count = 0;
}

void statistics::update_jps() {
	// assuming mutex is locked.
	long long int tt = gettime();
	if (tt - persecond_timestamp >= 10000) {
		jobs_per_second = (float) persecond_count / ((float) (tt - persecond_timestamp) / 1000.);
		persecond_count = 0;
		persecond_timestamp = tt;
	}
}

void statistics::increment_jobs_served() {
	pthread_mutex_lock(&mutex);
	jobs_served++;
	persecond_count++;
	update_jps();
	pthread_mutex_unlock(&mutex);
}

int statistics::get_jobs_served() {
	int res;
	pthread_mutex_lock(&mutex);
	res = jobs_served;
	pthread_mutex_unlock(&mutex);
	return res;
}

float statistics::get_jobs_per_second() {
	float jps;
	pthread_mutex_lock(&mutex);
	update_jps();
	jps = jobs_per_second;
	pthread_mutex_unlock(&mutex);
	return jps;
}
