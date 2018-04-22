#ifndef SOCKETS_H_   /* Include guard */
#define SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "protocolo.h" // Para #define ERROR -1

typedef struct sockaddr_in t_direccion;

// Estructura para manejo de clientes conectados
typedef struct t_cliente {
    int socket_client;
    char* nombreNodo;
    char* ipNodo;
    int puertoEscuchaMaster;
} t_cliente;

/* @DESC: Esta funcion evita que se produzca el error "Address already in use"
 * (El puerto ya está en uso) cuando usamos bind.
 * @NOTA: Sin embargo, si el error ya se produjó hay que esperar unos minutos
 * a que se libere el puerto, o mejor aún, utilizar
 * el siguiente comando en una terminal:
 *
 * 			sudo fuser -k numeroPuerto/tcp
 *
 * donde numeroPuerto es el puerto que queremos liberar.
 */
void noError_Address_already_in_use(int unSocket);

/* @DESC: Retorna una estructura struct sockaddr_in con el puerto e ip suministrados como parámetros
 * @NOTA: Si se va a utilizar la ip local pasando el macro INADDR_ANY (que vale 0) como parámetro debe hacerse como si fuese un
 * string, es decir, entre comillas dobles.
 * Ejemplo:
 * 		struct sockaddr_in unaDireccion = nuevaDireccion(8080, "INADDR_ANY");
 */
struct sockaddr_in nuevaDireccion(unsigned short int puerto, char* ip);

/* @DESC: Crea un nuevo socket de flujo (SOCK_STREAM) de la familia AF_INET. Es el único tipo de socket que vamos a usar en el TP.
 * Retorna un int con el File Descriptor (FD) asociado al mismo o -1 en caso de error.
 */
int nuevoSocket(void);

int aceptarClienteEn(int socketEscucha, struct sockaddr_in direccionCliente, t_cliente* cliente);

int agregarCliente(t_cliente* listaClientes, t_cliente* nuevoCliente, int socket);

struct sockaddr_in crearDireccion(int familia, in_addr_t direccion, int puerto);

int ponerseALaEscuchaEn(struct sockaddr_in direccion);

/* @DESC: Crea un nuevo socket y se conecta a través de el por medio de la función connect a la direccion pasada
 * por parámetro.
 * Retorna un int con el File Descriptor (FD) del socket creado o -1 en caso de error.
 */
int conectarseA(struct sockaddr_in unServidor);

#endif // SOCKETS_H_



