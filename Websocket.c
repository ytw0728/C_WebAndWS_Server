/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
#include "Websocket.h"

ws_list *l;
int port;

/**
 * Handler to call when CTRL+C is typed. This function shuts down the server
 * in a safe way.
 */
void sigint_handler(int sig) {
	if (sig == SIGINT || sig == SIGSEGV) {
		if (l != NULL) {
			list_free(l);
			l = NULL;
		}
		(void) signal(sig, SIG_DFL);
		exit(0);
	} else if (sig == SIGPIPE) {
		(void) signal(sig, SIG_IGN);
	}
}

/**
 * Shuts down a client in a safe way. This is only used for Hybi-00.
 */
void cleanup_client(void *args) {
	ws_client *n = args;
	if (n != NULL) {
		printf("Shutting client down..\n\n> ");
		fflush(stdout);
		list_remove(l, n);
	}
}

/**
 * This function listens for input from STDIN and tries to match it to a 
 * pattern that will trigger different actions.
 */
void *cmdline(void *arg) {
	pthread_detach(pthread_self());
	(void) arg; char buffer[1024];
	
	while (1) {
		memset(buffer, '\0', 1024);
		printf("> ");
		fflush(stdout);
		fgets(buffer, 1024, stdin);
		
		if (strncasecmp(buffer, "users", 5) == 0 || 
				strncasecmp(buffer, "online", 6) == 0 ||
				strncasecmp(buffer, "clients", 7) == 0) {
			list_print(l);
			continue;
		} else if (strncasecmp(buffer, "exit", 4) == 0 || 
				strncasecmp(buffer, "quit", 4) == 0) {
			raise(SIGINT);
			break;
		} else if ( strncasecmp(buffer, "help", 4) == 0 ) {
			printf("------------------------ HELP ------------------------\n");
			printf("|   To display information about the online users,   |\n");
			printf("|   type: 'users', 'online', or 'clients'.           |\n");
			printf("|                                                    |\n");
			printf("|   To send a message to a specific user from the    |\n");
			printf("|   server type: 'send <IP> <SOCKET> <MESSAGE>' or   |\n");
			printf("|   'write <IP> <SOCKET> <MESSAGE>'.                 |\n");
			printf("|                                                    |\n");
			printf("|   To send a message to all users from the server   |\n");
			printf("|   type: 'sendall <MESSAGE>' or 'writeall           |\n");
			printf("|   <MESSAGE>'.                                      |\n");
			printf("|                                                    |\n");
 			printf("|   To kick a user from the server and close the     |\n");
			printf("|   socket connection type: 'kick <IP> <SOCKET>'     |\n");
			printf("|   or 'close <IP> <SOCKET>'.                        |\n");
			printf("|                                                    |\n"); 
 			printf("|   To kick all users from the server and close      |\n");
			printf("|   all socket connections type: 'kickall' or        |\n");
			printf("|   'closeall'.                                      |\n");
			printf("|                                                    |\n");
			printf("|   To quit the server type: 'quit' or 'exit'.       |\n");
			printf("------------------------------------------------------\n");
			fflush(stdout);
			continue;
		} else if ( strncasecmp(buffer, "kickall", 7) == 0 ||
				strncasecmp(buffer, "closeall", 8) == 0) {
			list_remove_all(l);	
		} else if ( strncasecmp(buffer, "kick", 4) == 0 ||
				strncasecmp(buffer, "close", 5) == 0) {
			char *token = strtok(buffer, " "), *addr, *sock;

			if (token != NULL) {
				token = strtok(NULL, " ");

				if (token == NULL) {
					printf("The command was executed without parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					addr = token;	
				}

				token = strtok(NULL, "");

				if (token == NULL) {
					printf("The command was executed with too few parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					sock = token;	
				}

				ws_client *n = list_get(l, addr, 
						strtol(sock, (char **) NULL, 10));

				if (n == NULL) {
					printf("The client that was supposed to receive the "
						   "message, was not found in the userlist.\n");
					fflush(stdout);
					continue;
				}

				ws_closeframe(n, CLOSE_SHUTDOWN);
			}
		} else if ( strncasecmp(buffer, "sendall", 7) == 0 ||
			   strncasecmp(buffer, "writeall", 8) == 0) {
			char *token = strtok(buffer, " ");
			ws_connection_close status;

			if (token != NULL) {
				token = strtok(NULL, "");

				if (token == NULL) {
					printf("The command was executed without parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					ws_message *m = message_new();
					m->len = strlen(token);
					
					char *temp = malloc( sizeof(char)*(m->len+1) );
					if (temp == NULL) {
						raise(SIGINT);		
						break;
					}
					memset(temp, '\0', (m->len+1));
					memcpy(temp, token, m->len);
					m->msg = temp;
					temp = NULL;

					if ( (status = encodeMessage(m)) != CONTINUE) {
						message_free(m);
						free(m);
						raise(SIGINT);
						break;;
					}

					list_multicast_all(l, m);
					message_free(m);
					free(m);
				}
			}
		} else if ( strncasecmp(buffer, "send", 4) == 0 ||
				strncasecmp(buffer, "write", 5) == 0) {
			char *token = strtok(buffer, " "), *addr, *sock, *msg;
			ws_connection_close status;

			if (token != NULL) {
				token = strtok(NULL, " ");

				if (token == NULL) {
					printf("The command was executed without parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					addr = token;	
				}

				token = strtok(NULL, " ");

				if (token == NULL) {
					printf("The command was executed with too few parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					sock = token;	
				}

				token = strtok(NULL, "");
				
				if (token == NULL) {
					printf("The command was executed with too few parameters. Type "
						   "'help' to see how to execute the command properly."
						   "\n");
					fflush(stdout);
					continue;
				} else {
					msg = token;	
				}

				ws_client *n = list_get(l, addr, 
						strtol(sock, (char **) NULL, 10));

				if (n == NULL) {
					printf("The client that was supposed to receive the "
						   "message, was not found in the userlist.\n");
					fflush(stdout);
					continue;
				}

				ws_message *m = message_new();
				m->len = strlen(msg);
				
				char *temp = malloc( sizeof(char)*(m->len+1) );
				if (temp == NULL) {
					raise(SIGINT);		
					break;
				}
				memset(temp, '\0', (m->len+1));
				memcpy(temp, msg, m->len);
				m->msg = temp;
				temp = NULL;

				if ( (status = encodeMessage(m)) != CONTINUE) {
					message_free(m);
					free(m);
					raise(SIGINT);
					break;;
				}

				list_multicast_one(l, n, m);
				message_free(m);
				free(m);
			}
		} else {
			printf("To see functions available type: 'help'.\n");
			fflush(stdout);
			continue;
		}
	}

	pthread_exit((void *) EXIT_SUCCESS);
}

void *handleClient(void *args){
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	pthread_cleanup_push(&cleanup_client, args);

	int buffer_length = 0, string_length = 1, reads = 1;

	ws_client *n = args;
	n->thread_id = pthread_self();

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	char buffer[BUFFERSIZE];
	n->string = (char *) malloc(sizeof(char));

	if (n->string == NULL) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
		pthread_exit((void *) EXIT_FAILURE);
	}

	printf("Client connected with the following information:\n"
		   "\tSocket: %d\n"
		   "\tAddress: %s\n\n", n->socket_id, (char *) n->client_ip);
	printf("Checking whether client is valid ...\n\n");
	fflush(stdout);

	/**
	 * Getting headers and doing reallocation if headers is bigger than our
	 * allocated memory.
	 */
	do {
		memset(buffer, '\0', BUFFERSIZE);
		if ((buffer_length = recv(n->socket_id, buffer, BUFFERSIZE, 0)) <= 0){
			printf("%d\n", buffer_length);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			handshake_error("Didn't receive any headers from the client.", 
					ERROR_BAD, n);
			pthread_exit((void *) EXIT_FAILURE);
		}

		if (reads == 1 && strlen(buffer) < 14) {
			handshake_error("SSL request is not supported yet.", 
					ERROR_NOT_IMPL, n);
			pthread_exit((void *) EXIT_FAILURE);
		}

		string_length += buffer_length;

		char *tmp = realloc(n->string, string_length);
		if (tmp == NULL) {
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			handshake_error("Couldn't reallocate memory.", ERROR_INTERNAL, n);
			pthread_exit((void *) EXIT_FAILURE);
		}
		n->string = tmp;
		tmp = NULL;

		memset(n->string + (string_length-buffer_length-1), '\0', 
				buffer_length+1);
		memcpy(n->string + (string_length-buffer_length-1), buffer, 
				buffer_length);
		reads++;
	} while( strncmp("\r\n\r\n", n->string + (string_length-5), 4) != 0 
			&& strncmp("\n\n", n->string + (string_length-3), 2) != 0
			&& strncmp("\r\n\r\n", n->string + (string_length-8-5), 4) != 0
			&& strncmp("\n\n", n->string + (string_length-8-3), 2) != 0 );
	
	printf("User connected with the following headers:\n%s\n\n", n->string);
	fflush(stdout);

	ws_header *h = header_new();

	if (h == NULL) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		handshake_error("Couldn't allocate memory.", ERROR_INTERNAL, n);
		pthread_exit((void *) EXIT_FAILURE);
	}

	n->headers = h;

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	if ( parseHeaders(n->string, n, port) < 0 ) {
		pthread_exit((void *) EXIT_FAILURE);
	}

	if ( sendHandshake(n) < 0 && n->headers->type != UNKNOWN ) {
		pthread_exit((void *) EXIT_FAILURE);	
	}	

	list_add(l, n);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	printf("Client has been validated and is now connected\n\n");
	printf("> ");
	fflush(stdout);

	uint64_t next_len = 0;
	char next[BUFFERSIZE];
	memset(next, '\0', BUFFERSIZE);

	while (1) {
		if ( communicate(n, next, next_len) != CONTINUE) {
			break;
		}

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if (n->headers->protocol == CHAT) {
			list_multicast(l, n);
		} else if (n->headers->protocol == ECHO) {
			list_multicast_one(l, n, n->message);
		} else {
			list_multicast_one(l, n, n->message);
		}
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		if (n->message != NULL) {
			memset(next, '\0', BUFFERSIZE);
			memcpy(next, n->message->next, n->message->next_len);
			next_len = n->message->next_len;
			message_free(n->message);
			free(n->message);
			n->message = NULL;	
		}
	}
	
	printf("Shutting client down..\n\n");
	printf("> ");
	fflush(stdout);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	list_remove(l, n);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	pthread_cleanup_pop(0);
	pthread_exit((void *) EXIT_SUCCESS);
}
