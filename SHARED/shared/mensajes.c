/*
 * mensajes.c
 *
 *  Created on: 20/10/2017
 *      Author: utnso
 */

#include "mensajes.h"

t_protocolo* recibir_header(int socket_header){

	void* buffer_header = malloc(HEADER_SIZE);
	int bytes_recibidos = recv(socket_header, buffer_header, HEADER_SIZE, 0); // Recibo el HEADER

	// INICIO - Si el recv() se hizo correctamente,,,
	if( bytes_recibidos == HEADER_SIZE){ // Si se recibieron los 8 Bytes que conforman el HEADER

		t_protocolo* protocolo_Recepcion = crearProtocolo(0, 1, "");
		deserializar_header(buffer_header, protocolo_Recepcion);

		/* Libero la memoria reservada en el campo protocolo_Recepcion->mensaje por la función crearProtocolo() para el "".
		 * ¿Por qué? Porque el mensaje se va a recibir luego de recibir el HEADER y se volverá a hacer un malloc del tamanio_mensaje
		 * que ahora es conocido. El malloc para "" es meramente auxiliar.
		 */
		free(protocolo_Recepcion->mensaje); // Falta el free del puntero protocolo_Recepcion
		free(buffer_header); // Ya no usamos buffer_header

		// En este instante protocolo_Recepcion tiene completos los campos funcion(tipo_mensaje) y sizeMensaje(tamaño_mensaje)
		return protocolo_Recepcion;

	}	// FIN - Si el recv() se hizo correctamente
	else{	// INICIO - Manejo excepciones recv()

		free(buffer_header); // Libero la memoria de buffer_header pues ya no hay bytes para recibir

		if ( bytes_recibidos == 0){ // Cerraron la conexión en el socket socket_header
			return CERRARON_SOCKET;
		}else{
			return NULL; // Para cualquier otro caso
		}

	}// FIN - Manejo excepciones recv()

}

t_protocolo* recibir_mensaje(int socket_recepcion){

	// Recibo el HEADER
	t_protocolo* protocolo_Recepcion = recibir_header(socket_recepcion); // Falta el free de protocolo_Recepcion

	// Si hay algun error en el HEADER salgo...
	if ( protocolo_Recepcion == NULL ){
		return NULL;
	}

	if ( protocolo_Recepcion == CERRARON_SOCKET){
		return CERRARON_SOCKET;
	}

	// Si el HEADER se recibió correctamente entonces puedo recibir el contenido del MENSAJE
	void* buffer_mensaje = malloc(protocolo_Recepcion->sizeMensaje);

	int bytesRecibidos = recv(socket_recepcion, buffer_mensaje, protocolo_Recepcion->sizeMensaje, MSG_WAITALL);

	if ( bytesRecibidos != protocolo_Recepcion->sizeMensaje)
	{
		perror("Hubo un problema en la recepción del contenido del mensaje");

		return NULL;
	}
	else
	{
		// +1 para el '/0', pues el campo protocolo_Recepcion->mensaje es un char*
		protocolo_Recepcion->mensaje = malloc(protocolo_Recepcion->sizeMensaje + 1);

		deserializar_string(protocolo_Recepcion->mensaje, buffer_mensaje, protocolo_Recepcion->sizeMensaje);
		free(buffer_mensaje);

		/* OBSERVACION: Debe llamarse a eliminar_protocolo(protocolo_Recepcion) fuera de la función para liberar la memoria
		 * reservada por está.
		 */
		return protocolo_Recepcion;
	}
}

int enviar_mensaje(int TIPO_MENSAJE, char* mensaje, int socket_envio){

		// 1° Creó e inicializó el protocolo
		t_protocolo* protocoloAEnviar = crearProtocolo(TIPO_MENSAJE, strlen(mensaje), mensaje);

		// 2° Obtengo el tamaño del protocolo a enviar
		int bytesAEnviar = size_protocolo(protocoloAEnviar); // bytesAEnviar = 2*4(int)+strlen(mensaje) <- NO se cuenta el '/0' al final de mensaje

		// 3° Serializamos el protocolo (lo metemos en un buffer)
		void* protocoloSerializado = malloc(bytesAEnviar); // No se cuenta el byte del '/0' al final de protocolo->mensaje
		serializar_protocolo(protocoloSerializado, protocoloAEnviar);

		// 4° Enviamos el mensaje y verificamos que se envie correctamente
		int bytes_enviados = send(socket_envio, protocoloSerializado, bytesAEnviar, 0);

		// 5° Liberamos recursos
		eliminar_protocolo(protocoloAEnviar);
		free(protocoloSerializado);

		if (bytes_enviados == bytesAEnviar){ // Se enviaron la cantidad de bytes correcta
			return bytes_enviados;
		}else{ // Fallo en el envio
			return ERROR;
		}
	}

int aceptar_clienteEn(int socket_escucha, struct sockaddr_in direccionCliente) {

	socklen_t addrlen = sizeof(struct sockaddr_in);
	int fd_cliente;
	fd_cliente = accept(socket_escucha, (struct sockaddr*)&direccionCliente, &addrlen);

	if ( fd_cliente == -1 ) {
		perror("Error aceptando un cliente");
		return ERROR;
	}

	return fd_cliente;
}

