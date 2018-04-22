/*
 * Nodos.h
 *
 *  Created on: 21/10/2017
 *      Author: utnso
 */

#ifndef NODOS_H_
#define NODOS_H_
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <shared/protocolo.h>
#include <shared/mensajes.h>
#include <shared/estructuras.h>
#include <shared/sockets.h>
#include <shared/funcionesAuxiliares.h>
#include <sys/mman.h>
#include <math.h>

/*
===========================
INICIO funciones_NODOS
===========================
*/

// Estructura para el manejo de DataNodes
// Estructura para manejo de clientes conectados
typedef struct t_datanode {
    int socket_client;
    char nombreNodo[10];
    char ipNodo[16];
    int puertoEscuchaMaster;
} t_datanode;

// Estructura para el manejo de los bloques de los archivos de texto
typedef struct{
	int inicio;
	int longitud;
}t_bloque_texto;

/* @DESC: Busca y carga en nodoDisponible el nodo con mayor disponibilidad. Retorna un entero que es
 * el numero de bloque libre para nodoDisponible.
 */
int obtenerNodoYBloqueDisponible(char* nodoDisponible, int indiceUltimoNodoUsado);

/* @DESC: Busca y carga en nodoCopia el nodo con mayor disponibilidad, sin tener en cuenta nodoDisponible. Retorna un entero que es
 * el numero de bloque libre para nodoCopia.
 */
int obtenerNodoCopiaYBloqueDisponible(char* nodoDisponible, char* nodoCopia, int indiceUltimoNodoCopiaUsado);
int cargarListaNodosEstadoAnterior();
int verificarEstadoSeguro();
int buscarIndiceNodoTablaNodos(char* nodo, int cantidadNodos);
int mostrarInformacionNodo(char* nombreNodo);
/*
===========================
FIN funciones_NODOS
===========================
*/

/*
===========================
INICIO funciones_CONFIG
===========================
*/

int config_save_in_file(t_config *self, char* path);
void config_set_value(t_config *self, char *key, char *value);
int config_save(t_config *self);
void config_remove_key(t_config* self, char* key);

/*
===========================
FIN funciones_CONFIG
===========================
*/

int recuperarCantidadNodos();
int recuperarListaNodos(char** listaNodos);
int obtenerEspacioDisponibleNodo(char* nodo, t_config* tablaNodos);
int obtenerSocketNodo(char* nodo);
int obtenerIPYPuertoNodo(char* nodo, char* ipNodo);

int buscarIndiceNodo(char* nodo, int cantidadNodos);


int buscarSocketEn(t_list* lista, int socket, char* nombreNodo);
int eliminarDataNode(int socket, char* nodo);
int agregarNodo(char* nombreNodo, char** listaNodos);
int buscarNodo(char* nombreNodo);
int actualizarNombreYPuertoNodo(t_list* lista, char* nombreNodo, int puertoEscuchaMaster, int socket, char* ipNodo);

/*
===========================
INICIO funciones_TABLA_DE_NODOS
===========================
*/

int crearTablaNodos();
int formatearTablaNodos();
int actualizarTablaNodos(char* nombreNodo, int totalBloques, int bloquesDisponibles);
int eliminarNodoTablaNodos(char* nombreNodo);
int actualizarEspacioLibreNodo(char* nombreNodo, int sumarRestar);

/*
===========================
FIN funciones_TABLA_DE_NODOS
===========================
*/

/*
===========================
INICIO funciones_BITMAP
===========================
*/

int verificarBitmapsEstadoAnterior();
int actualizarBitmapBloques(char* nombreNodo, int bloqueDisponible, int ocupoBloque);
int generarBitMapBloquesVacio(char* nombreNodo, int cantidadBloques);
int recuperarNumeroBloqueLibre(char* nodo, int cantidadBloques);
int formatearBitmapBloques(char* nodo);
int recuperarCantidadBloques(char* nodo);
int pedirBloque(int bloqueDataBin, int socket);
int leerYLiberarBloquesArchivo(char* archivoDetalle);

/*
===========================
FIN funciones_BITMAP
===========================
*/

int verificarNodoEstadoAnterior(char* nombreNodo);
int verificarNodoEstadoSeguro(char* nombreNodo);
int sizeLista(char** array);
int archivoRequiereNodo(char* nombreArchivo, char* nombreNodo);
int cargarListaNodosEstadoSeguro();
int verificarArchivos();
int verificarComposicionArchivo(char* rutaArchivo, char* nombreArchivo, t_list* listaDataNodes);
bool _nombreNodoEstaEnLista(void* nodoLista);



#endif /* NODOS_H_ */
