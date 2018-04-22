#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> //Necesaria para socket() y stat()
#include <errno.h> //Necesaria para la variable global errno (informe de errores)
#include <sys/socket.h> //Necesaria para socket() e inet_addr()
#include <netinet/in.h> //Necesaria para inet_addr()
#include <commons/string.h> // Para string_new()

void noError_Address_already_in_use(int unSocket){
	/*Esta funcion evita que se produzca el error "Address already in use" (El puerto ya está en uso) cuando usamos bind.
	 * Sin embargo, si el error ya se produzco hay que esperar unos minutos a que se libere el puerto, o mejor aún, utilizar
	 * el siguiente comando en una terminal:
	 * 			sudo fuser -k numeroPuerto/tcp
	 * donde numeroPuerto es el puerto que queremos liberar.
	 */
	int activado = 1;
	setsockopt(unSocket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));
}
int nuevoSocket(void){
	int unSocket = socket(AF_INET, SOCK_STREAM, 0);
		if ( unSocket == -1){ // En caso de error
			return -1;
		}
	noError_Address_already_in_use(unSocket); //Evitamos que al usar este socket se produzca el error "Address already in use"
	return unSocket;
}

struct sockaddr_in nuevaDireccion(unsigned short int puerto, char* ip){

	struct sockaddr_in unaDireccion;

	unaDireccion.sin_family = AF_INET; // El valor de este campo es CONSTANTE para todo el TP
	unaDireccion.sin_port = htons(puerto);
	if ( strcmp(ip, "INADDR_ANY") == 0){
		unaDireccion.sin_addr.s_addr = htons(INADDR_ANY);} // Utilizo la ip de la propia máquina (en network byte order)
	else{
		unaDireccion.sin_addr.s_addr = inet_addr(ip);} // Utilizo la ip que recibo por parámetro

	memset(&(unaDireccion.sin_zero), '\0', 8); // Administrativo

	return unaDireccion;
}

struct sockaddr_in crearDireccion(int familia, in_addr_t direccion, int puerto) {

	struct sockaddr_in direccion_servidor;
	direccion_servidor.sin_family = familia;
	direccion_servidor.sin_addr.s_addr = direccion;
	direccion_servidor.sin_port = htons(puerto);
	memset(&(direccion_servidor.sin_zero), '\0', 8);

	return direccion_servidor;

}

int bind1(int unSocket, struct sockaddr_in* unaDireccion){
	// Enmascaro la llamada al bind original y la comprobación de errores
	int valor_bind = bind(unSocket, (struct sockaddr*)unaDireccion, sizeof(struct sockaddr));
	if ( valor_bind != 0){ //En caso de error
		perror("Falló el bind");
		return -1;
	}
	return EXIT_SUCCESS;
}

int ponerseALaEscuchaEn(struct sockaddr_in unaDireccion) {

	/* MEJORA: ponerseALaEscuchaEn podría recibir por parámetro la IP y el PUERTO donde ponerse a la escucha en lugar del
	 * struct sockaddr_in y llamar a nuevaDireccion() desde dentro Así, la línea:
	 * 		struct sockaddr_in direccionYama = nuevaDireccion(PUERTO, IP);
	 * quedaría dentro de ponerseALaEscuchaEn y el programador solo se limita a pasarle la IP y el PUERTO.
	 */

	int socketEscucha = nuevoSocket();
	if ( socketEscucha == -1) {
		perror("Error al obtener el socket de escucha.");
		return EXIT_FAILURE;
	}

    if ( bind1(socketEscucha, (struct sockaddr *)&unaDireccion) == -1) {
            perror("No se pudo realizar el 'bind'.");
            return EXIT_FAILURE;
    }

    // Ponerme a la escucha...
    if ( listen(socketEscucha, 10) == -1 ) {
            perror("Error al realizar la funcion 'listen'.");
            return EXIT_FAILURE;
    }

    return socketEscucha;
}

int aceptarClienteEn(int socketEscucha, struct sockaddr_in direccionCliente, t_cliente* cliente) {

	int sizeDireccion = sizeof(struct sockaddr_in);
	int nuevoFDCliente = 0;

	if ((nuevoFDCliente = accept(socketEscucha, (void *)&direccionCliente, (socklen_t*)&sizeDireccion)) == -1) {
		printf("Error aceptando un cliente.\n");
		exit(-1);
	} else {
		cliente->socket_client = nuevoFDCliente;
		cliente->nombreNodo = string_new();
		cliente->ipNodo = string_new();
		cliente->puertoEscuchaMaster = 0;
	}
	return nuevoFDCliente;
}

int conectarseA(struct sockaddr_in unServidor) {

	int unSocket = nuevoSocket();

	if ( unSocket == -1) {
		perror("No se pudo crear el socket solicitado.");
        return ERROR;
    }
    if ( connect(unSocket, (struct sockaddr *)&unServidor, sizeof(struct sockaddr)) == -1 ) {
        perror("Error al realizar la función 'connect'.");
        return ERROR;
    }
	return unSocket;
}
