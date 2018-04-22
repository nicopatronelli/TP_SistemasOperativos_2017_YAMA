RM= rm -rf
CC=gcc
DIR := ${CURDIR}
SHARED_DIR:=$(DIR)/../../SHARED/shared

build/MASTER: 
	$(CC) -shared -o MASTER MASTER.c funcionesMaster.c -lcommons -L$(SHARED_DIR)/libSHARED -I$(SHARED_DIR)/..

export_library_path:
	export LD_LIBRARY_PATH=$(SHARED_DIR)

all: export_library_path build/MASTER

install: all

clean: 
	$(RM) MASTER