#ifndef __LOG_MSG_H
#define __LOG_MSG_H

typedef enum {
	LOGTYPE_NULL,
	LOGTYPE_SYSLOG,
	LOGTYPE_LOCAL
} LogType;

#define TIMESTAMP_SIZE				20
#define MAX_LINE_SIZE				1024

#define LOG_MSG(level, err, ...) log_msg(__FILE__, __LINE__, level, err, __VA_ARGS__)

extern int log_level;
extern int timestamp_log;
extern FILE *log_fp;
extern LogType log_type;

void open_log_file(char *logfile);
void log_msg(const char *file, int line, unsigned short level, int err, char *fmt, ...);

#endif
