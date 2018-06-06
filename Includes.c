#include "Includes.h"

void error_handler(const char* msg){
	puts(msg);
	puts("\n");
	
	exit(EXIT_FAILURE);
}

void interrupt_handle(int sig){
	if( sig == SIGINT){
		char input;
		char c;

		do{
			printf("\r작업을 종료하시겠습니까 ? ( y / n ) >> ");
			while( input = getchar() ){
				if( input == '\n' || 0 < input && input <= 32 /* all interrupt key and space*/) continue;
				else{
					while( ( c = getchar() != '\n' ) && 0 >= c && c > 32 );
					break;
				}
			};
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
	#ifdef DEV
		switch( from ){
			case SYSTEM : printf("\rSYSTEM >> "); break;
			case WEBSERVER : printf("\rWEBSERVER >> "); break;
			case WSSERVER :	printf("\rWSSERVER >> "); break;
		}
	#endif

	switch (type) {
		case ERROR: 
			(void)sprintf(logbuffer,"[ ERROR ] %s ( point : %s )\n",msg,subMsg);
			fputs(logbuffer, stderr);
			// #ifdef DEV
				printf("%s\n", logbuffer);
			// #endif
			break;

		case LOG:
			#ifdef DEV
				(void)printf("[INFO: %s ] %s\n",subMsg, msg);
			#endif

		case FILELOG:
			(void)sprintf(logbuffer,"[ INFO: %s ] %s\n",subMsg, msg);
			break;
	}
	
	if( (fd = open("../server.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0 ){
		(void)write(fd,logbuffer,strlen(logbuffer)); 
		(void)write(fd,"\n",1);

		(void)close(fd);
	}
	else{
		printf("server.log FILE is not found.\n");
	}
}
