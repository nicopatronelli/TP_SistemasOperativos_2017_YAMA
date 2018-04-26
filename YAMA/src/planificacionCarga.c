/*
 * planificacionCarga.c
 *
 *  Created on: 8/12/2017
 *      Author: utnso
 */

#include "planificacionYAMA.h"

t_worker tabla_global_workers[CANTIDAD_WORKERS];

void incrementar_carga_por_transformacion(t_entrada_algoritmo* entrada_actual){

	/* IMPORTANTE: Actualizo la carga_actual y la carga_histórica del Nodo. Se le suma una
	 * unidad (1) por cada bloque que se le asigna a un Nodo (WORKER) para que procese. Por
	 * reducción local NO se adiciona carga_actual.
	 */

	int numero_nodo = obtener_numero_nodo(entrada_actual->nombre_nodo);
	tabla_global_workers[numero_nodo].carga_actual++;
	tabla_global_workers[numero_nodo].carga_historica++;

}
