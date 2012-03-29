/*
 * datastructures.hpp
 *
 *  Created on: 30.07.2011
 *      Author: simon
 */

#ifndef DATASTRUCTURES_HPP_
#define DATASTRUCTURES_HPP_

#include <string>
using namespace std;
class DatabaseInformation {
public:
	string database;
	string username;
	string password;

	DatabaseInformation(string db, string user, string pwd) {
		database = db;
		username = user;
		password = pwd;
	};
};

#endif /* DATASTRUCTURES_HPP_ */
