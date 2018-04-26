/*

 * funcionesDiversas.c
 *
 *  Created on: 26/11/2017
 *      Author: utnso

#include "funcionesDiversas.h"

t_job* crearJob(){
	t_job* job = malloc(sizeof(job));
	job->idJob= proximoJob();
	job->transformaciones= list_create();
	job->reducciones = list_create();
	job->reduccionesGlobales = list_create();
	log_info("Se creo con exito el job id: %d", job->idJob);
	return job;
}

int proximoJob(){
	int idJob = 0;
	idJob++;
	return idJob;
}


void recibir_copia0(t_protocolo* protocolo_Recepcion, int socket_YAMA_FILESYSTEM, t_Bloque* bloque_actual, int bloque){

	protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

	if ( protocolo_Recepcion->funcion == COPIA0_NO_EXISTE){
		bloque_actual->copia0.existe_copia = false;
		log_info(log_YAMA, "La COPIA 0 del bloque %d del archivo a procesar NO EXISTE.", bloque);
	}else{ // Si la COPIA0 existe entonces recibo su información

		bloque_actual->copia0.existe_copia = true;

		// PASO 2 - GUARDO EL NOMBRE_NODO DONDE ESTA EL BLOQUE (Ej: NODO1)
		strcpy(bloque_actual->copia0.nombre_nodo, protocolo_Recepcion->mensaje);

		// PASO 3 - RECIBO LA IP_NODO
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		strcpy(bloque_actual->copia0.ip_nodo, protocolo_Recepcion->mensaje);

		// PASO 4 - RECIBO EL PUERTO_WORKER_ESCUCHA_MASTER
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia0.puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);

		// PASO 5 - RECIBO EL NUMERO DEL BLOQUE_DATABIN DONDE SE GUARDA EL CONTENIDO DEL BLOQUE
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia0.numero_bloque_databin = atoi(protocolo_Recepcion->mensaje);

		// PASO 6 - RECIBO LOS BYTES_OCUPADOS DENTRO DEL BLOQUE DEL DATA.BIN (CUANTO DEL 1MiB)
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia0.bytes_ocupados = atoi(protocolo_Recepcion->mensaje);

	} // FIN else "Si la COPIA0 existe entonces recibo su información"

} // FIN recibir_copia0


void recibir_copia1(t_protocolo* protocolo_Recepcion, int socket_YAMA_FILESYSTEM, t_Bloque* bloque_actual, int bloque){

	protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

	if ( protocolo_Recepcion->funcion == COPIA1_NO_EXISTE){
		bloque_actual->copia1.existe_copia = false;
		log_info(log_YAMA, "La COPIA 1 del bloque %d del archivo a procesar NO EXISTE.", bloque);
	}else{ // Si la COPIA1 existe entonces recibo su información

		bloque_actual->copia1.existe_copia = true;

		// PASO 2 - GUARDO EL NOMBRE_NODO DONDE ESTA EL BLOQUE (Ej: NODO1)
		strcpy(bloque_actual->copia1.nombre_nodo, protocolo_Recepcion->mensaje);

		// PASO 3 - RECIBO LA IP_NODO
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		strcpy(bloque_actual->copia1.ip_nodo, protocolo_Recepcion->mensaje);

		// PASO 4 - RECIBO EL PUERTO_WORKER_ESCUCHA_MASTER
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia1.puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);

		// PASO 5 - RECIBO EL NUMERO DEL BLOQUE_DATABIN DONDE SE GUARDA EL CONTENIDO DEL BLOQUE
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia1.numero_bloque_databin = atoi(protocolo_Recepcion->mensaje);

		// PASO 6 - RECIBO LOS BYTES_OCUPADOS DENTRO DEL BLOQUE DEL DATA.BIN (CUANTO DEL 1MiB)
		eliminar_protocolo(protocolo_Recepcion);
		protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

		bloque_actual->copia1.bytes_ocupados = atoi(protocolo_Recepcion->mensaje);

	} // FIN else "Si la COPIA1 existe entonces recibo su información"

} // FIN recibir_copia1
*/
