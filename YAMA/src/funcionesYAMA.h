/*
 * funcionesYAMA.h
 *
 *  Created on: 19/10/2017
 *      Author: utnso
 */

#ifndef YAMA_SRC_FUNCIONESYAMA_H_
#define YAMA_SRC_FUNCIONESYAMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <shared/protocolo.h>
#include <shared/mensajes.h>

#define SOCKET_MASTER_DESCONECTADO -1
#define MENOR_CARGA_WORKER 1000 // La establezco yo en un valor muy alto

extern t_log* log_YAMA;
extern t_list* lista_MASTERs;
extern t_list* tabla_estados;
extern t_list* lista_jobs;
extern t_list* lista_nodos;
extern int nro_job;
extern int indice_nombre_temporal;
extern int nro_bloque_archivo_transformado_ok;
extern int nro_bloque_archivo_transformado_error;
extern int nro_job_actual;
extern char* nombre_nodo_auxiliar;
extern char* nombre_nodo_reduccion_local;

typedef struct{
	int socket;
}t_master; // Tipo de dato para los nodos de la lista lista_MASTERs

typedef struct{
	int job;
	int master;
	char nodo[10]; // Deberia nadar con 7 bytes - IMP: Una vez que se inicializa NO se modifica más
	int bloque;
	char etapa[25]; // Debería andar con 18 bytes - IMP: Una vez que se inicializa NO se modifica más (Para la siguiente etapa se crea una NUEVA entrada en la tabla de estados)
	char nombre_temporal[25]; // Debería andar con 17 bytes
	char* estado; // Aloco memoria cuando la voy a cargar - ¡CUIDADO!: Este campo si se va actualizando de forma dinámica
}t_entrada_estado; // Tipo de dato para cada entrada de la tabla de estados de YAMA (lista de solicitudes)

typedef struct{
	bool existe_copia;
	char nombre_nodo[10]; // Nombre del nodo donde esta guardada la copia actual (ej: NODO1)
	int puerto_worker_escucha_master; // Puerto del WORKER del nodo donde escucha conexiones de MASTERs
	char ip_nodo[20]; // ip del nodo donde se guarda la copia actual
	int numero_bloque_databin; // Numero del bloque dentro del archivo data.bin del Nodo donde se guarda el contenido de la copia actual
	int bytes_ocupados; // Cantidad de bytes ocupados por la copiaactual dentro del bloque del data.bin (cuanto de los 1024 Bytes disponibles)
	bool copia_elegida_para_transformar; // La planificación marcará este campo como true o false
	char nombre_temporal_trans[25]; // Nombre temporal que asigna YAMA para el resultado de tranasformar el bloque
}t_copia; // Tipo de dato donde persisto la información de una copia del bloque a procesar

typedef struct{
	t_copia copia0;
	t_copia copia1;
}t_Bloque; // Tipo de dato para persistir la información de cada bloque del archivo a procesar

typedef struct{
	char* nombre_nodo;
	int puerto_worker_escucha_master;
	char* ip;
}t_nodo;

typedef struct{
	char* nombre_nodo;
	int puerto_worker;
	char* ip;
	int carga_actual;
	int encargado;
	char* nombre_temporal_reduccion;
}t_nodo_job;

typedef struct{
	char* nombre_temporal;
}t_nombre_temporal;

/*
===============================
INICIO PROTOTIPOS FUNCIONES
===============================
*/

t_config* configuracion_YAMA(char* ruta_archivo_configuracion, char** FILESYSTEM_IP, int* FILESYSTEM_PUERTO, int* PUERTO_ESCUCHA_MASTER, int* RETARDO_PLANIFICACION, char** ALGORITMO_BALANCEO, int* DISPONIBILIDAD_BASE);


/* @DESC: Crea y agregue un nuevo t_master a la lista de MASTERs conectados a YAMA global. Le asigna el socket actual socket y el
 * correspondiente número de master nro_master con el que dicho MASTER es identificado unívocamente dentro de YAMA.
 */
void agregar_master(int socket, t_list* lista_master);


/* @DESC: Recorre la lista_master utilizando el socket_actual en busca del MASTER actual.
 * Retorna un entero con el índice del MASTER actual en la lista (que tomamos como el número
 * del MASTER actual y es unívoco para cada MASTER) o -1 en caso de ERROR.
 */
int master_actual(t_list* lista_master, int socket_actual);


/* @DESC: Realiza todas las tareas administrativas necesarias para desconectar un MASTER.
 *
 */
void desconectar_master(int numero_master, int socket_master, fd_set* master);


/* @DESC: Retorna el número de job asignado para el master cuyo fd es fd_master, aprovechando
 * que cada MASTER tendrá asociado, de forma unívoca, un nro de Job y viceversa.
 *
 */
int obtener_nro_job_para_master(int fd_master);


/* @DESC: Retorna un nombre temporal y unico en función de la VARIABLE GLOBAL indice_nombre_temporal.
 *
 */
char* _generar_nombre_temporal_unico();


/* @DESC: Actualiza la tabla de estados con la entrada correspondiente al bloque_actual del Job en procesamiento.
 * ¡CUIDADO!: Puede haber varias entradas por Job (es una entrada por bloque, NO una por Job)
 *
 */
void actualizar_tabla_estados(int cantidad_bloques_archivo, int nro_job, int numero_master, t_list* tabla_algoritmo_clock, t_list* lista_bloques_archivo);


/* @DESC: Envia la informacion del bloque seleccionado a transformar (por el planificador) al proceso MASTER que
 * inicio un nuevo job (envio una RUTA_ARCHIVO_A_TRANSFORMAR). Si t0do salio bien retorna OK (0) y caso contrario
 * retorna ERROR (-1).
 */
int enviar_info_bloque_a_transformar(int socket_YAMA_MASTER, int cantidad_bloques_archivo, t_list* lista_bloques_archivo);


/* @DESC: Actualizo el estado de la transformación del bloque de "En proceso" a "OK"
 *
 */
void actualizar_tabla_estados_bloque_ok(int nro_job, int nro_bloque);


/* @DESC: Grabo en en un archivo la versión actual de la tabla de estados. Dicho archivo tiene el nombre
 * tabla_estados_yama.csv y se pisa constantemente con la versión más actual.
 */
void grabar_tabla_estados_en_archivo();


/* @DESC: Funcion booleana auxiliar para la funcion actualizar_lista_nodos (se emplea en list_any_satisfy)
 *
 */
bool _ya_existe_el_nodo(void* nodo_actual);


/* @DESC: Actualizo la lista_nodos con los nuevos Nodos que participen el JOB del archivo actual y que
 * todavía no tenga en la misma.
 */
void actualizar_lista_nodos(t_list* lista_bloques_archivo);


/* @DESC: Creamos e inicializamos la lista_nodos con un nodo nulo como único elemento, para que luego
 * se pueda utilizar la función actualizar_lista_nodos según como fue diseñada.
 */
void inicializar_lista_nodos();


/* @DESC: Verifico si la entrada de la tabla de estados entrada_estado_actual tiene el nro de job
 * nro_job_actual y el nombre de nodo nombre_nodo_reduccion_local (ambas variables globales). Esta
 * funcion booleana se emplea dentro de chequear_nodo_iniciar_reduccion_local()
 */
bool _entrada_nodo_y_job_buscada(void* entrada_estado_actual);


/* @DESC: Verfico si la entrada de la tabla de estados entrada_estado_actual cuyo nro de job
 * es nro_job_actual y nombre de nodo es nombre_nodo_reduccion_local (ambas variables globales)
 * esta marcada como "OK", es decir, si la etapa de transformacion para un bloque particular de un
 * Nodo particular de un determinado Job se completo exitosamente.
 */
bool _transformacion_ok(void* entrada_estado_actual);


/* @DESC: Chequeo si puedo iniciar la etapa de REDUCCION LOCAL para el job nro_job_actual
 * dentro de un determinado Nodo.
 */
bool chequear_nodo_iniciar_reduccion_local(char* nombre_nodo_actual, int nro_job_actual);


/* @DESC: Retorna la lista con los nombres temporales de los resultados de las transformaciones para el
 * job nro_job_actual realizadas en el nodo nombre_nodo_reduccion_local.
 */
t_list* lista_nombres_temporales_trans_nodo_reduccion_local(char* nombre_nodo_reduccion_local, int nro_job_actual);


/* @DESC: Retorna una estructura t_nodo que contiene la información del nombre_nodo_actual, es decir,
 * su ip y puerto (además de un campo con su propio nombre).
 */
t_nodo* recuperar_info_nodo(char* nombre_nodo_actual);


/* @DESC: Cada vez que inicio una nueva etapa de reduccion local llamo a esta función para agregar
 * la correspondiente entrada en la tabla de estados. Esta función retorna el nombre temporal
 * asignado al resultado de la reducción global.
 */
char* agregar_entrada_reduccion_local_tabla_estados(int nro_job_actual, int fd_master, char* nombre_nodo_actual);


/* @DESC: Función booleada para usar en list_find de actualizar_tabla_estados_nodo_reduccion_local_ok.
 * Retorna true si la estrada de la tabla de estados analizada tiene como número de job el valor de
 * nro_job_actual (variable global), como nombre de Nodo nombre_nodo_auxiliar (variable global) y
 * en etapa "REDUCCION LOCAL". Sino, false;
 */
bool _entrada_estado_tiene_nro_job_y_nombre_nodo_y_etapa_REDU_LOCAL(void* entrada);


/* @DESC: Actualiza el campo estado de la entrada de la tabla de estados para la cual la
 * reducción local del Nodo nombre_nodo_actual. Marca el estado como "OK" si el flag pasado
 * es "OK" o marca el estado como "ERROR" si el flag pasado es "ERROR".
 *
 */
void actualizar_tabla_estados_nodo_reduccion_local(char*nombre_nodo_actual, int nro_job_actual, char* flag);


/* @DESC: Función booleana para list_filter en chequear_job_iniciar_reduccion_global. Retorna true
 * si la entrada analizada tiene el nro_job_actual y en etapa "REDUCCION LOCAL", o false en caso
 * contrario.
 */
bool _entrada_job_reduccion_local(void* entrada_estado_actual);


/* @DESC: Función booleana para list_all_satisfy en chequear_job_iniciar_reduccion_global. Retorna
 * true si la entrada analizada tiene en estado "OK" y false en caso contrario.
 *
 */
bool _reduccion_local_ok(void* entrada_estado_actual);


/* @DESC: Chequea si se puede iniciar la etapa de reducción global para el job nro_job_actual.
 * Devuelve true si es así y false en caso contrario.
 */
bool chequear_job_iniciar_reduccion_global(int nro_job_actual);


/* @DESC: Retorna la lista con los nombres de los Nodos que participan en el JOB cuyo número es
 * nro_job_actual;
 */
t_list* generar_lista_nodos_job_actual(int nro_job_actual);


/* @DESC: Cargo la ip y puerto del nodo_job utilizando la información de la lista de nodos.
 *
 */
void cargar_ip_y_puerto_nodo(t_nodo_job* nodo_job);


/* @DESC: Elige al Nodo encargado de la lista de nodos que participaron en el job y marca
 * su campo nodo_encargado en true.
 */
void elegir_nodo_encargado(t_list* lista_nodos_participantes);


/* @DESC: Retorna el elemento t_nodo_job que es el Nodo encargado de realizar la reducción actual
 * dentro de la lista de Nodos participantes.
 */
t_nodo_job* identificar_nodo_encargado_en_YAMA(t_list* lista_nodos_participantes);


/* @DESC: Agrega una nueva entrada en la tabla de estados correspondiente a la etapa de reducción global
 * del Job identificado con el nro_job_actual.
 */
char* agregar_entrada_reduccion_global_tabla_estados(int nro_job_actual, t_list* lista_nodos_participantes, int fd_master);


/* @DESC: Retorna true si la entrada de la tabla de estados tiene en número de job el valor de nro_job_actual
 * (variable global) y en el campo etapa "REDUCCION GLOBAL". Se utiliza en el list_find de la función
 * actualizar_tabla_estados_job_reduccion_global_ok().
 */
bool _entrada_estado_tiene_nro_job_y_etapa_REDU_GLOBAL(void* entrada);


/* @DESC: Actualiza la entrada de la reducción global para el job cuyo número es nro_job_actual. Si el flag se
 * pasa como "OK" el campo estado se marca como "OK". Si el flag se pasa como "ERROR" el campo estado se marca
 * como "ERROR".
 */
void actualizar_tabla_estados_job_reduccion_global(int nro_job_actual, char* flag);

/*
---------------------------
FIN PROTOTIPO FUNCIONES
---------------------------
*/

#endif /* YAMA_SRC_FUNCIONESYAMA_H_ */
