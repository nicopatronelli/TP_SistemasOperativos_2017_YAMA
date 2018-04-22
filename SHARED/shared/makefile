RM=rm -rf
CC=gcc
DIR := ${CURDIR}

export_library_path:
	export LD_LIBRARY_PATH=$(DIR)

build/libSHARED: 
	$(CC) -shared -fPIC -o libSHARED estructuras.c funcionesAuxiliares.c md5.c protocolo.c sockets.c

all: build/libSHARED export_library_path

install: all

clean:
	$(RM) libSHARED
