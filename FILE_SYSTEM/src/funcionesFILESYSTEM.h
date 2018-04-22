#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <commons/collections/dictionary.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <readline/readline.h>
#include <shared/sockets.h>
#include <shared/protocolo.h>
#include <shared/estructuras.h>
#include <shared/mensajes.h>
#include <shared/archivos.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include "Globales.h"
#include "Directorios.h"
#include "Nodos.h"

#define COPIA_0 0
#define COPIA_1 1

/* @DESC: Recibe la ruta del archivo perteneciente al file system local (Ubuntu en nuestro caso) y retorna
 * la cantidad de bloques que dicho archivo ocupará en el FILE_SYSTEM de YAMA.
 */
int cantidad_bloques_archivo_binario(char* ruta_archivo);

/* @DESC: Es similar a aceptarClienteEn con la diferencia de cargar el parámetro adicional ip_nodo con la ip
 * del proceso DATANODE que se conecta.
 */
int aceptarDataNode(int socketEscucha, struct sockaddr_in direccionCliente, t_datanode* cliente);

int verificarEstadoInseguro(char* nombreNodo);
int verificarEstadoAnterior();
int crearTablaDirectorios();
int crearTablaArchivos();
int validarArchivoYama(char* rutaArchivo);
int cargarEstadoAnterior();
int cargarTablaArchivos();
int inicializarTablaArchivos(int tablaArchivos);
int grabarArchivo(int indiceArchivo, char* ruta, char* nombre, int ocupado);
int buscarArchivo(char* ruta, char* nombre);
int borrarArchivo(char* archivo);
int borrarArchivoYama(char* archivo, int indiceDirectorio, char* rutaYama);
int formatearFileSystem();
int espacioDisponible(int bloquesNecesarios);
int enviarBloque(char* bloque, int socketNodo, int sizeBloque, int numeroBloque);
int sizeArchivo(FILE* archivo);
int crearEntradaTablaArchivos(char* ruta, char* nombre);
int crearArchivoDetalleArchivoCargado(char* rutaArchivoOriginal, char* rutaArchivoYAMA, int indiceDirectorio, int tamanio, int bloques, int tipoArchivo);
int actualizarArchivoDetalleArchivo(int numeroBloqueActual, int copia, int sizeOcupadoBloque, char* nodoDisponible, int bloqueDisponible, int indiceDirectorio, char* nombreArchivo);
int almacenarArchivoBinario(char* nombreArchivoOriginal, char* nombreArchivoYama, int indiceDirectorio);
int almacenarArchivoTexto(char* nombreArchivoOriginal, char* nombreArchivoYama, int indiceDirectorio);
int almacenarArchivoReduccionGlobal(char* bufferArchivoReduccionGlobal, char* rutaYama, int indiceDirectorio, char* rutaDirectorioYama);
int mostrarArchivo(char* archivo);
int moverArchivo(char* rutaYama, char* nombreArchivoYama, int indiceOrigen, int indiceDestino, char* nuevaRutaYama);
int copiarArchivo(char* archivo);
int crearCopiaBloqueEn(char* archivo, int bloque, char* nodo);
int informacionArchivo(char* archivo);
int validarArchivo(char* nombreArchivo);
int tieneHijos(int indiceDirectorioActual);
int actualizarSiBloqueOk(char* nodoDisponible, int bloqueDisponible, int numeroBloqueActual, int sizeOcupadoBloque, int indiceDirectorio, char* nombreArchivo, int* bloquesEnviados, int* bloquesPendientes, int numeroCopia);
t_bloque* leerBloqueArchivo(char* nombreArchivoDetalle, int numeroBloque, int numeroCopia);
int armarArchivo(char* rutaArchivoYama, char* rutaArchivoTemporal);
int recuperarInfoBloque(t_config* archivo, int numeroBloque, int numeroCopia, char* nodo, int* bloqueDataBin, int* bytesOcupados);
int recuperarInfoArchivo(char* rutaArchivoYama);
char* obtenerNombreArchivo(char* rutaArchivo);
int renombrarArchivoYama(char* rutaArchivoYama, char* nuevoNombreArchivo);
int recuperarCantidadBloquesArchivo(char* archivoDetalle);
t_list* recuperarInfoBloquesArchivoTexto(char* contenidoArchivo, int sizeArchivo);
int eliminarMetadata();
int crearMetadata();
int copiarArchivoAFSLocal(char* pathYAMA, char* pathFSLocal);
int actualizarNombreArchivoEnTablaArchivos(char* rutaArchivoYama, char* nombreArchivoYama, char* nuevoNombreArchivo);
int actualizarRutaArchivoEnTablaArchivos(char* rutaArchivoYama, char* nombreArchivoYama, char* nuevaRutaArchivo);
