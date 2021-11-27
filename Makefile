# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -Wextra

rebuild: clean all
all: twmailer-server twmailer-client

clean: 
	clear
	rm -f twmailer-client twmailer-server
	rm -f obj/*

# Compile Files to object files
./obj/helper.o: code/src/helper.c
	${COMPILER} ${CFLAGS} code/src/helper.c -o obj/helper.o -c

./obj/clientHelper.o: code/src/clientHelper.c
	${COMPILER} ${CFLAGS} code/src/clientHelper.c -o obj/clientHelper.o -c

./obj/clientCommands.o: code/src/clientCommands.c
	${COMPILER} ${CFLAGS} code/src/clientCommands.c -o obj/clientCommands.o -c

./obj/client.o: code/client.c
	${COMPILER} ${CFLAGS} code/client.c -o obj/client.o -c

./obj/serverHelper.o: code/src/serverHelper.c
	${COMPILER} ${CFLAGS} code/src/serverHelper.c -o obj/serverHelper.o -c

./obj/serverCommands.o: code/src/serverCommands.c
	${COMPILER} ${CFLAGS} code/src/serverCommands.c -o obj/serverCommands.o -c

./obj/server.o: code/server.c
	${COMPILER} ${CFLAGS} code/server.c -o obj/server.o -c



# Compile Executables
twmailer-server: ./obj/server.o ./obj/helper.o ./obj/serverHelper.o ./obj/serverCommands.o
	${COMPILER}  -o twmailer-server obj/server.o obj/helper.o obj/serverHelper.o obj/serverCommands.o ${CFLAGS} -pthread -lldap -llber

twmailer-client: ./obj/client.o ./obj/helper.o ./obj/clientHelper.o ./obj/clientCommands.o
	${COMPILER}  -o twmailer-client obj/client.o obj/helper.o obj/clientHelper.o obj/clientCommands.o ${CFLAGS}

