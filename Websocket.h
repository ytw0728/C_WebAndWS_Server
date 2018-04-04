
#ifndef _Websocket_H
#define	_Websocket_H

#include "Handshake.h"
#include "Communicate.h"
#include "Errors.h"


void sigint_handler(int sig);
void cleanup_client(void *args);
void *cmdline(void *arg);
void *handleClient(void *args);


#define PORT 4567
#define DEV 1

#endif