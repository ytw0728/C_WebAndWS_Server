#include "Includes.h"

// void server_error(const char *message, int server_socket) {
// 	shutdown(server_socket, SHUT_RD);

// 	printf("\nServer experienced an error: \n\t%s\nShutting down ...\n\n", 
// 			message);
// 	fflush(stdout);


// 	close(server_socket);
// 	exit(EXIT_FAILURE);
// }


void error_handler(const char* msg){
	puts(msg);
	puts("\n");
	
	exit(EXIT_FAILURE);
}



void brokenPipe(int sig){
	if( sig == SIGPIPE ){

	}
}

void inturrupt(int sig){
	if( sig == SIGINT){
		char input;
		do{
			printf("작업을 종료하시겠습니까 ? ( y / n ) >> ");
			input = getchar();
		}while( input != 'y' && input != 'n' && input != 'Y' && input != 'N');

		if( input == 'y' || input == 'Y' ) exit(0);
	}
}