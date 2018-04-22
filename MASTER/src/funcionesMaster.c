/*
 * funcionesMaster.c
 *
 *  Created on: 19/9/2017
 *      Author: utnso
 */

#include "funcionesMaster.h"

t_log* log_MASTER; // Definimos la variable log_MASTER como GLOBAL para poder acceder a el archivo de log desde cualquier función
int socket_MASTER_YAMA;
void* buffer_script_transformador;
void* buffer_script_reductor;
char* nombre_resultado_reduccion_global; // Variable global para el nombre del archivo donde se guarda el resultado de la reducción global
char* ruta_resultadoFinal;

t_config* configuracion_MASTER(char* ruta_archivo_configuracion, char** YAMA_IP, int* YAMA_PUERTO){
	t_config* config = config_create(ruta_archivo_configuracion);
	if ( config == NULL){
		log_error(log_MASTER, "No se pudo leer el archivo de configuración");
		return NULL;
	}
	else{
		*YAMA_IP = config_get_string_value(config,"YAMA_IP");  // Recupero la IP de YAMA del config.txt
		*YAMA_PUERTO = config_get_int_value(config,"YAMA_PUERTO"); // Recupero el PUERTO de YAMA del config.txt
		log_info(log_MASTER,"La ip de YAMA es: %s", *YAMA_IP);
		log_info(log_MASTER,"El puerto de YAMA es: %d", *YAMA_PUERTO);
		return config;
		}

} // FIN configuracion_MASTER


/*
=====================================
INICIO - Funciones para SCRIPTS
=====================================
*/

void* _guardar_script(char* ruta_script){

	return map_archivo(ruta_script);

}

void* guardar_script(char* ruta_script){

	FILE* script = fopen(ruta_script, "r");
	if ( script == NULL){
		log_error(log_MASTER, "No existe o no se puede abrir el script %s.", ruta_script);
		return NULL;
	}

	int tamanio_script = tamanioArchivo(ruta_script);
	void* buffer_script = malloc(tamanio_script);
	int bytes_leidos_script = fread(buffer_script, sizeof(char), tamanio_script, script);
	fclose(script);

	// Si la cantidad de bytes leidos no coincide con el tamaño del archivo significa que hubo un error
	if( bytes_leidos_script != tamanio_script){
		log_error(log_MASTER, "Error al leer el script %s\n.", ruta_script);
		return NULL;
	}

	return buffer_script;
} // FIN guardar_script

void liberar_buffer_script(void* buffer_script){
	int size_buffer = strlen(buffer_script);
	munmap(buffer_script, size_buffer);
} // FIN liberar_buffer_script

/*
-------------------------------
FIN - Funciones para SCRIPTS
-------------------------------
*/


/*
=========================================================================
INICIO - Funciones comunicacion MASTER-WORKER (desde el lado de MASTER)
=========================================================================
*/

void enviar_info_bloque_transformar_a_worker(t_info_bloque_a_transformar* info_bloque_actual){

	// Declaro el protocolo donde voy a recibir mensajes dentro de este hilo
	t_protocolo* protocolo_Recepcion;

	// Me conecto al WORKER que tiene el bloque a transformar
	t_direccion direccion_WORKER = nuevaDireccion(info_bloque_actual->puerto_worker_escucha_master, info_bloque_actual->ip_nodo);

	int socket_MASTER_WORKER = conectarseA(direccion_WORKER);
	if ( socket_MASTER_WORKER > 0 ) {
		log_trace(log_MASTER, "Se estableció conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
	}
	else{
		log_error(log_MASTER, "No se pudo establecer conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
		pthread_exit(NULL); // Como hay un error mato al hilo para salir del mismo
	}

	// Le indico al WORKER que quiero realizar una TRANSFORMACION
	enviar_mensaje(ETAPA_TRANSFORMACION, "Mensaje vacio", socket_MASTER_WORKER);

	/*** INICIO - Envio la información necesaria al WORKER ***/

	// 1° Envio el script transformador
	enviar_mensaje(SCRIPT_TRANSFORMADOR, (char*)buffer_script_transformador, socket_MASTER_WORKER);

	// 2" Envio el número de bloque del archivo data.bin a transformar
	char* numero_bloque_databin = string_itoa(info_bloque_actual->numero_bloque_databin);
	enviar_mensaje(BLOQUE_DATABIN_A_TRANSFORMAR, numero_bloque_databin, socket_MASTER_WORKER);
	free(numero_bloque_databin);

	// 3° Envio los bytes ocupados por dicho bloque (cuanto del 1MiB posible)
	char* bytes_ocupados = string_itoa(info_bloque_actual->bytes_ocupados);
	enviar_mensaje(BYTES_OCUPADOS_BLOQUE_A_TRANSFORMAR, bytes_ocupados, socket_MASTER_WORKER);
	free(bytes_ocupados);

	// 4° Envio el nombre temporal con el que se almacenará el resultado de transformar el bloque
	enviar_mensaje(NOMBRE_ARCHIVO_TEMPORAL_TRANSFORMACION, info_bloque_actual->nombre_archivo_temporal, socket_MASTER_WORKER);

	/*** FIN - Envio la información necesaria al WORKER ***/

	// 5° Espero confirmacion de exito o fracaso de la transformacion por parte de WORKER
	protocolo_Recepcion = recibir_mensaje(socket_MASTER_WORKER);

	if ( protocolo_Recepcion->funcion == TRANSFORMACION_OK){
		log_trace(log_MASTER, "WORKER me informa que una transformación se realizo correctamente.");
		eliminar_protocolo(protocolo_Recepcion);

		/* Le comunico a YAMA que la transformacion salio bien:
		 * 		Para ello le envio el tipo de mensaje correspondiente y, en el contenido del mensaje, le digo el
		 * 		numero del bloque del data.bin que se transformo ok... NOOOOOOO!!!! CUIDADO, le tengo que avisar
		 * 		el numero de bloque del archivo (lógico) que se transformo bien.
		 */

		// Le indico a YAMA el numero de bloque del archivo (lógico) que se transformo correctamente
		char* numero_bloque_archivo_transformado_ok_string = string_itoa(info_bloque_actual->numero_bloque_archivo);
		enviar_mensaje(BLOQUE_TRANSFORMADO_OK, numero_bloque_archivo_transformado_ok_string, socket_MASTER_YAMA);
		free(numero_bloque_archivo_transformado_ok_string);

		// También le indico a MASTER el nombre del Nodo donde se realizo la transformación exitosamente
		enviar_mensaje(NOMBRE_NODO_BLOQUE_TRANSFORMADO_OK, info_bloque_actual->nombre_nodo, socket_MASTER_YAMA);

	}
	else if ( protocolo_Recepcion->funcion == TRANSFORMACION_ERROR){
		log_error(log_MASTER, "WORKER me informa que una transformación NO se pudo realizar.");
		eliminar_protocolo(protocolo_Recepcion);

		/* Le comunico a YAMA que la transformacion NO se pudo realizar:
		 * 		Para ello le envio el tipo de mensaje correspondiente y, en el contenido del mensaje, le digo
		 * 		el numero del bloque del archivo (lógico) que no se pudo transformar.
		 */

		char* numero_bloque_archivo_transformado_error_string = string_itoa(info_bloque_actual->numero_bloque_archivo);
		enviar_mensaje(BLOQUE_TRANSFORMADO_ERROR, numero_bloque_archivo_transformado_error_string, socket_MASTER_YAMA);
		free(numero_bloque_archivo_transformado_error_string);

		// También le indico a MASTER el nombre del Nodo donde se realizo la transformación NO se pudo realizar
		enviar_mensaje(NOMBRE_NODO_BLOQUE_TRANSFORMADO_ERROR, info_bloque_actual->nombre_nodo, socket_MASTER_YAMA);

	}

	// Libero la memoria utilizada
	free(info_bloque_actual->nombre_nodo);
	free(info_bloque_actual->ip_nodo);
	free(info_bloque_actual->nombre_archivo_temporal);
	free(info_bloque_actual);

	//pthread_exit(NULL);

} // FIN enviar_info_bloque_a_worker


void enviar_info_nodo_reduccion_local_a_worker(t_info_nodo_a_reducir* nodo_a_reducir){

	// Declaro el protocolo donde voy a recibir mensajes dentro de este hilo
	t_protocolo* protocolo_Recepcion;

	// Me conecto al WORKER donde se va a realizar la REDUCCION LOCAL
	t_direccion direccion_WORKER = nuevaDireccion(nodo_a_reducir->puerto_worker_escucha_master, nodo_a_reducir->ip_nodo);

	int socket_MASTER_WORKER = conectarseA(direccion_WORKER);
	if ( socket_MASTER_WORKER > 0 ) {
		log_trace(log_MASTER, "Se estableció conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
	}
	else{
		log_error(log_MASTER, "No se pudo establecer conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
		pthread_exit(NULL); // Como hay un error mato al hilo para salir del mismo
	}

	// Le indico al WORKER que quiero realizar una REDUCCION LOCAL
	enviar_mensaje(ETAPA_REDUCCION_LOCAL, "Mensaje vacio", socket_MASTER_WORKER);

	/*** INICIO - Envio la información necesaria al WORKER ***/

	// 1° Envio el script reductor
	enviar_mensaje(SCRIPT_REDUCTOR_LOCAL, (char*)buffer_script_reductor, socket_MASTER_WORKER);

	// 2° Envio los nombres temporales trans que se van a reducir

	// 2a - Primero le envio a WORKER la cantidad de temporales a reducir localmente
	char* cant_temp_a_reducir_string = string_itoa(nodo_a_reducir->cantidad_temporales_a_reducir);
	enviar_mensaje(CANTIDAD_TEMPORALES_TRANS, cant_temp_a_reducir_string, socket_MASTER_WORKER);
	free(cant_temp_a_reducir_string); // Libero la memoria pues no la uso más

	// 2b - Ahora, le envio a WORKER cada nombre temporal a reducir localmente
	int indice_temp_actual;
	for(indice_temp_actual = 0; indice_temp_actual < list_size(nodo_a_reducir->lista_temporales_a_reducir); indice_temp_actual++){

		t_nombre_temporal_a_reducir* t_nombre_temporal = list_get(nodo_a_reducir->lista_temporales_a_reducir, indice_temp_actual);
		enviar_mensaje(NOMBRE_TEMPORAL_TRANS, t_nombre_temporal->nombre_temporal_a_reducir, socket_MASTER_WORKER);

	}

	// 3° Envio el nombre con el que guardar el resultado de la reducción local
	enviar_mensaje(NOMBRE_TEMPORAL_REDUCCION_LOCAL, nodo_a_reducir->nombre_temporal_reduccion, socket_MASTER_WORKER);

	/*** FIN - Envio la información necesaria al WORKER ***/

	// 4° Espero confirmacion de exito o fracaso de la transformacion por parte de WORKER
	protocolo_Recepcion = recibir_mensaje(socket_MASTER_WORKER);

	if ( protocolo_Recepcion->funcion == REDUCCION_LOCAL_OK){

		log_trace(log_MASTER, "WORKER me informa que una reducción local se realizo correctamente.");
		eliminar_protocolo(protocolo_Recepcion);

		/* Entonces, desde MASTER, le comunico a YAMA que una reducción local se hizo correctamente.
		 * IMPORTANTE: Necesito enviarle el número de Nodo donde se realizo correctamente la
		 * reducción local para que YAMA pueda identificarla y marcarla como "OK" en su tabla de
		 * estados.
		 */

		enviar_mensaje(ETAPA_REDUCCION_LOCAL_OK, nodo_a_reducir->nombre_nodo, socket_MASTER_YAMA);

	}
	else if ( protocolo_Recepcion->funcion == REDUCCION_LOCAL_ERROR){

		log_error(log_MASTER, "WORKER me informa que una reducción local NO se pudo realizar.");
		eliminar_protocolo(protocolo_Recepcion);

		/* Le comunico a YAMA que una reducción local NO se pudo realizar. Necesito enviarle,
		 * además, el número de Nodo que no se pudo reducir localmente para que YAMA pueda
		 * actualizar su tabla de estados.
		 */
		enviar_mensaje(ETAPA_REDUCCION_LOCAL_ERROR, nodo_a_reducir->nombre_nodo, socket_MASTER_YAMA);

	}

	// Libero la memoria utilizada
	free(nodo_a_reducir->ip_nodo);
	free(nodo_a_reducir->nombre_temporal_reduccion);
	free(nodo_a_reducir->nombre_nodo);
	liberar_lista_temporales_a_reducir(nodo_a_reducir->lista_temporales_a_reducir);
	//list_iterate(nodo_a_reducir->lista_temporales_a_reducir, free);
	//list_destroy(nodo_a_reducir->lista_temporales_a_reducir);
	free(nodo_a_reducir);

	//pthread_exit(NULL);

} // FIN enviar_info_nodo_reduccion_a_worker


t_info_nodo_redu_global* identificar_nodo_encargado(t_list* lista_nodos_participantes){

	t_info_nodo_redu_global* nodo_job;

	int indice_lista;
	for(indice_lista = 0; indice_lista < list_size(lista_nodos_participantes); indice_lista++){

		nodo_job = list_get(lista_nodos_participantes, indice_lista);

		if ( nodo_job->encargado == 1 ){
			return nodo_job;
		}

	} // FIN for

	return NULL; // El flujo correcto de ejecución NUNCA debería pasar por acá

} // FIN identificar_nodo_encargado


bool _es_nodo_ayudante(void* nodo_participante){

	if ( ((t_info_nodo_redu_global*)nodo_participante)->encargado == 0 ){ // Si el nodo participante NO es encargado
		return true;
	}else{
		return false;
	}

} // FIN _es_nodo_ayudante


void enviar_info_nodo_encargado(t_list* lista_nodos_participantes){

	// Variables auxiliares
	t_protocolo* protocolo_Recepcion;

	// 1° - Identifico quien es el Nodo encargado
	t_info_nodo_redu_global* nodo_encargado = identificar_nodo_encargado(lista_nodos_participantes);

	// 2° - Me conecto al Nodo encargado de realizar la REDUCCION GLOBAL
	t_direccion direccion_WORKER = nuevaDireccion(nodo_encargado->puerto_worker, nodo_encargado->ip);

	int socket_MASTER_WORKER = conectarseA(direccion_WORKER);
	if ( socket_MASTER_WORKER > 0 ) {
		log_trace(log_MASTER, "Se estableció conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
	}
	else{
		log_error(log_MASTER, "No se pudo establecer conexión con un WORKER en el socket %d.", socket_MASTER_WORKER);
		pthread_exit(NULL); // Como hay un error mato al hilo para salir del mismo
	}

	// 3° - Le envio la lista de nodos ayudantes (excluyendo el Nodo encargado)

	// 3a - Me quedo con la lista de nodos ayudantes (excluyo al Nodo encargado)
	t_list* lista_nodos_ayudantes = list_filter(lista_nodos_participantes, _es_nodo_ayudante);

	// 3b - Le informo al WORKER encargado que debe comenzar la reducción global
	enviar_mensaje(WORKER_ENCARGADO, "Mensaje vacio", socket_MASTER_WORKER);

	// 3c - Le envio al WORKER encargado la cantidad de nodos ayudantes
	int cantidad_nodos_ayudantes = list_size(lista_nodos_ayudantes);
	char* cantidad_nodos_ayudantes_string = string_itoa(cantidad_nodos_ayudantes);
	enviar_mensaje(CANTIDAD_NODOS_AYUDANTES, cantidad_nodos_ayudantes_string, socket_MASTER_WORKER);
	free(cantidad_nodos_ayudantes_string);

	// 3d - Envio la lista de nodos ayudantes

	t_info_nodo_redu_global* nodo_ayudante;
	char* puerto_worker_string;

	int indice_nodo_ayudante;
	for(indice_nodo_ayudante = 0; indice_nodo_ayudante < list_size(lista_nodos_ayudantes); indice_nodo_ayudante++){

		// Envio, uno a uno, la info de cada nodo ayudante
		nodo_ayudante = list_get(lista_nodos_ayudantes, indice_nodo_ayudante);

		// Envio la IP del Nodo ayudante
		enviar_mensaje(IP_NODO_AYUDANTE, nodo_ayudante->ip, socket_MASTER_WORKER);

		// Envio el PUERTO del Nodo ayudante
		puerto_worker_string = string_itoa(nodo_ayudante->puerto_worker);
		enviar_mensaje(PUERTO_NODO_AYUDANTE, puerto_worker_string, socket_MASTER_WORKER);
		free(puerto_worker_string);

		// Envio el nombre temporal de la reducción local
		enviar_mensaje(NOMBRE_REDUCCION_LOCAL_NODO_AYUDANTE, nodo_ayudante->nombre_temporal_reduccion, socket_MASTER_WORKER);

	} // FIN for

	// 4° - Envio la ruta del archivo temporal de reducción local del WORKER ENCARGADO
	enviar_mensaje(NOMBRE_REDUCCION_LOCAL_NODO_ENCARGADO, nodo_encargado->nombre_temporal_reduccion, socket_MASTER_WORKER);

	// 5° - Envio el script de reducción global al WORKER ENCARGADO
	enviar_mensaje(SCRIPT_REDUCCION_GLOBAL, (char*) buffer_script_reductor, socket_MASTER_WORKER);

	// 6° - Envio el nombre TEMPORAL con el que se guardará el resultado de la reducción global
	enviar_mensaje(NOMBRE_RESULTADO_REDUCCION_GLOBAL, nombre_resultado_reduccion_global, socket_MASTER_WORKER);

	// 7° - Por último, le envio al WORKER encargado la RUTA FINAL donde quiero almacenar el resultado de la reducción global en FILE_SYSTEM
	enviar_mensaje(RUTA_ARCHIVO_FINAL_FS, ruta_resultadoFinal, socket_MASTER_WORKER);

	// 8° - Quedo a la espera de que WORKER me responda si la reducción global si efectuo correctamente o no
	protocolo_Recepcion = recibir_mensaje(socket_MASTER_WORKER);
	if ( protocolo_Recepcion->funcion == ETAPA_REDUCCION_GLOBAL_OK) { // INICIO if ETAPA_REDUCCION_GLOBAL_OK

		log_trace(log_MASTER, "WORKER me informa que una reducción global se realizo correctamente.");
		eliminar_protocolo(protocolo_Recepcion);

		// Le comunico a YAMA que una reducción global se hizo correctamente.
		enviar_mensaje(ETAPA_REDUCCION_GLOBAL_OK, "Mensaje vacio", socket_MASTER_YAMA);

		// Recibo la confirmación de WORKER para saber si FILE_SYSTEM pudo almacenar el resultado de la reducción global
		protocolo_Recepcion = recibir_mensaje(socket_MASTER_WORKER);

		if ( protocolo_Recepcion->funcion == ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_OK ){

			// Le aviso a YAMA que el resultado de la reducción global se almaceno correctamente en FILE_SYSTEM
			enviar_mensaje(ETAPA_ALMACENAMIENTO_FINAL_OK, "Mensaje vacio", socket_MASTER_YAMA);

		}else if( protocolo_Recepcion->funcion == ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR ){

			// Le aviso a YAMA que el resultado de la reducción global NO se pudo almacenar en FILE_SYSTEM
			enviar_mensaje(ETAPA_ALMACENAMIENTO_FINAL_ERROR, "Mensaje vacio", socket_MASTER_YAMA);

		}
	}else{ // if ETAPA_REDUCCION_GLOBAL_ERROR ...

		log_trace(log_MASTER, "WORKER me informa que una reducción global no se pudo realizar.");
		eliminar_protocolo(protocolo_Recepcion);

		// Le comunico a YAMA que una reducción global NO se pudo realizar.
		enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_MASTER_YAMA);

	} // FIN if ETAPA_REDUCCION_GLOBAL_ERROR ...

	// Libero la memoria utilizada

	list_destroy(lista_nodos_ayudantes); // Viene del list_filter()
	liberar_lista_nodos_participantes(lista_nodos_participantes); // Libero la lista de nodos participantes


} // FIN enviar_info_nodo_encargado

/*
----------------------------------------------------------------------
FIN - Funciones comunicacion MASTER-WORKER (desde el lado de MASTER)
----------------------------------------------------------------------
*/


/*
=========================================================================
INICIO - Funciones auxiliares MASTER
=========================================================================
*/

void liberar_lista_temporales_a_reducir(t_list* lista_temporales) {

	// Variables
	int indice_elemento;
	t_nombre_temporal_a_reducir* elemento;

	// Calculo el tamaño de la lista
	int cantidad_elementos = list_size(lista_temporales);

	for (indice_elemento = 0; indice_elemento < cantidad_elementos; ++indice_elemento) {
		elemento = list_get(lista_temporales, indice_elemento);
		free(elemento->nombre_temporal_a_reducir);
	}

	list_iterate(lista_temporales, free);
	list_destroy(lista_temporales);

} // FIN liberar_lista_temporales_a_reducir

void liberar_lista_nodos_participantes(t_list* lista_nodos_participantes) {

	// Variables
	int indice_elemento;
	t_info_nodo_redu_global* elemento;

	// Calculo el tamaño de la lista
	int cantidad_elementos = list_size(lista_nodos_participantes);

	for (indice_elemento = 0; indice_elemento < cantidad_elementos; ++indice_elemento) {
		elemento = list_get(lista_nodos_participantes, indice_elemento);
		free(elemento->nombre_nodo);
		free(elemento->ip);
		free(elemento->nombre_temporal_reduccion);
	}

	list_iterate(lista_nodos_participantes, free);
	list_destroy(lista_nodos_participantes);

} // FIN liberar_lista_nodos_participantes



/*
----------------------------------------------------------------------
FIN - Funciones auxiliares MASTER
----------------------------------------------------------------------
*/

