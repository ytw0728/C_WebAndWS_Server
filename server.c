#include "webServer.h"
#include "webSocketServer.h"
#include <sys/stat.h> 			/* stat */

pthread_attr_t pthread_attr;
pthread_t pthread_id;

int main(int argc, char **argv){
	struct sigaction act;
	memset( &act, '\0', sizeof(act));

	act.sa_handler = interrupt_handle;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if( sigaction(SIGINT, &act, 0) == SIG_ERR){
		serverLog(SYSTEM, ERROR, "sigaction() error with SIGINT\n", "" );
		exit(1);
	}

	memset( &act, '\0', sizeof(act));
	act.sa_handler = SIG_IGN;
	sigemptyset( &act.sa_mask);
	act.sa_flags = 0;
	if( sigaction(SIGPIPE, &act, 0) == SIG_ERR){
		serverLog(SYSTEM, ERROR, "sigaction() error with SIGPIPE\n", "" );
		exit(1);
	}
	
	int i;
	for(i=4;i<32;i++)
		(void)close(i);
	// (void)setpgrp();	 // 데몬 안해 

	system("clear");
	serverLog(SYSTEM,LOG, "Server start", "MESSAGE");

	pthread_attr_init(&pthread_attr);
	pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&pthread_attr, 524288);

	if( pthread_create(&pthread_id, &pthread_attr, webSocketServerHandle, 0) < 0 ){
		serverLog(SYSTEM,ERROR,"webSocketServer Thread Error","failed");
		exit(-1);
	}

	webServerHandle(argc, argv);	
}
