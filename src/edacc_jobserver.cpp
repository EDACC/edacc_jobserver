//============================================================================
// Name        : edacc_jobserver.cpp
// Author      : Simon Gerber
// Version     : 1.0
// Copyright   : Copyright (c) 2011 by Simon Gerber
// Description : Job Server for the EDACC System
//============================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include "databaselist.hpp"
#include "datastructures.hpp"
#include "database.hpp"
#include "log.hpp"
#include "statistics.hpp"

static int jobserver_protocol_version = 3;

void *client_thread(void* _fd);
void *cleaner_thread(void*);
int read_config(string config_filename, string& hostname, map<string, DatabaseInformation> &db_infos,
				 int& port, int& jobserver_port);
void process_stat_client(int fd);
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count;

statistics stats;

using namespace std;
databaselist* databases;

void print_usage(string filename) {
	cout << "Usage: " << filename << " [-c <configfile>] [-l <logdir>] [-v <verbosity>] [-u <user>] [-g <group>]" << endl;
    // ------------------------------------------------------------------------------------X <-- last char here! (80 chars)
    cout << "Parameters:" << endl;
    cout << "  --verbosity=<verbosity>: integer value between 0 and 4 (from lowest to " << endl <<
    		"                           highest verbosity)" << endl;
    cout <<	"  --logdir=<log dir>:      use the <log dir> to log files. The Job Server will" << endl <<
    		"                           create a new log file called jobserver.log on every" << endl <<
    		"                           start. If jobserver.log exists it will be moved to " << endl <<
    		"                           jobserver_<number>.log." << endl;
    cout << "  --config=<config file>:  the configuration file for the Job Server." << endl;
    cout << "  --user=<user name>:      change to user <user name> on startup. It is" << endl <<
    		"                           recommended that you change to an unprivileged user" << endl <<
    		"                           when starting the Job Server as the root user." << endl;
    cout << "  --group=<group name>:    change to group <group name> on startup." << endl;
    cout << "  --help:                  prints this help." << endl;
}

/**
 * Checks whether the file <code>fileName</code> exists.
 * @param fileName path of the file
 * @return 1 if the file exists, 0 if not.
 */
int file_exists(const string& fileName) {
    if (access(fileName.c_str(), F_OK) == 0) {
        return 1;
    }
    return 0;
}

/**
 * Renames thes file/directory pointed at by <code>old_path</code> into <code>new_path</code>
 *
 * @return 1 on success, 0 on errors
 */
int rename(const string& old_path, const string& new_path) {
    return rename(old_path.c_str(), new_path.c_str()) == 0;
}

uid_t name_to_uid(char const *name)
{
  if (!name)
    return -1;
  long const buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (buflen == -1)
    return -1;
  // requires c99
  char buf[buflen];
  struct passwd pwbuf, *pwbufp;
  if (0 != getpwnam_r(name, &pwbuf, buf, buflen, &pwbufp)
      || !pwbufp)
    return -1;
  return pwbufp->pw_uid;
}

gid_t name_to_gid(char const *name) {
	if (!name) {
		return -1;
	}
	struct group *grp = getgrnam(name);
	return grp->gr_gid;
}


int main(int argc, char* argv[]) {

	string logdir = "";
	string config = "./config";
	string user = "";
	string group = "";
	int verbosity = 0;
    // parse command line arguments
	static const struct option long_options[] = {
        { "logdir", required_argument, 0, 'l' },
        { "config", required_argument, 0, 'c' },
        { "verbosity", required_argument, 0, 'v'},
        { "user", required_argument, 0, 'u'},
        { "group", required_argument, 0, 'g'},
        { "help", no_argument, 0, 'h'},
        {0,0,0,0} };

	while (optind < argc) {
		int index = -1;
		struct option * opt = 0;
		int result = getopt_long(argc, argv, "l:c:hv:u:g:", long_options, &index);
		if (result == -1)
			break; /* end of list */
		switch (result) {
		case 'l':
			logdir = string(optarg);
			break;
		case 'c':
			config = string(optarg);
			break;
		case 'v':
			verbosity = atoi(optarg);
			break;
		case 'u':
			user = string(optarg);
			break;
		case 'g':
			group = string(optarg);
			break;
		case 0: /* all parameter that do not */
			/* appear in the optstring */
			opt = (struct option *) &(long_options[index]);
			cout << opt->name << " was specified" << endl;
			if (opt->has_arg == required_argument)
                cout << "Arg: <" << optarg << ">" << endl;
            cout << endl;
			break;
		default:
			cout << endl;
			print_usage(string(argv[0]));
			return 1;
		}
	}

	if (group != "") {
		gid_t gid = name_to_gid(group.c_str());
		if (gid == -1) {
			cout << "Could not find gid for group " << group << "." << endl;
			return 1;
		}
		if (setgid(gid) != 0) {
			cout << "Could not change group id to " << gid << "." << endl;
			return 1;
		}
	}

	if (user != "") {
		uid_t uid = name_to_uid(user.c_str());
		if (uid == -1) {
			cout << "Could not find uid for user " << user << "." << endl;
			return 1;
		}
		if (setuid(uid) != 0) {
			cout << "Could not change user id to " << uid << "." << endl;
			return 1;
		}
	}

	srand(time(NULL));
	map<string, DatabaseInformation> db_infos;
	string hostname;
	int port = 3306, jobserver_port = 3307, max_connections = SOMAXCONN;
	if (read_config(config, hostname, db_infos, port, jobserver_port) != 0) {
		cout << endl;
		print_usage(string(argv[0]));
		return 1;
	}

	if (logdir != "") {
		int i = 0;
		string log_filename = "jobserver.log";
		while (file_exists(logdir + "/" + log_filename)) {
			stringstream ss;
			ss << "jobserver_" << i << ".log";
			log_filename = ss.str();
			i++;
		}
		if (log_filename != "jobserver.log") {
			rename(logdir + "/" + "jobserver.log", logdir + "/" + log_filename);
		}
		log_init(logdir + "/jobserver.log", verbosity);
	} else {
		log_init(verbosity);
	}

	databases = new databaselist(hostname, db_infos, port);

	client_count = 0;
	srand (time(NULL));
	struct sockaddr_in serv_addr, cli_addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_message(LOG_IMPORTANT, "Could not create socket.");
		return 1;
	}
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(jobserver_port);
    if (bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    	log_message(LOG_IMPORTANT, "Could not bind socket.");
    	return 1;
    };
    listen(fd, max_connections);

    pthread_attr_t attributes;
    if (pthread_attr_init(&attributes) != 0) {
    	log_message(LOG_IMPORTANT, "Could not initialize thread attributes.");
    	return 1;
    }
    if (pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) != 0) {
    	log_message(LOG_IMPORTANT, "Could not set detach state for threads.");
    	return 1;
    }
    if (pthread_attr_setstacksize(&attributes, 512*1024) != 0) {
    	log_message(LOG_IMPORTANT, "WARNING: Could not set stack size for threads.");
    }

    log_message(LOG_IMPORTANT, "EDACC Jobserver started.");
    log_message(LOG_IMPORTANT, "Listening for clients.");

    pthread_t cthread;
    pthread_create(&cthread, &attributes, cleaner_thread, NULL);

    int client_fd;
	int clilen = sizeof(cli_addr);
	while ((client_fd = accept(fd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen)) >= 0) {
		string ip = inet_ntoa(((struct sockaddr_in) cli_addr).sin_addr);
		log_message(LOG_IMPORTANT, "Handling connection from %s.", ip.c_str());
		int* tmp = new int;
		*tmp = client_fd;
		pthread_t thread;
		pthread_create( &thread, &attributes, client_thread, tmp);
	}
	return 0;
}

void *client_thread(void* _fd) {
	int fd = *((int*) _fd);
	delete (int*) _fd;
	bool stat_client = false;
	/** client authentication */
	int version_nw = htonl(jobserver_protocol_version);
	if (write(fd, &version_nw, 4) != 4) {
		log_message(LOG_IMPORTANT, "could not send version number. Disconnecting client.");
		close(fd);
		return NULL;
	}

	int version_client;
	if (read(fd, &version_client, 4) != 4) {
		log_message(LOG_IMPORTANT, "could not read client protocol version. Disconnecting client.");
		close(fd);
		return NULL;
	}
	version_client = ntohl(version_client);
	if (version_client != jobserver_protocol_version) {
		log_message(LOG_IMPORTANT, "Client is talking protocol version %d, I'm understanding protocol version %d only. Disconnecting client.",version_client,jobserver_protocol_version);
		close(fd);
		return NULL;
	}
	char magic[13];
	if (read(fd, magic, 12) != 12) {
		log_message(LOG_IMPORTANT, "Could not read magic number. Disconnecting client.");
		close(fd);
		return NULL;
	}
	magic[12] = 0;
	if (strcmp(magic, "EDACC_CLIENT") != 0) {
		if (strcmp(magic, "EDACC_STATS ") != 0) {
			log_message(LOG_IMPORTANT, "Received wrong magic number from client. Disconnecting client.");
			close(fd);
			return NULL;
		}
		stat_client = true;
	}
	int hash_num = rand();
	int hash_num_nw = htonl(hash_num);
	if (write(fd, &hash_num_nw, 4) != 4) {
		log_message(LOG_IMPORTANT, "Could not send a random number for authentication. Disconnecting client.");
		close(fd);
		return NULL;
	}
	unsigned char client_md5[16];
	if (read(fd, client_md5, 16) != 16) {
		log_message(LOG_IMPORTANT, "Could not receive md5 checksum. Disconnecting client.");
		close(fd);
		return NULL;
	}
	char md5String[33];
	char* md5StringPtr;
	int i;
	for (i = 0, md5StringPtr = md5String; i < 16; ++i, md5StringPtr += 2)
		sprintf(md5StringPtr, "%02x", client_md5[i]);
	md5String[32] = '\0';
	log_message(LOG_IMPORTANT, "CLIENT MD5SUM: %s", md5String);

	int len;
	if (read(fd, &len, 4) != 4) {
		log_message(LOG_IMPORTANT, "could not read database length. Disconnecting client.");
		close(fd);
		return NULL;
	}
	len = ntohl(len);
	if (len > 1024) {
		log_message(LOG_IMPORTANT, "database length %d to big. Disconnecting client.", len);
		close(fd);
		return NULL;
	}
	char db_c[len+1];
	if (read(fd, db_c, len+1) != len+1) {
		log_message(LOG_IMPORTANT, "could not read database name. Disconnecting client.");
		close(fd);
		return NULL;
	}
	db_c[len] = '\0';
	string db(db_c);
	unsigned char host_md5[16];
	databases->getDbMD5(db, host_md5, hash_num);
	for (int i = 0; i < 16; i++) {
		if (host_md5[i] != client_md5[i]) {
			log_message(LOG_IMPORTANT, "md5 checksum mismatch at pos %d. Disconnecting client.", i);
			close(fd);
			return NULL;
		}
	}
	if (stat_client) {
		process_stat_client(fd);
		close(fd);
		return NULL;
	}

	int exp_id, grid_queue_id, solver_binary_id;
	short func_id;
	pthread_mutex_lock(&client_mutex);
	client_count++;
	log_message(LOG_IMPORTANT, "Client connected. Counting %d clients.", client_count);
	pthread_mutex_unlock(&client_mutex);
	while (read(fd, &func_id, 2) == 2) {
		func_id = ntohs(func_id);
		if (func_id == 0) {
			if (read(fd, &grid_queue_id, 4) == 4) {
				grid_queue_id = ntohl(grid_queue_id);
				vector<int> exp_ids;
				int size;
				if (databases->get_possible_exp_ids_for_grid_queue_id(db, grid_queue_id, exp_ids) != 0) {
					size = htonl(0);
					if (write(fd, &size, 4) != 4) {
						break;
					}
				} else {
					size = htonl(exp_ids.size());
					if (write(fd, &size, 4) != 4) {
						break;
					}
					bool error = false;
					for (unsigned int i = 0; i < exp_ids.size(); i++) {
						int tmp = htonl(exp_ids[i]);
						if (write(fd, &tmp, 4) != 4) {
							error = true;
							break;
						}
					}
					if (error) {
						break;
					}
				}
			} else {
				break;
			}
		} else if (func_id == 1) {
			if (read(fd, &solver_binary_id, 4) == 4 && read(fd, &exp_id, 4) == 4) {
				exp_id = ntohl(exp_id);
				solver_binary_id = ntohl(solver_binary_id);
				int job_id = htonl(databases->get_random_job(db, exp_id, solver_binary_id));

				if (write(fd, &job_id, 4) != 4) {
					break;
				}
			} else {
				log_error(AT, "Error.");
				break;
			}
		}
	}
	pthread_mutex_lock(&client_mutex);
	client_count--;
	log_message(LOG_IMPORTANT, "Client disconnected. Counting %d clients.", client_count);
	pthread_mutex_unlock(&client_mutex);
	close(fd);
	return NULL;
}

float float_swap(float value){
    union v {
        float       f;
        unsigned int    i;
    };
    union v val;
    val.f = value;
    val.i = htonl(val.i);
    return val.f;
}

void process_stat_client(int fd) {
	short func_id;
	log_message(LOG_IMPORTANT, "Stats client connected.");
	while (read(fd, &func_id, 2) == 2) {
		func_id = ntohs(func_id);
		if (func_id == 0) {
			int job_count = htonl(stats.get_jobs_served());
			if (write(fd, &job_count, 4) != 4) {
				log_message(LOG_IMPORTANT, "Could not send job count.");
				break;
			}
		} else if (func_id == 1) {
			float jps = float_swap(stats.get_jobs_per_second());
			if (write(fd, &jps, 4) != 4) {
				log_message(LOG_IMPORTANT, "Could not send jobs/sec.");
				break;
			}
		}
	}
	log_message(LOG_IMPORTANT, "Disconnecting stats client.");
}

void *cleaner_thread(void *) {
	while (true) {
		sleep(600);
		databases->clear_not_used_databases();
	}
	return NULL;
}

/**
 * Strips off leading and trailing whitespace
 * from a string.
 * @param str string that should be trimmed
 * @return the trimmed string
 */
string trim_whitespace(const string& str) {
    size_t beg = 0;
    while (beg < str.length() && str[beg] == ' ') beg++;
    size_t end = str.length() - 1;
    while (end > 0 && str[end] == ' ') end--;
    return str.substr(beg, end - beg + 1);
}

/**
 * Reads the configuration file './config' that consists of lines of
 * key-value pairs separated by an '=' character into the references
 * that are passed in.
 *
 * @param hostname The hostname (DNS/IP) of the database host.
 * @param username DB username
 * @param password DB password
 * @param database DB name
 * @param port DB port
 * @param jobserver_port jobserver port
 */
int read_config(string config_filename, string& hostname, map<string, DatabaseInformation> &db_infos,
				 int& port, int& jobserver_port) {
	ifstream configfile(config_filename.c_str());
	if (!configfile.is_open()) {
		log_message(0, "Couldn't open configuration file. Make sure configuration file '%s' "
				"exists.", config_filename.c_str());
		return 1;
	}
    port = 3306; // hardcoded for now
	string line;
	bool found_db = 0, found_user = 0, found_pwd = 0;
	string database = "";
	string username = "";
	string password = "";
	while (getline(configfile, line)) {
		istringstream iss(line);
		string id, val, eq;
		iss >> id >> eq;
        size_t eq_pos = line.find_first_of("=", 0);
        val = trim_whitespace(line.substr(eq_pos + 1, line.length()));
		if (id == "host") {
			hostname = val;
		}
		else if (id == "database") {
			found_db = 1;
			database = val;
		}
		else if (id == "username") {
			found_user = 1;
			username = val;
		}
		else if (id == "password") {
			found_pwd = 1;
			password = val;
		}
        else if (id == "port") {
            port = atoi(val.c_str());
        }
        else if (id == "jobserver_port") {
            jobserver_port = atoi(val.c_str());
        }
		if (found_db && found_user && found_pwd) {
			log_message(LOG_IMPORTANT, "Adding database %s with username %s.", database.c_str(), username.c_str());
			db_infos.insert(pair<string, DatabaseInformation>(database, DatabaseInformation(database, username, password)));
			found_db = false;
			found_user = false;
			found_pwd = false;
		}
	}
	configfile.close();
	return 0;
}

