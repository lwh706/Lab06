CC=gcc
SRC_DIR=src
BIN_DIR=bin
LIB_DIR=lib
INCLUDE_DIR=include
CFLAG=-m64

.PHONY: clean rebuild

all: TCPCLIENT MSGHDL

TCPCLIENT:
	${CC} ${CFLAG} ${SRC_DIR}/TCPCLIENT.c -o ${BIN_DIR}/TCPCLIENT

MSGHDL:
	${CC} ${CFLAG} ${SRC_DIR}/MSGHDL.c -o ${BIN_DIR}/MSGHDL

clean:
	-rm -rf ${BIN_DIR}/TCPCLIENT
	-rm -rf ${BIN_DIR}/MSGHDL

rebuild: clean all