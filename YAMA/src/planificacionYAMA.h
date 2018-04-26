/*
 * planificacionYAMA.h
 *
 *  Created on: 5/11/2017
 *      Author: utnso
 */

#ifndef YAMA_SRC_PLANIFICACIONYAMA_H_
#define YAMA_SRC_PLANIFICACIONYAMA_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <stdbool.h>
#include "funcionesYAMA.h"

#define CANTIDAD_WORKERS 11
#define LONGITUD_NOMBRE_NODO 10 // Ejemplo: NODO1
#define COPIA_0 0
#define COPIA_1 1
#define CARGA_HISTORICA 1000
#define DISPONILIDAD_MAXIMA -1
#define MAXIMA_CARGA_ACTUAL 0

typedef struct{
	unsigned int carga_actual; // WL(w): Carga actual del WORKER w
	unsigned int carga_historica; // Carga histórica del WORKER
}t_worker;

typedef struct{
	bool puntero_clock; // true (el clock está en esta entrada) o false (el clock NO está en esta entrada)
	char nombre_nodo[10]; // nombre_worker
	int disponibilidad_worker; // A(w): Availability o función de disponibilidad
	t_list* lista_bloques_asignados;
}t_entrada_algoritmo;

typedef struct{
	int nro_bloque;
}t_bloque_a_procesar; // Representa el bloque que se le va a asignar a un Nodo (WORKER) para que lo procese


// Variables globales
extern t_log* log_YAMA;
extern char* NOMBRE_NODO_ACTUAL; // Variable global para la función chequear_nodo()
extern int DISPONIBILIDAD_ACTUAL; // Variable global para la función _chequear_igual_disponibilidad()
extern t_worker tabla_global_workers[CANTIDAD_WORKERS]; // Variable global con la tabla de workers y su carga_actual y carga_histórica
/*
===============================================================
INICIO PROTOTIPOS FUNCIONES - auxiliares de planificación
===============================================================
*/

/* @DESC: Función booleana que se utiliza en la función chequear_nodo()
 *
 */
bool _ya_existe_este_nombre_de_nodo(void* entrada_actual);


/* @DESC: Convierte del formato "NODON" a N. Ej: Si recibe el char* "NODO1" retorna el entero 1
 *
 */
int obtener_numero_nodo(char* nombre_nodo);


/* @DESC: Función booleana que se utiliza en la función chequear_todos_los_nodos_tienen_la_misma_disponibilidad()
 *
 */
bool _chequear_igual_disponibilidad(void* entrada_actual);


/* @DESC: Función booleana que se utiliza en la función devolver_entrada_con_puntero_clock()
 *
 */
bool _entrada_tiene_puntero_clock(void* entrada);


/* @DESC: Retorna un entero con el indice que posee la entrada con el puntero clock en true
 * en la tabla (lista) del algoritmo.
 */
int calcular_indice_de_entrada_con_puntero(t_list* tabla_algoritmo_clock);

/* @DESC: Retorna el nombre del Nodo que se le asigno al bloque_actual. Por ejemplo, si el bloque_actual es 3
 * y se le asigno el Nodo 1 retorna NODO1.
 */
char* obtener_nodo_asignado_al_bloque(int bloque_actual, t_list* tabla_algoritmo_clock);


/* @DESC: Retorna el nombre del Nodo de la entrada actual (que pertenece a la tabla del algoritmo)
 *
 */
char* obtener_nombre_nodo_entrada_actual(t_entrada_algoritmo* entrada_actual);


/* @DESC: Marca como true el campo copia_elegida_para_transformar de la copia elegida para
 * transformar del bloque_actual, y como false, el campo_copia_elegida_para_transformar de
 * la copia del bloque_actual que NO fue elegida por la planificación para transformar.
 *
 */
void marcar_copia_elegida_para_transformar(t_list* lista_bloques_archivo, t_entrada_algoritmo* entrada_actual, int numero_bloque_actual);


/*
------------------------------------------------------
FIN PROTOTIPO FUNCIONES - auxiliares de planificación
------------------------------------------------------
*/


/*
===============================================================
INICIO PROTOTIPOS FUNCIONES - genéricas de planificación
===============================================================
*/

/* @DESC: Inicializa la tabla de workers (array de t_worker) con carga_actual y carga_historica en 0.
 *
 */
void inicializar_tabla_global_workers(t_worker tabla_global_workers[]);


/* @DESC: Chequea si el Nodo actual está en mi tabla del algoritmo. Si está no hace nada.
 * Si NO está entonces agrega una nueva entrada t_entrada_algoritmo y la INCIALIZA.
 */
void chequear_nodo(t_Bloque* bloque_actual, t_list* lista_algoritmo_clock, bool existe_nodo, int DISPONIBILIDAD_BASE, int nro_copia);


/* @DESC: Posiciona el puntero (clock) en la entrada cuyo Nodo (WORKER) sea el de menor
 * carga histórica. Es genérica para ambos algoritmos, es decir, vale tanto para CLOCK
 * como para W-CLOCK.
 */
void posicionar_clock_en_nodo_menor_carga_historica(t_list* tabla_algoritmo);


/* @DESC: Se aplica antes de la función posicionar_clock_en_nodo_mayor_disponibilidad() para saber
 * si TODOS los Nodos de la tabla_algoritmo tienen la misma disponibilidad. Devuelve true de
 * ser así y false en caso contrario.
 */
bool chequear_todos_los_nodos_tienen_la_misma_disponibilidad(t_list* tabla_algoritmo);


/* @DESC: Posiciona el puntero (clock) en la entrada cuyo Nodo (WORKER) sea el de mayor
 * disponibilidad actual. Es genérica para ambos algoritmos, es decir, vale tanto para CLOCK
 * como para W-CLOCK.
 */
void posicionar_clock_en_nodo_mayor_disponibilidad(t_list* tabla_algoritmo);


/* @DESC: Retorna la entrada de la tabla del algoritmo (de tipo t_entrada_algoritmo) donde
 * está posicionado el puntero (clock), es decir, cuyo campo puntero_clock es true.
 */
t_entrada_algoritmo* devolver_entrada_con_puntero_clock(t_list* tabla_algoritmo_clock);


/* @DESC: Retorna true si el bloque_actual (al que le queremos asignar un Nodo o WORKER) se encuentra en el
 * Nodo (o WORKER) de la entrada (de la tabla del algoritmo) donde está el puntero clock. Caso contrario, retorna
 * false.
 */
bool bloque_actual_esta_en_worker_de_entrada_con_puntero(int numero_bloque_actual, t_list* tabla_algoritmo_clock, t_list* lista_bloques_archivo);


/* @DESC: Evalúa la disponibilidad del Nodo (o WORKER) que pertenece a la entrada (de la tabla del algoritmo)
 * donde está apuntando el puntero clock. SI la disponibilidad es mayor a cero retorna true, y sino, retorna
 * false.
 */
bool disponibilidad_worker_entrada_con_puntero_es_mayor_a_cero(t_list* tabla_algoritmo_clock);


/* @DESC: Saca el puntero_clock de la entrada que lo tiene actualmente y lo pone en la siguiente
 *
 */
void avanzar_clock_a_la_siguiente_entrada(t_entrada_algoritmo* entrada_con_puntero, t_list* tabla_algoritmo_clock);


/* @DESC: Retorna true si el bloque_actual (al que le queremos asignar un Nodo o WORKER) se encuentra en el
 * Nodo (o WORKER) de la entrada_actual. Caso contrario, retorna false.
 */
bool bloque_actual_esta_en_worker_de_entrada_actual(t_entrada_algoritmo* entrada_actual, int numero_bloque_actual, t_list* lista_bloques_archivo);


/* @DESC: Evalúa la disponibilidad del Nodo (o WORKER) que de la entrada_actual. SI la disponibilidad
 * es mayor a cero retorna true, y sino, retorna false.
 */
bool disponibilidad_worker_entrada_actual_es_mayor_a_cero(t_entrada_algoritmo* entrada_actual);


/* @DESC: Función principal de planificación donde se inicia el módulo de planificación desde YAMA.c
 * Llama a todas las restantes funciones. Es genérica para ambos algoritmos.
 */
t_list* planificacion(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, char* ALGORITMO_BALANCEO, int DISPONIBILIDAD_BASE);

/*
------------------------------------------------------
FIN PROTOTIPO FUNCIONES - genéricas de planificación
------------------------------------------------------
*/


/*
===============================================================
INICIO PROTOTIPOS FUNCIONES - ALGORITMO CLOCK
===============================================================
*/

/* @DESC: Genera la tabla inicial para el algoritmo CLOCK.
 *
 */
t_list* generar_tabla_algoritmo_clock(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, int DISPONIBILIDAD_BASE);


/* @DESC: Corre el algoritmo CLOCK sobre completando la tabla_algoritmo_clock con los bloques asignados
 * a cada WORKER.
 */
void aplicar_algoritmo_clock(t_list* lista_bloques_archivo, t_list* tabla_algoritmo_clock, int DISPONIBILIDAD_BASE);

/*
------------------------------------------------------
FIN PROTOTIPO FUNCIONES - ALGORITMO CLOCK
------------------------------------------------------
*/


/*
===============================================================
INICIO PROTOTIPOS FUNCIONES - ALGORITMO W-CLOCK
===============================================================
*/

/* @DESC: Genera la tabla inicial para el algoritmo WCLOCK.
 *
 */
t_list* generar_tabla_algoritmo_Wclock(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, int DISPONIBILIDAD_BASE);


/* @DESC: Función auxiliar para generar_tabla_algoritmo_Wclock
 *
 */
void chequear_nodo_Wclock(t_Bloque* bloque_actual, t_list* lista_algoritmo_wclock, bool existe_nodo, int DISPONIBILIDAD_BASE, int nro_copia);


/* @DESC: Calcula la disponibilidad A(w) para el Nodo nombre_nodo según la regla del algoritmo WCLOCK.
 *
 */
int calcular_disponibilidad_worker_Wclock(char* nombre_nodo, int DISPONIBILIDAD_BASE);


/* @DESC: Calcula la máxima carga actual WLmax entre todos los Nodos (WORKERs). Se emplea en la función
 * calcular_disponibilidad_worker_Wclock().
 */
int calcular_WLmax();


/*
------------------------------------------------------
FIN PROTOTIPO FUNCIONES - ALGORITMO W-CLOCK
------------------------------------------------------
*/


/*
===============================================================
INICIO PROTOTIPOS FUNCIONES - Carga de Nodos
===============================================================
*/

/* @DESC: Incrementa la carga actual y la carga histórica del Nodo (WORKER) perteneciente a la
 * entrada_actual en una unidad cada una. Esto debe hacerse cada vez que un Nodo es seleccionado
 * por la planificación para transformar un bloque.
 */
void incrementar_carga_por_transformacion(t_entrada_algoritmo* entrada_actual);

/*
------------------------------------------------------
FIN PROTOTIPO FUNCIONES - Carga de Nodos
------------------------------------------------------
*/


#endif /* YAMA_SRC_PLANIFICACIONYAMA_H_ */
