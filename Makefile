# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -pthread -luuid

all: twmailer-server twmailer-client

clean: 
	rm -f twmailer-client twmailer-server

twmailer-server: server.c 
	${COMPILER}  -o twmailer-server server.c ${CFLAGS}

twmailer-client: client.c 
	${COMPILER}  -o twmailer-client client.c ${CFLAGS}
