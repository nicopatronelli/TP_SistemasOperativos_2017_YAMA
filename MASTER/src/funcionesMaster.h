/*
 * funcionesMaster.h
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/funcionesAuxiliares.h>
#include <shared/sockets.h>
#include <shared/mensajes.h>
#include <shared/protocolo.h>
#include <shared/archivos.h>
#include <pthread.h>

#define CANTIDAD_HILOS 50

extern t_log* log_MASTER;
extern int socket_MASTER_YAMA; // La declaro global para que los hilos puedan comunicarse con YAMA directamente
extern char* ruta_resultadoFinal; // Es la ruta donde se guardará el archivo final en FILE_SYSTEM después de la reducción global

// Variables globales para los hilos
extern void* buffer_script_transformador;
extern void* buffer_script_reductor;
/* Variable global para el nombre del archivo donde se guarda el resultado de la reducción global (es único para
 * cada MASTER, así que no hay problemas con que sea global).
 */
extern char* nombre_resultado_reduccion_global;

typedef struct{
	int numero_bloque_archivo; // Es el numero de bloque del archivo a transformar (es lógico)
	char* nombre_nodo; // Aloco memoria cuando la relleno
	char* ip_nodo; // Aloco memoria cuando la relleno
	int puerto_worker_escucha_master;
	int numero_bloque_databin; // Es el numero que ocupa el bloque dentro del data.bin (es físico)
	int bytes_ocupados;
	char* nombre_archivo_temporal; // Aloco memoria cuando la relleno
}t_info_bloque_a_transformar; // Estructura donde guarda la info que envia YAMA de cada bloque a transformar

typedef struct{
	int cantidad_temporales_a_reducir;
	t_list* lista_temporales_a_reducir; // Creo la lista cuando la voy a utilizar
	char* nombre_nodo; // Aloco memoria cuando la voy a utilizar
	char* ip_nodo; // Aloco memoria cuando la voy a utilizar
	int puerto_worker_escucha_master;
	char* nombre_temporal_reduccion; // Aloco memoria cuando la voy a utilizar
}t_info_nodo_a_reducir;

typedef struct{
	char* nombre_temporal_a_reducir;
}t_nombre_temporal_a_reducir; // Es el tipo de dato que tienen los elementos de la lista_temporales_a_reducir

typedef struct{
	char* nombre_nodo;
	int puerto_worker;
	char* ip;
	int encargado;
	char* nombre_temporal_reduccion;
}t_info_nodo_redu_global;

/*
===============================
INICIO PROTOTIPOS FUNCIONES
===============================
*/

t_config* configuracion_MASTER(char* ruta_archivo_configuracion, char** YAMA_IP, int* YAMA_PUERTO);


/* @DESC: Versión actual y funcional de guardar_script(). Guarda el contenido de un script en un buffer
 * y lo retorna.
 *
 */
void* _guardar_script(char* ruta_script);


/* @DESC: No funcional.
 *
 */
void* guardar_script(char* ruta_script);


/* @DESC:
 *
 */
void liberar_buffer_script(void* buffer_script);


/* @DESC: Envia la información necesaria al WORKER correspondiente para que realice la
 * transformación de un bloque. Dentro de la misma función se comunica a YAMA del éxito o fracaso
 * de la operación.
 */
void enviar_info_bloque_transformar_a_worker(t_info_bloque_a_transformar* info_bloque_actual);


/* @DESC: Envia la información necesaria al WORKER correspondiente para que realice la
 * reducción local en un Nodo. Dentro de la misma función se comunica a YAMA del éxito o fracaso
 * de la operación.
 */
void enviar_info_nodo_reduccion_local_a_worker(t_info_nodo_a_reducir* nodo_a_reducir);

/* @DESC: Envia la información necesaria al WORKER ENCARGADO para que realice la
 * reducción global. Dentro de la misma función se comunica a YAMA del éxito o fracaso
 * de la operación.
 */
void enviar_info_nodo_encargado(t_list* lista_nodos_participantes);


/* @DESC: Retorna el nodo encargado de realizar la reducción global.
 *
 */
t_info_nodo_redu_global* identificar_nodo_encargado(t_list* lista_nodos_participantes);

void liberar_lista_temporales_a_reducir(t_list* lista_temporales);

void liberar_lista_nodos_participantes(t_list* lista_nodos_participantes);

/*
---------------------------
FIN PROTOTIPO FUNCIONES
---------------------------
*/


#endif /* FUNCIONESMASTER_H_ */
