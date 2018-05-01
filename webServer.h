#ifndef _WEBSERVER_H
#define _WEBSERVER_H

#include "Includes.h"

#define BUFSIZE 8096
#define ERROR 		42
#define SORRY 		43
#define LOG 	44
#define FILELOG   	45


typedef struct web_client_info{
	int socket_id;
	int client_idx;
	char* client_ip;
	pthread_t thread_id;
} web_client_info;

void webServerLog(int type, char *s1, char *s2, int num);
web_client_info* web_client_new(int client_sock, char* addr);
void *web(void *args);
int webServerHandle(int argc, char** argv);

#endif