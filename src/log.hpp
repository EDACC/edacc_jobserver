/*
 * log.hpp
 *
 *  Created on: 30.07.2011
 *      Author: simon
 */

#ifndef LOG_HPP_
#define LOG_HPP_

#include <string>
using std::string;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

// logging level constants
const int LOG_DEBUG = 4;
const int LOG_INFO = 3;
const int LOG_IMPORTANT = 0;

extern FILE* logfile;
extern int log_verbosity;

extern int log_init(string filename, int verbosity);
void log_init(int verbosity);
extern void log_close();

extern void log_message(int verbosity, const char* format, ...);
extern void log_error(const char* location, const char* format, ...);

extern string get_log_tail();

#endif /* LOG_HPP_ */
