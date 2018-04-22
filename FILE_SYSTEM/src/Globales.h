#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <sys/types.h>

// tamaño del bloque
#define SIZE_BLOQUE 1048576

// cantidad de directorios
#define CANTIDAD_DIRECTORIOS 100

// Log
t_log* archivoLog;

// Lista de Nodos
t_list* listaDataNodes;

// Tabla de Directorios
struct t_directory* arrayDirectorios;

// Tabla de Archivos
struct t_archivo* arrayArchivos;

// Variables para prueba
int almacenoCopias;
int nodosSuficientes;

// Semaforo para sincronizar el mensaje SET BLOQUE OK/NO OK
sem_t set_bloque_OK;
sem_t set_bloque_NO_OK;

// Semaforo para sincronizar el mensaje GET BLOQUE OK/NO OK
sem_t get_bloque_OK;
sem_t get_bloque_NO_OK;

// Contenido de bloque para pasarle al hilo consola
char* contenidoBloque;

// Variables para reconocer un estado seguro del FS
int estadoSeguro;
int modoClean;
int estadoCargado;
t_list* listaNodosEstadoAnterior;
t_list* listaNodosEstadoSeguro;

// SET global de sockets de DataNodes
fd_set maestroFD;

// Variable global para list_any_satisfy
char* nombreNodoActual;

// Retardo OK de bloques
int retardoOKBloques;
