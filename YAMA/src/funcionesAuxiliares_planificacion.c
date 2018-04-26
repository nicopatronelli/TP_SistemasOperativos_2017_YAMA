/*
 * funcionesAuxiliares_planificacion.c
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

bool _ya_existe_este_nombre_de_nodo(void* entrada_actual){

	int existe = strcmp(((t_entrada_algoritmo*)entrada_actual)->nombre_nodo, NOMBRE_NODO_ACTUAL);

	if ( existe == 0)
		return true; // NOMBRE_NODO_ACTUAL ya existe en la entrada_actual de la tabla del algoritmo
	else
		return false; // NOMBRE_NODO_ACTUAL no existe en la entrada_actual de la tabla del algoritmo

} // FIN _ya_existe_este_nombre_de_nodo


int obtener_numero_nodo(char* nombre_nodo){ // Convierte del formato "NODON" a N. Ej: Si recibe el char* "NODO1" retorna el entero 1

	char* numero_nodo_string;
	int numero_nodo;

	numero_nodo_string = string_substring_from(nombre_nodo, 4); // Me quedo solo con el número del nodo (todavía es un char*)

	numero_nodo = atoi(numero_nodo_string); // Convierto de char* a int

	free(numero_nodo_string);

	return numero_nodo;

} // FIN obtener_numero_nodo


bool _chequear_igual_disponibilidad(void* entrada_actual){

	if ( ((t_entrada_algoritmo*)entrada_actual)->disponibilidad_worker == DISPONIBILIDAD_ACTUAL )
		return true; // La disponibilidad del Nodo de la entrada actual es igual a la DISPONIBILIDAD_ACTUAL
	else
		return false; // La disponibilidad del Nodo de la entrada actual es DISTINTA a la DISPONIBILIDAD_ACTUAL

} // FIN _chequear_igual_disponibilidad


bool _entrada_tiene_puntero_clock(void* entrada) {

	return ((t_entrada_algoritmo*)entrada)->puntero_clock == true;

 } // FIN _entrada_tiene_puntero_clock


int calcular_indice_de_entrada_con_puntero(t_list* tabla_algoritmo_clock){

	// Busco el nodo (t_entrada_algoritmo) cuyo campo puntero_clock es true
	bool flag = false;
	int nro_entrada = 0; // Arranca por el primer elemento de la tabla_algoritmo_clock (lista en realidad)
	t_entrada_algoritmo* entrada_actual;

	while( flag != true){

		entrada_actual = list_get(tabla_algoritmo_clock, nro_entrada);
		if ( entrada_actual->puntero_clock == true){
			flag = true;
		}else{
			nro_entrada++;
		}

	} // Fin while

	return nro_entrada; // Es el indice de la tabla (lista) del algoritmo de la entrada que posee el puntero_clock en true

} // FIN calcular_indice_de_entrada_con_puntero


char* obtener_nodo_asignado_al_bloque(int bloque_actual, t_list* tabla_algoritmo_clock){

	t_entrada_algoritmo* entrada_actual;
	t_bloque_a_procesar* bloque_asignado;

	int nro_entrada;
	for(nro_entrada = 0; nro_entrada<list_size(tabla_algoritmo_clock); nro_entrada++){
		// Me paro en una entrada de la tabla
		entrada_actual = list_get(tabla_algoritmo_clock, nro_entrada);

		int bloque_auxiliar;
		for(bloque_auxiliar = 0; bloque_auxiliar < list_size(entrada_actual->lista_bloques_asignados); bloque_auxiliar++){
			// Recorro todos los bloques asignados que tiene dicha entrada
			bloque_asignado = list_get(entrada_actual->lista_bloques_asignados, bloque_auxiliar);
			if ( bloque_asignado->nro_bloque == bloque_actual ){
				return entrada_actual->nombre_nodo;
			}
		} // FIN for interno

	} // FIN for externo

	/* OBSERVACION: Como a todos los bloques se les asigna un Nodo, esta función nunca podría pasar por
	 * este return. Lo pongo para eliminar el warning de "no retorno en función con retorno distinto de void"
	 */
	return "ERROR";

} // FIN obtener_nodo


char* obtener_nombre_nodo_entrada_actual(t_entrada_algoritmo* entrada_actual){

	return entrada_actual->nombre_nodo;

} // FIN obtener_nodo_entrada_actual


void marcar_copia_elegida_para_transformar(t_list* lista_bloques_archivo, t_entrada_algoritmo* entrada_actual, int numero_bloque_actual){

	t_Bloque* bloque_actual = list_get(lista_bloques_archivo, numero_bloque_actual);

	char* nombre_nodo_elegido = malloc(10); // NODONN
	strcpy(nombre_nodo_elegido, obtener_nombre_nodo_entrada_actual(entrada_actual));

	if ( strcmp(bloque_actual->copia0.nombre_nodo, nombre_nodo_elegido) == 0 ){
		// Se elegio la copia0 del bloque_actual para transformar
		bloque_actual->copia0.copia_elegida_para_transformar = true;
		bloque_actual->copia1.copia_elegida_para_transformar = false;
	}else if ( strcmp(bloque_actual->copia1.nombre_nodo, nombre_nodo_elegido) == 0 ) {
		// Se elegio la copia1 del bloque_actual para transformar
		bloque_actual->copia0.copia_elegida_para_transformar = false;
		bloque_actual->copia1.copia_elegida_para_transformar = true;
	}else{
		// Dado que SI o SI se elige una de las dos copias del bloque_actual, el flujo de ejecución nunca debería llega hasta acá
		log_error(log_YAMA, "OCURRIO UN ERROR AL MARCAR LA COPIA ELEGIDA PARA TRANSFORMAR DENTRO DE PLANIFICACION.");
	}

	free(nombre_nodo_elegido);

} // FIN marcar_copia_elegida_para_transformar
