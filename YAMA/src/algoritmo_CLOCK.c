/*
 * algoritmo_CLOCK.c
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
INICIO - funciones ALGORITMO CLOCK
==================================================
*/

/* La tabla para el algoritmo de planificación (sea Clock o W-Clock) es única para cada Job, así que debemos generarla
 * por cada Job nuevo.
 *
 * Ejemplo:
 *
 * A(w) | Worker | Bloques
 * 2    |    1   |   /
 * 2    |    2   |   /
 * 2    |    3   |   /
 *
 * Cada entrada de esta tabla es de tipo t_entrada_algoritmo (genérico para ambos algoritmos)
 */

// @OBSERVACION: Es una tabla implementada como una lista

t_list* generar_tabla_algoritmo_clock(t_list* lista_bloques_archivo, int cantidad_bloques_archivo, int DISPONIBILIDAD_BASE){

	t_list* lista_algoritmo_clock = list_create(); // Es la lista (tabla) para el algoritmo Clock del JOB actual que voy a retornar

	t_Bloque* bloque_actual;
	bool existe_nodo;

	int i;
	for(i=0; i<cantidad_bloques_archivo; i++){

		bloque_actual = list_get(lista_bloques_archivo, i);

		/* Tengo que calcular (listar o conocer), de alguna manera, los nombres de los NODOS involucrados en este Job
		 * para armar la tabla del algoritmo Clock.
		 */
		if ( bloque_actual->copia0.existe_copia == true){
			chequear_nodo(bloque_actual, lista_algoritmo_clock, existe_nodo, DISPONIBILIDAD_BASE, COPIA_0);
		}

		if (bloque_actual->copia1.existe_copia == true){
			chequear_nodo(bloque_actual, lista_algoritmo_clock, existe_nodo, DISPONIBILIDAD_BASE, COPIA_1);
		}

	} // FIN for

	return lista_algoritmo_clock;

} // FIN generar_tabla_algoritmo_clock


bool bloque_actual_esta_en_worker_de_entrada_actual(t_entrada_algoritmo* entrada_actual, int numero_bloque_actual, t_list* lista_bloques_archivo){

	// Me fijo si en el Nodo (WORKER) de la entrada_actual está el bloque_actual

	// 1° - Obtengo el numero de Nodo (WQRKER) de la entrada_actual
	int numero_nodo_entrada_actual = obtener_numero_nodo(entrada_actual->nombre_nodo);

	// 2° - Como la lista_bloques_archivo está ordenada (indexada) por número de bloque, es decir, el nodo 0 tiene la
	// info del bloque 0, el nodo 2 la info del bloque 1 y así sucesivamente, simplemente hago:
	t_Bloque* bloque_auxiliar = list_get(lista_bloques_archivo, numero_bloque_actual);

	// En bloque_auxiliar obtengo la info donde está guardado el bloque_actual del archivo

	// 3° - Ahora me fijo si el numero_nodo_entrada_actual es igual al numero_nodo donde está el bloque actual
	if ( obtener_numero_nodo(bloque_auxiliar->copia0.nombre_nodo) == numero_nodo_entrada_actual || obtener_numero_nodo(bloque_auxiliar->copia1.nombre_nodo) == numero_nodo_entrada_actual ){
		return true;
	}else{
		return false;
	}

} // FIN bloque_actual_esta_en_worker_de_entrada_actual


bool disponibilidad_worker_entrada_actual_es_mayor_a_cero(t_entrada_algoritmo* entrada_actual){

	if ( entrada_actual->disponibilidad_worker > 0){
		return true;
	}else{
		return false;
	}

} // FIN disponibilidad_worker_entrada_actual_es_mayor_a_cero


void aplicar_algoritmo_clock(t_list* lista_bloques_archivo, t_list* tabla_algoritmo_clock, int DISPONIBILIDAD_BASE){

	/*** (1) - INICIO: Se hace una única vez por cada Job ***/

	// - Calcular la disponibilidad de cada WORKER(NODO): /LISTO (lo hace generar_tabla_algoritmo_clock)

	/* - Posicionar el clock (puntero) en el WORKER con mayor disponibilidad. Si hay empate (lo que siempre va a
	 * suceder en Clock, pues la disponibilidad de cada WORKER es simplemente igual al valor de DISPONIBILIDAD_BASE) se elige,
	 * para darle el puntero, el WORKER con menor carga histórica. /LISTO
	 */

	if ( chequear_todos_los_nodos_tienen_la_misma_disponibilidad(tabla_algoritmo_clock) == false){ // Si los Nodos tienen diferente disponibilidad...
		posicionar_clock_en_nodo_mayor_disponibilidad(tabla_algoritmo_clock); // ... defino por disponibilidad
	}else{
		posicionar_clock_en_nodo_menor_carga_historica(tabla_algoritmo_clock); // ... sino defino por carga histórica (desampata FIFO)
	}

	/*** (2)  - PASO 2: Se hace una vez por cada bloque del archivo a procesar ***/

	/* Ahora necesito saber cual es bloque_actual. Para ello calculo la cantidad de bloques y empiezo por el
	 * bloque 0.
	 */

	t_entrada_algoritmo* entrada_con_puntero;
	int numero_bloque_actual;
	t_bloque_a_procesar* t_numero_bloque_actual;

	int cantidad_bloques = list_size(lista_bloques_archivo);
	numero_bloque_actual = 0; // Arranco con el bloque 0 del archivo a procesar

	while(numero_bloque_actual < cantidad_bloques){ // INICIO while PASO 2

			// if PRINCIPAL
			if ( bloque_actual_esta_en_worker_de_entrada_con_puntero(numero_bloque_actual, tabla_algoritmo_clock, lista_bloques_archivo) == true ){
				if ( disponibilidad_worker_entrada_con_puntero_es_mayor_a_cero(tabla_algoritmo_clock) == true ){

					/* Entonces le asigno al Nodo (WORKER) de la entrada donde apunta el puntero clock
					 * el bloque_actual.
					 */
					entrada_con_puntero = devolver_entrada_con_puntero_clock(tabla_algoritmo_clock);

					// Asigno el bloque_actual al WORKER de la entrada a la que apunta el puntero_clock
					t_numero_bloque_actual = malloc(sizeof(t_bloque_a_procesar));
					t_numero_bloque_actual->nro_bloque = numero_bloque_actual;
					list_add(entrada_con_puntero->lista_bloques_asignados, t_numero_bloque_actual);

					// Marco la copia del bloque_actual elegida para ser transformada
					marcar_copia_elegida_para_transformar(lista_bloques_archivo, entrada_con_puntero, numero_bloque_actual);

					//Luego, disminuyo su disponiblidad en 1 unidad.
					entrada_con_puntero->disponibilidad_worker--;

					// Incremento la carga actual e histórica en 1 del Nodo de esta entrada
					incrementar_carga_por_transformacion(entrada_con_puntero);

					// Avanzo el clock a la siguiente entrada de la tabla
					avanzar_clock_a_la_siguiente_entrada(entrada_con_puntero, tabla_algoritmo_clock);

					/* FIN_PLANIFICACION_BLOQUE_ACTUAL: Ya asigne un Nodo (WORKER) para procesar el bloque_actual.
					 * Por lo tanto, pongo el flag del 2do while en true para salir y aumento numero_bloque_actual
					 * para pasar a analizar el siguiente bloque del archivo.
					 */
					numero_bloque_actual++;

				}else{
					/* Si el bloque_actual está en el Nodo (WORKER) de la entrada que tiene el puntero
					 * pero su disponibilidad ES CERO, entonces se debe restaurar la disponibilidad
					 * del worker al valor de DISPONIBILIDAD_BASE y luego avanzar el clock a la siguiente
					 * entrada de la tabla del algoritmo, repitiendo el PASO 2.
					 */

					// Obtengo la entrada (t_entrada_algoritmo) a la que apunta el puntero_clock
					entrada_con_puntero = devolver_entrada_con_puntero_clock(tabla_algoritmo_clock);

					// Restauro la disponibilidad del Nodo (WORKER) de la entrada_con_puntero a A(w)=DISPONIBILIDAD_BASE
					entrada_con_puntero->disponibilidad_worker = DISPONIBILIDAD_BASE;

					// Avanzo el clock a la siguiente entrada de la tabla
					avanzar_clock_a_la_siguiente_entrada(entrada_con_puntero, tabla_algoritmo_clock);

					/* OBSERVACION: En este punto NO asigne el bloque_actual, así que no aumento el numero_bloque_actual,
					 * sino que debo seguir intentando hasta que pueda asignarle el bloque_actual a un WORKER. En
					 * otras palabras, tengo que REPETIR EL PASO 2.
					 */

				} // Fin else el bloque_actual está en el Nodo (WORKER) de la entrada del puntero pero su disponibilidad no es mayor a cero

			}  // FIN if PRINCIPAL - bloque_actual_esta_en_worker_de_entrada_con_puntero() == true

			else{ // else PRINCIPAL: Si el bloque_actual NO está en el worker_de_entrada_con_puntero

				/* Busco la siguiente entrada con un worker que contenga al bloque_actual y también tenga A(w)>0.
				 *
				 */

				// Mi punto de partida es la entrada a donde apunta el puntero_clock
				entrada_con_puntero = devolver_entrada_con_puntero_clock(tabla_algoritmo_clock);

				int indice_entrada_con_puntero = calcular_indice_de_entrada_con_puntero(tabla_algoritmo_clock);
				t_entrada_algoritmo* entrada_actual;
				bool ya_asigne_bloque_actual = false;

				// Arranco desde la siguiente entrada a la que tiene el puntero_clock
				int indice_actual;
				while (true){ // INICIO while(true)

					/* Barro desde la entrada siguiente a la que tiene el puntero_clock hasta el final de la
					 * tabla del algoritmo (lista).
					 *
					 * CUIDADO: Si el indice_entrada_con_puntero es el último de la tabla (lista) y yo le sumo 1,
					 * me estoy yendo fuera de la lista (no existe ese elemento). Sin embargo, lo que va a pasar
					 * es que no va a entrar al for.
					 */

					int indice_entrada_siguiente_a_la_del_puntero = indice_entrada_con_puntero + 1;

					for(indice_actual = indice_entrada_siguiente_a_la_del_puntero; indice_actual < list_size(tabla_algoritmo_clock); indice_actual++){

						entrada_actual = list_get(tabla_algoritmo_clock, indice_actual);
						if ( bloque_actual_esta_en_worker_de_entrada_actual(entrada_actual, numero_bloque_actual, lista_bloques_archivo) == true ){
							if ( disponibilidad_worker_entrada_actual_es_mayor_a_cero(entrada_actual) == true ){

								// Asigno el bloque_actual al Nodo (WORKER) de la entrada_actual
								t_numero_bloque_actual = malloc(sizeof(t_bloque_a_procesar));
								t_numero_bloque_actual->nro_bloque = numero_bloque_actual;
								list_add(entrada_actual->lista_bloques_asignados, t_numero_bloque_actual);

								// Marco la copia del bloque_actual elegida para ser transformada
								marcar_copia_elegida_para_transformar(lista_bloques_archivo, entrada_actual, numero_bloque_actual);

								// Disminuyo la disponibilidad del WORKER de la entrada_actual
								entrada_actual->disponibilidad_worker--;

								// Incremento la carga actual e histórica en 1 del Nodo de esta entrada
								incrementar_carga_por_transformacion(entrada_actual);

								/* FIN_PLANIFICACION_BLOQUE_ACTUAL: Ya le asigne un Nodo (WORKER) al bloque_actual.
								 * Por lo tanto, pongo el flag del 2do while en true para salir y aumento numero_bloque_actual
								 * para pasar a analizar el siguiente bloque del archivo.
								 */

								numero_bloque_actual++; // Paso al siguiente bloque a procesar
								ya_asigne_bloque_actual = true; // Este flag es para no entrar al 2do for y salir del while(true)

								break; // Ya asigne el bloque a un WORKER así que salgo de ESTE for
							}
						}

					} // Fin 1er for: Si salgo correctamente del 1er for significa que NO asigne el bloque_actual todavía

					if ( ya_asigne_bloque_actual == true){
						break; // Salgo del while(true)
					}

					/* Barro desde el principio de la tabla (lista) del algoritmo hasta la entrada donde
					 * está el clock del puntero actual, es decir, si completo este 2do for y caigo en la
					 * entrada donde apunta el puntero_clock habré dado una vuelta completa.
					 */
					for(indice_actual = 0; indice_actual < indice_entrada_con_puntero; indice_actual++){

						entrada_actual = list_get(tabla_algoritmo_clock, indice_actual);
						if ( bloque_actual_esta_en_worker_de_entrada_actual(entrada_actual, numero_bloque_actual, lista_bloques_archivo ) == true ){
							if ( disponibilidad_worker_entrada_actual_es_mayor_a_cero(entrada_actual) == true ){

								// Asigno el numero_bloque_actual al Nodo (WORKER) de la entrada_actual
								t_numero_bloque_actual = malloc(sizeof(t_bloque_a_procesar));
								t_numero_bloque_actual->nro_bloque = numero_bloque_actual;
								list_add(entrada_actual->lista_bloques_asignados, t_numero_bloque_actual);

								// Marco la copia del bloque_actual elegida para ser transformada
								marcar_copia_elegida_para_transformar(lista_bloques_archivo, entrada_actual, numero_bloque_actual);

								// Disminuyo la disponibilidad del WORKER de la entrada_actual
								entrada_actual->disponibilidad_worker--;

								// Incremento la carga actual e histórica en 1 del Nodo de esta entrada
								incrementar_carga_por_transformacion(entrada_actual);

								/* FIN_PLANIFICACION_BLOQUE_ACTUAL:Ya le asigne un Nodo (WORKER) al bloque_actual.
								 * Por lo tanto, pongo el flago del 2do while en true para salir y aumento numero_bloque_actual
								 * para pasar a analizar el siguiente bloque del archivo.
								 */

								numero_bloque_actual++; // Paso al siguiente bloque a procesar
								ya_asigne_bloque_actual = true; // Este flag es para salir del while(true)

								break; // Ya asigne el bloque a un WORKER así que salgo de ESTE 2do for
							}
						}

					} // Fin 2do for

					if ( ya_asigne_bloque_actual == true){
						break; // Salgo del while(true)
					}

					/* Si llego a este punto significa significa que recorrí todas las entradas de la tabla y ninguna
					 * tiene disponibilidad, es decir, di una vuelta completa y no pude asignar el bloque actual. En
					 * este caso, el algoritmo dice que hay que seter la disponibilidad de TODOS los WORKERs al valor de
					 * DISPONIBILIDAD_BASE.
					 */

					// Restauro la disponibilidad de TODOS los WORKERS a DISPONIBILIDAD BASE
					for(indice_actual = 0; indice_actual < list_size(tabla_algoritmo_clock); indice_actual++){

						entrada_actual = list_get(tabla_algoritmo_clock, indice_actual);
						entrada_actual->disponibilidad_worker = DISPONIBILIDAD_BASE;

					} // FIN for

					break; // Salgo del while(true) y vuelvo a arrancar desde el WHILE PRINCIPAL


				} // FIN while(true)

			} // FIN else - Si el bloque_actual NO está en el worker_de_entrada_con_puntero

	}  // FIN WHILE PRINCIPAL - while(numero_bloque_actual < cantidad_bloques)


	/* OBSERVACION: Para este punto voy a tener la tabla_algoritmo_clock completa con la asignación de los
	 * bloques en los correspondientes WORKER (habrá concluido la planificación de este Job).
	 */


} //*** FIN aplicar_algoritmo_clock


/*
--------------------------------------------------
FIN - funciones ALGORITMO CLOCK
--------------------------------------------------
*/
