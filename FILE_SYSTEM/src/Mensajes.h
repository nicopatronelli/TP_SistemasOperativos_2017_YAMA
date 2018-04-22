/*
 * Mensajes.h

 *
 *  Created on: 15/10/2017
 *      Author: utnso
 */
#include <shared/protocolo.h>

int enviarConexionAceptada(int socket);
void definirAccionHeader(t_protocolo *protocolo, uint32_t socket);
int recuperarYEnviarDatosArchivo(char* nombreArchivo, int indiceDirectorio, int socket);
