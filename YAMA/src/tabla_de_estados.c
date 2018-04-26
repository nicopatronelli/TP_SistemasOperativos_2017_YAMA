/*
 * tabla_de_estados.c
 *
 *  Created on: 12/12/2017
 *      Author: utnso
 */

#include "funcionesYAMA.h"
#include "planificacionYAMA.h"

/*
==========================================================
INICIO - Funciones para la TABLA DE ESTADOS
==========================================================
*/

char* _generar_nombre_temporal_unico(){

	/* [BUENA PRACTICA DE PROGRAMACION]: En lugar de preocuparme YO de incrementar este indice cada vez
	 * que llamo a la función (ya sea antes o después), como es una variable global la incremento desde
	 * dentro mismo de la función. Así, siempre que llame a _generar_nombre_temporal_unico() tengo
	 * la certeza que me dará un nombre unívoco sin tener que hacer nada más.
	 *
	 */
	indice_nombre_temporal++; // Uso la variable global indice_nombre_temporal para generar nombres temporales unicos

	// EJEMPLO: tmp_1
	char* indice_nombre_temporal_string = string_itoa(indice_nombre_temporal);
	char* nombre_temporal_unico = malloc(strlen("tmp_") + strlen(indice_nombre_temporal_string) + 1); // + 1 del '\0'

	strcpy(nombre_temporal_unico, "tmp_");
	strcat(nombre_temporal_unico, indice_nombre_temporal_string);

	free(indice_nombre_temporal_string);

	return nombre_temporal_unico;

} // FIN _generar_nombre_temporal_unico


void actualizar_tabla_estados(int cantidad_bloques_archivo, int nro_job, int numero_master, t_list* tabla_algoritmo_clock, t_list* lista_bloques_archivo){

	t_Bloque* bloque_actual;

	int nro_bloque_actual;
	for(nro_bloque_actual = 0; nro_bloque_actual < cantidad_bloques_archivo; nro_bloque_actual++){

			t_entrada_estado* entrada_estado_actual = malloc(sizeof(t_entrada_estado));

			entrada_estado_actual->job = nro_job;
			entrada_estado_actual->master = numero_master;
			strcpy(entrada_estado_actual->nodo, obtener_nodo_asignado_al_bloque(nro_bloque_actual, tabla_algoritmo_clock));
			entrada_estado_actual->bloque = nro_bloque_actual;
			strcpy(entrada_estado_actual->etapa, "TRANSFORMACION");
			char* nombre_temporal_unico = _generar_nombre_temporal_unico();
			strcpy(entrada_estado_actual->nombre_temporal, nombre_temporal_unico);
			free(nombre_temporal_unico);
			entrada_estado_actual->estado = malloc(strlen("En proceso") + 1); // + 1 del '\0'
			strcpy(entrada_estado_actual->estado, "En proceso");

			list_add(tabla_estados, entrada_estado_actual);

			bloque_actual = list_get(lista_bloques_archivo, nro_bloque_actual);
			if ( bloque_actual->copia0.copia_elegida_para_transformar == true ){
				strcpy(bloque_actual->copia0.nombre_temporal_trans, entrada_estado_actual->nombre_temporal);
			}else if ( bloque_actual->copia1.copia_elegida_para_transformar == true ){
				strcpy(bloque_actual->copia1.nombre_temporal_trans, entrada_estado_actual->nombre_temporal);
			}else{
				// El flujo NUNCA debería pasar por acá, ya que SI o SI se elige una copia de cada bloque del archivo
				log_error(log_YAMA, "Hubo un error en la función actualizar_tabla_estados de funcionesYAMA.c");
			}

	} // FIN for

} // FIN actualizar_tabla_estados


void grabar_tabla_estados_en_archivo(){

	// Me bajo la versión actual de la tabla de estados a un archivo (lo piso constantemente)

	FILE* tabla_estados_archivo = fopen("tabla_estados_yama.csv", "w+");

	fprintf(tabla_estados_archivo, "JOB, MASTER, NODO, BLOQUE, ETAPA, ARCHIVO TEMPORAL, ESTADO\n");

	t_entrada_estado* entrada_auxiliar;
	int i;
	for(i=0; i<list_size(tabla_estados); i++){
		entrada_auxiliar = list_get(tabla_estados, i);
		fprintf(tabla_estados_archivo, "%d, %d, %s, %d, %s, %s, %s\n", entrada_auxiliar->job, entrada_auxiliar->master, entrada_auxiliar->nodo, entrada_auxiliar->bloque, entrada_auxiliar->etapa, entrada_auxiliar->nombre_temporal, entrada_auxiliar->estado);
	}

	fclose(tabla_estados_archivo);

	log_info(log_YAMA, "[IMPORTANTE]: Se ha actualizado el archivo con la tabla de estados: tabla_estados_yama.csv");

} // FIN imprimir_tabla_estados


bool _entrada_estado_tiene_nro_job_y_nro_bloque(void* entrada) {

	return ( ((t_entrada_estado*)entrada)->job == nro_job_actual && ((t_entrada_estado*)entrada)->bloque == nro_bloque_archivo_transformado_ok );

} // FIN _entrada_estado_tiene_nro_job_y_nro_bloque


void actualizar_tabla_estados_bloque_ok(int nro_job, int nro_bloque){

	t_entrada_estado* entrada_a_actualizar;

	entrada_a_actualizar = list_find(tabla_estados, _entrada_estado_tiene_nro_job_y_nro_bloque);

	/* En este punto el entrada_a_actualizar->estado tiene hecho malloc(strlen("En proceso") + 1).
	 * Por lo tanto, primero hacemos free y luego reservamos memoria para copiar "OK".
	 *
	 */
	free(entrada_a_actualizar->estado);

	entrada_a_actualizar->estado = malloc(strlen("OK") + 1); // +1 del '\0'
	strcpy( entrada_a_actualizar->estado, "OK" );


} // FIN actualizar_tabla_estados_bloque_ok

void inicializar_lista_nodos(){

	lista_nodos = list_create();

	/* No voy a utilizar el primer nodo de la lista (elemento de indice 0), para así arrancar en el
	 * indice 1 y hacer coincidir el indice de la lista de nodos con el número de Nodo correspondiente.
	 * Por lo tanto, cargo el primero nodo como un Nodo vacío.
	 */
	t_nodo* nodo_vacio = malloc(sizeof(t_nodo));

	nodo_vacio->nombre_nodo = malloc(strlen("NODON") + 1); // + 1 del '\0'
	strcpy(nodo_vacio->nombre_nodo, "NODON");
	nodo_vacio->ip = malloc(strlen("0") + 1); // + 1 del '\0'
	strcpy(nodo_vacio->ip, "0");
	nodo_vacio->puerto_worker_escucha_master = 0;

	list_add(lista_nodos, nodo_vacio);

}


bool _ya_existe_el_nodo(void* nodo_actual){

	int afirmativo = 0;
	afirmativo = strcmp( ((t_nodo*)nodo_actual)->nombre_nodo, nombre_nodo_auxiliar );
	if ( afirmativo == 0 ){
		return true; // El Nodo cuyo nombre es nombre_nodo_auxiliar ya esta en la lista de nodos
	}else{
		return false; // El Nodo cuyo nombre es nombre_nodo_auxiliar NO existe aún en la lista de nodos
	}

} // FIN _ya_existe_el_nodo


void actualizar_lista_nodos(t_list* lista_bloques_archivo){

	int nro_bloque;
	t_Bloque* bloque_actual;

	for(nro_bloque = 0; nro_bloque < list_size(lista_bloques_archivo); nro_bloque++){

		bloque_actual = list_get(lista_bloques_archivo, nro_bloque);

		// 1° Primero reviso la copia0 del bloque_actual

		// Hago que nombre_nodo_auxiliar apunte al valor de bloque_actual->copia0.nombre_nodo para usar list_any_satisfy
		nombre_nodo_auxiliar = bloque_actual->copia0.nombre_nodo;

		if ( list_any_satisfy(lista_nodos, _ya_existe_el_nodo) == false ){

			// Si todavía no tengo el nombre nodo donde esta la copia0 del bloque actual en mi lista entonces lo agrego

			// Aloco memoria para el nuevo nodo
			t_nodo* nuevo_nodo = malloc(sizeof(t_nodo));

			// Copio el nombre del Nodo
			nuevo_nodo->nombre_nodo = malloc(strlen(bloque_actual->copia0.nombre_nodo) + 1); // +1 del '\0'
			strcpy(nuevo_nodo->nombre_nodo, bloque_actual->copia0.nombre_nodo);

			// Copio la ip del Nodo
			nuevo_nodo->ip = malloc(strlen(bloque_actual->copia0.ip_nodo) + 1); // +1 del '\0'
			strcpy(nuevo_nodo->ip, bloque_actual->copia0.ip_nodo);

			// Copio el puerto de escucha de MASTERs del Nodo (WORKER)
			nuevo_nodo->puerto_worker_escucha_master = bloque_actual->copia0.puerto_worker_escucha_master;

			// Añado el nuevo nodo a mi lista de nodos
			list_add(lista_nodos, nuevo_nodo);

		} // FIN chequeo copia0 del bloque actual

		/* 2° Ahora reviso la copia1 del bloque_actual: Tengo que comparar contra nombre_nodo, ¿pero también contra
		 * bloque_actual->copia0.nombre_nodo, pues puede que ya lo haya agregado a la lista en el paso anterior...?
		 * !NOOOO! Pues la copia0 y copia1 de un mismo bloque NUNCA van a estar en un mismo Nodo.
		 */

		// Hago que nombre_nodo_auxiliar apunte al valor de bloque_actual->copia0.nombre_nodo para usar list_any_satisfy
		nombre_nodo_auxiliar = bloque_actual->copia1.nombre_nodo;

		if ( list_any_satisfy(lista_nodos, _ya_existe_el_nodo) == false ){

			// Si todavía no tengo el nombre nodo donde esta la copia1 del bloque actual en mi lista entonces lo agrego

			// Aloco memoria para el nuevo nodo
			t_nodo* nuevo_nodo = malloc(sizeof(t_nodo));

			// Copio el nombre del Nodo
			nuevo_nodo->nombre_nodo = malloc(strlen(bloque_actual->copia1.nombre_nodo) + 1); // +1 del '\0'
			strcpy(nuevo_nodo->nombre_nodo, bloque_actual->copia1.nombre_nodo);

			// Copio la ip del Nodo
			nuevo_nodo->ip = malloc(strlen(bloque_actual->copia1.ip_nodo) + 1); // +1 del '\0'
			strcpy(nuevo_nodo->ip, bloque_actual->copia1.ip_nodo);

			// Copio el puerto de escucha de MASTERs del Nodo (WORKER)
			nuevo_nodo->puerto_worker_escucha_master = bloque_actual->copia1.puerto_worker_escucha_master;

			// Añado el nuevo nodo a mi lista de nodos
			list_add(lista_nodos, nuevo_nodo);

		} // FIN chequeo copia1 del bloque actual

	} //FIN for

} // FIN agregar_nodo_si_NO_esta


bool _entrada_nodo_y_job_buscada(void* entrada_estado_actual){

	// Si la entrada actual de la tabla de estados tiene el mismo nombre que nombre_nodo_reduccion_local ok_1 vale 0
	int ok_1 = strcmp( ((t_entrada_estado*)entrada_estado_actual)->nodo, nombre_nodo_reduccion_local);

	// Si la entrada actual de la tabla de estados tiene el mismo nro de job que nro_job_actual ok_1 vale 1
	int ok_2 = ( ((t_entrada_estado*)entrada_estado_actual)->job == nro_job_actual );

	if ( ok_1 == 0 && ok_2 == 1 ){
		return true; // Es una entrada del tipo NODO = nombre_nodo_reduccion_local & JOB = nro_job_actual
	}else{
		return false; // No es la entrada que busco
	}

} // FIN _entrada_nodo_y_job_buscada


bool _transformacion_ok(void* entrada_estado_actual){

	if ( strcmp( ((t_entrada_estado*)entrada_estado_actual)->estado, "OK" ) == 0){
		return true;
	}else{
		return false;
	}

} // FIN _transformacion_ok


bool chequear_nodo_iniciar_reduccion_local(char* nombre_nodo_actual, int nro_job_actual){

	/* nro_job_actual es una variable global. La paso por parámetro para hacer enfásis de que la estoy
	 * usandod dentro de la función.
	 */

	// Hago que nombre_nodo_reduccion_local (variable global para list_filter) apunte al nombre_nodo_actual
	nombre_nodo_reduccion_local = nombre_nodo_actual;

	t_list* lista_auxiliar = list_filter(tabla_estados, _entrada_nodo_y_job_buscada);

	bool puedo_iniciar_reduccion_local_en_nodo = list_all_satisfy(lista_auxiliar, _transformacion_ok);

	free(lista_auxiliar);

	if ( puedo_iniciar_reduccion_local_en_nodo == true){
		return true; // Se puede iniciar la REDUCCION_LOCAL en dicho nodo (para este job)
	}else{
		return false;
	}

} // FIN chequear_nodo_iniciar_reduccion_local


t_list* lista_nombres_temporales_trans_nodo_reduccion_local(char* nombre_nodo_actual, int nro_job_actual){

	// nombre_nodo_reduccion_local es una variable global con el nombre del Nodo donde se va a iniciar la reducción local
	nombre_nodo_reduccion_local = nombre_nodo_actual;

	// Me armo una lista auxiliar donde solo tenga las entradas con Job = nro_job_actual y Nodo = nombre_nodo_reduccion_local
	t_list* lista_auxiliar = list_filter(tabla_estados, _entrada_nodo_y_job_buscada);

	// Creo una lista donde me voy a guardar los nombres de los archivos temporales transformados
	t_list* lista_nombres_temporales = list_create();

	int indice_lista_auxiliar;
	for( indice_lista_auxiliar = 0; indice_lista_auxiliar < list_size(lista_auxiliar); indice_lista_auxiliar++ ){

		// Recupero la entrada de la tabla de estados actual (perteneciente a las que filtre)
		t_entrada_estado* entrada_estado_actual = list_get(lista_auxiliar, indice_lista_auxiliar);

		// Aloco memoria para un struct de t_nombre_temporal
		t_nombre_temporal* t_nombre_temporal_actual = malloc(sizeof(t_nombre_temporal));

		// Aloco memoria para almacenar el nombre temporal de la transformación en el único campo de dicha estructura
		t_nombre_temporal_actual->nombre_temporal = malloc(strlen(entrada_estado_actual->nombre_temporal) + 1); // + 1 del '\0'

		// Ya puedo copiar el nombre temporal de la transformación
		strcpy(t_nombre_temporal_actual->nombre_temporal, entrada_estado_actual->nombre_temporal);

		// Añado el elemento t_nombre_temporal_actual a mi lista de nombres temporales
		list_add(lista_nombres_temporales, t_nombre_temporal_actual);

	} // FIN for

	return lista_nombres_temporales;

} // FIN lista_nombres_temporales_trans_nodo_a_reducir

char* agregar_entrada_reduccion_local_tabla_estados(int nro_job_actual, int fd_master, char* nombre_nodo_actual){

	t_entrada_estado* nueva_entrada = malloc(sizeof(t_entrada_estado));

	nueva_entrada->job = nro_job_actual;
	nueva_entrada->master = master_actual(lista_MASTERs, fd_master); // lista_MASTERs es una lista global
	strcpy(nueva_entrada->nodo, nombre_nodo_actual);
	nueva_entrada->bloque = -1;
	strcpy(nueva_entrada->etapa, "REDUCCION LOCAL");

	char* nombre_temporal = _generar_nombre_temporal_unico();
	strcpy(nueva_entrada->nombre_temporal, nombre_temporal);
	free(nombre_temporal);

	nueva_entrada->estado = malloc(strlen("En proceso") + 1); // + 1 del '\0'
	strcpy(nueva_entrada->estado, "En proceso");

	// Agrego la nueva entrada a la tabla de estados
	list_add(tabla_estados, nueva_entrada);

	// Retorno el nombre_temporal generado para guardar el archivo de la reducción local
	return nueva_entrada->nombre_temporal;

} // FIN agregar_entrada_reduccion_local_tabla_estados


char* agregar_entrada_reduccion_global_tabla_estados(int nro_job_actual, t_list* lista_nodos_participantes, int fd_master){

	t_entrada_estado* nueva_entrada = malloc(sizeof(t_entrada_estado));

	nueva_entrada->job = nro_job_actual;
	nueva_entrada->master = master_actual(lista_MASTERs, fd_master); // lista_MASTERs es una lista global

	// En el campo Nodo cargo el nombre del Nodo encargado de realizar la reducción global para este job
	t_nodo_job* nodo_encargado = identificar_nodo_encargado_en_YAMA(lista_nodos_participantes);
	strcpy(nueva_entrada->nodo, nodo_encargado->nombre_nodo);

	nueva_entrada->bloque = -1;
	strcpy(nueva_entrada->etapa, "REDUCCION GLOBAL");

	char* nombre_temporal = _generar_nombre_temporal_unico();
	strcpy(nueva_entrada->nombre_temporal, nombre_temporal);
	free(nombre_temporal);

	nueva_entrada->estado = malloc(strlen("En proceso") + 1); // + 1 del '\0'
	strcpy(nueva_entrada->estado, "En proceso");

	// Agrego la nueva entrada a la tabla de estados
	list_add(tabla_estados, nueva_entrada);

	// Retorno el nombre_temporal generado para guardar el archivo de la reducción global
	return nueva_entrada->nombre_temporal;

}


bool _entrada_estado_tiene_nro_job_y_nombre_nodo_y_etapa_REDU_LOCAL(void* entrada){

	// Retorna 1 si es true
	int ok_1 = ( ((t_entrada_estado*)entrada)->job == nro_job_actual );

	// Retorna 0 si es true
	int ok_2 = strcmp(((t_entrada_estado*)entrada)->nodo, nombre_nodo_auxiliar);

	// Retorna 0 si es true
	int ok_3 = strcmp(((t_entrada_estado*)entrada)->etapa, "REDUCCION LOCAL");

	if ( ok_1 == 1 && ok_2 == 0 && ok_3 == 0){ // Si se cumplen las tres condiciones es la entrada que busco
		return true;
	}
	else{
		return false;
	}

} // FIN _entrada_estado_tiene_nro_job_y_nombre_nodo


void actualizar_tabla_estados_nodo_reduccion_local(char*nombre_nodo_actual, int nro_job_actual, char* flag){

	/* nro_job_actual es una variable global. La paso por parámetro para hacer enfásis de que la estoy
	 * usando dentro de la función.
	 */

	t_entrada_estado* entrada_a_actualizar;

	/* _entrada_estado_tiene_nro_job_y_nombre_nodo usa nombre_nodo_auxiliar, así que hago
	 *  que dicho char* apunte al char* nombre_nodo_actual que quiero utilizar
	 */
	nombre_nodo_auxiliar = nombre_nodo_actual;

	entrada_a_actualizar = list_find(tabla_estados, _entrada_estado_tiene_nro_job_y_nombre_nodo_y_etapa_REDU_LOCAL);

	/* En este punto el entrada_a_actualizar->estado tiene hecho malloc(strlen("En proceso") + 1).
	 * Por lo tanto, primero hacemos free y luego reservamos memoria para copiar "OK".
	 */
	free(entrada_a_actualizar->estado);

	if ( strcmp(flag, "OK") == 0 ){
		entrada_a_actualizar->estado = malloc(strlen("OK") + 1); // +1 del '\0'
		strcpy( entrada_a_actualizar->estado, "OK" );
	}

	if ( strcmp(flag, "ERROR") == 0 ){
			entrada_a_actualizar->estado = malloc(strlen("ERROR") + 1); // +1 del '\0'
			strcpy( entrada_a_actualizar->estado, "ERROR" );
	}


} // FIN actualizar_tabla_estados_nodo_reduccion_local


bool _entrada_job_reduccion_local(void* entrada_estado_actual){

	// Si la entrada actual de la tabla de estados tiene el mismo nro de job que nro_job_actual ok_1 vale 1
	int ok_1 = ( ((t_entrada_estado*)entrada_estado_actual)->job == nro_job_actual );

	// Si la entrada actual de la tabla de estados tiene en etapa "REDUCCION LOCAL" entonces ok_2 vale 0
	int ok_2 = strcmp(((t_entrada_estado*)entrada_estado_actual)->etapa, "REDUCCION LOCAL");

	if ( ok_1 == 1 && ok_2 == 0 ){
		return true; // Es una entrada del tipo JOB = nro_job_actual & ETAPA = "REDUCCION LOCAL"
	}else{
		return false; // No es la entrada que busco
	}

} // FIN _entrada_job_reduccion_local


bool _reduccion_local_ok(void* entrada_estado_actual){

	if ( strcmp( ((t_entrada_estado*)entrada_estado_actual)->estado, "OK" ) == 0){
		return true;
	}else{
		return false;
	}

} // FIN _reduccion_local_ok


bool chequear_job_iniciar_reduccion_global(int nro_job_actual){

	/* nro_job_actual es una variable global: La paso por parámetro para hacer enfásis de que la estoy
	 * usando dentro de la función.
	 */

	// Me quedo con una sublista de la tabla de estados con las entradas del Job actual que sean de reducción local
	t_list* lista_auxiliar = list_filter(tabla_estados, _entrada_job_reduccion_local);

	// Chequeo si todas las entradas de esta sublista tienen Estado = "OK"
	bool puedo_iniciar_reduccion_global_en_job= list_all_satisfy(lista_auxiliar, _reduccion_local_ok);

	if ( puedo_iniciar_reduccion_global_en_job == true){
		return true; // Se puede iniciar la REDUCCION GLOBAL para el nro_job_actual
	}else{
		return false;
	}

} // FIN chequear_job_iniciar_reduccion_global


void cargar_ip_y_puerto_nodo(t_nodo_job* nodo_job){

	// lista_de_nodos es una lista global
	t_nodo* nodo_actual;

	int indice_lista_nodos;
	for(indice_lista_nodos = 0; indice_lista_nodos < list_size(lista_nodos); indice_lista_nodos++){

		nodo_actual = list_get(lista_nodos, indice_lista_nodos);
		if ( strcmp(nodo_actual->nombre_nodo, nodo_job->nombre_nodo) == 0 ){

			// Si es el Nodo que busco, entonces me copio su ip y puerto

			// Copio la IP
			nodo_job->ip = malloc(strlen(nodo_actual->ip) + 1); // + 1 del '\0'
			strcpy(nodo_job->ip, nodo_actual->ip);

			// Copio el PUERTO
			nodo_job->puerto_worker = nodo_actual->puerto_worker_escucha_master;

		}

	} // FIN for

} // FIN cargar_ip_y_puerto_nodo


t_list* generar_lista_nodos_job_actual(int nro_job_actual){

	// nro_job_actual es una variable global que utiliza list_filter

	/* Me quedo con una sublista de la tabla de estados con las entradas del Job actual que sean de reducción local:
	 *  Voy a tener tantas cantidad de entradas como Nodos hayan participado en el Job, pues se hace una
	 *  reducción local por Nodo.
	 */
	t_list* lista_auxiliar = list_filter(tabla_estados, _entrada_job_reduccion_local);

	t_entrada_estado* entrada_actual;
	t_list* lista_nodos_job = list_create(); // Creo la lista donde voy a guardar los Nodos participantes del Job actual

	int nro_worker_actual;

	int nro_entrada;
	for(nro_entrada = 0; nro_entrada < list_size(lista_auxiliar); nro_entrada++){

		// Creo un elemento del tipo t_nodo_job (es el tipo de dato de la lista_nodos_job)
		t_nodo_job* nodo_job = malloc(sizeof(t_nodo_job));

		// Ahora voy completando los campos que necesito de cada t_nodo_job

		// 1° - Cargo el nombre del Nodo
		entrada_actual = list_get(lista_auxiliar, nro_entrada);
		nodo_job->nombre_nodo = malloc(strlen(entrada_actual->nodo) + 1); // + 1 del '\0'
		strcpy(nodo_job->nombre_nodo, entrada_actual->nodo);

		// 2° - Copio la carga actual del Nodo (WORKER)
		nro_worker_actual = obtener_numero_nodo(nodo_job->nombre_nodo);
		nodo_job->carga_actual = tabla_global_workers[nro_worker_actual].carga_actual;

		// 3° - Cargo la IP y el PUERTO del Nodo
		cargar_ip_y_puerto_nodo(nodo_job);

		// 4° - Por defecto, todos los Nodos de la lista NO son encargados (luego eligiré al encargado)
		nodo_job->encargado = 0; // false

		// 5° - Por último, me guardo el nombre temporal con el que el nodo_job guardo el resultado de la reducción local
		nodo_job->nombre_temporal_reduccion = malloc(strlen(entrada_actual->nombre_temporal) + 1); // + 1 del '\0'
		strcpy(nodo_job->nombre_temporal_reduccion, entrada_actual->nombre_temporal);

		// Finalmente, añado el elemento t_nodo_job a la lista de nodos job actual

		list_add(lista_nodos_job, nodo_job); // Añado el Nodo que participo en el Job a la lista

	} // FIN for

	return lista_nodos_job;

} // generar_lista_nodos_job_actual


void elegir_nodo_encargado(t_list* lista_nodos_participantes){

	// Eligo al Nodo cuyo WORKER tenga menor carga actual WL(w)

	t_nodo_job* nodo_actual;
	t_nodo_job* nodo_encargado; // Es el que tiene menor carga actual
	int menor_carga_actual = MENOR_CARGA_WORKER; // La establezco yo en un valor muy alto

	int indice_nodo;
	for(indice_nodo = 0; indice_nodo < list_size(lista_nodos_participantes); indice_nodo++){

		// Recorro la lista de nodos participantes
		nodo_actual = list_get(lista_nodos_participantes, indice_nodo);
		if ( nodo_actual->carga_actual < menor_carga_actual) {

			// Elijo, al menos por ahora, al nodo_actual como el nodo encargado
			nodo_encargado = nodo_actual;

			// Actualizo la menor carga actual para la próxima iteración
			menor_carga_actual = nodo_actual->carga_actual;
		}

	} // FIN for

	// En nodo_encargado tengo la dirección de memoria del nodo_actual elegido como encargado
	nodo_encargado->encargado = 1; // Lo marco como encargado (true)

} // FIN elegir_nodo_encargado


t_nodo_job* identificar_nodo_encargado_en_YAMA(t_list* lista_nodos_participantes){

	t_nodo_job* nodo_job;

	int indice_lista;
	for(indice_lista = 0; indice_lista < list_size(lista_nodos_participantes); indice_lista++){

		nodo_job = list_get(lista_nodos_participantes, indice_lista);

		if ( nodo_job->encargado == 1 ){
			return nodo_job;
		}

	} // FIN for

	return NULL; // El flujo correcto de ejecución NUNCA debería pasar por acá

} // FIN identificar_nodo_encargado_en_YANA


bool _entrada_estado_tiene_nro_job_y_etapa_REDU_GLOBAL(void* entrada){

	// Retorna 1 si es true
	int ok_1 = ( ((t_entrada_estado*)entrada)->job == nro_job_actual );

	// Retorna 0 si es true
	int ok_2 = strcmp(((t_entrada_estado*)entrada)->etapa, "REDUCCION GLOBAL");

	if ( ok_1 == 1 && ok_2 == 0){ // Si se cumplen las dos cndiciones es la entrada que busco
		return true;
	}
	else{
		return false;
	}


} // FIN _entrada_estado_tiene_nro_job_y_etapa_REDU_GLOBAL


void actualizar_tabla_estados_job_reduccion_global(int nro_job_actual, char* flag){

	/* nro_job_actual es una variable global. La paso por parámetro para hacer enfásis de que la estoy
	 * usando dentro de la función.
	 */

	t_entrada_estado* entrada_a_actualizar;

	entrada_a_actualizar = list_find(tabla_estados, _entrada_estado_tiene_nro_job_y_etapa_REDU_GLOBAL);

	/* En este punto la entrada_a_actualizar->estado tiene hecho malloc(strlen("En proceso") + 1).
	 * Por lo tanto, primero hacemos free y luego reservamos memoria para copiar "OK".
	 */
	free(entrada_a_actualizar->estado);

	if ( strcmp(flag, "OK") == 0 ){
		entrada_a_actualizar->estado = malloc(strlen("OK") + 1); // +1 del '\0'
		strcpy( entrada_a_actualizar->estado, "OK" );
	}

	if ( strcmp(flag, "ERROR") == 0){
		entrada_a_actualizar->estado = malloc(strlen("ERROR") + 1); // +1 del '\0'
		strcpy( entrada_a_actualizar->estado, "ERROR" );
	}

} // FIN actualizar_tabla_estados_job_reduccion_global



/*
----------------------------------------------------------
FIN - Funciones para la TABLA DE ESTADOS
----------------------------------------------------------
*/
