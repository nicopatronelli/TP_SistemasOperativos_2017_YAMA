RM=rm -rf
CC=gcc
DIR := ${CURDIR}
SHARED_DIR:=$(DIR)/../../SHARED/shared
CONFIG_PATH:=$(CURDIR)/../config.txt
ENVS_PATH:=~/.bashrc

build/FILE_SYSTEM: 
	$(CC) -shared -o FILE_SYSTEM File_System.c -lcommons -L$(SHARED_DIR)/libSHARED -I$(SHARED_DIR)/..

export_library_path:
	export LD_LIBRARY_PATH=$(SHARED_DIR)

export_config_path:
	echo "export CONFIG_PATH_FS=$(CONFIG_PATH)" >> $(ENVS_PATH)

update_envs:
	. $(ENVS_PATH)

run_fs:
	./FILE_SYSTEM

all: export_library_path export_config_path update_envs build/FILE_SYSTEM run_fs

install: all

clean:
	$(RM) FILE_SYSTEM