# http://www.lab.dit.upm.es/~cdatlab/exs/rpc/test/
# rpcgen example Makefile
# Internetworking with TCP/IP, Volume III
#
CC=gcc
PROG_SERVER = Server/server 
PROG_CLIENT = Client/client

DEFS = 
CFLAGS = -g ${DEFS} ${INCLUDE}

SRC_SERVER = Server/server.c 
SRC_CLIENT = Client/client.c

all: ${PROGS}

server: ${SRC_SERVER}
	${CC} ${SRC_SERVER} -o ${PROG_SERVER}

client: ${SRC_CLIENT}
	${CC} ${SRC_CLIENT} -o ${PROG_CLIENT}

run_server: ${PROG_SERVER}
	${PROG_SERVER}

run_client: ${PROG_CLIENT}
	${PROG_CLIENT} 127.0.0.1 Minh

clean: FRC
	rm -f Server/Makefile.bak Server/a.out Server/tags Server/core Server/make.out Server/*.o Client/Makefile.bak Client/a.out Client/tags Client/core Client/make.out Client/*.o ${PROG_SERVER} ${PROG_CLIENT} 

FRC:

install: all FRC
	@echo nothing to install.

