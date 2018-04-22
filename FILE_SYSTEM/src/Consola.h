#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <shared/sockets.h>
#include <shared/md5.h>
#include <shared/protocolo.h>
#include <shared/estructuras.h>
#include <sys/syscall.h>

#define gettid() syscall(SYS_gettid)

void* comandosFileSystem (void);

