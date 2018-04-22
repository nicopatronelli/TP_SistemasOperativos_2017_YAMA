/*
 * archivos.h
 *
 *  Created on: 6/11/2017
 *      Author: utnso
 */

#ifndef SHARED_ARCHIVOS_H_
#define SHARED_ARCHIVOS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // Para mmap
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h> // Para memcpy()
#include <unistd.h>
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include "protocolo.h" // Para OK y ERROR
#include "funcionesAuxiliares.h" //Necesaria para tamanioArchivo()

#define TAMANIO_BLOQUE_DATABIN 1048576 // 1048576 Bytes = 1 MiB (FIJO, lo establece el TP)

#define DATANODE 0
#define WORKER 1

/*
===============================
INICIO PROTOTIPOS FUNCIONES
===============================
*/

/* @DESC: Mapea un archivo a memoria. Si el archivo existe lo mapea y retorna un void* con TODO el contenido
 * del mismo. Si el archivo NO existe entonces lo crea y retorna un void* con el contenido del mismo, que
 * inicialmente será vacío.
 */
void* map_archivo(char* ruta_archivo);


/* @DESC: mapea en memoria el archivo ubicado en RUTA_DATABIN encargándose de toda la gestión administrativa.
 * Devuelve un puntero void* que apunta a donde comienza el archivo mapeado en memoria.
 * El parámetro flag puede ser DATANODE (0) o WORKER (1), dependiendo desde que proceso estoy llamando a
 * esta función.
 * Debe utilizar la función unmap_databin() para eliminar los recursos empleados en map_databin().
 */
void* map_databin(char* RUTA_DATABIN, int TAMANIO_DATABIN, int flag, t_log* log);


/* @DESC: desmapea el mapeo archivo_databin correspondiente al archivo ubicado en RUTA_DATABIN
 * Devuelve un 0 si todo salió bien o -1 en caso contrario.
 */
void unmap_databin(char* RUTA_DATABIN, void* archivo_databin, t_log* log);

/* @DESC: desmapea el mapeo buffer_contenido_archivo correspondiente al archivo ubicado en ruta_archivo
 * Devuelve un 0 si todo salió bien o -1 en caso contrario.
 */
void unmap_archivo(char* ruta_archivo, void* buffer_contenido_archivo, t_log* log);

/* @DESC: Lee el bloque numero_bloque_databin del archivo mapeado en databin_mapeado. Retorna
 * un puntero void* con el contenido realmente ocupado por el bloque o NULL en caso de error.
 * De fondo hace una llamada a get_bloque(), pero hace el liberar_bloque() de la misma dentro.
 * Sin embargo, si debe usarse la función liberar_bloque() para liberar la memoria alocada por
 * el valor de retorno de la función.
 */
void* get_bloque_ocupado(int numero_bloque_databin, int bytes_ocupados, void* databin_mapeado);


/* @DESC: Lee el bloque numero_bloque del archivo mapeado en databin_mapeado.
 * Retorna un puntero void* que apunta a donde comienza el bloque leido o NULL si se produzco un fallo.
 * Para liberar la memoria utilizada para guardar el bloque en memoria debe usarse la función liberar_bloque()
 */
void* get_bloque(void* databin_mapeado, int numero_bloque);


/* @DESC: Libera la memoria reservada por la función get_bloque()
 *
 */
void liberar_bloque(void* bloque);


/* @DESC: Guarda el contenido de contenido_a_grabar en el bloque numero_bloque del archivo mapeado en memoria databin_mapeado.
 * Retorna 0 si el bloque se guardo correctamente o -1 en caso de error.
 * NO realiza ningún malloc de fondo, por lo que no es necesario emplear alguna función para liberar memoria.
 */
int set_bloque(void* databin_mapeado, int numero_bloque, void* contenido_a_grabar);


/* @DESC: Crea un nombre único de archivo (auxiliar) y graba su contenido el contenido del bloque en el mismo.
 * Es necesario persistir un bloque de datos en un archivo porque el script transformador recibe por entrada un archivo y no un
 * buffer de bytes. Recordar hacer el free correspondiente del char* que retorna esta función.
 */
char* bloque_a_archivo(void* contenido_bloque, int numero_bloque);


/* @DESC: Graba el contenido de un buffer en un archivo.
 *
 */
int persistir_buffer_en_archivo(char* nombre_archivo, void* buffer);

/*
---------------------------
FIN PROTOTIPO FUNCIONES
---------------------------
*/

#endif /* SHARED_ARCHIVOS_H_ */
