/*
 * planificacionYAMA.c
 *
 *  Created on: 5/11/2017
 *      Author: utnso
 */

#include "planificacionYAMA.h"

// Variables globales
t_log* log_YAMA;
char* NOMBRE_NODO_ACTUAL;
int DISPONIBILIDAD_ACTUAL;
t_worker tabla_global_workers[CANTIDAD_WORKERS];


/*
==================================================
INICIO - funciones genéricas de planificación
==================================================
*/

/* @OBSERVACION: Para el primer job que le mandan a ejecutar a YAMA inicializo la tabla de workers con carga_actual y
 * carga_historica en 0. Para los subsiguientes jobs simplemente la voy a ir actualizando. Esta tabla va a persistir (en memoria)
 * mientras el proceso YAMA viva. Cuando YAMA finalice (por algún error o pulsando Ctrl+C) se perdera, lo que creo es correcto.
 * Otra forma sería persistir la tabla de workers global en un archivo (como una estructura administrativa) y levantarla
 * al volver a correr YAMA.
 *
 * Es un WORKER por Nodo (computadora física), por lo tanto, con, por ejemplo, 10 workers represento 10 computadoras
 * físicas distintas (más que suficiente, pues en las pruebas se usan 4 Nodos -> 4 computadoras distintas)
 */
void inicializar_tabla_global_workers(t_worker tabla_global_workers[]){

		int nro_worker;

		// Arrancamos en nro_worker=1 porque los nombres de los nodos arrancan en 1. Ej: NODO1
		for(nro_worker=1; nro_worker<CANTIDAD_WORKERS; nro_worker++){

			// Incializo en cero los campos carga actual y carga histórica de cada WORKER
			tabla_global_workers[nro_worker].carga_actual = 0;
			tabla_global_workers[nro_worker].carga_historica = 0;

		} // fin for

} // FIN inicializar_tabla_global_workers()


void chequear_nodo(t_Bloque* bloque_actual, t_list* lista_algoritmo_clock, bool existe_nodo, int DISPONIBILIDAD_BASE, int nro_copia){

	NOMBRE_NODO_ACTUAL = malloc(LONGITUD_NOMBRE_NODO);

	if ( nro_copia == COPIA_0){
		strcpy(NOMBRE_NODO_ACTUAL, bloque_actual->copia0.nombre_nodo);
	}else if ( nro_copia == COPIA_1){
		strcpy(NOMBRE_NODO_ACTUAL, bloque_actual->copia1.nombre_nodo);
	}

	existe_nodo = list_any_satisfy(lista_algoritmo_clock, _ya_existe_este_nombre_de_nodo);

	if ( existe_nodo == false ){ // Si no tengo el nodo actual en mi lista del algoritmo entonces lo agrego

		t_entrada_algoritmo* entrada_actual = malloc(sizeof(t_entrada_algoritmo));

		strcpy(entrada_actual->nombre_nodo, NOMBRE_NODO_ACTUAL);
		entrada_actual->disponibilidad_worker = DISPONIBILIDAD_BASE; // Para el algoritmo Clock la A(w)= DISPONIBILIDAD_BASE (no se considera la carga historica NUNCA)
		entrada_actual->puntero_clock = false; // Asumo que no tengo el puntero (caso general)
		entrada_actual->lista_bloques_asignados = list_create(); // Arranca sin bloques asignados (ver tabla del algoritmo en el ejemplo del TP)

		list_add(lista_algoritmo_clock, entrada_actual); // Agrego la entrada_actual a la lista del algoritmo Clocks
	}

	free(NOMBRE_NODO_ACTUAL);

} // FIN chequear_nodo


void posicionar_clock_en_nodo_menor_carga_historica(t_list* tabla_algoritmo){ // Es genérica para ambos algoritmos

		t_entrada_algoritmo* entrada_actual;
		t_entrada_algoritmo* entrada_menor_carga_historica;
		int numero_nodo_actual;
		int menor_carga_historica = CARGA_HISTORICA; // Lo defino yo como un valor máximo muy alto

		int nro_entrada;
		for (nro_entrada=0; nro_entrada< list_size(tabla_algoritmo); nro_entrada++){

		entrada_actual = list_get(tabla_algoritmo, nro_entrada);
		numero_nodo_actual = obtener_numero_nodo(entrada_actual->nombre_nodo);

			if ( tabla_global_workers[numero_nodo_actual].carga_historica < menor_carga_historica ){
				menor_carga_historica = tabla_global_workers[numero_nodo_actual].carga_historica;
				entrada_menor_carga_historica = entrada_actual;

				/* IMPORTANTE: En caso de empate quedará asignado el puntero en el primero Nodo con
				 * menor carga histórica, es decir, define FIFO.
				 */
				}

		} // FIN for

	// Una vez que tengo la entrada con el Nodo (WORKER) de menor carga histórica le pongo el puntero (Clock) a true
	entrada_menor_carga_historica->puntero_clock = true;

} // FIN elegir_nodo_menor_carga_historica


bool chequear_todos_los_nodos_tienen_la_misma_disponibilidad(t_list* tabla_algoritmo){

	/* IMPORTANTE: Antes de aplicar la función posicionar_clock_en_nodo_mayor_disponibilidad()
	 * tengo que verificar que NO todos los Nodos tengan la misma disponibilidad (sino se produciría un empate).
	 * Comprobarlo es muy sencillo. Tomo un elemento cualquiera de la lista (por ejemplo el 0) y me quedo con su valor de
	 * disponibilidad. Luego, recorro la lista comparando esta disponibilidad con los valores de los demás Nodos.
	 * Si siempre se mantiene igual entonces todos los Nodos tienen igual disponibilidad. En cambio, a la primer
	 * diferencia ya tengo la confirmación que NO todos los Nodos tienen igual disponibilidad.
	 */
	t_entrada_algoritmo* entrada_actual = list_get(tabla_algoritmo, 0);

	// Establezco como DISPONIBILIDAD_ACTUAL la disponibilidad del primer Nodo
	DISPONIBILIDAD_ACTUAL = entrada_actual->disponibilidad_worker;

	bool misma_disponibilidad = true; // Supongo que todos los Nodos tienen igual disponibilidad

	misma_disponibilidad = list_all_satisfy(tabla_algoritmo, _chequear_igual_disponibilidad);

	return misma_disponibilidad;

}

void posicionar_clock_en_nodo_mayor_disponibilidad(t_list* tabla_algoritmo){

	/* IMPORTANTE: Aplico esta función sabiendo de antemano que NO todos los Nodos tienen la misma
	 * disponibilidad. Por lo tanto, no puede producirse un empate.
	 */

	t_entrada_algoritmo* entrada_actual;
	t_entrada_algoritmo* entrada_nodo_mayor_disponibilidad;

	// La defino yo como -1 (y como nunca puede ser negativa, entonces siempre será menor que la de cualquier WORKER)
	int disponibilidad_maxima = DISPONILIDAD_MAXIMA;

	int nro_entrada;
	for(nro_entrada=0; nro_entrada<list_size(tabla_algoritmo); nro_entrada++){

		entrada_actual = list_get(tabla_algoritmo, nro_entrada);
		if ( entrada_actual->disponibilidad_worker > disponibilidad_maxima ){
			entrada_nodo_mayor_disponibilidad = entrada_actual;
		}

	} // Fin for

	// Una vez que tengo la entrada con el Nodo (WORKER) de mayor disponibilidad le pongo el puntero (Clock) a true
	entrada_nodo_mayor_disponibilidad->puntero_clock = true;

} // FIN posicionar_clock_en_nodo_mayor_disponibilidad


t_entrada_algoritmo* devolver_entrada_con_puntero_clock(t_list* tabla_algoritmo_clock){

	t_entrada_algoritmo* entrada_con_puntero_clock = list_find(tabla_algoritmo_clock, _entrada_tiene_puntero_clock);

	return entrada_con_puntero_clock; // Retorno la entrada de la tabla del algoritmo a la que apunta el puntero clock

}


bool bloque_actual_esta_en_worker_de_entrada_con_puntero(int numero_bloque_actual, t_list* tabla_algoritmo_clock, t_list* lista_bloques_archivo){

	// Primero obtengo la entrada de la tabla del algoritmo donde está colocado el puntero clock
	t_entrada_algoritmo* entrada_con_puntero_clock = devolver_entrada_con_puntero_clock(tabla_algoritmo_clock);

	/* Ahora me fijo si en el Nodo (WORKER) de la entrada_con_puntero_clock está el bloque_actual
	 *
	 */

	// 1° - Obtengo el numero de Nodo (WQRKER) de la entrada donde está apuntando el clock en la tabla del algoritmo
	int numero_nodo_entrada_con_puntero = obtener_numero_nodo(entrada_con_puntero_clock->nombre_nodo);

	// 2° - Como la lista_bloques_archivo está ordenada (indexada) por número de bloque, es decir, el nodo 0 tiene la
	// info del bloque 0, el nodo 2 la info del bloque 1 y así sucesivamente, simplemente hago:
	t_Bloque* bloque_auxiliar = list_get(lista_bloques_archivo, numero_bloque_actual);

	// En bloque_auxiliar obtengo la info donde está guardado el bloque_actual del archivo


	// 3° - Ahora me fijo si el numero_nodo_entrada_con_puntero es igual al numero_nodo donde está el bloque actual
	if ( obtener_numero_nodo(bloque_auxiliar->copia0.nombre_nodo) == numero_nodo_entrada_con_puntero || obtener_numero_nodo(bloque_auxiliar->copia1.nombre_nodo) == numero_nodo_entrada_con_puntero ){
		return true;
	}else{
		return false;
	}

} // FIN bloque_actual_esta_en_worker_de_entrada_con_puntero


bool disponibilidad_worker_entrada_con_puntero_es_mayor_a_cero(t_list* tabla_algoritmo_clock){

	t_entrada_algoritmo* entrada_con_puntero_clock = devolver_entrada_con_puntero_clock(tabla_algoritmo_clock);
	if ( entrada_con_puntero_clock->disponibilidad_worker > 0){
		return true;
	}else{
		return false;
	}

} // FIN disponibilidad_worker_entrada_con_puntero_es_mayor_a_cero


void avanzar_clock_a_la_siguiente_entrada(t_entrada_algoritmo* entrada_con_puntero, t_list* tabla_algoritmo_clock){

	t_entrada_algoritmo* entrada_siguiente_al_puntero; // La entrada siguiente de la tabla del algoritmo a la que tiene el puntero_clock

	int indice_entrada_con_puntero = calcular_indice_de_entrada_con_puntero(tabla_algoritmo_clock);
	entrada_con_puntero->puntero_clock = false;

	int indice_ultima_entrada_tabla = (list_size(tabla_algoritmo_clock) - 1);

	if ( indice_entrada_con_puntero == indice_ultima_entrada_tabla ){
		// Si estoy en la ultima entrada de la tabla tengo que poner el clock en la primer entrada
		entrada_siguiente_al_puntero = list_get(tabla_algoritmo_clock, 0);
		entrada_siguiente_al_puntero->puntero_clock = true;
	}else{
		// Sino estoy en la última entrada de la tabla entonces si pongo el clock en la que sigue
		entrada_siguiente_al_puntero = list_get(tabla_algoritmo_clock, indice_entrada_con_puntero + 1);
		entrada_siguiente_al_puntero->puntero_clock = true;
	}

} // FIN avanzar_clock_a_la_siguiente_entrada


t_list* planificacion(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, char* ALGORITMO_BALANCEO, int DISPONIBILIDAD_BASE){

	// LA SALIDA TIENE QUE SER EL PLAN DE EJECUCION DEL JOB A ENVIARLE A MASTER

	if ( strcmp(ALGORITMO_BALANCEO, "CLOCK") == OK) {

		t_list* tabla_algoritmo_clock = generar_tabla_algoritmo_clock(lista_bloques_archivo, cantidad_bloques_archivo, DISPONIBILIDAD_BASE);
		aplicar_algoritmo_clock(lista_bloques_archivo, tabla_algoritmo_clock, DISPONIBILIDAD_BASE); // Actualiza la tabla_algoritmo_clock con los valores correspondientes
		log_info(log_YAMA, "La planificación del JOB %d se realizo correctamente segun el ALGORITMO CLOCK: Se ha creado el plan de ejecución.", nro_job);
		return tabla_algoritmo_clock;

	}

	if ( strcmp(ALGORITMO_BALANCEO, "WCLOCK") == OK) {

		t_list* tabla_algoritmo_Wclock = generar_tabla_algoritmo_Wclock(lista_bloques_archivo, cantidad_bloques_archivo, DISPONIBILIDAD_BASE);
		aplicar_algoritmo_clock(lista_bloques_archivo, tabla_algoritmo_Wclock, DISPONIBILIDAD_BASE); // Actualiza la tabla_algoritmo_clock con los valores correspondientes
		log_info(log_YAMA, "La planificación del JOB %d se realizo correctamente segun el ALGORITMO WCLOCK: Se ha creado el plan de ejecución.", nro_job);
		return tabla_algoritmo_Wclock;

	}

	return NULL; // Hubo un error

} // FIN planificacion

/*
--------------------------------------------------
FIN - funciones genéricas de planificación
--------------------------------------------------
*/


