/*
 * log.h
 *
  *  Created on: 31 October  2012
 *      Author: Arthur Tiv
 */

#ifndef LOG_FILE_H_
#define LOG_FILE_H_

enum log_mode {
	LOG_TO_FILE,
	LOG_PRNT_SCR,
	__LOG_LAST
};

#define LOG_FILE_SUCCESS	0
#define LOG_FILE_FAIL		-1

void error(const char *msg);
int log_init(enum log_mode mode, char * log_file);
int log_write(const char *fmt,...);
int log_init(enum log_mode mode, char * log_file);

#endif /* LOG_FILE_H_ */
