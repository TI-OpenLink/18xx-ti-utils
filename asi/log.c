/*
 * log_file.c
 *
  *  Created on: 31 October  2012
 *      Author: Arthur Tiv
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "log.h"
#include "public_types.h"

FILE *logfile;
//char file_name_prefix[]="/sdcard/asi_log";
int log_to_file=false;
char file_name[MAX_FILE_NAME]={0};
int log_printf_scr=false;

static int prepend_timestamp( char s[], size_t capacity )	{
	int errno = LOG_FILE_SUCCESS;
    const char fmt[] = "[%y-%m-%d %H:%M:%S]";
    /*
        If you use different specifiers, this calculation may need to
        change. 2 char specifier == 2 char replacement value presently
    */
    size_t n = sizeof ( fmt );
    size_t len = strlen ( s );
    /*
        The value of n includes a null character, which will be
        replaced by a single whitespace after adding the timestamp
    */
    if ( len + n < capacity ) {
        time_t init = time ( NULL );
        /*
            Avoid a temporary buffer by shifting
            and filling in the original string
        */
        //memmove ( s + n, s, len + 1 );
        strftime ( (s + len), n, fmt, localtime ( &init ) );
        /*
            Replace the null character that strftime
            appends to create a single string again
        */
        //s[n - 1] = ' ';
    }
    else
        errno = LOG_FILE_FAIL;
    return errno;
}

void error(const char *msg)
{
	log_write(" ERROR: %s\n", msg);
}
int log_write(const char *fmt,...)
{
    int ret=0;

	va_list args;
	va_start(args, fmt);
    if(log_to_file==true) {
    	logfile = fopen(file_name, "a");
        ret = vfprintf(logfile, fmt, args);
		fclose(logfile);
    }
    if(log_printf_scr==true) {
    	ret = vfprintf(stdout, fmt, args);
    }
	va_end(args);
    return ret;
}

int log_init(enum log_mode mode, char * log_file)
{
	if((mode==LOG_TO_FILE) && (log_file!=NULL)){
		/* copy the file name to be the prefix of the final file name */
		memset(file_name, 0x0, MAX_FILE_NAME);
		memcpy(file_name, log_file, strlen(log_file));

		if(strrchr(log_file,'/')!=(log_file+strlen(log_file)-1))
		{
			strcat(file_name, "/");
		}

		printf(" The final file name path is %s \n", file_name);


		if( prepend_timestamp(file_name, MAX_FILE_NAME) != LOG_FILE_SUCCESS) {
			printf("LOG MODULE: Time stamp create operation failed \n");
			return LOG_FILE_FAIL;
		}
		else {
			strcat(file_name, ".log");
			logfile = fopen(file_name, "w");
			if (logfile == NULL) {
				printf("LOG MODULE: Open log file %s failed \n", file_name);
				return LOG_FILE_FAIL;
			}
			fprintf(logfile, "LOG MODULE: time stamp : %s\n", file_name);
			log_to_file=true;
			fclose(logfile);
		}
	}
	else if(mode==LOG_PRNT_SCR) {
		log_printf_scr=true;
	}
	else {
		return LOG_FILE_FAIL;
	}

    return LOG_FILE_SUCCESS;
}
