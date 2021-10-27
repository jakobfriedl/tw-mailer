# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -pthread -luuid 

all: tw-server tw-client

clean: 
	rm -f tw-client tw-server

tw-server: server.c 
	${COMPILER}  -o tw-server server.c ${CFLAGS}

tw-client: client.c 
	${COMPILER}  -o tw-client client.c ${CFLAGS}
