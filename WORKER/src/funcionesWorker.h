/*
 * funcionesWorker.h
 *
 *  Created on: 2/11/2017
 *      Author: utnso
 */

#ifndef FUNCIONESWORKER_H_
#define FUNCIONESWORKER_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h> // Para memcpy()
#include <unistd.h>
#include <sys/mman.h> // Para mmap
#include <sys/types.h>
#include <fcntl.h>
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include <commons/config.h> //Necesaria para manipular el archivo de configuracion (config.txt)
#include <shared/funcionesAuxiliares.h> //Necesaria para tamanioArchivo()
#include <commons/collections/list.h>
#include <shared/protocolo.h> // Para OK y ERROR
#include <shared/archivos.h>

extern t_log* log_WORKER; // Definimos la variable log_WORKER como GLOBAL para poder acceder a el archivo de log desde cualquier función
extern int numero_script_trans; // Variable global para generar nombres unicos para los scripts transformadores
extern int numero_script_reduc; // Variable global para generar nombres unicos para los scripts reductores

typedef struct {
	/* IMPORTANTE: ruta_script_trans NO necesita que se aloque memoria porque lo hace la función
	 * script_trans_a_archivo(). Luego, si, debe hacerse free.
	 */
	char* ruta_script_trans; // Es de la forma script_trans_2.sh - Le aloco memoria cuando lo guardo
	int nro_bloque_databin;
	int bytes_ocupados;
	char* nombre_temp_trans; // Es de la forma tmp_N - Le aloco memoria cuando lo guardo
}t_info_trans;

typedef struct{
	char* ruta_script_reduc_local;
	char* nombre_temp_reduc_local;
	t_list* lista_temp_trans; // Es la lista con los nombres temporales trans a reducir localmente
}t_info_reduc_local;

typedef struct{
	char* nombre_temp_trans;
}t_temp_trans;

typedef struct{
	char* nombre_nodo;
	int puerto_worker;
	char* ip;
	char* nombre_temporal_reduccion;
}t_info_nodo_ayudante;

typedef struct{
	char* nombre_temporal_reducido;
}t_nombre_temporal_reducido; // Es el tipo de dato que tienen los elementos de la lista_nombres_temporales

/*
===============================
INICIO PROTOTIPOS FUNCIONES
===============================
*/

t_config* configuracion_WORKER(char* ruta_archivo_configuracion, char** RUTA_DATABIN, int* PUERTO_ESCUCHA_MASTER, int* PUERTO_FS, char** IP_FS);


/* @DESC: Aplica el script transformador por un bloque.
 *
 */
int aplicar_transformacion(t_info_trans* info_trans_actual, void* databin_mapeado);


/* @DESC: Aparea los archivos temporales a reducir localmente y luego le aplica el script
 * reductor local.
 */
int aplicar_reduccion_local(t_info_reduc_local* info_reduc_local);


/* @DESC: Aparea los archivos temporales reducidos localmente y luego le aplica el script
 * reductor local para generar el resultado final del JOB.
 */
int aplicar_reduccion_global(t_list* lista_temporales, char* ruta_script, char* ruta_archivo_resultado);


/* @DESC: Crea un nombre único de archivo (auxiliar) y graba en él el script transformador que se encuentra en un buffer.
 * Retorna el nombre del archivo con el que se guardo.
 */
char* script_trans_a_archivo(void* buffer_script_trans);


/* @DESC: Crea un nombre único de archivo (auxiliar) y graba en él el script reductor que se encuentra en un buffer.
 * Retorna el nombre del archivo con el que se guardo.
 */
char* script_reduc_a_archivo(void* buffer_script_reduc);


/*
---------------------------
FIN PROTOTIPO FUNCIONES
---------------------------
*/


// prototipos vero

void definirAccionEjecucionHeader(t_protocolo *protocolo, uint32_t socket);

// Liberar listas
void liberar_lista_nombres_temporales(t_list* lista_nombres_temporales);

void liberar_lista_temp_trans(t_list* lista_temp_trans);

#endif /* FUNCIONESWORKER_H_ */
