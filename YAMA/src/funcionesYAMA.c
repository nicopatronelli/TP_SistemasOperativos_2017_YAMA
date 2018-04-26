/*
 * funcionesYAMA.c
 *
 *  Created on: 19/10/2017
 *      Author: utnso
 */

#include "funcionesYAMA.h"

t_log* log_YAMA; // Definimos la variable log_YAMA como GLOBAL para poder acceder a el archivo de log desde cualquier función
t_list* lista_MASTERs; // Definimos la variable lista_MASTERs como GLOBAL para poder acceder a la misma desde cualquier función
t_list* tabla_estados;
t_list* lista_jobs;
t_list* lista_nodos; // Lista global donde me guardo la ip y el puerto de cada Nodo que participa en CUALQUIER Job, asumiendo que el Nodo N siempre tendra el mismo puerto e ip
int nro_job; // Variable global para numerar e identificar los distintos jobs
int indice_nombre_temporal; // Variable global para armar los nombres temporales
char* nombre_nodo_auxiliar;

// Variables para BLOQUE_TRANSFORMADO_OK (Necesitan ser globales para list_find en actualizar_tabla_estados_bloque_ok)
int nro_bloque_archivo_transformado_ok;
int nro_bloque_archivo_transformado_error;
int nro_job_actual; // La uso también para la función chequear_nodo_iniciar_reduccion_local
char* nombre_nodo_reduccion_local; // Variable global para la funcion chequear_nodo_iniciar_reduccion_local

t_config* configuracion_YAMA(char* ruta_archivo_configuracion, char** FILESYSTEM_IP, int* FILESYSTEM_PUERTO, int* PUERTO_ESCUCHA_MASTER, int* RETARDO_PLANIFICACION, char** ALGORITMO_BALANCEO, int* DISPONIBILIDAD_BASE){
	t_config* config = config_create(ruta_archivo_configuracion);
	if ( config == NULL){
		log_error(log_YAMA, "No se pudo leer el archivo de configuración de YAMA.");
		return NULL;
	}
	else{
		*FILESYSTEM_IP = config_get_string_value(config,"FS_IP"); // Recupero la IP del FILESYSTEM del config.txt
		*FILESYSTEM_PUERTO = config_get_int_value(config,"FS_PUERTO"); // Recupero el PUERTO del FILESYSTEM del config.txt
		*PUERTO_ESCUCHA_MASTER = config_get_int_value(config,"PUERTO_ESCUCHA_MASTER"); // Recupero el PUERTO donde YAMA va a escuchar conexiones de varios procesos MASTER
		*RETARDO_PLANIFICACION = config_get_int_value(config,"RETARDO_PLANIFICACION"); // Cargo el RETARDO DE PLANIFICACIÓN para el algoritmo de planificación que use YAMA
		*ALGORITMO_BALANCEO = config_get_string_value(config,"ALGORITMO_BALANCEO"); // Cargo el ALGORITMO DE BALANCEO que va a usar YAMA
		*DISPONIBILIDAD_BASE = config_get_int_value(config, "DISPONIBILIDAD_BASE"); // Cargo la DISPONIBILIDAD BASE para la planificación

		log_info(log_YAMA,"La ip del FILESYSTEM es: %s", *FILESYSTEM_IP);
		log_info(log_YAMA,"El puerto del FILESYSTEM es: %d", *FILESYSTEM_PUERTO);
		log_info(log_YAMA,"El puerto donde YAMA va escuchar conexiones de distintos MASTER es: %d", *PUERTO_ESCUCHA_MASTER);
		log_info(log_YAMA, "El retardo de la planificacion es: %d", *RETARDO_PLANIFICACION);
		log_info(log_YAMA, "El algoritmo de balanceo es: %s", *ALGORITMO_BALANCEO);
		log_info(log_YAMA, "La disponibilidad base es: %d", *DISPONIBILIDAD_BASE);

		return config;
		}
}

/*
==========================================================
INICIO - Funciones manejo de lista de MASTERs desde YAMA
==========================================================
*/

void agregar_master(int socket, t_list* lista_master){

	t_master* nuevo_MASTER = malloc(sizeof(t_master));

	nuevo_MASTER->socket = socket;
	list_add(lista_master, nuevo_MASTER);

} // FIN agregar_master


void quitar_master(int nro_master, t_list* lista_master){

	// POR AHORA NO LA PIENSO USAR
	//list_remove_and_destroy_element(lista_master, int index, free);

} // FIN quitar_master


int master_actual(t_list* lista_master, int socket_actual){

	t_master* master_actual;

	int master;
	for(master=0; master < list_size(lista_master); master++){
		master_actual = list_get(lista_MASTERs, master);
		if ( master_actual->socket == socket_actual ){
			return master;
		}
	}

	return ERROR;

} // FIN master_actual


void desconectar_master(int numero_master, int socket_master, fd_set* master){

	t_master* master_aux = list_get(lista_MASTERs, numero_master); // Obtengo el t_master del numero_master correspondiente
	master_aux->socket = SOCKET_MASTER_DESCONECTADO; // Establezco su socket en -1, pues se cerrará

	FD_CLR(socket_master, master); // Elimino el fd_i del conjunto master
	close(socket_master); // Cierro el socket de conexión con dicho MASTER pues ya cerraron la conexión del otro lado

} // FIN desconectar_master



int obtener_nro_job_para_master(int fd_master){

	int numero_master = master_actual(lista_MASTERs, fd_master);

	// Recorro la tabla de estados hasta matchear con el nro de job para el numero_master obtenido
	int nro_entrada_actual;
	for(nro_entrada_actual = 0; nro_entrada_actual < list_size(tabla_estados); nro_entrada_actual++){

		// Obtenga la entrada actual de la tabla de estados
		t_entrada_estado* entrada_actual = list_get(tabla_estados, nro_entrada_actual);

		if( entrada_actual->master == numero_master ){
			return entrada_actual->job;
		}

	} // FIN for

	return ERROR; // En un flujo de ejecución correcto nunca se debería pasar por acá, pues todo job tiene un nro de master

} // FIN obtener_nro_job_para_master


/*
----------------------------------------------------------
FIN - Funciones manejo de lista de MASTERs desde YAMA
----------------------------------------------------------
*/


/*
==========================================================
INICIO - Funciones para la comunicación de YAMA y MASTER
==========================================================
*/


int enviar_info_bloque_a_transformar(int fd_master, int cantidad_bloques_archivo, t_list* lista_bloques_archivo){


	// Envio la info de cada bloque a procesar (de la copia elegida en la planificación)
	t_Bloque* bloque_actual;

	// Variables auxiliares para string_itoa
	char* nro_bloque_actual_string;
	char* puerto_worker_escucha_master_string;
	char* numero_bloque_databin_string;
	char* bytes_ocupados_string;

	int nro_bloque_actual;
	for(nro_bloque_actual = 0; nro_bloque_actual < cantidad_bloques_archivo; nro_bloque_actual++){

		bloque_actual = list_get(lista_bloques_archivo, nro_bloque_actual);

		if ( bloque_actual->copia0.copia_elegida_para_transformar == true){

			// La copia0 del bloque_actual es la elegida para transformar

			// 1° - Le indico a MASTER que quiero realizar una transformacion y le envio el nro de bloque (lógico) del archivo a transformar
			nro_bloque_actual_string = string_itoa(nro_bloque_actual);
			enviar_mensaje(TRANSFORMACION, nro_bloque_actual_string, fd_master);
			free(nro_bloque_actual_string);

			// 2° - Le envio todos los datos necesarios para que pueda llevar a cabo la tranformación
			enviar_mensaje(NOMBRE_NODO, bloque_actual->copia0.nombre_nodo, fd_master);
			enviar_mensaje(IP_WORKER, bloque_actual->copia0.ip_nodo, fd_master);

			puerto_worker_escucha_master_string = string_itoa(bloque_actual->copia0.puerto_worker_escucha_master);
			enviar_mensaje(PUERTO_WORKER, puerto_worker_escucha_master_string, fd_master);
			free(puerto_worker_escucha_master_string);

			numero_bloque_databin_string = string_itoa(bloque_actual->copia0.numero_bloque_databin);
			enviar_mensaje(NUMERO_BLOQUE_DATABIN, numero_bloque_databin_string, fd_master);
			free(numero_bloque_databin_string);

			bytes_ocupados_string = string_itoa(bloque_actual->copia0.bytes_ocupados);
			enviar_mensaje(BYTES_OCUPADOS_BLOQUE_DATABIN, bytes_ocupados_string, fd_master);
			free(bytes_ocupados_string);

			enviar_mensaje(NOMBRE_ARCHIVO_TEMPORAL_TRANS, bloque_actual->copia0.nombre_temporal_trans ,fd_master);

		}else if ( bloque_actual->copia1.copia_elegida_para_transformar == true){

			// La copia1 del bloque_actual es la elegida para transformar

			// 1° - Le indico a MASTER que quiero realizar una transformacion y le envio el nro de bloque (lógico) del archivo a transformar
			nro_bloque_actual_string = string_itoa(nro_bloque_actual);
			enviar_mensaje(TRANSFORMACION, nro_bloque_actual_string, fd_master);
			free(nro_bloque_actual_string);

			// 2° - Le envio todos los datos necesarios para que pueda llevar a cabo la tranformación
			enviar_mensaje(NOMBRE_NODO, bloque_actual->copia1.nombre_nodo, fd_master);
			enviar_mensaje(IP_WORKER, bloque_actual->copia1.ip_nodo, fd_master);

			puerto_worker_escucha_master_string = string_itoa(bloque_actual->copia1.puerto_worker_escucha_master);
			enviar_mensaje(PUERTO_WORKER, puerto_worker_escucha_master_string, fd_master);
			free(puerto_worker_escucha_master_string);

			numero_bloque_databin_string = string_itoa(bloque_actual->copia1.numero_bloque_databin);
			enviar_mensaje(NUMERO_BLOQUE_DATABIN, numero_bloque_databin_string, fd_master);
			free(numero_bloque_databin_string);

			bytes_ocupados_string = string_itoa(bloque_actual->copia1.bytes_ocupados);
			enviar_mensaje(BYTES_OCUPADOS_BLOQUE_DATABIN, bytes_ocupados_string, fd_master);
			free(bytes_ocupados_string);

			enviar_mensaje(NOMBRE_ARCHIVO_TEMPORAL_TRANS, bloque_actual->copia1.nombre_temporal_trans,fd_master);
		}else{

			// El flujo de ejecución NUNCA debería pasar por aquí
			log_error(log_YAMA, "Error al enviar info a MASTER de cada bloque a procesar: NO se eligio ninguna copia del bloque %d para transformar.", nro_bloque_actual);

			return ERROR;
		}

	} // Fin for envio info a MASTER de cada bloque a TRANSFORMAR

	// Informo que todo salió bien
	log_trace(log_YAMA, "La información de los bloques a transformar referidos al JOB %d se han enviado correctamente al MASTER Nro %d.", nro_job, master_actual(lista_MASTERs, fd_master));

	return OK;

} // FIN enviar_info_bloque_a_transformar


/*
----------------------------------------------------------
FIN - Funciones para la comunicación de YAMA y MASTER
----------------------------------------------------------
*/


/*
==========================================================
INICIO - Funciones auxiliares YAMA
==========================================================
*/

t_nodo* recuperar_info_nodo(char* nombre_nodo_actual){

	// lista_nodos es una lista global

	t_nodo* nodo_actual;

	int indice_lista_nodos;
	for ( indice_lista_nodos = 0; indice_lista_nodos < list_size(lista_nodos); indice_lista_nodos++ ){

		nodo_actual = list_get(lista_nodos, indice_lista_nodos);

		// Cuando obtengo la info del Nodo que necesito retorno su estructura
		if ( strcmp( nodo_actual->nombre_nodo, nombre_nodo_actual ) == 0 ){
			return nodo_actual;
		}

	} // FIN for

	return NULL; // En un flujo de ejecución correcto nunca debería pasarse por acá

} // FIN recuperar_info_nodo


/*
----------------------------------------------------------
FIN - Funciones auxiliares YAMA
----------------------------------------------------------
*/
