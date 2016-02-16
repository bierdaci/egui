#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#include "logmsg.h"

int log_level = 1;
int time_stamp_log = 0;

FILE *log_fp = NULL;
LogType log_type = LOGTYPE_LOCAL;
static pthread_mutex_t mutexlock = PTHREAD_MUTEX_INITIALIZER;

static void msg_to_syslog(unsigned short level, char *msg)
{
	int sl;

	switch (level) {
	case 0:  sl = LOG_ERR;    break;
	case 1:  sl = LOG_NOTICE; break;
	case 2:  sl = LOG_INFO;   break;
	default: sl = LOG_DEBUG;  break;
	}

	syslog(sl, msg);
}

static void time_stamp(char *buf)
{
	time_t now;
	struct tm *tm;

	pthread_mutex_lock(&mutexlock);

	time(&now);
	tm = localtime(&now);

	strftime(buf, TIMESTAMP_SIZE, "%Y-%m-%d-%H:%M:%S", tm);

	pthread_mutex_unlock(&mutexlock);
}

void log_msg(const char *file, int line, unsigned short level, int err, char *fmt, ...)
{
	FILE *fp = log_fp;
	va_list args;
	char *time = NULL;
	char tbuf[TIMESTAMP_SIZE];
	char mbuf[MAX_LINE_SIZE];

	if (level > log_level || log_type == LOGTYPE_NULL)
		return;

	va_start(args, fmt);

	if (fp == NULL)
		fp = stderr;

	if (time_stamp_log) {
		time_stamp(tbuf);
		time = tbuf;
	}

	snprintf(mbuf, sizeof(mbuf), "%s[%d](%d/%lu): %s%s%.*s%s",
			file, line,
			getpid(), pthread_self() / 100000,
			(time ? time : ""),
			(time ? ": " : ""),
			level, "          ",
			(level ? "" : "ERROR: "));

	vsnprintf(mbuf + strlen(mbuf), sizeof(mbuf) - strlen(mbuf), fmt, args);

	va_end(args);

	if (err) {
		snprintf(mbuf + strlen(mbuf),
				sizeof(mbuf) - strlen(mbuf),
				": (%s)", strerror(err));
	}

	pthread_mutex_lock(&mutexlock);

	switch (log_type) {
	case LOGTYPE_LOCAL:
		fprintf(fp, "%s\n", mbuf);
		fflush(fp);
		break;

	case LOGTYPE_SYSLOG:
		msg_to_syslog(level, mbuf);
		break;

	default:
		break;
	}

	pthread_mutex_unlock(&mutexlock);
}

void open_log_file(char *logfile)
{
	if (log_fp != NULL)
		fclose(log_fp);

	if (strcmp(logfile, "NULL") == 0) {
		log_type = LOGTYPE_NULL;
		log_fp = NULL;
	}
	else if (strcmp(logfile, "SYSLOG") == 0) {
		log_type = LOGTYPE_SYSLOG;
		log_fp = NULL;
	}
	else {
		log_type = LOGTYPE_LOCAL;
		if ((log_fp = fopen(logfile, "a")) == NULL)
			LOG_MSG(0, errno, "can't open log file '%s'", logfile);
	}
}

