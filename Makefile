# http://www.lab.dit.upm.es/~cdatlab/exs/rpc/test/
# rpcgen example Makefile
# Internetworking with TCP/IP, Volume III
#
CC = gcc
PROG_SERVER = Server/server 
PROG_CLIENT = Client/client

SRC_SERVER = Server/server.c 
SRC_CLIENT = Client/client.c
SRC_RSA = Cryption/rsa.c

all: ${PROGS}

server: ${SRC_SERVER} ${SRC_RSA}
	${CC} ${SRC_SERVER} ${SRC_RSA} -o ${PROG_SERVER}

client: ${SRC_CLIENT}
	${CC} ${SRC_CLIENT} -o ${PROG_CLIENT}

run_server: ${PROG_SERVER}
	${PROG_SERVER}

run_client: ${PROG_CLIENT}
	${PROG_CLIENT} 127.0.0.1 Minh

clean: ${PROG_SERVER} ${PROG_CLIENT} 
	rm -f ${PROG_SERVER} ${PROG_CLIENT} 
