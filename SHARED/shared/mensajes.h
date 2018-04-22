/*
 * mensajes.h
 *
 *  Created on: 20/10/2017
 *      Author: utnso
 */

#ifndef SHARED_MENSAJES_H_
#define SHARED_MENSAJES_H_

#include <stdio.h> //Biblioteca estandard para entrada y salida
#include <stdlib.h> //Manejo de memoria dinamica y otros
#include <string.h> //Biblioteca estandard para el manejo de strings
#include <stdbool.h> //Para incluir el tipo de dato bool, y usar true y false
#include <sys/socket.h> //Necesaria para socket() e inet_addr()
#include <netinet/in.h> //Necesaria para inet_addr()
#include <arpa/inet.h> //Necesaria para inet_addr()
#include <sys/types.h> //Necesaria para socket() y stat()
#include <unistd.h> //Necesaria para close() -> Para cerrar un socket
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include "protocolo.h"

#define ERROR_RECV ((void*)(-1))
#define CERRARON_SOCKET ((void*)1)

/*** INICIO DE PROTOTIPOS DE FUNCIONES en mensajes.c ***/


/* @DESC: Se encarga de recibir el header, deserializarlo y manejar los errores correspondientes.
 * Crea y devuelve el protocolo donde se va a recibir el header primero (y posteriormente el contenido del mensaje, en la función
 * recibir_mensaje()). El único free que queda pendiente para esta función es el del puntero al protocolo (el protocolo->mensaje que
 * tiene "" provisoriamente se libera dentro).
 */
t_protocolo* recibir_header(int socket_header);


/* @DESC: Se encarga de recibir el HEADER, y luego el MENSAJE, haciendo de fondo la deserialización.
 * Si tuvo éxito, devuelve el protocolo cargado con el tipo de mensaje, el tamaño del mensaje y el contenido del mensaje.
 * Si del otro lado cerraron la conexión (enviaron un 0) retorna el puntero CERRARON_CONEXION. En caso de fallo general
 * retorna NULL.
 * Para liberar la memoria alocada por esta función debe utilizar eliminar_protocolo().
 */
t_protocolo* recibir_mensaje(int socket_recepcion);


/* @DESC: Devuelve la cantidad de bytes enviados o -1 en caso de error.
 * Esta función no conlleva ningún free adicional (lo hace internamente).
 */
int enviar_mensaje(int tipo_mensaje, char* mensaje, int socket_envio);


/* @DESC: Devuelve el fd del socket asignado al nuevo cliente (no el socket de escucha)
 * o -1 en caso de error.
 */
int aceptar_clienteEn(int socket_escucha, struct sockaddr_in direccionCliente);

#endif /* SHARED_MENSAJES_H_ */
