/*
 * funcionesDataNode.h
 *
 *  Created on: 6/11/2017
 *      Author: utnso
 */

#ifndef FUNCIONESDATANODE_H_
#define FUNCIONESDATANODE_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <shared/protocolo.h>
#include <shared/archivos.h>
#include <shared/mensajes.h>

extern t_log* log_DATANODE;

/*
===============================
INICIO PROTOTIPOS FUNCIONES
===============================
*/

t_config* configuracion_DATANODE(char* ruta_archivo_configuracion, char** FILESYSTEM_IP, int* FILESYSTEM_PUERTO, char** NODO, int* PUERTO_WORKER_ESCUCHA_MASTER, char** RUTA_DATABIN, int* TAMANIO_DATABIN, char** IP_NODO);

int enviarInformacionNodo(int socket, int tipo_mensaje, char* nombreNodo, int cantidad_bloques, int puerto_worker_escucha_master, char* ip_nodo);

int definirAccionHeader(t_protocolo* mensaje, int socket);

/*
---------------------------
FIN PROTOTIPO FUNCIONES
---------------------------
*/

#endif /* FUNCIONESDATANODE_H_ */
