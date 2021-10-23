# Makefile for TW-Mailer

COMPILER = gcc
CFLAGS = -g -Wall -pthread

all: tw-server tw-client

clean: 
	rm -f tw-client tw-server

tw-server: server.c 
	${COMPILER} ${CFLAGS} -o tw-server server.c

tw-client: client.c 
	${COMPILER} ${CFLAGS} -o tw-client client.c
