CC 		= gcc
# CFLAGS 	= -Wall -Wextra -Werror -pedantic -ggdb -DRUPIFY -g -lssl -lcrypto
# CFLAGS = -Wall
CFLAGS = -w

# INCL = -L /usr/include/openssl -I /usr/include/openssl
INCL = -I/usr/local/include/json-c
LIBRARIES = -lssl -lcrypto -L/susr/local/lib -ljson-c `mysql_config --libs_r` 
OBJECTS = Includes.o webServer.o webSocketServer.o message.o
DEVOBJECTS = DEVIncludes.o webServer.o DEVwebSocketServer.o message.o
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

# utf8.o: utf8.c utf8.h
# 	$(CC) $(CFLAGS) -c utf8.c	

webServer.o: webServer.c webServer.h Includes.h
	$(CC) $(INCL) $(CFLAGS) -c webServer.c

webSocketServer.o: webSocketServer.c webSocketServer.h Includes.h message.h
	$(CC) $(INCL) $(CFLAGS) -c webSocketServer.c 

DEVwebSocketServer.o: webSocketServer.c webSocketServer.h Includes.h message.h
	$(CC) $(INCL) $(CFLAGS) -D DEV -c webSocketServer.c 

message.o: message.c message.h Includes.h 
	$(CC) $(INCL) $(CLFAGS) -c message.c
