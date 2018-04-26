
#ifndef FILE_SYSTEM_YAMA_H_
#define FILE_SYSTEM_YAMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <shared/sockets.h>
#include <shared/protocolo.h>
#include <shared/estructuras.h>


//EStructura de la Tabla de Estados
typedef enum{
	TRANSFORMACION, REDUCCION_LOCAL, REDUCCION_GLOBAL
}t_etapa;

typedef enum{
	EN_PROCESO,FIN_OK,FIN_ERROR
}t_estado;

typedef struct {
	uint32_t job;
	uint32_t master;
	uint32_t nodo;
	uint32_t bloque;
	t_etapa etapa;
	char* archivoTemporal;
	t_estado estado;
} t_TablaEstados;

#endif FILE_SYSTEM_YAMA_H_
