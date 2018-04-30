#include "Includes.h"

void server_error(const char *message, int server_socket) {
	shutdown(server_socket, SHUT_RD);

	printf("\nServer experienced an error: \n\t%s\nShutting down ...\n\n", 
			message);
	fflush(stdout);


	close(server_socket);
	exit(EXIT_FAILURE);
}