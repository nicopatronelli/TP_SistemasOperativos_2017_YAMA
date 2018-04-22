RM=rm -rf
CC=gcc
DIR := ${CURDIR}

export_library_path:
	export LD_LIBRARY_PATH=$(DIR)

build/libSHARED: 
	$(CC) -shared -fPIC -o libSHARED ./shared/estructuras.c ./shared/funcionesAuxiliares.c ./shared/md5.c ./shared/protocolo.c ./shared/sockets.c

all: build/libSHARED export_library_path

install: all

clean:
	$(RM) libSHARED
