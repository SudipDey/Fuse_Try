#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
void debugf(const char *s, ...);

int suppress_debug = (0!=0);
int checked = 0;

#ifndef LOGFILE
#define LOGFILE "/tmp/fusexmp.log"
#endif

#define MAX_STRING 200
char debugline[MAX_STRING];
void debugf(const char *s, ...)
{
	/* OK, I'm going to nail this damn bug for once and all... */
	/* Time to size the string by vfprint'ing it to /dev/null... */
	FILE *errfile;
	static char buff[MAX_STRING * 2];
	va_list ap;

	if (checked == 0) {
		checked = 1;
		errfile = fopen(LOGFILE, "r");
		if (errfile == NULL) {
			/* Only want to log if file exists already... */
//			suppress_debug = (0 == 0);
		} else {
			fclose(errfile);
		}
	}

	va_start(ap, s);

	{
		FILE *nullfile;
		int string_length;

		nullfile = fopen("/dev/null", "w");
		if (nullfile == NULL) {
			errfile = fopen(LOGFILE, "a");
			if (errfile != NULL) {
				fprintf(errfile,
					"Major error - cannot open /dev/null\n");
				fflush(errfile);
				fclose(errfile);
			}
			exit(1);
		}
		string_length = vfprintf(nullfile, s, ap);
		fclose(nullfile);
		if (string_length < MAX_STRING - 2) {
			vsprintf(buff, s, ap);
		} else {
			sprintf(buff, "[%d char debugf string excised]\n",
				string_length);
		}
	}
	va_end(ap);
	strcpy(debugline, buff);

	/* Suppress logging to file, but still allow in window if present */

	if (suppress_debug) {
		return;
	}

	fprintf(stderr, "%s", buff);
	errfile = fopen(LOGFILE, "a");
	if (errfile != NULL) {
		fprintf(errfile, "%s", buff);
		fflush(errfile);
		fclose(errfile);
	}

}
