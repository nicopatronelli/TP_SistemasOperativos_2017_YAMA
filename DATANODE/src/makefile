RM= rm -rf
CC=gcc
DIR := ${CURDIR}
SHARED_DIR:=$(DIR)/../../SHARED/shared

build/DATANODE: 
	$(CC) -shared -o DATANODE DATANODE.c Mensajes_Nodo.c -lcommons -L$(SHARED_DIR)/libSHARED -I$(SHARED_DIR)/..

export_library_path:
	export LD_LIBRARY_PATH=$(SHARED_DIR)

all: export_library_path build/DATANODE

install: all

clean: 
	$(RM) DATANODE