# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -Wextra

rebuild: clean all
all: twmailer-server twmailer-client

clean: 
	clear
	rm -f twmailer-client twmailer-server

twmailer-server: src/server.c 
	${COMPILER}  -o twmailer-server src/server.c ${CFLAGS} -pthread -lldap -llber

twmailer-client: src/client.c 
	${COMPILER}  -o twmailer-client src/client.c ${CFLAGS}
