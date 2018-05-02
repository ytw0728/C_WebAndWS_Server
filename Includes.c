#include "Includes.h"

void error_handler(const char* msg){
	puts(msg);
	puts("\n");
	
	exit(EXIT_FAILURE);
}

void brokenPipe(int sig){
	if( sig == SIGPIPE ){

	}
}

void interrupt_handle(int sig){
	if( sig == SIGINT){
		char input;
		do{
			printf("\r작업을 종료하시겠습니까 ? ( y / n ) >> ");
			input = getchar();
			char c;
			while( ( c = getchar() != '\n') && c != ' ' );

		}while( input != 'y' && input != 'n' && input != 'Y' && input != 'N');

		if( input == 'y' || input == 'Y' ){
			printf("\nBye Bye~! :)\n\n");
			exit(0);			
		}
	}
}


void serverLog(int from, int type, char* msg, char* subMsg ){
	int fd ;
	char logbuffer[BUFSIZE*2];
	switch( from ){
		case SYSTEM : printf("\rSYSTEM >> "); break;
		case WEBSERVER : printf("\rWEBSERVER >> "); break;
		case WSSERVER :	printf("\rWSSERVER >> "); break;
	}

	switch (type) {
		case ERROR: 
			(void)sprintf(logbuffer,"[ ERROR ] %s ( pid : %d )\n",msg,getpid()); 
			printf("%s", logbuffer);
			break;

		case LOG:
			(void)printf("[INFO: %s ] %s\n",subMsg, msg);

		case FILELOG:
			(void)sprintf(logbuffer,"[ INFO: %s ] %s\n",subMsg, msg);
			break;
	}	
	
	if( (fd = open("server.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0 ){
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);

		(void)close(fd);
	}
}
