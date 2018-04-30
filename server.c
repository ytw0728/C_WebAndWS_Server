#include "webServer.h"
#include "webSocketServer.h"
#include <sys/stat.h> 			/* stat */


pthread_attr_t pthread_attr;
pthread_t pthread_id;

int main(int argc, char **argv){

	pthread_attr_init(&pthread_attr);
	pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&pthread_attr, 524288);

	if( pthread_create(&pthread_id, &pthread_attr, webSocketServerHandle, 0) < 0 ){
		webServerLog(ERROR,"webSocketServer Thread Error","failed",0);
		return -1;
	}

	webServerHandle(argc, argv);
}




