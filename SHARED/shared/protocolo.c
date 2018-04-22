/*
 * protocolo.c
 *
 *  Created on: 23/4/2017
 *      Author: utnso
 */
#include "protocolo.h"

#define PROCESO_YAMA 1
#define PROCESO_FILE_SYSTEM 2
#define PROCESO_DATANODE 3
#define PROCESO_WORKER 4
#define PROCESO_MASTER 5

int enviarMensajeHandshake (int socket_cliente, int HANDSHAKE){

 	int bytesAEnviar, bytesEnviados;

 	t_handshake *protocoloHandshake = crearHandshake(HANDSHAKE, 0);
 	bytesAEnviar = size_handshake(protocoloHandshake);
 	unsigned char* buffer = malloc(bytesAEnviar);
 	memset(buffer, '\0', bytesAEnviar);
	serializar_handshake(buffer, protocoloHandshake);

	if ((bytesEnviados = send(socket_cliente, buffer, bytesAEnviar, 0)) < 0){
		return EXIT_FAILURE; }

	eliminar_handshake(protocoloHandshake);
	free(buffer);

	return EXIT_SUCCESS;
}

int serializar_int(int nro, void *buffer) {
	int tamanio_int = sizeof(int);

	memcpy(buffer, &nro, tamanio_int);

	return tamanio_int;
}

int deserializar_int(int* nro, void* buffer) {

	int tamanio_int = sizeof(int);

	memcpy(nro, buffer, tamanio_int);

	return tamanio_int;
}

int serializar_string(char* string, void* buffer, int tamanio_receptor) {

	memcpy(buffer, string, tamanio_receptor);
	// tamanio_receptor viene de strlen(protocolo->mensaje), por lo que no contabiliza el byte de '/0'

	return (tamanio_receptor);
}

int deserializar_string(char* string, void* buffer, int longitud_string) {

	memcpy(string, buffer, longitud_string);
	string[longitud_string] = '\0'; // Convierto el void* en un char*

	return longitud_string;
}

unsigned int size_handshake(t_handshake *handshake) {

	unsigned int sizeHandshake = 0;
	sizeHandshake = 2 * sizeof(uint32_t); // Tipo de Mensaje (uint32_t) + Tamaño Mensaje (uint32_t)
	return sizeHandshake;
}

int size_protocolo(t_protocolo* protocolo) {

	int tamanio;
	int tamanioString = strlen(protocolo->mensaje); //size_protocolo NO contabiliza el byte de '/0' al final de protocolo->mensaje

	tamanio = 2 * sizeof(int) + tamanioString;

	return tamanio;
}

void deserializar_handshake (unsigned char *buffer, t_handshake *handshake){

    int offset = 0;

    offset += deserializar_int(&handshake->funcion, buffer + offset);
    offset += deserializar_int(&handshake->sizeMensaje, buffer + offset);

}

int deserializarHeader (unsigned char* buffer, t_protocolo* protocolo){
	deserializar_header(buffer, protocolo);
	return true;
}

void deserializar_header (void* buffer, t_protocolo* protocolo){

    int offset = 0;

    offset += deserializar_int(&protocolo->funcion, buffer);
    offset += deserializar_int(&protocolo->sizeMensaje, buffer + offset);

}

t_handshake *crearHandshake (int header ,int tamanioMensaje){

	t_handshake *handshake = malloc(sizeof(t_handshake));

	handshake->funcion = header;
	handshake->sizeMensaje = tamanioMensaje;

	return handshake;
}

t_protocolo *crearProtocolo (int header ,int tamanio_mensaje, char* mensaje ){

	/* (?)DUDA: Si el único campo dinámico de t_protocolo es mensaje porque tengo que hacer un malloc del tipo t_protocolo,
	 * ... con hacer protocolo->mensaje = malloc(tamanioMensaje) bastaría? Si borro:
	 *
	 * t_protocolo *protocolo = malloc(sizeof(t_protocolo));
	 *
	 * da SIGSEGV (Segmentation Fault)
	 */
	t_protocolo *protocolo = malloc(sizeof(t_protocolo));

	protocolo->funcion = header;
	protocolo->sizeMensaje = tamanio_mensaje;
	protocolo->mensaje = malloc(tamanio_mensaje + 1); //
	/* tamanio_mensaje viene de strlen(mensaje) cuando se invoca a crearProtocolo. Por lo tanto, tamanio_mensaje coincide
	 * con la longitud del char*, pero NO contabiliza el byte de final de string '/0'. Por lo tanto, sumo 1 a mano en el malloc.
	 * Hago esto porque strcpy copia char*, y sino reservo el byte adicional no se va a poder copiar/agregar el '/0'.
	 */
	strcpy(protocolo->mensaje, mensaje);

	return protocolo;
}

void serializar_handshake(unsigned char *buffer, t_handshake *handshake) {

	int offset = 0;
	offset += serializar_int(handshake->funcion, buffer + offset);
	offset += serializar_int(handshake->sizeMensaje, buffer + offset);

}

void serializar_protocolo(void* buffer, t_protocolo *protocolo) {

	int offset = 0;

	offset += serializar_int(protocolo->funcion, buffer);
	offset += serializar_int(protocolo->sizeMensaje, buffer + offset);
	serializar_string(protocolo->mensaje, buffer + offset, protocolo->sizeMensaje);
}

int deserializarProtocolo(unsigned char *buffer, t_protocolo *protocolo) {
	deserializar_protocolo(buffer, protocolo);
	return true;
}

void deserializar_protocolo(unsigned char *buffer, t_protocolo *protocolo) {

	int offset = 0;

	offset += deserializar_int(&protocolo->funcion, buffer + offset);
	offset += deserializar_int(&protocolo->sizeMensaje, buffer + offset);

	// Solo si el mensaje es mayor a cero serializo el string del protocolo
	if (protocolo->sizeMensaje > 0) {
		protocolo->mensaje = malloc(protocolo->sizeMensaje);
		deserializar_string(protocolo->mensaje, buffer + offset, protocolo->sizeMensaje);
		free(protocolo->mensaje);
	}

}

int serializarProtocolo(unsigned char* buffer, t_protocolo* protocolo){
	serializar_protocolo(buffer, protocolo);
	return true;
}

int enviarBuffer(unsigned char* buffer, int socket, int size){
	return send(socket, buffer, size, 0);
}

int liberarProtocolo(t_protocolo* protocolo){
	eliminar_protocolo(protocolo);
	return true;
}

int liberarBuffer(unsigned char* buffer){
	free(buffer);
	return true;
}

void eliminar_handshake(t_handshake *handshake) {
	free(handshake);
}

void eliminar_protocolo(t_protocolo *protocolo) {
	/*
	 * MUY IMPORTANTE: Cuando en un struct tenemos campos que son punteros para los cuales hacemos diversos malloc,
	 * NO alcanza con hacer un free del struct, sino que debemos hacer un free por cada campo que tenga un malloc:
	 *
	 * REGLA: 1 MALLOC - 1 FREE
	 *
	 * En este caso, si hacemos free(protocolo) no estaremos liberando la memoria alocada para el campo protocolo->mensaje,
	 * generándose así un memory leak.
	 */

	free(protocolo->mensaje);
	free(protocolo);

	/* El orden de libeación es importante: si lo hago al revés pierdo la referencia a protocolo y no puedo liberar
	 * el campo protocolo->mensaje.
	 */
}
