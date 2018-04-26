/*
 * algoritmo_WCLOCK.c
 *
 *  Created on: 4/12/2017
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
INICIO -  funciones ALGORITMO W-CLOCK
==================================================
*/


t_list* generar_tabla_algoritmo_Wclock(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, int DISPONIBILIDAD_BASE){

	t_list* lista_algoritmo_wclock = list_create(); // Es la lista (tabla) para el algoritmo W-Clock del JOB actual que voy a retornar

	t_Bloque* bloque_actual;
	bool existe_nodo;

	int i;
	for(i=0; i<cantidad_bloques_archivo; i++){

		bloque_actual = list_get(lista_bloques_archivo, i);

		/* Tengo que calcular (listar o conocer), de alguna manera, los nombres de los NODOS involucrados en este Job
		 * para armar la tabla del algoritmo Clock.
		 */
		if ( bloque_actual->copia0.existe_copia == true){
			chequear_nodo_Wclock(bloque_actual, lista_algoritmo_wclock, existe_nodo, DISPONIBILIDAD_BASE, COPIA_0);
		}

		if (bloque_actual->copia1.existe_copia == true){
			chequear_nodo_Wclock(bloque_actual, lista_algoritmo_wclock, existe_nodo, DISPONIBILIDAD_BASE, COPIA_1);
		}

	} // FIN for

	return lista_algoritmo_wclock;

} // FIN generar_tabla_algoritmo_Wclock


void chequear_nodo_Wclock(t_Bloque* bloque_actual, t_list* lista_algoritmo_wclock, bool existe_nodo, int DISPONIBILIDAD_BASE, int nro_copia){

	NOMBRE_NODO_ACTUAL = malloc(LONGITUD_NOMBRE_NODO);

	if ( nro_copia == COPIA_0){
		strcpy(NOMBRE_NODO_ACTUAL, bloque_actual->copia0.nombre_nodo);
	}else if ( nro_copia == COPIA_1){
		strcpy(NOMBRE_NODO_ACTUAL, bloque_actual->copia1.nombre_nodo);
	}

	existe_nodo = list_any_satisfy(lista_algoritmo_wclock, _ya_existe_este_nombre_de_nodo);

	if ( existe_nodo == false ){ // Si no tengo el nodo actual en mi lista del algoritmo entonces lo agrego

		t_entrada_algoritmo* entrada_actual = malloc(sizeof(t_entrada_algoritmo));

		strcpy(entrada_actual->nombre_nodo, NOMBRE_NODO_ACTUAL);
		entrada_actual->disponibilidad_worker = calcular_disponibilidad_worker_Wclock(entrada_actual->nombre_nodo, DISPONIBILIDAD_BASE); // Para el algoritmo WClock la A(w)= BASE + PWL(w)
		entrada_actual->puntero_clock = false; // Asumo que no tengo el puntero (caso general)
		entrada_actual->lista_bloques_asignados = list_create(); // Arranca sin bloques asignados (ver tabla del algoritmo en el ejemplo del TP)

		list_add(lista_algoritmo_wclock, entrada_actual); // Agrego la entrada_actual a la lista del algoritmo WClock
	}

	free(NOMBRE_NODO_ACTUAL);

} // FIN chequear_nodo_wclock


int calcular_disponibilidad_worker_Wclock(char* nombre_nodo, int DISPONIBILIDAD_BASE){

	/* Para el algoritmo wclock.
	 *
	 *  A(w) = DISPONIBILIDAD_BASE + PWL(w), donde PWL(w) = WLmax - WL(w) // Carga actual máxima entre
	 *  todos los WORKERS menos la carga actual del WORKER w
	 *
	 */

	// Obtengo el número de Nodo (WORKER) a partir del nombre_nodo
	int nro_worker_actual = obtener_numero_nodo(nombre_nodo);

	// A(w) = DISPONIBILIDAD_BASE + (WLmax - WL(w))
	int WLmax = calcular_WLmax();
	int disponibilidad_actual_worker = DISPONIBILIDAD_BASE + ( WLmax - tabla_global_workers[nro_worker_actual].carga_actual );

	return disponibilidad_actual_worker;

}


int calcular_WLmax(){

	// Calculamos la máxima carga actual entre todos los Nodos (WORKERS)

	int WLmax = MAXIMA_CARGA_ACTUAL; // La defino yo como un valor bajo (-1)

	int nro_worker;
	for(nro_worker = 1; nro_worker < CANTIDAD_WORKERS; nro_worker++){

		if ( tabla_global_workers[nro_worker].carga_actual > WLmax){
			WLmax = tabla_global_workers[nro_worker].carga_actual;
		}

	} // fin for

	return WLmax;
}

/*
--------------------------------------------------
FIN - funciones ALGORITMO W-CLOCK
--------------------------------------------------
*/


