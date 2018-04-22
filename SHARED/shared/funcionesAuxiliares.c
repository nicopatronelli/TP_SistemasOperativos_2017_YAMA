/*
 * funcionesAuxiliares.c
 *
 *  Created on: 11/5/2017
 *      Author: utnso
 */

#include "funcionesAuxiliares.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> //Necesaria para struct stat y la función stat(), para calcular el tamaño de un archivo
#include <sys/types.h> //Necesaria para socket() y stat()
#include <errno.h> //Necesaria para la variable global errno (informe de errores)
#include <sys/socket.h> //Necesaria para socket() e inet_addr()
#include <netinet/in.h> //Necesaria para inet_addr()
#include <arpa/inet.h> //Necesaria para inet_addr()
#include "protocolo.h"

int tamanioArchivo(char* rutaArchivo){
	struct stat infoArchivo;
	if ( stat(rutaArchivo, &infoArchivo) == -1 ){
		perror("Fallo en la función tamanioArchivo.");
		return ERROR;
	}
	int tamanioArchivo = infoArchivo.st_size;

	return tamanioArchivo;
}

