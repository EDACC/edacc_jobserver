/*
 * statistics.hpp
 *
 *  Created on: 19.11.2011
 *      Author: simon
 */

#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_
#include <pthread.h>

class statistics {
private:
	pthread_mutex_t mutex;
	int jobs_served;
	int persecond_count;
	long long int persecond_timestamp;
	float jobs_per_second;
	void update_jps();
public:
	statistics();
	void increment_jobs_served();
	int get_jobs_served();
	float get_jobs_per_second();
};

#endif /* STATISTICS_HPP_ */
