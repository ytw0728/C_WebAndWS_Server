#ifndef _INCLUDES_H
#define _INCLUDES_H

#include <pthread.h> 			/* pthread_create, pthread_t, pthread_attr_t 
								   pthread_mutex_init */
#include <ctype.h> 				/* isdigit, isblank */
#include <stdio.h> 				/* printf, fflush(stdout), sprintf */
#include <stdlib.h> 			/* atoi, malloc, free, realloc */
#include <stdint.h> 			/* uint32_t */
#include <errno.h> 				/* errno */
#include <string.h> 			/* strerror, memset, strncpy, memcpy */
#include <strings.h> 			/* strncasecmp */
#include <unistd.h> 			/* close */
#include <math.h> 				/* log10 */
#include <signal.h> 			/* sigaction */

#include <sys/types.h> 			/* socket, setsockopt, accept, send, recv */
#include <sys/socket.h> 		/* socket, setsockopt, inet_ntoa, accept */
#include <netinet/in.h> 		/* sockaddr_in, inet_ntoa */
#include <arpa/inet.h> 			/* htonl, htons, inet_ntoa */

#include <fcntl.h>

#define BUFSIZE 8096

// for log
#define SYSTEM 		10
#define WEBSERVER 	11
#define	WSSERVER	12
#define ERROR 	40
#define LOG 	41
#define FILELOG 42


void error_handler(const char* msg);

// signals
void interrupt_handle(int sig);

// log
void serverLog(int from, int type, char* msg, char* subMsg );
#endif