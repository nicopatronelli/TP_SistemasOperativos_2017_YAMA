/*
 * funcionesDataNode.c
 *
 *  Created on: 6/11/2017
 *      Author: utnso
 */

#include "funcionesDataNode.h"

t_log* log_DATANODE; // Definimos la variable log_DATANODE como GLOBAL para poder acceder a el archivo de log desde cualquier función

t_config* configuracion_DATANODE(char* ruta_archivo_configuracion, char** FILESYSTEM_IP, int* FILESYSTEM_PUERTO, char** NODO, int* PUERTO_WORKER_ESCUCHA_MASTER, char** RUTA_DATABIN, int* TAMANIO_DATABIN, char** IP_NODO){
	t_config* config = config_create(ruta_archivo_configuracion);
	if ( config == NULL){
		log_error(log_DATANODE, "No se pudo leer el archivo de configuración");
		return NULL;
	}
	else{
		*FILESYSTEM_IP = config_get_string_value(config,"IP_FILESYSTEM");  // Recupero la IP de FILESYSTEM del config.txt
		*FILESYSTEM_PUERTO = config_get_int_value(config,"PUERTO_FILESYSTEM"); // Recupero el PUERTO de FILESYSTEM del config.txt
		*NODO = config_get_string_value(config,"NOMBRE_NODO"); // Recupero el nombre del Nodo del config.txt
		*PUERTO_WORKER_ESCUCHA_MASTER = config_get_int_value(config, "PUERTO_WORKER_ESCUCHA_MASTER");
		*RUTA_DATABIN = config_get_string_value(config, "RUTA_DATABIN"); // Recupero la ruta del archivo data.bin del Nodo del config.txt
		*TAMANIO_DATABIN = config_get_int_value(config, "TAMANIO_DATABIN");
		*IP_NODO = config_get_string_value(config, "IP_NODO"); // Recupero la IP propia del NODO

		log_info(log_DATANODE, "La IP de este Nodo es la %s", *IP_NODO);
		log_info(log_DATANODE, "La IP de FILE_SYSTEM es: %s", *FILESYSTEM_IP);
		log_info(log_DATANODE, "El puerto de FILE_SYSTEM es: %d", *FILESYSTEM_PUERTO);
		log_info(log_DATANODE, "Este Nodo es el %s", *NODO);
		log_info(log_DATANODE, "La RUTA de archivo DATA.BIN de este Nodo es %s", *RUTA_DATABIN);
		log_info(log_DATANODE, "El tamaño del archivo DATA.BIN es de %d MiB", *TAMANIO_DATABIN);
		log_info(log_DATANODE, "El puerto donde el WORKER de este nodo escuchará procesos MASTER es el %d", *PUERTO_WORKER_ESCUCHA_MASTER);

		return config;

		}
}

int enviarInformacionNodo(int socket, int tipo_mensaje, char* nombreNodo, int cantidad_bloques, int puerto_worker_escucha_master, char* ip_nodo){

	char* cantidad_bloques_string = string_itoa(cantidad_bloques);
	char* puerto_worker_escucha_master_string = string_itoa(puerto_worker_escucha_master);

 	char* nombreYCantidadBloques = malloc(strlen(nombreNodo) + strlen(";") + strlen(cantidad_bloques_string) + strlen(";") + strlen(puerto_worker_escucha_master_string) + strlen(";") + strlen(ip_nodo) + 1); // + 1 del '\0'
 	strcpy(nombreYCantidadBloques, nombreNodo);
 	strcat(nombreYCantidadBloques, ";");
 	strcat(nombreYCantidadBloques, cantidad_bloques_string);
 	strcat(nombreYCantidadBloques, ";");
 	strcat(nombreYCantidadBloques, puerto_worker_escucha_master_string);
 	strcat(nombreYCantidadBloques, ";");
 	strcat(nombreYCantidadBloques, ip_nodo);

 	int bytesEnviados = enviar_mensaje(tipo_mensaje, nombreYCantidadBloques, socket);

	// Libero los recursos utilizados
 	free(cantidad_bloques_string);
 	free(puerto_worker_escucha_master_string);
 	free(nombreYCantidadBloques);

	return bytesEnviados;
}
