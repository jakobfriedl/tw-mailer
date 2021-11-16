# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -pthread -luuid

rebuild: clean all
all: twmailer-server twmailer-client

clean: 
	clear
	rm -f twmailer-client twmailer-server

twmailer-server: src/server.c 
	${COMPILER}  -o twmailer-server src/server.c ${CFLAGS}

twmailer-client: src/client.c 
	${COMPILER}  -o twmailer-client src/client.c ${CFLAGS}
