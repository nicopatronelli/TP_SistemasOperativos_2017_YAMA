/*
 * archivos.c
 *
 *  Created on: 6/11/2017
 *      Author: utnso
 */

#include "archivos.h"

void* map_archivo(char* ruta_archivo){

	char mode[] = "0777"; // Permisos totales
	int permisos = strtol(mode, 0, 8); // Administración para los permisos

	int fd_archivo = open(ruta_archivo, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);

	chmod(ruta_archivo, permisos); // Aplico permisos al archivo

	int tamanio_archivo = tamanioArchivo(ruta_archivo);

	/* IMPORTANTE: No hace falta hacer (y no se debe hacer) archivo_mapeado = malloc(tamanio_archivo), pues
	 * el mmap lo hace de fondo. A su vez, el unmap debe hacer el free correspondiente, por lo que no tenemos que hacer
	 * nada.
	 */
	void* archivo_mapeado = mmap(NULL, tamanio_archivo, PROT_READ | PROT_WRITE, MAP_SHARED, fd_archivo, 0);

	if ( archivo_mapeado == MAP_FAILED){
		printf("Error al mapear el archivo %s.", ruta_archivo);
		return NULL;
	}

	close(fd_archivo); // Cerramos el archivo pues ya lo tenemos mapeado en memoria, no es necesario mantenerlo abierto

	return archivo_mapeado;

}

void* map_databin(char* RUTA_DATABIN, int TAMANIO_DATABIN, int flag, t_log* log){

		char mode[] = "0777"; // Permisos totales
		int permisos = strtol(mode, 0, 8); // Administración para los permisos

		// Asumo que el data.bin existe, entonces lo abro con todos los permisos
		int fd_archivo = open(RUTA_DATABIN, O_RDWR, S_IRUSR | S_IRGRP | S_IROTH);

		/* Si el archivo data.bin NO existe Y SOY UN DATANODE, entonces lo creo y YO DECIDO la cantidad de bloques
		 * que lo conforman (ergo, su tamaño).
		 */
		if ( fd_archivo < 0 && flag == DATANODE ){

			fd_archivo = open(RUTA_DATABIN, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);

			// Cada bloque tiene 1048576 bytes (1 MiB) - Creamos un data.bin de CANTIDAD_BLOQUES_DATABIN bloques
			ftruncate(fd_archivo, TAMANIO_DATABIN * TAMANIO_BLOQUE_DATABIN);
		}else if ( fd_archivo < 0 && flag == WORKER ){

			// Si el archivo data.bin no existe y soy un WORKER no puedo crearlo, por lo que aborto
			log_error(log, "No se encontro el archivo data.bin en la ruta %s", RUTA_DATABIN);

			return NULL;
		}

		chmod(RUTA_DATABIN, permisos); // Aplico permisos al archivo

		int tamanio_archivo = tamanioArchivo(RUTA_DATABIN);

		/* IMPORTANTE: No hace falta hacer (y no se debe hacer) archivo_mapeado = malloc(tamanio_archivo), pues
		 * el mmap lo hace de fondo. A su vez, el unmap debe hacer el free correspondiente, por lo que no tenemos que hacer
		 * nada.
		 */
		void* archivo_mapeado = mmap(NULL, tamanio_archivo, PROT_READ | PROT_WRITE, MAP_SHARED, fd_archivo, 0);

		if ( archivo_mapeado == MAP_FAILED){
			log_error(log, "Error al mapear el archivo data.bin del Nodo en memoria.");
			return NULL;
		}

		close(fd_archivo); // Cerramos el archivo pues ya lo tenemos mapeado en memoria, no es necesario mantenerlo abierto

		return archivo_mapeado;

} // FIN map_databin


void unmap_databin(char* RUTA_DATABIN, void* archivo_databin, t_log* log){

	int tamanio_archivo = tamanioArchivo(RUTA_DATABIN);

	if ( munmap(archivo_databin, tamanio_archivo) == -1 ){
		log_error(log, "Error en el unmaping del archivo.");
	}

} // FIN unmap_databin

void unmap_archivo(char* ruta_archivo, void* buffer_contenido_archivo, t_log* log){

	int tamanio_archivo = tamanioArchivo(ruta_archivo);

	if ( munmap(buffer_contenido_archivo, tamanio_archivo) == -1 ){
		log_error(log, "Error en el unmaping del archivo.");
	}

} // FIN unmap_archivo


void* get_bloque(void* databin_mapeado, int numero_bloque){

	// Desplazamiento desde el inicio del data.bin hasta donde inicia el bloque en cuestión
	int offset_bloque = numero_bloque * TAMANIO_BLOQUE_DATABIN;

	void* contenido_bloque = malloc(TAMANIO_BLOQUE_DATABIN);
	void* salida_memcpy = memcpy(contenido_bloque, databin_mapeado + offset_bloque, TAMANIO_BLOQUE_DATABIN);
	if ( salida_memcpy == contenido_bloque){ // Si el retorno de memcpy coincide con el contenido del bloque todo salió bien
		return contenido_bloque;
	}else{ // En caso de error
		return NULL;
	}

} // FIN get_bloque


void liberar_bloque(void* bloque){

	free(bloque);

} // FIN liberar_bloque


int set_bloque(void* databin_mapeado, int numero_bloque, void* contenido_a_grabar){

	// Desplazamiento desde el inicio del data.bin hasta donde inicia el bloque en cuestión
	int offset_bloque = numero_bloque * TAMANIO_BLOQUE_DATABIN;

	int tamanio_contenido_a_grabar = strlen(contenido_a_grabar);

	/* Si hago memcpy(databin_mapeado + offset_bloque, contenido_a_grabar, TAMANIO_BLOQUE_DATABIN); tira SEGSIG
	 * Para calcular el tamaño del contenido a grabar en el bloque NO puedo hacer sizeof(contenido_a_grabar) porque da 4 Bytes, que
	 * es el tamaño de un void* lógicamente. Si desferencio y hago sizeof(*contenido_a_grabar) obtenemos 1 Byte, que es el tamaño
	 * del tipo de dato void. Por lo tanto, la única opción que me queda es considerar al void* como un char* y calcular su longitud
	 * con strlen, aprovechando la ventaja de que cada caracter ocupa 1 Byte, entonces el tamaño de contenido_a_grabar coincidirá
	 * con la cantidad de caracteres que lo conforman si se lo ve como un char*. Recordemos que strlen da el tamaño de un char* sin
	 * contar el '\0'.
	 */

	//void* salida_memcpy = memcpy(databin_mapeado + offset_bloque, contenido_a_grabar, TAMANIO_BLOQUE_DATABIN);
	void* salida_memcpy = memcpy(databin_mapeado + offset_bloque, contenido_a_grabar, tamanio_contenido_a_grabar);

	if ( salida_memcpy == databin_mapeado + offset_bloque){
		return OK;
	}else{
		return ERROR;
	}
} // FIN set_bloque


int persistir_buffer_en_archivo(char* nombre_archivo, void* buffer){

	FILE* buffer_archivo = fopen(nombre_archivo, "w+"); // Creo un nuevo archivo para persistir el bloque (píso el anterior si existe)
	if ( buffer_archivo == NULL){
		perror("Error al crear el archivo donde se guarda el script transformador.");
		return ERROR;
	}

	char mode[] = "0777"; // Permisos totales
	int permisos = strtol(mode, 0, 8); // Administración para los permisos
	chmod(nombre_archivo, permisos); // Aplico permisos al archivo

	fwrite(buffer, strlen((char*)buffer), 1, buffer_archivo); // Escribo el contenido del bloque en el archivo
	fclose(buffer_archivo);

	return OK;

} // FIN persistir_buffer_en_archivo


char* bloque_a_archivo(void* contenido_bloque, int numero_bloque){

	/* Uso el numero_bloque para armar el nombre del archivo auxiliar donde se va a guardar el bloque.
	 * Esto lo hago para evitar nombres repetidos. Notar que con usar el numero_bloque alcanza para no tener nombres de archivos
	 * temporales, pues dos archivos distintos jamás compartirán un mismo bloque del data.bin
	 */

	// Convierto numero_bloque de int a char*
	//char* numero_bloque_string = malloc(4); // Hago un malloc(4) por ser un tamaño más que suficiente para guardar un char* con un número entero
	//strcpy(numero_bloque_string, string_itoa(numero_bloque));
	char* numero_bloque_string = string_itoa(numero_bloque);

	/* Reservo memoria para nombre_archivo (ruta del archivo donde se va a guardar el bloque)
	 * Ejemplo: bloque_2.bin
	 */
	char* nombre_archivo = malloc(strlen("bloque_") + strlen(numero_bloque_string) + strlen(".bin") + 1); // + 1 del '\0'
	strcpy(nombre_archivo, "bloque_"); // nombre_archivo = "bloque_"
	strcat(nombre_archivo, numero_bloque_string); // nombre_archivo = "bloque_N" siendo N el numero_bloque
	strcat(nombre_archivo, ".bin");  // nombre_archivo = "bloque_N.bin"

	free(numero_bloque_string);

	// Me guardo el contenido del bloque en un archivo
	persistir_buffer_en_archivo(nombre_archivo, contenido_bloque);

	// Retorno la RUTA donde se guardo el archivo con el bloque para que WORKER sepa donde está
	return nombre_archivo;

} // FIN bloque_a_archivo


void* get_bloque_ocupado(int numero_bloque_databin, int bytes_ocupados, void* databin_mapeado){

	// 1° Leo el contenido completo del bloque
	void* bloque_completo = get_bloque(databin_mapeado, numero_bloque_databin);

	// 2° Me quedo solamente con los bytes ocupados realmente
	void* bloque_ocupado = calloc(bytes_ocupados + 1, 1); // EL +1 ES POR VALGRIND
	void* salida_memcpy = memcpy(bloque_ocupado, bloque_completo, bytes_ocupados);
	liberar_bloque(bloque_completo); // Libero la memoria usada por el bloque_completo pues ya no lo utilizo
	if ( salida_memcpy == bloque_ocupado){ // Si el retorno de memcpy coincide con el contenido del bloque ocupado todo salió bien
		return bloque_ocupado;
	}else{ // En caso de error
		return NULL;
	}

} // FIN get_bloque_ocupado
