#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <ostream>
#include <sstream>

extern bool log_verbose;

#define LOG_TO(stream,level,message) { std::ostringstream log_oss_; log_oss_ << (level) << " " << __FILE__ << ":" << __LINE__ << " - " << message << '\n'; std::fputs(log_oss_.str().c_str(), (stream)); }

#define LOG_ERROR(message) LOG_TO(stderr, "ERROR  ", message)
#define LOG_WARNING(message) LOG_TO(stderr, "WARNING", message)
#define LOG_INFO(message) LOG_TO(stderr, "INFO   ", message)
#define LOG_VERBOSE(message) if(log_verbose) { LOG_TO(stdout, "VERBOSE", message) }

#ifdef DEBUG
#define LOG_DEBUG(message) if(log_verbose) { LOG_TO(stdout, "DEBUG  ", message) }
#else
#define LOG_DEBUG(message)
#endif

#endif
