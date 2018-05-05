CC 		= gcc
# CFLAGS 	= -Wall -Wextra -Werror -pedantic -ggdb -DRUPIFY -g -lssl -lcrypto
CFLAGS = -Wall
# INCL = -L /usr/include/openssl -I /usr/include/openssl
LIBRARIES = -lssl -lcrypto 
OBJECTS = Includes.o webServer.o webSocketServer.o
DEVOBJECTS = DEVIncludes.o webServer.o webSocketServer.o
EXEC 	= server

.PHONY: server

all: clean server

dev: clean devServer
	./$(EXEC) $(PORT) $(DIR)

run: all
	./$(EXEC) $(PORT) $(DIR)

server: $(OBJECTS) 
	$(CC) $(INCL) $(CFLAGS) $(OBJECTS) -lpthread $(EXEC).c -o $(EXEC) $(LIBRARIES)

devServer: $(DEVOBJECTS) 
	$(CC) $(INCL) $(CFLAGS) $(OBJECTS) -lpthread $(EXEC).c -o $(EXEC) $(LIBRARIES)

clean:
	rm -f $(EXEC) *.o

Includes.o: Includes.c Includes.h
	$(CC) $(CFLAGS) -c Includes.c

DEVIncludes.o: Includes.c Includes.h
	$(CC) $(CFLAGS) -D DEV -c Includes.c


webServer.o: webServer.c webServer.h Includes.h
	$(CC) $(CFLAGS) -c webServer.c

webSocketServer.o: webSocketServer.c webSocketServer.h Includes.h
	$(CC) $(CFLAGS) -c webSocketServer.c 

	
