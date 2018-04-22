#include "funcionesFILESYSTEM.h"

int cantidad_bloques_archivo_binario(char* ruta_archivo){

	int tamanio_archivo = tamanioArchivo(ruta_archivo);
	int cantidad_bloques_auxiliar = tamanio_archivo / TAMANIO_BLOQUE_DATABIN; // EL resultado de la division entera es un entero
	int bloque_adicional = tamanio_archivo % TAMANIO_BLOQUE_DATABIN;
	int cantidad_bloques_total;

	if ( bloque_adicional == 0 ){	// La división tiene resto 0
		cantidad_bloques_total = cantidad_bloques_auxiliar;
	}else{
		cantidad_bloques_total = cantidad_bloques_auxiliar + 1;
	}

	return cantidad_bloques_total;
}

int aceptarDataNode(int socketEscucha, struct sockaddr_in direccionCliente, t_datanode* cliente) {

	short int sizeDireccion = sizeof(struct sockaddr_in);
	int nuevoFDCliente = 0;

	if ((nuevoFDCliente = accept(socketEscucha, (struct sockaddr *)&direccionCliente, (socklen_t*)&sizeDireccion)) == -1) {
		printf("Error aceptando un cliente.\n");
		exit(-1);
	} else {
		cliente->socket_client = nuevoFDCliente;
		//cliente->nombreNodo = string_new();
		//cliente->ipNodo = string_new();
		cliente->puertoEscuchaMaster = 0;
	}

	//strcpy(cliente->ipNodo, inet_ntoa(direccionCliente.sin_addr));

	return nuevoFDCliente;
}

// Esta funcion es como cantidad_bloques_archivo,
// pero en lugar de calcular los bloques de un archivo calcula los bloques de un buffer
int cantidad_bloques_buffer(char* buffer_archivo){

	int tamanio_buffer = strlen(buffer_archivo);
	int cantidad_bloques_auxiliar = tamanio_buffer / TAMANIO_BLOQUE_DATABIN; // EL resultado de la division entera es un entero
	int bloque_adicional = tamanio_buffer % TAMANIO_BLOQUE_DATABIN;
	int cantidad_bloques_total;

	if ( bloque_adicional == 0 ){	// La división tiene resto 0
		cantidad_bloques_total = cantidad_bloques_auxiliar;
	}else{
		cantidad_bloques_total = cantidad_bloques_auxiliar + 1;
	}

	return cantidad_bloques_total;
}

int cantidad_bloques_archivo_reduccion_global(char* archivo){

	int tamanio_archivo = strlen(archivo);
	int cantidad_bloques_auxiliar = tamanio_archivo / TAMANIO_BLOQUE_DATABIN; // EL resultado de la division entera es un entero
	int bloque_adicional = tamanio_archivo % TAMANIO_BLOQUE_DATABIN;
	int cantidad_bloques_total;

	if ( bloque_adicional == 0 ){	// La división tiene resto 0
		cantidad_bloques_total = cantidad_bloques_auxiliar;
	}else{
		cantidad_bloques_total = cantidad_bloques_auxiliar + 1;
	}

	return cantidad_bloques_total;
}

int verificarEstadoAnterior() {

/* Para que exista un estado anterior deben existir las siguientes estructuras
 *
 * 1) Tabla de Directorios
 * 2) Tabla de Archivos
 * 3) Tabla de Nodos
 * 4) Debe existir el bitmap de cada uno de los Nodos
 *
 * En caso de que UNA SOLA de estas estructuras no exista, se
 * considerará que NO HAY un estado anterior.
 *
 */

	int estadoAnterior = true;

	// Me fijo si existe la Tabla de Directorios, la Tabla de Archivos y la Tabla de Nodos

	FILE* tablaDirectorios = fopen("metadata/directorios.dat", "r");
	FILE* tablaNodos = fopen("metadata/nodos.bin", "r");
	FILE* tablaArchivos = fopen("metadata/archivos/archivos.dat", "r");

	if (tablaDirectorios == NULL) {
		log_error(archivoLog, "No existe la Tabla de Directorios.");
		estadoAnterior = false;
	} else {
		log_info(archivoLog, "Existe la Tabla de Directorios.");
	}

	if (tablaNodos == NULL) {
		log_error(archivoLog, "No existe la Tabla de Nodos.");
		estadoAnterior = false;
	} else {
		log_info(archivoLog, "Existe la Tabla de Nodos.");
		// Por cada nodo debo verificar si existe su bitmap de bloques
		// Si me da OK es que existen todos los bitmaps
		if (!verificarBitmapsEstadoAnterior())
			estadoAnterior = false;
	}

	if (tablaArchivos == NULL) {
		log_error(archivoLog, "No existe la Tabla de Archivos.");
		estadoAnterior = false;
	} else {
		log_info(archivoLog, "Existe la Tabla de Archivos.");
	}

	// Libero la tablaNodos y la tabla de directorios
	if (tablaNodos != NULL) fclose(tablaNodos);
	if (tablaDirectorios != NULL) fclose(tablaDirectorios);
	if (tablaArchivos != NULL) fclose(tablaArchivos);

	return estadoAnterior;

}

int grabarArchivo(int indiceArchivo, char* ruta, char* nombre, int ocupado) {

	arrayArchivos[indiceArchivo].index = indiceArchivo;
	strcpy(arrayArchivos[indiceArchivo].ruta, ruta);
	strcpy(arrayArchivos[indiceArchivo].nombre, nombre);
	arrayArchivos[indiceArchivo].ocupado = ocupado;

	return true;

}

int inicializarTablaArchivos(int tablaArchivos) {

	// Variables
	int indiceArchivo = 0;

	// Mapeo el arrayDirectorios a memoria y lo consisto en archivo
	arrayArchivos =  (struct t_archivo*) mmap(NULL, sizeof(struct t_archivo) * CANTIDAD_DIRECTORIOS, PROT_READ | PROT_WRITE, MAP_SHARED, tablaArchivos, 0);

	if (arrayArchivos == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
	}

	for (indiceArchivo = 0; indiceArchivo < CANTIDAD_DIRECTORIOS; ++indiceArchivo) {
		grabarArchivo(indiceArchivo, "", "", false);
	}

	msync(arrayArchivos, sizeof(struct t_archivo) * CANTIDAD_DIRECTORIOS, MS_SYNC);

	log_info(archivoLog, "La Tabla de Archivos se ha creado formateada.");
	close(tablaArchivos);
	return true;

}

int crearTablaArchivos() {

	// Variables
    int sizeArchivo = sizeof(struct t_archivo) * CANTIDAD_DIRECTORIOS;

    // Creo archivo Tabla de Archivos
    FILE* tablaArchivos = fopen("metadata/archivos/archivos.dat", "w+");

    // Le doy el tamaño que necesito
	fseek(tablaArchivos, sizeArchivo -1, SEEK_SET);
    fputc('\0', tablaArchivos);
	fclose(tablaArchivos);

	// Recupero el FD asociado al FILE*
	int tablaArchivosFD = open("metadata/archivos/archivos.dat", O_RDWR);

	if (tablaArchivosFD < 0) {
		log_error(archivoLog, "Error al crear la Tabla de Archivos.\n");
		return false;
	} else {
		inicializarTablaArchivos(tablaArchivosFD);
		return true;
	}
}

int crearTablaDirectorios() {

	// Variables
    int sizeArchivo = sizeof(struct t_directory) * CANTIDAD_DIRECTORIOS;

    // Creo archivo Tabla de Directorios
    FILE* tablaDirectorios = fopen("metadata/directorios.dat", "w+");

    // Le doy el tamaño que necesito
	fseek(tablaDirectorios, sizeArchivo -1, SEEK_SET);
    fputc('\0', tablaDirectorios);
	fclose(tablaDirectorios);

	// Recupero el FD asociado al FILE*
	int tablaDirectoriosFD = open("metadata/directorios.dat", O_RDWR);

	if (tablaDirectorios < 0) {
		log_error(archivoLog, "Error al crear la Tabla de Directorios.\n");
		return false;
	} else {
		inicializarTablaDirectorios(tablaDirectoriosFD);
		return true;
	}
}

t_bloque* leerBloqueArchivo(char* nombreArchivoDetalle, int numeroBloque, int numeroCopia) {

	// Paso los valores a string
	char* numeroBloqueString = string_itoa(numeroBloque);
	char* numeroCopiaString = string_itoa(numeroCopia);

	// Abro el detalle del archivo con config
    t_config* tablaArchivo = config_create(nombreArchivoDetalle);

    // Armo la key para recuperar la informacion desde el archivo
	char* datosBloque = malloc(strlen("BLOQUE") + strlen(numeroBloqueString) + strlen("COPIA") + strlen(numeroCopiaString) + 1);
    strcpy(datosBloque, "BLOQUE");
    strcat(datosBloque, numeroBloqueString);
    strcat(datosBloque, "COPIA");
    strcat(datosBloque, numeroCopiaString);

    // Armo la key para recuperar la cantidad de bytes ocupados en el bloque
	char* bytesBloque = malloc(strlen("BLOQUE") + strlen(numeroBloqueString) + strlen("BYTES") + 1);
    strcpy(bytesBloque, "BLOQUE");
    strcat(bytesBloque, numeroBloqueString);
    strcat(bytesBloque, "BYTES");

    // Libero lo que ya use
    free(numeroBloqueString);
    free(numeroCopiaString);

    // Recupero la informacion para ese bloque si es que existe
    if (config_has_property(tablaArchivo, datosBloque)) {

    	int bytesOcupados = config_get_int_value(tablaArchivo, bytesBloque);
        char** infoBloque = config_get_array_value(tablaArchivo, datosBloque);

        // Con el nodo en el cual está alocado el bloque, recupero su IP y el puertoEscuchaMaster
        char* ipNodo = malloc(16);
        int puertoEscuchaMaster = obtenerIPYPuertoNodo(infoBloque[0], ipNodo);

    	// Pido memoria para el bloque que debo alocar
    	t_bloque* bloque = malloc(sizeof(t_bloque));

        // Guardo los datos del bloque en la estructura
        bloque->numero = numeroBloque;
        strcpy(bloque->nodo, infoBloque[0]);
        strcpy(bloque->ipNodo, ipNodo);
        bloque->puertoEscuchaMaster = puertoEscuchaMaster;
        bloque->bloqueDataBin = atoi(infoBloque[1]);
        bloque->espacioOcupado = bytesOcupados;

        // Libero los recursos
        free(ipNodo);
        free(datosBloque);
        free(bytesBloque);
        int i = 0;
        while(infoBloque[i] != NULL) {
        	free(infoBloque[i]);
        	i++;
        }
        free(infoBloque);

        // Libero el config
        config_destroy(tablaArchivo);

        return bloque;
    } else {

    	// Libero los recursos
        free(datosBloque);
        free(bytesBloque);

        // Libero el config
        config_destroy(tablaArchivo);

        return NULL;

    }
}

// Recupera la info de un archivo y la imprime por pantalla
int imprimirInfo(char* nombreArchivo, int indiceDirectorio) {

	// Variables
	int i = 0; // Para loopear entre los bloques
	int j = 0; // Para liberar recursos

	// Paso valores a formato string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);
	char* copia0String = string_itoa(COPIA_0);
	char* copia1String = string_itoa(COPIA_1);

	// Armo la ruta del archivo detalle para recuperar la información
	char* archivoDetalle = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(archivoDetalle, "metadata/archivos/");
	strcat(archivoDetalle, indiceDirectorioString);
	strcat(archivoDetalle, "/");
	strcat(archivoDetalle, nombreArchivo);

	// Libero el indice porque ya no lo uso mas
	free(indiceDirectorioString);

	// Genero el config para abrir y leer el archivo
	t_config* detalle = config_create(archivoDetalle);

	// Libero la ruta del archivoDetalle porque ya no la uso mas
	free(archivoDetalle);

	// Recupero la informacion del archivo
	int tamanio = config_get_int_value(detalle, "TAMANIO");
	int bloques = config_get_int_value(detalle, "BLOQUES");

	// Imprimo por pantalla los valores recuperados
	printf("El archivo posee la siguiente información.\n");
	printf("TAMAÑO = %d bytes\n", tamanio);
	printf("CANTIDAD DE BLOQUES = %d\n", bloques);

	// Itero segun la cantidad de bloques y voy imprimiendo los datos
	for (i = 0; i < bloques; ++i) {

		// Paso el i a string
		char* iString = string_itoa(i);

		// Armo la key para la COPIA_0
		char* bloqueCopia0 = malloc(strlen("BLOQUE") + strlen(iString) + strlen("COPIA") + strlen(copia0String) + 1);
		strcpy(bloqueCopia0, "BLOQUE");
		strcat(bloqueCopia0, iString);
		strcat(bloqueCopia0, "COPIA");
		strcat(bloqueCopia0, copia0String);

		// Armo la key para la COPIA_1
		char* bloqueCopia1 = malloc(strlen("BLOQUE") + strlen(iString) + strlen("COPIA") + strlen(copia1String) + 1);
		strcpy(bloqueCopia1, "BLOQUE");
		strcat(bloqueCopia1, iString);
		strcat(bloqueCopia1, "COPIA");
		strcat(bloqueCopia1, copia1String);

		// Si existe la COPIA_0, la imprimo. Si no digo que no existe
		if (config_has_property(detalle, bloqueCopia0)) {

			// Recupero los datos para el bloque dado y los guardo para mostrarlos
			char* nodoCopia0 = malloc(10);
			char* bloqueNodoCopia0 = malloc(5);
			char** infoCopia0 = config_get_array_value(detalle, bloqueCopia0);

			// Guardo la data
			strcpy(nodoCopia0, infoCopia0[0]);
			strcpy(bloqueNodoCopia0, infoCopia0[1]);

			// La imprimo por pantalla
			printf("La copia 0 del bloque %d se encuentra en el numero de bloque %s del nodo %s\n", i, bloqueNodoCopia0, nodoCopia0);

			// Libero los recursos que use
			free(nodoCopia0);
			free(bloqueNodoCopia0);

			// Libero la info del bloque 0
			j = 0;
			while(infoCopia0[j] != NULL) {
				free(infoCopia0[j]);
				j++;
			}
			free(infoCopia0);
		} else {
			printf("No existe la copia 0 del bloque %d\n", i);
		}

		// Si existe la COPIA_1, la imprimo. Si no digo que no existe
		if (config_has_property(detalle, bloqueCopia1)) {

			// Recupero los datos para el bloque dado y los guardo para mostrarlos
			char* nodoCopia1 = malloc(10);
			char* bloqueNodoCopia1 = malloc(5);
			char** infoCopia1 = config_get_array_value(detalle, bloqueCopia1);

			// Guardo la data
			strcpy(nodoCopia1, infoCopia1[0]);
			strcpy(bloqueNodoCopia1, infoCopia1[1]);

			// La imprimo por pantalla
			printf("La copia 1 del bloque %d se encuentra en el numero de bloque %s del nodo %s\n", i, bloqueNodoCopia1, nodoCopia1);

			// Libero los recursos que use
			free(nodoCopia1);
			free(bloqueNodoCopia1);

			// Libero la info del bloque 1
			j = 0;
			while(infoCopia1[j] != NULL) {
				free(infoCopia1[j]);
				j++;
			}
			free(infoCopia1);
		} else {
			printf("No existe la copia 1 del bloque %d\n", i);
		}

		// Libero recursos
		free(bloqueCopia0);
		free(bloqueCopia1);
		free(iString);

	} // FIN DEL FOR

	// Libero recursos
	free(copia0String);
	free(copia1String);

	// Libero el config
	config_destroy(detalle);

	return true;

}

int moverArchivo(char* rutaArchivoYama, char* nombreArchivo, int indiceOrigen, int indiceDestino, char* nuevaRutaYama) {

	// Variables
	int archivoMovido = false;

	// Convierto de int a string.
	char* indiceOrigenString = string_itoa(indiceOrigen);
	char* indiceDestinoString = string_itoa(indiceDestino);

	// Armo la ruta origen
	char* rutaOrigen = malloc(strlen("metadata/archivos/") + strlen(indiceOrigenString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(rutaOrigen, "metadata/archivos/");
	strcat(rutaOrigen, indiceOrigenString);
	strcat(rutaOrigen, "/");
	strcat(rutaOrigen, nombreArchivo);

	// Armo la ruta destino
	char* rutaDestino = malloc(strlen("metadata/archivos/") + strlen(indiceDestinoString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(rutaDestino, "metadata/archivos/");
	strcat(rutaDestino, indiceDestinoString);
	strcat(rutaDestino, "/");
	strcat(rutaDestino, nombreArchivo);

	if ((rename(rutaOrigen, rutaDestino)) == 0) {
		archivoMovido = true;
		log_trace(archivoLog, "El archivo %s ha sido movido correctamente.", nombreArchivo);
		actualizarRutaArchivoEnTablaArchivos(rutaArchivoYama, nombreArchivo, nuevaRutaYama);
	} else {
		log_error(archivoLog, "Error al mover el archivo %s.", nombreArchivo);
	}

	// Libero recursos
	free(indiceOrigenString);
	free(indiceDestinoString);
	free(rutaOrigen);
	free(rutaDestino);

	return archivoMovido;
}

// Parsea la ruta ingresada y si es valida llama a la función para mostrar los datos por pantalla
int recuperarInfoArchivo(char* rutaArchivoYama) {

	// Variables
	int i = 0; // Para liberar los directorios

	// Parseo la ruta del archivo Yama
	char** directorios = string_split(rutaArchivoYama, "/");

	// Valido que el directorio que me ingresaron exista en YAMA FS
	int indiceDirectorio = validarRutaDirectoriosConNombreArchivo(rutaArchivoYama);

	// Me guardo la cantidad de directorios que tengo que buscar
	int cantidadDirectorios = contarDirectorios(directorios, 1);

	// Si el directorio es valido, recupero la info
	if (indiceDirectorio != -1) {
		imprimirInfo(directorios[cantidadDirectorios], indiceDirectorio);
	} else {
		log_error(archivoLog, "La ruta %s no existe.", rutaArchivoYama);
		return false;
	}

	// Libero recursos
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);

	return true;

}

// CPTO
int copiarArchivoAFSLocal(char* pathYAMA, char* pathFSLocal) {

	// Variables
	int i = 0;
	int bloqueDataBinCopia0 = 0;
	int bloqueDataBinCopia1 = 0;
	int bytesOcupados = 0;
	int bloqueCopia0Encontrado = false;
	int bloqueCopia1Encontrado = false;
	int socketNodoCopia0 = 0;
	int socketNodoCopia1 = 0;
	int offset = 0;
	char* ultimoNodoUsado = malloc(10);
	strcpy(ultimoNodoUsado, "");

	// Parseo para saber cuántas entradas tengo en la ruta
	char** directorios = string_split(pathYAMA, "/");

	// Cuento la cantidad de entradas que tengo, la última es el nombre del archivo
	int cantidadEtiquetasDirectorios = contarDirectorios(directorios, 1);

	// Busco el índice del directorio donde se aloja el archivo
	int indiceDirectorio = buscarIndiceDirectorio(directorios[cantidadEtiquetasDirectorios-1]);

	// Convierto int a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);

	// Armo el nombre del archivo detalle
	char* nombreArchivoDetalle = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(directorios[cantidadEtiquetasDirectorios]) + 1);
	strcpy(nombreArchivoDetalle, "metadata/archivos/");
	strcat(nombreArchivoDetalle, indiceDirectorioString);
	strcat(nombreArchivoDetalle, "/");
	strcat(nombreArchivoDetalle, directorios[cantidadEtiquetasDirectorios]);

	// Libero recursos
	free(indiceDirectorioString);

	// Abro la tabla de archivos con config para ver la data del archivo
    t_config* tablaArchivo = config_create(nombreArchivoDetalle);

    // Leo los datos que necesito
    int cantidadBloques = config_get_int_value(tablaArchivo, "BLOQUES");
    int tamanioArchivo = config_get_int_value(tablaArchivo, "TAMANIO");

    // Aloco un gran buffer para el archivo
    char* bufferArchivo = malloc(tamanioArchivo);

    // Itero pidiendo los bloques a los diferentes DataNodes
    for (i = 0; i < cantidadBloques; ++i) {

    	// Pido la información para los dos bloques (COPIA_0 y COPIA_1)
    	// Puede no existir alguna copia
    	char* nodoCopia0 = malloc(10);
    	char* nodoCopia1 = malloc(10);

    	bloqueCopia0Encontrado = recuperarInfoBloque(tablaArchivo, i, COPIA_0, nodoCopia0, &bloqueDataBinCopia0, &bytesOcupados);
    	bloqueCopia1Encontrado = recuperarInfoBloque(tablaArchivo, i, COPIA_1, nodoCopia1, &bloqueDataBinCopia1, &bytesOcupados);

    	// Encontré las dos copias
    	if (bloqueCopia0Encontrado && bloqueCopia1Encontrado) {
    		// Obtengo el socket de los dos nodos porque los voy a usar mas adelante
    		socketNodoCopia0 = obtenerSocketNodo(nodoCopia0);
    		socketNodoCopia1 = obtenerSocketNodo(nodoCopia1);
    		// Si es NULL el ultimo usado (estoy en el primer bloque), le mando al primero
    		if (strcmp(ultimoNodoUsado, "") == 0) {
    			if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
        			memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        			free(contenidoBloque);
        			offset = offset + bytesOcupados;
        			strcpy(ultimoNodoUsado, nodoCopia0);
        		} else {
        			// Como contenidoBloque no tiene información, le pido al otro nodo
        			if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
        			// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
        				memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        				free(contenidoBloque);
            			offset = offset + bytesOcupados;
            			strcpy(ultimoNodoUsado, nodoCopia1);
        			} else {
        				log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
        				log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
        				return false;
        			}
        		}
    		} else {
    			// Encontré las dos copias pero ya usé un nodo
    			// Pregunto cuál es el nodo que usé
    			if ((strcmp(ultimoNodoUsado, nodoCopia0)) == 0) {
    				// El último nodo que usé es el nodoCopia0, le envío al nodoCopia1
    				if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
    				// Si el contenido es el correcto
    					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
    					free(contenidoBloque);
    					offset = offset + bytesOcupados;
    					strcpy(ultimoNodoUsado, nodoCopia1);
    				} else {
    					// Le mando al nodo que ya había usado antes porque no me queda otro
    					if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
    					// Si el contenido es el correcto, perfecto si no fallo ya que no tengo más nodos
        					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        					free(contenidoBloque);
        					offset = offset + bytesOcupados;
        					strcpy(ultimoNodoUsado, nodoCopia0);
    					} else {
    						log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
    						log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
    						return false;
    					}
    				}
    			} else {
    				// El último nodo que usé NO es nodoCopia0, le mando a nodoCopia0
    				if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
    				// Si el contenido es el correcto
						memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
						free(contenidoBloque);
						offset = offset + bytesOcupados;
						strcpy(ultimoNodoUsado, nodoCopia0);
					} else {
						// Le mando al nodo que ya había usado antes porque no me queda otro
						if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
						// Si el contenido es el correcto, perfecto si no fallo ya que no tengo más nodos
							memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
							free(contenidoBloque);
							offset = offset + bytesOcupados;
							strcpy(ultimoNodoUsado, nodoCopia1);
						} else {
							log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
							log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
							return false;
						}
					}
    			}
    		}
    	} else { // ELSE del if que pregunta por las dos copias
    		// No encontré las dos copias
    		log_info(archivoLog, "Encontré solo una de las dos copias.");
    		// Me fijo qué copia encontré y pido esa, si falla ya está, no tengo más copias
    		if (bloqueCopia0Encontrado) {
	    		// Obtengo el socket unicamente de nodoCopia0 que es el que tengo
    			log_info(archivoLog, "La copia que encontré está en el nodo %s.", nodoCopia0);
	    		socketNodoCopia0 = obtenerSocketNodo(nodoCopia0);
    			if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
					// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
					free(contenidoBloque);
					offset = offset + bytesOcupados;
					strcpy(ultimoNodoUsado, nodoCopia0);
				} else {
					log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
					log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
					return false;
				}
    		} else {
    			// Encontré la COPIA_1, le mando a ese nodo
	    		// Obtengo el socket unicamente de nodoCopia1 que es el que tengo
    			log_info(archivoLog, "La copia que encontré está en el nodo %s.", nodoCopia1);
	    		socketNodoCopia1 = obtenerSocketNodo(nodoCopia1);
    			if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
					// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
					free(contenidoBloque);
					offset = offset + bytesOcupados;
					strcpy(ultimoNodoUsado, nodoCopia1);
				} else {
					log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
					log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
					return false;
				}
    		}
    	} // Fin del if que pregunta por las dos copias
    	// Libero el espacio para los nodos
    	free(nodoCopia0);
    	free(nodoCopia1);
	} // Fin del for que itera por los bloques

    // Creo el archivo en el FS Local, en la ruta indicada
    FILE* archivoTemporal = fopen(pathFSLocal, "w+");

    // Verifico que el archivo se haya creado correctamente
    if (archivoTemporal == NULL) {
    	log_error(archivoLog, "Error al crear el archivo en el FS Local.");

    } else {
    	log_trace(archivoLog, "El archivo %s se ha creado correctamente.", pathFSLocal);
        // Escribo el contenido del archivo temporal
        fwrite(bufferArchivo, tamanioArchivo, 1, archivoTemporal);
    	fclose(archivoTemporal);
    }


	// Libero recursos
    i = 0;
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
    free(directorios);
	free(nombreArchivoDetalle);
	config_destroy(tablaArchivo);
	free(ultimoNodoUsado);
	free(bufferArchivo);

	return true;

}

// Arma un buffer con el contenido del archivo y lo graba en una ruta temporal
// Devuelve true si pudo hacerlo y false si falló
int armarArchivo(char* rutaArchivoYama, char* rutaArchivoTemporal) {

	// Variables
	int i = 0;
	int j = 0; // Para liberar directorios
	int bloqueDataBinCopia0 = 0;
	int bloqueDataBinCopia1 = 0;
	int bytesOcupados = 0;
	int offset = 0;
	char* ultimoNodoUsado = malloc(10);
	strcpy(ultimoNodoUsado, "");

	// Parseo para saber cuántas entradas tengo en la ruta
	char** directorios = string_split(rutaArchivoYama, "/");

	// Cuento la cantidad de entradas que tengo, la última es el nombre del archivo
	int cantidadEtiquetasDirectorios = contarDirectorios(directorios, 1);

	// Busco el índice del directorio donde se aloja el archivo
	int indiceDirectorio = buscarIndiceDirectorio(directorios[cantidadEtiquetasDirectorios-1]);

	// Paso el indice de directorio a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);

	// Armo el nombre del archivo detalle
	char* nombreArchivoDetalle = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(directorios[cantidadEtiquetasDirectorios]) + 1);
	strcpy(nombreArchivoDetalle, "metadata/archivos/");
	strcat(nombreArchivoDetalle, indiceDirectorioString);
	strcat(nombreArchivoDetalle, "/");
	strcat(nombreArchivoDetalle, directorios[cantidadEtiquetasDirectorios]);

	// Libero el indiceDirectorioString porque ya lo use
	free(indiceDirectorioString);

	// Abro la tabla de archivos con config para ver la data del archivo
    t_config* tablaArchivo = config_create(nombreArchivoDetalle);

    // Leo los datos que necesito
    int cantidadBloques = config_get_int_value(tablaArchivo, "BLOQUES");
    int tamanioArchivo = config_get_int_value(tablaArchivo, "TAMANIO");

    // Aloco un gran buffer para el archivo
    char* bufferArchivo = malloc(tamanioArchivo);

    // Itero pidiendo los bloques a los diferentes DataNodes
    for (i = 0; i < cantidadBloques; ++i) {

    	// Pido la información para los dos bloques (COPIA_0 y COPIA_1)
    	// Puede no existir alguna copia
    	char* nodoCopia0 = malloc(10);
    	char* nodoCopia1 = malloc(10);

    	int bloqueCopia0Encontrado = recuperarInfoBloque(tablaArchivo, i, COPIA_0, nodoCopia0, &bloqueDataBinCopia0, &bytesOcupados);
    	int bloqueCopia1Encontrado = recuperarInfoBloque(tablaArchivo, i, COPIA_1, nodoCopia1, &bloqueDataBinCopia1, &bytesOcupados);

    	// Encontré las dos copias
    	if (bloqueCopia0Encontrado && bloqueCopia1Encontrado) {
    		// Obtengo el socket de los dos nodos porque los voy a usar mas adelante
    		int socketNodoCopia0 = obtenerSocketNodo(nodoCopia0);
    		int socketNodoCopia1 = obtenerSocketNodo(nodoCopia1);
    		// Si es NULL el ultimo usado (estoy en el primer bloque), le mando al primero
    		if (strcmp(ultimoNodoUsado, "") == 0) {
    			if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
        			memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        			free(contenidoBloque);
        			offset = offset + bytesOcupados;
        			strcpy(ultimoNodoUsado, nodoCopia0);
        		} else {
        			// Como contenidoBloque no tiene información, le pido al otro nodo
        			if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
        			// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
        				memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        				free(contenidoBloque);
            			offset = offset + bytesOcupados;
            			strcpy(ultimoNodoUsado, nodoCopia1);
        			} else {
        				log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
        				log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
        				free(bufferArchivo);
        				config_destroy(tablaArchivo);
        				// Libero los directorios
        				j = 0;
        				while(directorios[j] != NULL) {
        					free(directorios[j]);
        					j++;
        				}
        				free(directorios);
        				return false;
        			}
        		}
    		} else {
    			// Encontré las dos copias pero ya usé un nodo
    			// Pregunto cuál es el nodo que usé
    			if ((strcmp(ultimoNodoUsado, nodoCopia0)) == 0) {
    				// El último nodo que usé es el nodoCopia0, le envío al nodoCopia1
    				if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
    				// Si el contenido es el correcto
    					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
    					free(contenidoBloque);
    					offset = offset + bytesOcupados;
    					strcpy(ultimoNodoUsado, nodoCopia1);
    				} else {
    					// Le mando al nodo que ya había usado antes porque no me queda otro
    					if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
    					// Si el contenido es el correcto, perfecto si no fallo ya que no tengo más nodos
        					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
        					free(contenidoBloque);
        					offset = offset + bytesOcupados;
        					strcpy(ultimoNodoUsado, nodoCopia0);
    					} else {
    						log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
    						log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
    						free(bufferArchivo);
    						config_destroy(tablaArchivo);
            				// Libero los directorios
            				j = 0;
            				while(directorios[j] != NULL) {
            					free(directorios[j]);
            					j++;
            				}
            				free(directorios);
    						return false;
    					}
    				}
    			} else {
    				// El último nodo que usé NO es nodoCopia0, le mando a nodoCopia0
    				if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
    				// Si el contenido es el correcto
						memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
						free(contenidoBloque);
						offset = offset + bytesOcupados;
						strcpy(ultimoNodoUsado, nodoCopia0);
					} else {
						// Le mando al nodo que ya había usado antes porque no me queda otro
						if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
						// Si el contenido es el correcto, perfecto si no fallo ya que no tengo más nodos
							memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
							free(contenidoBloque);
							offset = offset + bytesOcupados;
							strcpy(ultimoNodoUsado, nodoCopia1);
						} else {
							log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
							log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
							free(bufferArchivo);
							config_destroy(tablaArchivo);
	        				// Libero los directorios
	        				j = 0;
	        				while(directorios[j] != NULL) {
	        					free(directorios[j]);
	        					j++;
	        				}
	        				free(directorios);
							return false;
						}
					}
    			}
    		}
    	} else { // ELSE del if que pregunta por las dos copias
    		// No encontré las dos copias
    		log_info(archivoLog, "Encontré solo una de las dos copias.");
    		// Me fijo qué copia encontré y pido esa, si falla ya está, no tengo más copias
    		if (bloqueCopia0Encontrado) {
	    		// Obtengo el socket unicamente de nodoCopia0 que es el que tengo
    			log_info(archivoLog, "La copia que encontré está en el nodo %s.", nodoCopia0);
	    		int socketNodoCopia0 = obtenerSocketNodo(nodoCopia0);
    			if (pedirBloque(bloqueDataBinCopia0, socketNodoCopia0)) {
					// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
					memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
					free(contenidoBloque);
					offset = offset + bytesOcupados;
					strcpy(ultimoNodoUsado, nodoCopia0);
				} else {
					log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
					log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
					free(bufferArchivo);
					config_destroy(tablaArchivo);
    				// Libero los directorios
    				j = 0;
    				while(directorios[j] != NULL) {
    					free(directorios[j]);
    					j++;
    				}
    				free(directorios);
					return false;
				}
    		} else {
    			if (bloqueCopia1Encontrado) {
					// Encontré la COPIA_1, le mando a ese nodo
					// Obtengo el socket unicamente de nodoCopia1 que es el que tengo
					log_info(archivoLog, "La copia que encontré está en el nodo %s.", nodoCopia1);
					int socketNodoCopia1 = obtenerSocketNodo(nodoCopia1);
					if (pedirBloque(bloqueDataBinCopia1, socketNodoCopia1)) {
						// Si tiene contenido entonces el bloque es correcto, si no fallo (ya que me fallaron los dos nodos que contenian al bloque)
						memcpy(bufferArchivo + offset, contenidoBloque, bytesOcupados);
						free(contenidoBloque);
						offset = offset + bytesOcupados;
						strcpy(ultimoNodoUsado, nodoCopia1);
					} else {
						log_error(archivoLog, "El bloque %d del archivo %s no pudo ser recuperado.", i, directorios[cantidadEtiquetasDirectorios]);
						log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
						free(bufferArchivo);
						config_destroy(tablaArchivo);
        				// Libero los directorios
        				j = 0;
        				while(directorios[j] != NULL) {
        					free(directorios[j]);
        					j++;
        				}
        				free(directorios);
						return false;
					}
    			} else {
    				log_error(archivoLog, "No encontré ninguna de las dos copias para el bloque %d del archivo %s.", i, directorios[cantidadEtiquetasDirectorios]);
    				log_error(archivoLog, "Se aborta el proceso de recupero de archivo.");
    				free(bufferArchivo);
    				config_destroy(tablaArchivo);
    				// Libero los directorios
    				j = 0;
    				while(directorios[j] != NULL) {
    					free(directorios[j]);
    					j++;
    				}
    				free(directorios);
    				return false;
    			}
    		}
    	} // Fin del if que pregunta por las dos copias
    	// Libero el espacio para los nodos
    	free(nodoCopia0);
    	free(nodoCopia1);
	} // Fin del for que itera por los bloques

    // Armo la ruta del archivo temporal que voy a guardar como salida
    strcpy(rutaArchivoTemporal, "metadata/temp/");
    strcat(rutaArchivoTemporal, directorios[cantidadEtiquetasDirectorios]);

    // Creo el archivo temporal
    FILE* archivoTemporal = fopen(rutaArchivoTemporal, "w+");

    // Escribo el contenido del archivo temporal
    fwrite(bufferArchivo, tamanioArchivo, 1, archivoTemporal);

	// Libero recursos
	j = 0;
	while(directorios[j] != NULL) {
		free(directorios[j]);
		j++;
	}
	free(directorios);
	free(nombreArchivoDetalle);
	config_destroy(tablaArchivo);
	free(ultimoNodoUsado);
	fclose(archivoTemporal);
	free(bufferArchivo);

	return true;

}

int recuperarCantidadBloquesArchivo(char* archivoDetalle) {

	// Variables
	int cantidadBloques = -1;

	// Abro el archivo de detalle como t_config
	t_config* info = config_create(archivoDetalle);

	// Recupero la cantidad de bloques que tiene el archivo
	cantidadBloques = config_get_int_value(info, "BLOQUES");

	// Cierro el config del archivo de detalle
	config_destroy(info);

	return cantidadBloques;

}

int recuperarYEnviarDatosArchivo(char* nombreArchivo, int indiceDirectorio, int socket) {

	// Variables
	int i = 0;

	// Convierto el indice del directorio a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);

	// Armo el nombre del archivo detalle
	char* nombreArchivoDetalle = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(nombreArchivoDetalle, "metadata/archivos/");
	strcat(nombreArchivoDetalle, indiceDirectorioString);
	strcat(nombreArchivoDetalle, "/");
	strcat(nombreArchivoDetalle, nombreArchivo);

	// Libero lo que ya use
	free(indiceDirectorioString);

	// Recupero la cantidad de bloques que tiene ese archivo
	int bloques = recuperarCantidadBloquesArchivo(nombreArchivoDetalle);

	// Convierto la cantidad de bloques a String
	char* bloquesString = string_itoa(bloques);

	// Le envío a Yama la cantidad de bloques que tiene ese archivo
	enviar_mensaje(CANTIDAD_DE_BLOQUES, bloquesString, socket);

	// Libero la cantidad de bloques
	free(bloquesString);

	// Por cada bloque que tiene el archivo recupero su información y la envío
	for (i = 0; i < bloques; i++) {

		// Recupero la información del bloque COPIA 0
		t_bloque* bloqueArchivo = leerBloqueArchivo(nombreArchivoDetalle, i, COPIA_0);

		// Recupero la información del bloque COPIA 1
		t_bloque* bloqueArchivoCopia = leerBloqueArchivo(nombreArchivoDetalle, i, COPIA_1);

		// Si no existe ninguna copia de un bloque, aborto el proceso y aviso a Yama
		if ((bloqueArchivo == NULL) && (bloqueArchivoCopia == NULL)) {
			log_error(archivoLog, "No existe el bloque %d para el archivo %s.", i, nombreArchivo);
			log_error(archivoLog, "Se aborta el proceso.");
			// No existe un bloque
			enviar_mensaje(BLOQUE_NO_EXISTE, "mensaje vacio", socket);
			return false;
		}

		// Si existe la COPIA 0 envío la información
		if (bloqueArchivo != NULL) {
			// ...indicando en qué nodo está...
			enviar_mensaje(NOMBRE_DE_NODO, bloqueArchivo->nodo, socket);
			log_info(archivoLog, "Se envio el NOMBRE_DE_NODO %s a través del socket %d.", bloqueArchivo->nodo, socket);
			// ...su correspondiente IP...
			enviar_mensaje(IP_DE_NODO, bloqueArchivo->ipNodo, socket);
			log_info(archivoLog, "Se envio la IP_DE_NODO %s a través del socket %d.", bloqueArchivo->ipNodo, socket);
			// ...en qué puerto escucha a Master...
			// Convierto el puerto de escucha Master a string para poder enviarlo
			char* puertoEscuchaMasterString = string_itoa(bloqueArchivo->puertoEscuchaMaster);
			enviar_mensaje(PUERTO_ESCUCHA_MASTER_NODO, puertoEscuchaMasterString, socket);
			// Lo libero porque ya lo envie
			free(puertoEscuchaMasterString);
			log_info(archivoLog, "Se envio el PUERTO_ESCUCHA_MASTER_NODO %d a través del socket %d.", bloqueArchivo->puertoEscuchaMaster, socket);
			// ...en qué posición de su data.bin...
			// Convierto el numero de bloque databin a string para poder enviarlo
			char* bloqueDataBinString = string_itoa(bloqueArchivo->bloqueDataBin);
			enviar_mensaje(BLOQUE_DATABIN, bloqueDataBinString, socket);
			// Lo libero porque ya lo envie
			free(bloqueDataBinString);
			log_info(archivoLog, "Se envio el BLOQUE_DATABIN %d a través del socket %d.", bloqueArchivo->bloqueDataBin, socket);
			// ...e indicando cuánto ocupa en bytes
			// Convierto el espacio ocupado a string para poder enviarlo
			char* espacioOcupadoString = string_itoa(bloqueArchivo->espacioOcupado);
			enviar_mensaje(BYTES_OCUPADOS, espacioOcupadoString, socket);
			// Lo libero porque ya lo envie
			free(espacioOcupadoString);
			log_info(archivoLog, "Se enviaron los BYTES_OCUPADOS %d a través del socket %d.", bloqueArchivo->espacioOcupado, socket);
		} else {
			enviar_mensaje(COPIA0_NO_EXISTE, "1", socket);
			log_info(archivoLog, "Se envió el mensaje COPIA0_NO_EXISTE a través del socket %d.", socket);
		}

		// Si existe la COPIA 1 envío la información
		if (bloqueArchivoCopia != NULL) {
			// ...indicando en qué nodo está...
			enviar_mensaje(NOMBRE_DE_NODO, bloqueArchivoCopia->nodo, socket);
			log_info(archivoLog, "Se envio el NOMBRE_DE_NODO %s a través del socket %d.", bloqueArchivoCopia->nodo, socket);
			// ...su correspondiente IP...
			enviar_mensaje(IP_DE_NODO, bloqueArchivoCopia->ipNodo, socket);
			log_info(archivoLog, "Se envio la IP_DE_NODO %s a través del socket %d.", bloqueArchivoCopia->ipNodo, socket);
			// ...en qué puerto escucha a Master...
			// Convierto el puerto de escucha Master a string para poder enviarlo
			char* puertoEscuchaMasterString = string_itoa(bloqueArchivoCopia->puertoEscuchaMaster);
			enviar_mensaje(PUERTO_ESCUCHA_MASTER_NODO, puertoEscuchaMasterString, socket);
			free(puertoEscuchaMasterString);
			log_info(archivoLog, "Se envio el PUERTO_ESCUCHA_MASTER_NODO %d a través del socket %d.", bloqueArchivoCopia->puertoEscuchaMaster, socket);
			// ...en qué posición de su data.bin...
			// Convierto el bloque databin a string
			char* bloqueDataBinString = string_itoa(bloqueArchivoCopia->bloqueDataBin);
			enviar_mensaje(BLOQUE_DATABIN, bloqueDataBinString, socket);
			free(bloqueDataBinString);
			log_info(archivoLog, "Se envio el BLOQUE_DATABIN %d a través del socket %d.", bloqueArchivoCopia->bloqueDataBin, socket);
			// ...e indicando cuánto ocupa en bytes
			// Convierto el espacio ocupado a string para poder enviarlo
			char* espacioOcupadoString = string_itoa(bloqueArchivoCopia->espacioOcupado);
			enviar_mensaje(BYTES_OCUPADOS, espacioOcupadoString, socket);
			free(espacioOcupadoString);
			log_info(archivoLog, "Se enviaron los BYTES_OCUPADOS %d a través del socket %d.", bloqueArchivoCopia->espacioOcupado, socket);
		} else {
			enviar_mensaje(COPIA1_NO_EXISTE, "1", socket);
			log_info(archivoLog, "Se envió el mensaje COPIA1_NO_EXISTE a través del socket %d.", socket);
		}

		// Libero los recursos siempre y cuando sean distintos de NULL
		if (bloqueArchivo != NULL) free(bloqueArchivo);
		if (bloqueArchivoCopia != NULL) free(bloqueArchivoCopia);

	}

	// Libero los recursos
	free(nombreArchivoDetalle);

	return true;

}

// Elimina una entrada de la tabla de archivos
int eliminarEntradaTablaArchivos(char* rutaArchivoYama, char* nombreArchivoYama) {

	// Busco el archivo en la Tabla de Archivos
	int indiceArchivoEncontrado = buscarArchivo(rutaArchivoYama, nombreArchivoYama);

	// Si lo encontré, lo marco como eliminado
	if (indiceArchivoEncontrado > -1) {
		arrayArchivos[indiceArchivoEncontrado].ocupado = 0;
		log_trace(archivoLog, "El archivo ha sido eliminado de la posición %d de la Tabla de Archivos correctamente.", indiceArchivoEncontrado);
		return true;
	} else {
		log_error(archivoLog, "El archivo no existe en la Tabla de Archivos.");
		return false;
	}

}

int buscarArchivo(char* ruta, char* nombre) {

	/* Busca el archivo que tenga esta "ruta" y "nombre" en la Tabla de Archivos.
	 * Devuelve el indiceArchivo si lo encuentra y -1 en caso de que no.
	 */

	// Variables
	int indiceArchivo = -1;
	int i = 0;

	// Loopeo hasta encontrar donde está ese directorio
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		if ( ((strcmp(arrayArchivos[i].ruta, ruta)) == 0 ) && ((strcmp(arrayArchivos[i].nombre, nombre)) == 0) ) {
			indiceArchivo = i;
			break;
		}
	}

	return indiceArchivo;

}

// Actualiza la nueva ruta del archivo en Archivos.dat
int actualizarRutaArchivoEnTablaArchivos(char* rutaArchivoYama, char* nombreArchivoYama, char* nuevaRutaArchivo) {

	// Variables
	int indiceArchivoEncontrado = -1;

	// Busco si existe ese archivo en la tabla de archivos
	indiceArchivoEncontrado = buscarArchivo(rutaArchivoYama, nombreArchivoYama);

	if (indiceArchivoEncontrado > -1) {
		// Si lo encontré, lo actualizo...
		strcpy(arrayArchivos[indiceArchivoEncontrado].ruta, nuevaRutaArchivo);
		log_trace(archivoLog, "La nueva ruta del archivo ha sido actualizada correctamente en la Tabla de Archivos, en la posición %d.", indiceArchivoEncontrado);
		return true;
	} else {
		log_error(archivoLog, "No se pudo actualizar el nombre del archivo ya que no fue encontrado en la Tabla de Archivos.");
		return false;
	}

}


// Actualiza el nuevo nombre del archivo en Archivos.dat
int actualizarNombreArchivoEnTablaArchivos(char* rutaArchivoYama, char* nombreArchivoYama, char* nuevoNombreArchivo) {

	// Variables
	int indiceArchivoEncontrado = -1;

	// Busco si existe ese archivo en la tabla de archivos
	indiceArchivoEncontrado = buscarArchivo(rutaArchivoYama, nombreArchivoYama);

	if (indiceArchivoEncontrado > -1) {
		// Si lo encontré, lo actualizo...
		strcpy(arrayArchivos[indiceArchivoEncontrado].nombre, nuevoNombreArchivo);
		log_trace(archivoLog, "El nuevo nombre de archivo ha sido actualizado correctamente en la Tabla de Archivos, en la posición %d", indiceArchivoEncontrado);
		return true;
	} else {
		log_error(archivoLog, "No se pudo actualizar el nombre del archivo ya que no fue encontrado en la Tabla de Archivos.");
		return false;
	}

}

// Renombra el archivo yama con lo especificado en nuevoNombreArchivo
int renombrarArchivoYama(char* rutaArchivoYama, char* nuevoNombreArchivo) {

	// Variables
	int i = 0; // Para liberar memoria
	int archivoRenombrado = false;

	// Me guardo la cantidad de directorios que tengo en la ruta que me ingresaron
	char** directorios = string_split(rutaArchivoYama, "/");
	int cantidadDirectorios = contarDirectorios(directorios, 1);

	// Busco el numero de directorio donde está alocado ese archivo
	int indiceDirectorio = buscarIndiceDirectorio(directorios[cantidadDirectorios-1]);

	// Paso el numero de directorio a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);

	// Armo la ruta del archivo a renombrar
	char* rutaArchivoViejo = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(directorios[cantidadDirectorios]) + 1);
	strcpy(rutaArchivoViejo, "metadata/archivos/");
	strcat(rutaArchivoViejo, indiceDirectorioString);
	strcat(rutaArchivoViejo, "/");
	strcat(rutaArchivoViejo, directorios[cantidadDirectorios]);

	// Armo la ruta del archivo ya renombrado
	char* rutaArchivoRenombrado = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(nuevoNombreArchivo) + 1);
	strcpy(rutaArchivoRenombrado, "metadata/archivos/");
	strcat(rutaArchivoRenombrado, indiceDirectorioString);
	strcat(rutaArchivoRenombrado, "/");
	strcat(rutaArchivoRenombrado, nuevoNombreArchivo);

	// Libero lo ya usado
	free(indiceDirectorioString);

	// Renombro el archivo llamando a la funcion rename
	if ((rename(rutaArchivoViejo, rutaArchivoRenombrado) ) == 0 ) {
		log_trace(archivoLog, "El archivo %s ha sido renombrado correctamente.", rutaArchivoYama);
		archivoRenombrado = true;
	} else {
		log_error(archivoLog, "El archivo %s no pudo ser renombrado.", rutaArchivoYama);
	}

	// Libero los recursos
	free(rutaArchivoViejo);
	free(rutaArchivoRenombrado);

	// Libero los directorios
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);


	return archivoRenombrado;

}

// Valida que exista un archivo dado en el YAMA FS
int validarArchivoYama(char* rutaArchivo) {

	// Variables
	int archivoExiste = false;
	int i = 0;

	// Me fijo si la ruta que me ingresaron tiene una "/". Si no tiene, retorno que el archivo no es valido.
	if (!string_contains(rutaArchivo, "/")) {
		log_error(archivoLog, "La ruta %s no es una ruta válida para el YAMA FS.", rutaArchivo);
		return archivoExiste;
	}

	// Parseo la ruta de archivo
	char** directorios = string_split(rutaArchivo, "/");

	// Avanzo para saber cual es el ultimo directorio
	while(directorios[i] != NULL) {
		i++;
	}

	// Busco el indice del directorio donde esta alocado el archivo
	int indiceDirectorio = buscarIndiceDirectorio(directorios[i-2]);
	char* indiceString = string_itoa(indiceDirectorio);

	// Armo la entrada para encontrar el archivo detalle
	char* archivoDetalle = malloc(strlen("metadata/archivos/") + strlen(indiceString) + strlen("/") + strlen(directorios[i-1]) + 1);
	strcpy(archivoDetalle, "metadata/archivos/");
	strcat(archivoDetalle, indiceString);
	strcat(archivoDetalle, "/");
	strcat(archivoDetalle, directorios[i-1]);

	// Guardo el contenido del archivo en un buffer
	if ((fopen(archivoDetalle, "r")) != NULL) {
		archivoExiste = true;
	}

	// Libero los recursos
	i = 0;
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);
	free(archivoDetalle);
	free(indiceString);

	return archivoExiste;

}

int actualizarArchivoDetalleArchivo(int numeroBloqueActual, int copia, int sizeOcupadoBloque, char* nodoDisponible, int bloqueDisponible, int indiceDirectorio, char* nombreArchivo) {

	// Paso los valores de int a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);
    char* numeroBloqueActualString = string_itoa(numeroBloqueActual);
    char* copiaString = string_itoa(copia);
    char* bloqueDisponibleString = string_itoa(bloqueDisponible);
    char* sizeOcupadoBloqueString = string_itoa(sizeOcupadoBloque);

	// Armo la ruta para actualizar la información del archivo
	char* info = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(info, "metadata/archivos/");
	strcat(info, indiceDirectorioString);
	strcat(info, "/");
	strcat(info, nombreArchivo);

    // Armo la entrada nuevo del bloque
	char* entradaBloque = malloc(strlen("BLOQUE") + strlen(numeroBloqueActualString) + strlen("COPIA") + strlen(copiaString) + 1);
    strcpy(entradaBloque, "BLOQUE");
    strcat(entradaBloque, numeroBloqueActualString);
    strcat(entradaBloque, "COPIA");
    strcat(entradaBloque, copiaString);

    // Armo ahora la informacion del nodo para esa entrada
	char* datosNodo = malloc(strlen("[") + strlen(nodoDisponible) + strlen(", ") + strlen(bloqueDisponibleString) + strlen("]") + 1);
    strcpy(datosNodo, "[");
    strcat(datosNodo, nodoDisponible);
    strcat(datosNodo, ", ");
    strcat(datosNodo, bloqueDisponibleString);
    strcat(datosNodo, "]");

    // Armo la entrada para la información de la cantidad de bytes que ocupa ese bloque
	char* entradaBytesBloque = malloc(strlen("BLOQUE") + strlen(numeroBloqueActualString) + strlen("BYTES") + 1);
    strcpy(entradaBytesBloque, "BLOQUE");
    strcat(entradaBytesBloque, numeroBloqueActualString);
    strcat(entradaBytesBloque, "BYTES");

	// Abro la tabla de archivos con config
    t_config* tablaArchivo = config_create(info);

    // Doy de alta la nueva entrada para ese bloque
	config_set_value(tablaArchivo, entradaBloque, datosNodo);
	config_set_value(tablaArchivo, entradaBytesBloque, sizeOcupadoBloqueString);
	config_save(tablaArchivo);

	// Libero las entradas que arme
	free(info);
	free(entradaBloque);
	free(datosNodo);
	free(entradaBytesBloque);

	// Libero los strings temporales
	free(indiceDirectorioString);
    free(numeroBloqueActualString);
    free(copiaString);;
    free(bloqueDisponibleString);
    free(sizeOcupadoBloqueString);

	// Libero la tabla de archivos
	config_destroy(tablaArchivo);

	return true;

}

int cargarArrayArchivos(struct t_archivo* archivo, int indiceArchivo, struct t_archivo* arrayArchivos) {

	arrayArchivos[indiceArchivo].index = archivo->index;
	strcpy(arrayArchivos[indiceArchivo].ruta, archivo->ruta);

	return true;

}

int cargarTablaArchivos() {

	// Variables
	int indiceArchivo = 0;
	int estadoCargado = false;

	// Abro la tabla de directorios
	FILE* tablaArchivos = fopen("metadata/archivos/archivos.dat", "r+");
	int tablaArchivosFD = fileno(tablaArchivos);

	// Mapeo el archivo a memoria
	arrayArchivos =  (struct t_archivo*) mmap(NULL, sizeof(struct t_archivo) * CANTIDAD_DIRECTORIOS, PROT_READ | PROT_WRITE, MAP_SHARED, tablaArchivosFD, 0);

	if (arrayArchivos == MAP_FAILED) {
	    log_error(archivoLog, "Error al mapear la Tabla de Archivos.");
	    return estadoCargado;
	} else {
		// Declaro el Registro de Directorio que me va a servir para leer
		t_archivo* archivoLeido = malloc(sizeof(t_archivo));
		for (indiceArchivo = 0; indiceArchivo < CANTIDAD_DIRECTORIOS; ++indiceArchivo) {
			// Leer Registro de Directorio
			fread(archivoLeido, sizeof(t_archivo), 1, tablaArchivos);
			cargarArrayArchivos(archivoLeido, indiceArchivo, arrayArchivos);
		}
		free(archivoLeido);
		estadoCargado = true;
		return estadoCargado;
	}
}

int cargarEstadoAnterior() {

	// Variable
	int estadoCargado = false;

	// Cargo la tabla de directorios en memoria
	estadoCargado = cargarTablaDirectorios();

	// Cargo la tabla de archivos en memoria
	estadoCargado = cargarTablaArchivos();

	// Recupero la lista de nodos de este estado seguro para compararla con la lista de datanodes conectados
	// a medida que se me vayan conectando luego
	estadoCargado = cargarListaNodosEstadoAnterior();

	return estadoCargado;

}

int borrarArchivoYama(char* archivo, int indiceDirectorio, char* rutaYama) {

	// Paso el indice del directorio a String
	char* indiceDirectorioString = string_itoa(indiceDirectorio);

	// Armo la entrada para borrar el archivo
	char* entradaBorrar = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(archivo) + 1);
	strcpy(entradaBorrar, "metadata/archivos/");
	strcat(entradaBorrar, indiceDirectorioString);
	strcat(entradaBorrar, "/");
	strcat(entradaBorrar, archivo);

	// Libero
	free(indiceDirectorioString);

	// Me quedo con la cantidad de bytes que ocupan la ruta y el nombre
	int sizeRutaYNombre = strlen(rutaYama);

	// Desde la última posición empiezo a sumar hasta llegar a la "/"
	int i = sizeRutaYNombre;
	int retrocedi = 0;
	while(rutaYama[i] != '/') {
		i--;
		retrocedi++;
	}

	int longitud = sizeRutaYNombre - retrocedi;

	// Me quedo únicamente con la ruta de yama
	char* rutaArchivoYama = string_substring_until(rutaYama, longitud);

	// Tengo que leer bloque por bloque e ir actualizando el bitmap de bloques de cada nodo que contiene este archivo
	if (leerYLiberarBloquesArchivo(entradaBorrar)) {
		// Si salió bien la operación, recién puedo borrar el archivo
		// Luego elimino la entrada del archivo detalle
		if ( (remove(entradaBorrar) == 0) ) {
			log_trace(archivoLog, "El archivo '%s' ha sido eliminado correctamente.", archivo);
			free(entradaBorrar);
			eliminarEntradaTablaArchivos(rutaArchivoYama, archivo);
			free(rutaArchivoYama);
			return true;
		} else {
			log_error(archivoLog, "Error al borrar el archivo '%s'.\n", archivo);
			free(entradaBorrar);
			free(rutaArchivoYama);
			return false;
		}
	} else {
		log_error(archivoLog, "Hubo un error al querer liberar los bloques que componen el archivo.");
		log_error(archivoLog, "La operación de eliminación no pudo ser realizada.");
		free(entradaBorrar);
		free(rutaArchivoYama);
		return false;
	}
}

int borrarArchivo(char* archivo) {
	if ( (remove(archivo) == 0) ) {
		log_trace(archivoLog, "El archivo '%s' ha sido eliminada correctamente.", archivo);
		return true;
	} else {
		log_error(archivoLog, "Error al borrar el archivo '%s'.\n", archivo);
		return false;
	}
}

int formatearTablaArchivos() {
	remove("metadata/archivos/archivos.dat");
	crearTablaArchivos();
	return true;

}

int formatearFileSystem() {

	// Pido confirmación.
	puts("¿Está seguro que desea formatear el File System? S/N");

	char* linea =  readline(">");

	// Se confirmó la operación.
	if ( string_starts_with("s", linea) | string_starts_with("S", linea) ) {
		estadoSeguro = cargarListaNodosEstadoSeguro();
		if (estadoSeguro == true) {
			log_trace(archivoLog, "El File System pasa a un Estado Seguro.");
			free(linea);
			return true;
		} else {
			log_error(archivoLog, "El File System continua en Estado NO Seguro.");
			free(linea);
			return false;
		}
	}
	// Se canceló la operación.
	if ( string_starts_with("n", linea) | string_starts_with("N", linea) ) {
		puts("Formateo del File System cancelado.");
		free(linea);
		return false;
	} else {
		puts("Respuesta no esperada. Formateo del File System cancelado.");
		free(linea);
		return false;
	}
}

int eliminarMetadata() {

	// Variables
	char* comando = malloc(strlen("rm -r metadata") + 1); // +1 del '\0'

	// Copio el comando para borrar
	strcpy(comando, "rm -r metadata");

	// Realizo el comando
	system(comando);

	// Libero los recursos
	free(comando);

	return true;

}

int crearMetadata() {

	// Creo el directorio "metadata"
	mkdir("metadata", S_IRWXU);

	// Creo el directorio "archivos"
	mkdir("metadata/archivos", S_IRWXU);

	// Creo el directorio "bitmaps"
	mkdir("metadata/bitmaps", S_IRWXU);

	// Creo el directorio "temp"
	mkdir("metadata/temp", S_IRWXU);

	return true;

}


int espacioDisponible(int bloquesNecesarios) {

	// Variables
	int hayEspacio = false;

	// Abro la tabla de nodos
    t_config* tablaNodos = config_create("metadata/nodos.bin");

    // Recupero cantidad de espacio disponible
    int espacioDisponible = config_get_int_value(tablaNodos, "LIBRE");

    // Pregunto si tengo sólo 2 datanodes conectados, tengo que tener la mitad de espacio en cada nodo
    if (list_size(listaDataNodes) == 2) {

    	// Recupero los nombres de los nodos
    	t_datanode* nodo1 = list_get(listaDataNodes, 0);
    	t_datanode* nodo2 = list_get(listaDataNodes, 1);

    	// Armo la entrada para leer la info del espacio disponible del nodo1
    	char* entradaLibreNodo1 = malloc(strlen(nodo1->nombreNodo) + strlen("Libre") + 1);
    	strcpy(entradaLibreNodo1, nodo1->nombreNodo);
    	strcat(entradaLibreNodo1, "Libre");

    	// Armo la entrada para leer la info del espacio disponible del nodo2
    	char* entradaLibreNodo2 = malloc(strlen(nodo2->nombreNodo) + strlen("Libre") + 1);
    	strcpy(entradaLibreNodo2, nodo2->nombreNodo);
    	strcat(entradaLibreNodo2, "Libre");

    	// Recupero el espacio de cada nodo
    	int espacioDisponibleNodo1 = config_get_int_value(tablaNodos, entradaLibreNodo1);
    	int espacioDisponibleNodo2 = config_get_int_value(tablaNodos, entradaLibreNodo2);

    	// Libero las entradas
    	free(entradaLibreNodo1);
    	free(entradaLibreNodo2);

    	// Si el espacio disponible de cualquiera de los dos nodos es menor a la mitad de los bloques necesarios, informo que no tengo espacio
    	if ( ((bloquesNecesarios / 2) > espacioDisponibleNodo1) || ((bloquesNecesarios / 2) > espacioDisponibleNodo2) ) {
    		log_error(archivoLog, "No puedo almacenar el archivo ya que uno de los nodos conectados no posee la mitad de los bloques necesarios.");
    		log_info(archivoLog, "La cantidad de bloques necesarios para almacenar este archivo es de %d bloques.", bloquesNecesarios);
    		log_info(archivoLog, "El nodo %s posee %d bloques disponibles.", nodo1->nombreNodo, espacioDisponibleNodo1);
    		log_info(archivoLog, "El nodo %s posee %d bloques disponibles.", nodo2->nombreNodo, espacioDisponibleNodo2);
    	} else {
    		hayEspacio = true;
    	}
    } else {
        // Si tengo más de dos nodos, con tener espacio disponible me alcanza
        if (espacioDisponible >= bloquesNecesarios) {
        	hayEspacio = true;
        }
    }

    // Libero la tabla de nodos
    config_destroy(tablaNodos);

	return hayEspacio;

}

int sizeArchivo(FILE* archivo) {
	fseek(archivo, 0L, SEEK_END);
	int size = ftell(archivo);
	return size;
}

char* obtenerNombreArchivo(char* rutaArchivo) {

	// Variables
	int i = 0;

	// Parseo la ruta del archivo.
	// Sé que en la última posicion (antes del NULL) voy a tener el nombre del archivo con su extensión
	char** nombre = string_split(rutaArchivo, "/");

	// Mientras no sea NULL, voy sumando
	while(nombre[i] != NULL) {
		i++;
	}

	//Copio el nombre del archivo a la salida
	char* nombreArchivo = malloc(strlen(nombre[i-1]) + 1);
	strcpy(nombreArchivo, nombre[i-1]);

	// Libero recursos
	i = 0;
	while(nombre[i] != NULL) {
		free(nombre[i]);
		i++;
	}
	free(nombre);

	return nombreArchivo;

}

int buscarEspacioLibreTablaArchivos() {

	// Variables
	int i = 1;
	int espacioLibre = -1;
	struct t_archivo* archivoLeido = malloc(sizeof(t_archivo));

	// Abro la Tabla de Directorios para buscar el directorio
	FILE* tablaArchivos = fopen("metadata/archivos/archivos.dat", "r");

	// Lectura anticipada para entrar al while
	fread(archivoLeido, sizeof(t_archivo), 1, tablaArchivos);

	// Loopeo hasta encontrar la primera entrada que tenga el "ocupado" en 0
	while(archivoLeido->ocupado != 0) {
		fread(archivoLeido, sizeof(t_archivo), 1, tablaArchivos);
		i++;
		if (i > CANTIDAD_DIRECTORIOS - 1) break;
	}

	// Si iteré 100 veces es porque no encontré lugar
	if (i > CANTIDAD_DIRECTORIOS - 1) {
		log_error(archivoLog, "No se encontró espacio libre para guardar el archivo.");
	} else {
		// Si no, encontré lugar para guardar el directorio
		espacioLibre = archivoLeido->index;
	}

	// Libero recursos
	free(archivoLeido);

	return espacioLibre;

}

int crearEntradaTablaArchivos(char* ruta, char* nombre) {

	// Variablaes
	int indiceNuevoArchivo = -1;

	// Busco espacio libre para guardar el directorio
	indiceNuevoArchivo = buscarEspacioLibreTablaArchivos();

	// Si no lo encuentro, lo informo.
	if (indiceNuevoArchivo == -1) {
		log_error(archivoLog, "No hay espacio para crear un nuevo archivo en la Tabla de Archivos.");
	} else {
		// Grabo el directorio en el lugar que encontré
		grabarArchivo(indiceNuevoArchivo, ruta, nombre, true);
		log_trace(archivoLog, "El archivo ha sido dado de alta en la posición %d de la Tabla de Archivos correctamente.", indiceNuevoArchivo);
	}

	return indiceNuevoArchivo;

}

int crearArchivoDetalleArchivoCargado(char* nombreArchivo, char* rutaArchivoYAMA, int indiceDirectorio, int tamanio, int bloques, int tipoArchivo) {

	// Variables
	char* tipo = NULL;

	// Paso los valores a string
	char* indiceDirectorioString = string_itoa(indiceDirectorio);
	char* tamanioString = string_itoa(tamanio);
	char* bloquesString = string_itoa(bloques);

	// Abro la tabla de archivos
	//FILE* tablaArchivos = fopen("metadata/archivos/archivos.dat", "a");

/*	// Armo la entrada que quiero dar de alta en la tabla de archivos
	char* entrada = malloc(strlen(rutaArchivoYAMA) + strlen("/") + strlen(nombreArchivo) + strlen("\n") + 1);
	strcpy(entrada, rutaArchivoYAMA);
	strcat(entrada, "/");
	strcat(entrada, nombreArchivo);
	strcat(entrada, "\n");*/

	// Armo la info para crear la información del archivo
	char* info = malloc(strlen("metadata/archivos/") + strlen(indiceDirectorioString) + strlen("/") + strlen(nombreArchivo) + 1);
	strcpy(info, "metadata/archivos/");
	strcat(info, indiceDirectorioString);
	strcat(info, "/");
	strcat(info, nombreArchivo);

	// Creo la información del archivo fisicamente
	FILE* infoArchivo = fopen(info, "w+");

	// Abro el config
	t_config* configArchivo = config_create(info);

	// Indico qué tipo de archivo es
	if (tipoArchivo == 1) {
		tipo = malloc(strlen("BINARIO") + 1);
		strcpy(tipo, "BINARIO");
	} else {
		tipo = malloc(strlen("TEXTO") + 1);
		strcpy(tipo, "TEXTO");
	}

	// Si lo creé correctamente, grabo la entrada en la tabla de archivos
	if (infoArchivo != NULL) {
		//fwrite(entrada, strlen(entrada), 1, tablaArchivos);
		txt_write_in_file(infoArchivo,"TIPO=");
		txt_write_in_file(infoArchivo,"TAMANIO=0");
		txt_write_in_file(infoArchivo,"BLOQUES=0");
		config_set_value(configArchivo, "TIPO", tipo);
		config_set_value(configArchivo, "TAMANIO", tamanioString);
		config_set_value(configArchivo, "BLOQUES", bloquesString);
		config_save(configArchivo);
	} else {
		log_error(archivoLog, "Error al crear el detalle del archivo %s.", nombreArchivo);
	}

	// Libero recursos
	free(tipo);
	free(indiceDirectorioString);
	free(tamanioString);
	free(bloquesString);
	//free(entrada);
	free(info);

	// Cierro los archivos
	fclose(infoArchivo);
	//fclose(tablaArchivos);

	// Liberar el config
	config_destroy(configArchivo);

	return true;

}

int calcularTamanioUltimoBloque(int sizeArchivo, int bloques) {

	// Variables
	int sizeUltimoBloque = 0;

	// Calculo el size
	sizeUltimoBloque = sizeArchivo - ( (bloques - 1) * TAMANIO_BLOQUE_DATABIN );

	return sizeUltimoBloque;

}

/*
===========================
INICIO actualizarSiBloqueOk
===========================
*/

int actualizarSiBloqueOk(char* nodoDisponible, int bloqueDisponible, int numeroBloqueActual, int sizeOcupadoBloque, int indiceDirectorio, char* nombreArchivo, int* bloquesEnviados, int* bloquesPendientes, int numeroCopia) {

	// Funciones necesarias para actualizar los datos de los bloques y los nodos
	actualizarBitmapBloques(nodoDisponible, bloqueDisponible, true);
	actualizarArchivoDetalleArchivo(numeroBloqueActual, numeroCopia, sizeOcupadoBloque, nodoDisponible, bloqueDisponible, indiceDirectorio, nombreArchivo);
	actualizarEspacioLibreNodo(nodoDisponible, -1);

	// Si estoy en la copia 0 actualizo
	if (numeroCopia == COPIA_0) {
		(*bloquesEnviados)++;
		(*bloquesPendientes)--;
	}

	return OK;

}

/*
--------------------------------------------------
FIN actualizarSiBloqueOk
--------------------------------------------------
*/

/*
===========================
INICIO almacenarArchivoBinario
===========================
*/
int almacenarArchivoBinario(char* rutaArchivoOriginal, char* rutaArchivoYama, int indiceDirectorio) {

	/* 1) "Cortar" el archivo en registros completos de 1MB
	 * 2) Designar el DataNode y el bloque que va a almacenar cada copia
	 * 3) Enviar el contenido del bloque a los DataNodes designados
	 */

	// Variables
	int bloquesEnviados = 0;
	int copiasGeneradas = 0;
	int numeroBloqueActual = 0;
	int sizeOcupadoBloque = 0;
	int indiceUltimoNodoUsado = -1;
	int indiceUltimoNodoCopiaUsado = -1;
	char* contenidoBloque;

	// Obtengo el nombre del archivo
	char* nombreArchivo = obtenerNombreArchivo(rutaArchivoOriginal);

	// Obtengo la cantidad de datanodes
	int dataNodesConectados = list_size(listaDataNodes);

	// Si el parámetro "almacenoCopias" es true, busco nodos para alojar copias
	if (almacenoCopias == true) {
		// Tengo que tener al menos dos nodos para poder operar con copias
		if (dataNodesConectados < 2) {
			//No puedo almacenar el archivo porque tengo un solo DataNode (o cero)
			log_error(archivoLog, "No se puede almacenar el archivo porque la cantidad de Nodos no es suficiente.");
			nodosSuficientes = false;
			return ERROR;
		} else {
			log_info(archivoLog, "La cantidad de nodos es suficiente para poder almacenar copias.");
			nodosSuficientes = true;
		}
	}

	// Calculo la cantidad de bloques totales del archivo
	int bloquesTotales = cantidad_bloques_archivo_binario(rutaArchivoOriginal);

	// Bloques Pendientes
	int bloquesPendientes = bloquesTotales;

	// Abro el archivo para lectura
	FILE* archivo = fopen(rutaArchivoOriginal, "r");

	// Calculo el tamaño
	int sizeArchivo = tamanioArchivo(rutaArchivoOriginal);
	int sizeUltimoBloque = calcularTamanioUltimoBloque(sizeArchivo, bloquesTotales);

	// Aloco el buffer que va a alojar el archivo temporalmente
	char* contenidoArchivo = malloc(sizeArchivo);

	// Leo el archivo a mi buffer
	fread(contenidoArchivo, sizeArchivo, 1, archivo);

	// Cierro el archivo
	if (archivo != NULL) fclose(archivo);

	// Me fijo si tengo espacio para almacenar los bloques en el File System
	int hayEspacio = espacioDisponible(bloquesTotales*2);

	// Si tengo espacio en el File System, envío los bloques
	if (hayEspacio) {

		// Creo la entrada en la tabla de archivos
		crearArchivoDetalleArchivoCargado(nombreArchivo, rutaArchivoYama, indiceDirectorio, sizeArchivo, bloquesTotales, 1);

		// Doy de alta el archivo en la Tabla de Archivos
		crearEntradaTablaArchivos(rutaArchivoYama, nombreArchivo);

		log_info(archivoLog, "Almacenando archivo %s...", rutaArchivoOriginal);
		log_info(archivoLog, "El archivo será dividido en %d bloque(s).", bloquesTotales);

		while(bloquesPendientes > 0) {

			char* nodoDisponible = malloc(10);
			char* nodoCopia = malloc(10);

			// Obtengo el Nodo y el Numero de Bloque que va a almacenar este bloque
			int bloqueDisponible = obtenerNodoYBloqueDisponible(nodoDisponible, indiceUltimoNodoUsado);

			// Paso el bloqueDisponible a string
			char* bloqueDisponibleString = string_itoa(bloqueDisponible);

			// No tengo espacio para almacenar el bloque
			if (bloqueDisponible == -1) {
				log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
				log_error(archivoLog, "Se aborta el proceso.");
				free(contenidoArchivo);
				free(nodoDisponible);
				free(nodoCopia);
				return false;
			}

			// Obtengo el socket del nodo que encontré
			int socketNodoDisponible = obtenerSocketNodo(nodoDisponible);

			log_trace(archivoLog, "El bloque %d del archivo será enviado al nodo %s.", numeroBloqueActual, nodoDisponible);

			// Si es el ultimo bloque, aloco lo que en verdad necesito, si no 1 MB
			if (numeroBloqueActual+1 == bloquesTotales) {
				contenidoBloque = calloc(sizeUltimoBloque + 1, 1);
			} else {
				contenidoBloque = calloc(TAMANIO_BLOQUE_DATABIN + 1, 1);
			}

			int offset = TAMANIO_BLOQUE_DATABIN * numeroBloqueActual;

			// Copiamos el buffer contenidoArchivo a contenido_bloque (si es el ultimo bloque copiamos el size), si no 1 MB siempre
			if (numeroBloqueActual+1 == bloquesTotales) {
				memcpy(contenidoBloque, contenidoArchivo + offset, sizeUltimoBloque);
				sizeOcupadoBloque = sizeUltimoBloque;
			} else {
				memcpy(contenidoBloque, contenidoArchivo + offset, TAMANIO_BLOQUE_DATABIN);
				sizeOcupadoBloque = TAMANIO_BLOQUE_DATABIN;
			}

			// Envio el numero de bloque y luego el contenido
			enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleString, socketNodoDisponible);
			enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoDisponible);

			// Libero el bloqueDisponibleString porque ya lo envie
			free(bloqueDisponibleString);

			// Espero 1 el mensaje de OK o no OK
			sleep(retardoOKBloques);
			int bloqueOK = sem_trywait(&set_bloque_OK);
			int bloqueNOOK = sem_trywait(&set_bloque_NO_OK);

			// Recibí un OK del bloque
			if (bloqueOK == 0) {
				actualizarSiBloqueOk(nodoDisponible, bloqueDisponible, numeroBloqueActual, sizeOcupadoBloque, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
				log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoDisponible);
				log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
			} else {
				if (bloqueNOOK == 0) {
					// Recibí un NO OK del bloque
					log_error(archivoLog, "Error al grabar el bloque. Se busca un nuevo nodo.");

					// Busco otro nodo y reintento el envío
					char* nodoReintento = malloc(10);
					int bloqueDisponibleReintento = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoReintento, indiceUltimoNodoCopiaUsado);
					log_info(archivoLog, "El bloque %d del archivo será enviado ahora al nodo %s.", numeroBloqueActual, nodoReintento);

					// Convierto el bloqueDisponibleReintento a string
					char* bloqueDisponibleReintentoString = string_itoa(bloqueDisponibleReintento);

					// No tengo espacio para almacenar el bloque
					if (bloqueDisponibleReintento == -1) {
						log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
						log_error(archivoLog, "Se aborta el proceso.");
						free(contenidoArchivo);
						free(nodoDisponible);
						free(nodoCopia);
						return false;
					}

					// Obtengo el socket del nuevo nodo
					int socketNodoReintento = obtenerSocketNodo(nodoReintento);

					// Envio el numero del bloque y el contenido al nuevo nodo
					enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleReintentoString, socketNodoReintento);
					enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoReintento);

					// Libero el bloqueDisponibleReintentoString porque ya lo envie
					free(bloqueDisponibleReintentoString);

					// Espero el mensaje de OK o no OK
					sleep(retardoOKBloques);
					bloqueOK = sem_trywait(&set_bloque_OK);
					bloqueNOOK = sem_trywait(&set_bloque_NO_OK);

					// Recibí un OK del bloque
					if (bloqueOK == 0) {
						actualizarSiBloqueOk(nodoReintento, bloqueDisponibleReintento, numeroBloqueActual, sizeOcupadoBloque, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
						log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoReintento);
						log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
					} else {
						// Recibí un NO OK del bloque
						if (bloqueNOOK == 0) {
							log_error(archivoLog, "El nodo de reintento %s no pudo grabar el bloque.", nodoReintento);
							log_error(archivoLog, "Se aborta la copia del archivo %s.", nombreArchivo);
							free(contenidoArchivo);
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							return false;
						} else {
							log_error(archivoLog, "El nodo de reintento %s no me respondió ni OK ni NO OK.", nodoReintento);
							log_error(archivoLog, "Se aborta el proceso.");
							free(contenidoArchivo);
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							return false;
						}
					}
				} else {
					log_error(archivoLog, "El nodo %s no me respondió ni OK ni NO OK.", nodoDisponible);
					log_error(archivoLog, "Se aborta el proceso.");
					free(contenidoArchivo);
					free(nodoDisponible);
					free(nodoCopia);
					return false;
				}
			}

			// Si almacenoCopias es true y la cantidad de Nodos es suficiente
			// Envio Copia
			if ((almacenoCopias == true) && (nodosSuficientes == true)) {

				// Busco un bloque disponible en el nodo copia
				int bloqueDisponibleNodoCopia = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoCopia, indiceUltimoNodoCopiaUsado);

				// No tengo espacio para almacenar el bloque
				if (bloqueDisponible == -1) {
					log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
					log_error(archivoLog, "Se aborta el proceso.");
					free(contenidoArchivo);
					free(nodoDisponible);
					free(nodoCopia);
					return false;
				} else {

					// Busco el socket de ese nodo
					int socketNodoCopia = obtenerSocketNodo(nodoCopia);

					// Convierto el bloqueDisponibleNodoCopia a string para enviarlo
					char* bloqueDisponibleNodoCopiaString = string_itoa(bloqueDisponibleNodoCopia);

					enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleNodoCopiaString, socketNodoCopia);
					enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoCopia);

					// Libero el bloqueDisponibleNodoCopiaString porque ya lo envie
					free(bloqueDisponibleNodoCopiaString);

					// Espero el mensaje de OK o no OK
					sleep(retardoOKBloques);
					bloqueOK = sem_trywait(&set_bloque_OK);

					if (bloqueOK == 0) {
						actualizarSiBloqueOk(nodoCopia, bloqueDisponibleNodoCopia, numeroBloqueActual, sizeOcupadoBloque, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_1);
						copiasGeneradas++;
						log_trace(archivoLog, "Se ha almacenado la copia 1 del bloque %d al nodo %s.", numeroBloqueActual, nodoCopia);
					} else {
						log_error(archivoLog, "No se pudo generar la copia 1 para el bloque %d en el nodo %s.", bloqueDisponible, nodoCopia);
					}
				}

			}

			// Actualizo cuántos bloques de este archivo me quedan por enviar
			bloquesPendientes = bloquesTotales - bloquesEnviados;

			// Actualizo el índice del último nodo que usé
			indiceUltimoNodoUsado = buscarIndiceNodo(nodoDisponible, dataNodesConectados);

			// Actualizo el índice del último nodo que usé para copias
			indiceUltimoNodoCopiaUsado = buscarIndiceNodo(nodoCopia, dataNodesConectados);

			// Libero recursos
			free(nodoDisponible);
			free(contenidoBloque);
			free(nodoCopia);

			// Incremento el numero de bloque
			numeroBloqueActual++;

		}

		log_trace(archivoLog, "Se ha(n) enviado %d de %d bloque(s) para el archivo %s.", bloquesEnviados+copiasGeneradas, bloquesTotales*2, nombreArchivo);

	} else {
		log_error(archivoLog, "No hay %d bloque(s) disponible(s) para almacenar el archivo %s.", bloquesTotales*2, nombreArchivo);
	}

	// Libero recursos
	free(contenidoArchivo);

	return true;

}

/*
--------------------------------------------------
FIN almacenarArchivoBinario
--------------------------------------------------
*/

t_list* recuperarInfoBloquesArchivoTexto(char* contenidoArchivo, int sizeArchivo) {

	// Variables auxiliares que necesito (las declaro UNA VEZ fuera de los bucles para NO estar redeclarardolas el tiempo y consumir más memoria
	int sizeArchivoRestante = sizeArchivo; // Arranco con el tamaño del archivo y voy bajando a medido que leo bloques
	int caracteresAEliminar = 0; // Arranco en 0
	int offsetArchivo = 0; // Este offset es para ir moviendome por el contenidoArchivo (me paro donde termine de leer el último bloque de texto)
	int inicioBloque = 0; // El primer bloque arranca en el byte 0 de contenidoArchivo
	int longitudBloque = 0;
	bool flag = false;

	// En esta lista me voy a guardar la info de cada bloque de texto (NO ASI SU CONTENIDO)
	t_list* listaInfoBloques = list_create();

	while ( sizeArchivoRestante > 0) {

		// Copio 1 MiB a un bloque auxiliar (YO supongo que va a ocupar el bloque entero, cuando en realidad no va a ser así)
		char* bloqueAuxiliar = calloc(TAMANIO_BLOQUE_DATABIN + 1, 1); // Pongo +1 para solucionar un error de valgrind
		strncpy(bloqueAuxiliar, contenidoArchivo + offsetArchivo, TAMANIO_BLOQUE_DATABIN);
		//strcpy(bloqueAuxiliar, contenidoArchivo + offsetArchivo);

		/* ME PARO AL FINAL DEL BLOQUE DE 1MiB QUE COPIE Y ME EMPIEZO A MOVER HACIA ATRÁS CARACTER POR CARACTER BUSCANDO UN '\n'.
		 * LA VARIABLES caracteresAEliminar VA A IR EN AUMENTO HASTA ENCONTRAR UN '\n', Y SU VALOR FINAL (AL SALIR DEL BUCLE)
		 * VA A REPRESENTAR LOS CARACTERES QUE NO FORMAN PARTE DEL BLOQUE DEFINITIVO.
		 */

		while(flag == false && sizeArchivoRestante > TAMANIO_BLOQUE_DATABIN ) {

			if ( bloqueAuxiliar[TAMANIO_BLOQUE_DATABIN - caracteresAEliminar] != '\n' ) {
				caracteresAEliminar++;
			} else {
				flag = true;
			}

		} // FIN while buscar '\n'

		/* La longitudBloque es en realidad los bytes que ocupa el bloque actual, es decir, su tamaño. En general, va a ser
		 * siempre un poco menor a 1 MiB. Y ESTO SIEMPRE SE CALCULA ASÍ.
		 *
		 */
		longitudBloque = TAMANIO_BLOQUE_DATABIN - caracteresAEliminar;

		// Aloco memoria para agregar la informacion de este bloque
		t_bloque_texto* bloqueActual = malloc(sizeof(t_bloque_texto));

		// Completo la info del bloque
		bloqueActual->inicio = inicioBloque; // Para el primer bloque vale 0

		if ( sizeArchivoRestante > TAMANIO_BLOQUE_DATABIN){
			bloqueActual->longitud = longitudBloque;
		}else{ // ES EL ÚLTIMO BLOQUE
			bloqueActual->longitud = sizeArchivoRestante;
		}

		// Agrego el bloque a la lista
		list_add(listaInfoBloques, bloqueActual);

		/*
		======================================================================
		INICIO - calculos/ajustes/pasos previos para la siguiente iteración
		======================================================================
		*/

		// Actualizo el inicio para el siguiente bloque
		inicioBloque = inicioBloque + (TAMANIO_BLOQUE_DATABIN - caracteresAEliminar);

		// A medida que leo bloques voy bajando el tamaño del archivo hasta que llegue a 0 (para salir del while principal)
		sizeArchivoRestante = sizeArchivoRestante - (TAMANIO_BLOQUE_DATABIN - caracteresAEliminar);

		offsetArchivo = offsetArchivo + (TAMANIO_BLOQUE_DATABIN - caracteresAEliminar);

		free(bloqueAuxiliar); // IMPORTANTE: Libero la memoria asignada al bloque_auxiliar para hacerle un malloc en la siguiente iteración

		caracteresAEliminar = 0;

		flag = false;

		/*
		------------------------------------------------------------------------
		FIN - calculos/ajustes/pasos previos para la siguiente iteración
		------------------------------------------------------------------------
		*/


	} // FIN while ( sizeArchivo != 0)--> Bucle principal

	/*
	 *  IMPRIMO RESULTADOS PARA VER SI SALIO COMO LO ESPERABAMOS
	 */

	return listaInfoBloques;

}

/*
===========================
INICIO almacenarArchivoTexto
===========================
*/
int almacenarArchivoTexto(char* rutaArchivoOriginal, char* rutaArchivoYama, int indiceDirectorio) {

	/* 1) "Cortar" el archivo en registros alineados a 1MB (dependiendo del último "\n")
	 * 2) Designar el DataNode y el bloque que va a almacenar cada copia
	 * 3) Enviar el contenido del bloque a los DataNodes designados
	 */

	// Variables
	int bloquesEnviados = 0;
	int copiasGeneradas = 0;
	int numeroBloqueActual = 0;
	int bloquesTotales = 0;
	int bloquesPendientes = 0;
	int indiceUltimoNodoUsado = -1;
	int indiceUltimoNodoCopiaUsado = -1;

	// Obtengo la cantidad de datanodes
	int dataNodesConectados = list_size(listaDataNodes);

	// Si el parámetro "almacenoCopias" es true, busco nodos para alojar copias
	if (almacenoCopias == true) {
		// Tengo que tener al menos dos nodos para poder operar con copias
		if (dataNodesConectados < 2) {
			//No puedo almacenar el archivo porque tengo un solo DataNode (o cero)
			log_error(archivoLog, "No se puede almacenar el archivo porque la cantidad de Nodos no es suficiente.");
			nodosSuficientes = false;
			return ERROR;
		} else {
			log_info(archivoLog, "La cantidad de nodos es suficiente para poder almacenar copias.");
			nodosSuficientes = true;
		}
	}

	// Obtengo el nombre del archivo
	char* nombreArchivo = obtenerNombreArchivo(rutaArchivoOriginal);

	// Abro el archivo para lectura
	FILE* archivo = fopen(rutaArchivoOriginal, "r");

	// Calculo el tamaño
	int sizeArchivo = tamanioArchivo(rutaArchivoOriginal);

	// Aloco el buffer que va a alojar el archivo temporalmente
	char* contenidoArchivo = calloc(sizeArchivo + 1, 1);

	// Leo el archivo a mi buffer
	fread(contenidoArchivo, sizeArchivo, 1, archivo);

	// Cierro el archivo
	if (archivo != NULL) fclose(archivo);

	// Obtengo la lista con la información de los bloques para ese archivo
	 t_list* listaBloques = recuperarInfoBloquesArchivoTexto(contenidoArchivo, sizeArchivo);

	// La cantidad de bloques en la lista es mi cantidad total de bloques
	bloquesTotales = list_size(listaBloques);

	// La cantidad de bloques pendientes de envío es la totalidad de los bloques
	bloquesPendientes = bloquesTotales;

	// Me fijo si tengo espacio para almacenar los bloques en el File System
	int hayEspacio = espacioDisponible(bloquesTotales*2);

	// Si tengo espacio en el File System, envío los bloques
	if (hayEspacio) {

		// Creo la entrada en la tabla de archivos
		crearArchivoDetalleArchivoCargado(nombreArchivo, rutaArchivoYama, indiceDirectorio, sizeArchivo, bloquesTotales, 0);

		// Doy de alta el archivo en la Tabla de Archivos
		crearEntradaTablaArchivos(rutaArchivoYama, nombreArchivo);

		log_info(archivoLog, "Almacenando archivo %s...", rutaArchivoOriginal);
		log_info(archivoLog, "El archivo será dividido en %d bloque(s).", bloquesTotales);

		while(bloquesPendientes > 0) {


			char* nodoDisponible = malloc(10);
			char* nodoCopia = malloc(10);

			// Obtengo el Nodo y el Numero de Bloque que va a almacenar este bloque
			int bloqueDisponible = obtenerNodoYBloqueDisponible(nodoDisponible, indiceUltimoNodoUsado);

			// Convierto a string el bloque disponible para poder enviarlo
			char* bloqueDisponibleString = string_itoa(bloqueDisponible);

			// No tengo espacio para almacenar el bloque
			if (bloqueDisponible == -1) {
				log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
				log_error(archivoLog, "Se aborta el proceso.");
				free(contenidoArchivo);
				free(nodoDisponible);
				free(nodoCopia);
				free(bloqueDisponibleString);
				free(nombreArchivo);
				return false;
			}

			// Obtengo el socket del nodo que encontré
			int socketNodoDisponible = obtenerSocketNodo(nodoDisponible);

			log_info(archivoLog, "El bloque %d del archivo será enviado al nodo %s.", numeroBloqueActual, nodoDisponible);

			// Recupero la info del bloque
			t_bloque_texto* bloqueTexto = list_get(listaBloques, numeroBloqueActual);

			//Aloco memoria para el bloque
			char* contenidoBloque = calloc(bloqueTexto->longitud + 1, 1);

			// Copiamos el buffer contenidoArchivo a contenidoBloque
			memcpy(contenidoBloque, contenidoArchivo + bloqueTexto->inicio, bloqueTexto->longitud);

			// Envio el numero de bloque y luego el contenido
			enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleString, socketNodoDisponible);
			enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoDisponible);

			// Libero el numero de bloque convertido porque ya lo envie
			free(bloqueDisponibleString);

			// Espero 1 el mensaje de OK o no OK
			sleep(retardoOKBloques);
			int bloqueOK = sem_trywait(&set_bloque_OK);
			int bloqueNOOK = sem_trywait(&set_bloque_NO_OK);

			// Recibí un OK del bloque
			if (bloqueOK == 0) {
				actualizarSiBloqueOk(nodoDisponible, bloqueDisponible, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
				log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoDisponible);
				log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
			} else {
				if (bloqueNOOK == 0) {
					// Recibí un NO OK del bloque
					log_error(archivoLog, "Error al grabar el bloque. Se busca un nuevo nodo.");

					// Busco otro nodo y reintento el envío
					char* nodoReintento = malloc(10);
					int bloqueDisponibleNodoReintento = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoReintento, indiceUltimoNodoCopiaUsado);
					log_info(archivoLog, "El bloque %d del archivo será enviado ahora al nodo %s.", numeroBloqueActual, nodoReintento);

					// Obtengo el socket del nuevo nodo
					int socketNodoReintento = obtenerSocketNodo(nodoReintento);

					// No tengo espacio para almacenar el bloque
					if (bloqueDisponibleNodoReintento == -1) {
						log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
						log_error(archivoLog, "Se aborta el proceso.");
						free(contenidoArchivo);
						free(nodoDisponible);
						free(nodoCopia);
						free(bloqueDisponibleString);
						free(nombreArchivo);
						return false;
					}

					// Convierto a string el bloqueDisponibleNodoCopia
					char* bloqueDisponibleNodoReintentoString = string_itoa(bloqueDisponibleNodoReintento);

					// Envio el numero del bloque y el contenido al nuevo nodo
					enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleNodoReintentoString, socketNodoReintento);
					enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoReintento);

					// Libero el numero de bloque convertido porque ya lo envie
					free(bloqueDisponibleNodoReintentoString);

					// Espero el mensaje de OK o no OK
					sleep(retardoOKBloques);
					bloqueOK = sem_trywait(&set_bloque_OK);
					bloqueNOOK = sem_trywait(&set_bloque_NO_OK);

					// Recibí un OK del bloque
					if (bloqueOK == 0) {
						actualizarSiBloqueOk(nodoReintento, bloqueDisponibleNodoReintento, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
						log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoReintento);
						log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
					} else {
						// Recibí un NO OK del bloque
						if (bloqueNOOK == 0) {
							log_error(archivoLog, "El nodo de reintento %s no pudo grabar el bloque.", nodoReintento);
							log_error(archivoLog, "Se aborta la copia del archivo %s.", nombreArchivo);
							free(contenidoArchivo);
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							free(nombreArchivo);
							// Libero los recursos de la lista con la info de los bloques
							list_iterate(listaBloques, free); // libero la memoria de cada bloque
							list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
							return false;
						} else {
							log_error(archivoLog, "El nodo de reintento %s no me respondió ni OK ni NO OK.", nodoReintento);
							log_error(archivoLog, "Se aborta el proceso.");
							free(contenidoArchivo);
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							free(nombreArchivo);
							// Libero los recursos de la lista con la info de los bloques
							list_iterate(listaBloques, free); // libero la memoria de cada bloque
							list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
							return false;
						}
					}
				} else {
					log_error(archivoLog, "El nodo %s no me respondió ni OK ni NO OK.", nodoDisponible);
					log_error(archivoLog, "Se aborta el proceso.");
					free(contenidoArchivo);
					free(nodoDisponible);
					free(nodoCopia);
					free(nombreArchivo);
					// Libero los recursos de la lista con la info de los bloques
					list_iterate(listaBloques, free); // libero la memoria de cada bloque
					list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
					return false;
				}
			}

			// Si almacenoCopias es true y la cantidad de Nodos es suficiente
			// Envio Copia
			if ((almacenoCopias == true) && (nodosSuficientes == true)) {

				// Obtengo el bloque disponible en el nodo copia
				int bloqueDisponibleNodoCopia = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoCopia, indiceUltimoNodoCopiaUsado);

				// No tengo espacio para almacenar el bloque
				if (bloqueDisponibleNodoCopia == -1) {
					log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
					log_error(archivoLog, "Se aborta el proceso.");
					free(contenidoArchivo);
					free(nodoDisponible);
					free(nodoCopia);
					free(bloqueDisponibleString);
					free(nombreArchivo);
					return false;
				} else {
					// Obtengo el socket del nodo copia
					int socketNodoCopia = obtenerSocketNodo(nodoCopia);

					// Convierto a string el numero de bloque
					char* bloqueDisponibleNodoCopiaString = string_itoa(bloqueDisponibleNodoCopia);

					enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleNodoCopiaString, socketNodoCopia);
					enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoCopia);

					// Libero el bloqueDisponibleNodoCopiaString porque ya lo envie
					free(bloqueDisponibleNodoCopiaString);

					// Espero el mensaje de OK o no OK
					sleep(retardoOKBloques);
					bloqueOK = sem_trywait(&set_bloque_OK);

					if (bloqueOK == 0) {
						actualizarSiBloqueOk(nodoCopia, bloqueDisponibleNodoCopia, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_1);
						copiasGeneradas++;
						log_trace(archivoLog, "Se ha almacenado la copia 1 del bloque %d en el nodo %s.", numeroBloqueActual, nodoCopia);
					} else {
						log_error(archivoLog, "No se pudo generar la copia 1 para el bloque %d en el nodo %s.", bloqueDisponible, nodoCopia);
					}
				}
			}

			// Actualizo cuántos bloques de este archivo me quedan por enviar
			bloquesPendientes = bloquesTotales - bloquesEnviados;

			// Actualizo el índice del último nodo que usé
			indiceUltimoNodoUsado = buscarIndiceNodo(nodoDisponible, dataNodesConectados);

			// Actualizo el índice del último nodo que usé para copias
			indiceUltimoNodoCopiaUsado = buscarIndiceNodo(nodoCopia, dataNodesConectados);

			// Libero recursos
			free(nodoDisponible);
			free(contenidoBloque);
			free(nodoCopia);

			// Incremento el numero de bloque
			numeroBloqueActual++;

		}

		log_trace(archivoLog, "Se ha(n) enviado %d de %d bloque(s) para el archivo %s.", bloquesEnviados+copiasGeneradas, bloquesTotales*2, nombreArchivo);

	} else {
		log_error(archivoLog, "No hay %d bloque(s) disponible(s) para almacenar el archivo %s.", bloquesTotales*2, nombreArchivo);
	}

	// Libero recursos
	free(nombreArchivo);
	free(contenidoArchivo);

	// Libero los recursos de la lista con la info de los bloques
	list_iterate(listaBloques, free); // libero la memoria de cada bloque
	list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)

	return true;

}

/*
--------------------------------------------------
FIN almacenarArchivoTexto
--------------------------------------------------
*/

/*
===========================
INICIO almacenarArchivoReduccionGlobal
===========================
*/
int almacenarArchivoReduccionGlobal(char* bufferArchivoReduccionGlobal, char* rutaYama, int indiceDirectorio, char* rutaDirectorioYama) {

	/* 1) "Cortar" el archivo en registros alineados a 1MB (dependiendo del último "\n")
	 * 2) Designar el DataNode y el bloque que va a almacenar cada copia
	 * 3) Enviar el contenido del bloque a los DataNodes designados
	 */

	// Variables
	int bloquesEnviados = 0;
	int copiasGeneradas = 0;
	int numeroBloqueActual = 0;
	int bloquesTotales = 0;
	int bloquesPendientes = 0;
	int indiceUltimoNodoUsado = -1;
	int indiceUltimoNodoCopiaUsado = -1;

	// Protocolo para recibir el OK y el NO OK de los bloques
	t_protocolo* protocolo_Recepcion;

	// Obtengo la cantidad de datanodes
	int dataNodesConectados = list_size(listaDataNodes);

	// Si el parámetro "almacenoCopias" es true, busco nodos para alojar copias
	if (almacenoCopias == true) {
		// Tengo que tener al menos dos nodos para poder operar con copias
		if (dataNodesConectados < 2) {
			//No puedo almacenar el archivo porque tengo un solo DataNode (o cero)
			log_error(archivoLog, "No se puede almacenar el archivo porque la cantidad de Nodos no es suficiente.");
			nodosSuficientes = false;
			return ERROR;
		} else {
			log_info(archivoLog, "La cantidad de nodos es suficiente para poder almacenar copias.");
			nodosSuficientes = true;
		}
	}

	// Calculo el tamaño
	int sizeArchivo = strlen(bufferArchivoReduccionGlobal);

	// Obtengo el nombre del archivo
	char* nombreArchivo = obtenerNombreArchivo(rutaYama);

	// Obtengo la lista con la información de los bloques para ese archivo
	 t_list* listaBloques = recuperarInfoBloquesArchivoTexto(bufferArchivoReduccionGlobal, sizeArchivo);

	// La cantidad de bloques en la lista es mi cantidad total de bloques
	bloquesTotales = list_size(listaBloques);

	// La cantidad de bloques pendientes de envío es la totalidad de los bloques
	bloquesPendientes = bloquesTotales;

	// Me fijo si tengo espacio para almacenar los bloques en el File System
	int hayEspacio = espacioDisponible(bloquesTotales*2);

	// Si tengo espacio en el File System, envío los bloques
	if (hayEspacio) {

		// Creo la entrada en la tabla de archivos
		crearArchivoDetalleArchivoCargado(nombreArchivo, rutaYama, indiceDirectorio, sizeArchivo, bloquesTotales, 0);

		// Doy de alta el archivo en la Tabla de Archivos
		crearEntradaTablaArchivos(rutaDirectorioYama, nombreArchivo);

		log_info(archivoLog, "Almacenando archivo %s...", rutaYama);
		log_info(archivoLog, "El archivo será dividido en %d bloque(s).", bloquesTotales);

		while(bloquesPendientes > 0) {


			char* nodoDisponible = malloc(10);
			char* nodoCopia = malloc(10);

			// Obtengo el Nodo y el Numero de Bloque que va a almacenar este bloque
			int bloqueDisponible = obtenerNodoYBloqueDisponible(nodoDisponible, indiceUltimoNodoUsado);

			// Convierto a string el bloque disponible para poder enviarlo
			char* bloqueDisponibleString = string_itoa(bloqueDisponible);

			// No tengo espacio para almacenar el bloque
			if (bloqueDisponible == -1) {
				log_error(archivoLog, "No tengo espacio para almacenar el bloque %d.", numeroBloqueActual);
				log_error(archivoLog, "Se aborta el proceso.");
				free(bufferArchivoReduccionGlobal);
				free(nodoDisponible);
				free(nodoCopia);
				free(bloqueDisponibleString);
				free(nombreArchivo);
				return false;
			}

			// Obtengo el socket del nodo que encontré
			int socketNodoDisponible = obtenerSocketNodo(nodoDisponible);

			log_info(archivoLog, "El bloque %d del archivo será enviado al nodo %s.", numeroBloqueActual, nodoDisponible);

			// Recupero la info del bloque
			t_bloque_texto* bloqueTexto = list_get(listaBloques, numeroBloqueActual);

			//Aloco memoria para el bloque
			char* contenidoBloque = calloc(bloqueTexto->longitud + 1, 1);

			// Copiamos el buffer contenidoArchivo a contenidoBloque
			memcpy(contenidoBloque, bufferArchivoReduccionGlobal + bloqueTexto->inicio, bloqueTexto->longitud);

			// Envio el numero de bloque y luego el contenido
			enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleString, socketNodoDisponible);
			enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoDisponible);

			// Libero el numero de bloque convertido porque ya lo envie
			free(bloqueDisponibleString);

			protocolo_Recepcion = recibir_mensaje(socketNodoDisponible);

			// Recibí un OK del bloque
			if (protocolo_Recepcion->funcion == SET_BLOQUE_SUCCESS) {
				actualizarSiBloqueOk(nodoDisponible, bloqueDisponible, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
				log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoDisponible);
				log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
			} else {
				if (protocolo_Recepcion->funcion == SET_BLOQUE_FAILURE) {
					// Recibí un NO OK del bloque
					log_error(archivoLog, "Error al grabar el bloque. Se busca un nuevo nodo.");

					// Busco otro nodo y reintento el envío
					char* nodoReintento = malloc(10);
					int bloqueDisponibleNodoReintento = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoReintento, indiceUltimoNodoCopiaUsado);
					log_info(archivoLog, "El bloque %d del archivo será enviado ahora al nodo %s.", numeroBloqueActual, nodoReintento);

					// Obtengo el socket del nuevo nodo
					int socketNodoReintento = obtenerSocketNodo(nodoReintento);

					// Convierto a string el bloqueDisponibleNodoCopia
					char* bloqueDisponibleNodoReintentoString = string_itoa(bloqueDisponibleNodoReintento);

					// Envio el numero del bloque y el contenido al nuevo nodo
					enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleNodoReintentoString, socketNodoReintento);
					enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoReintento);

					// Libero el numero de bloque convertido porque ya lo envie
					free(bloqueDisponibleNodoReintentoString);

					// Espero el mensaje de OK o no OK
					eliminar_protocolo(protocolo_Recepcion);
					protocolo_Recepcion = recibir_mensaje(socketNodoReintento);

					// Recibí un OK del bloque
					if (protocolo_Recepcion->funcion == SET_BLOQUE_SUCCESS) {
						actualizarSiBloqueOk(nodoReintento, bloqueDisponibleNodoReintento, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_0);
						log_trace(archivoLog, "Bloque %d grabado OK en el nodo %s.", numeroBloqueActual, nodoReintento);
						log_info(archivoLog, "Se han enviado %d bloque(s). Quedan %d bloque(s) por enviar.", bloquesEnviados, bloquesPendientes);
					} else {
						// Recibí un NO OK del bloque
						if (protocolo_Recepcion->funcion == SET_BLOQUE_FAILURE) {
							log_error(archivoLog, "El nodo de reintento %s no pudo grabar el bloque.", nodoReintento);
							log_error(archivoLog, "Se aborta la copia del archivo %s.", nombreArchivo);
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							free(nombreArchivo);
							// Libero los recursos de la lista con la info de los bloques
							list_iterate(listaBloques, free); // libero la memoria de cada bloque
							list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
							return false;
						} else {
							log_error(archivoLog, "El nodo de reintento %s no me respondió ni OK ni NO OK.", nodoReintento);
							log_error(archivoLog, "Se aborta el proceso.");
							free(nodoDisponible);
							free(nodoCopia);
							free(nodoReintento);
							free(nombreArchivo);
							// Libero los recursos de la lista con la info de los bloques
							list_iterate(listaBloques, free); // libero la memoria de cada bloque
							list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
							return false;
						}
					}
				} else {
					log_error(archivoLog, "El nodo %s no me respondió ni OK ni NO OK.", nodoDisponible);
					log_error(archivoLog, "Se aborta el proceso.");
					free(nodoDisponible);
					free(nodoCopia);
					free(nombreArchivo);
					// Libero los recursos de la lista con la info de los bloques
					list_iterate(listaBloques, free); // libero la memoria de cada bloque
					list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)
					return false;
				}
			}

			// Si almacenoCopias es true y la cantidad de Nodos es suficiente
			// Envio Copia
			if ((almacenoCopias == true) && (nodosSuficientes == true)) {

				// Obtengo el bloque disponible en el nodo copia
				int bloqueDisponibleNodoCopia = obtenerNodoCopiaYBloqueDisponible(nodoDisponible, nodoCopia, indiceUltimoNodoCopiaUsado);

				// Obtengo el socket del nodo copia
				int socketNodoCopia = obtenerSocketNodo(nodoCopia);

				// Convierto a string el numero de bloque
				char* bloqueDisponibleNodoCopiaString = string_itoa(bloqueDisponibleNodoCopia);

				enviar_mensaje(SET_BLOQUE_NUMERO, bloqueDisponibleNodoCopiaString, socketNodoCopia);
				enviar_mensaje(SET_BLOQUE_CONTENIDO, contenidoBloque, socketNodoCopia);

				// Libero el bloqueDisponibleNodoCopiaString porque ya lo envie
				free(bloqueDisponibleNodoCopiaString);

				// Espero el mensaje de OK o no OK
				eliminar_protocolo(protocolo_Recepcion);
				protocolo_Recepcion = recibir_mensaje(socketNodoCopia);

				if (protocolo_Recepcion->funcion == SET_BLOQUE_SUCCESS) {
					actualizarSiBloqueOk(nodoCopia, bloqueDisponibleNodoCopia, numeroBloqueActual, bloqueTexto->longitud, indiceDirectorio, nombreArchivo, &bloquesEnviados, &bloquesPendientes, COPIA_1);
					copiasGeneradas++;
					log_trace(archivoLog, "Se ha almacenado la copia 1 del bloque %d en el nodo %s.", numeroBloqueActual, nodoCopia);
				} else {
					log_error(archivoLog, "No se pudo generar la copia 1 para el bloque %d en el nodo %s.", bloqueDisponible, nodoCopia);
				}

			}

			// Actualizo cuántos bloques de este archivo me quedan por enviar
			bloquesPendientes = bloquesTotales - bloquesEnviados;

			// Actualizo el índice del último nodo que usé
			indiceUltimoNodoUsado = buscarIndiceNodo(nodoDisponible, dataNodesConectados);

			// Actualizo el índice del último nodo que usé para copias
			indiceUltimoNodoCopiaUsado = buscarIndiceNodo(nodoCopia, dataNodesConectados);

			// Libero recursos
			free(nodoDisponible);
			free(contenidoBloque);
			free(nodoCopia);

			// Incremento el numero de bloque
			numeroBloqueActual++;

			// Libero el protocolo para la siguiente iteración
			eliminar_protocolo(protocolo_Recepcion);

		} // FIN WHILE

		log_trace(archivoLog, "Se ha(n) enviado %d de %d bloque(s) para el archivo %s.", bloquesEnviados+copiasGeneradas, bloquesTotales*2, nombreArchivo);

	} else {
		log_error(archivoLog, "No hay %d bloque(s) disponible(s) para almacenar el archivo %s.", bloquesTotales*2, nombreArchivo);
	}

	// Libero recursos
	free(nombreArchivo);

	// Libero los recursos de la lista con la info de los bloques
	list_iterate(listaBloques, free); // libero la memoria de cada bloque
	list_destroy(listaBloques); // libero la memoria del tipo lista en sí (la reservada por list_create)

	return true;

}

/*
--------------------------------------------------
FIN almacenarArchivoReduccionGlobal
--------------------------------------------------
*/

int tieneHijos(int indiceDirectorio) {

	// Variables
	int i = 0;

	// Loopeo hasta encontrar algún directorio que tenga como padre a indiceDirectorio
	for(i = 1; i < CANTIDAD_DIRECTORIOS; i++) {
		if(arrayDirectorios[i].padre == indiceDirectorio) break;
	}

	// Si recorrí toda la lista y no encontré ninguno, entonces no tiene hijos
	if (i > CANTIDAD_DIRECTORIOS - 1)
		return false;
	else
		return true;

}

// Valida que exista un archivo en el File System Local
int validarArchivo(char* nombreArchivo) {

	/* Me fijo si con la ruta que me pasaron puedo abrir el archivo
	 * Si puedo, la ruta es correcta y retorno true.
	 * Si no, la ruta es incorrecta y retorno false
	 */

	// Variables
	int archivoValido = false;

	if ((fopen(nombreArchivo, "r")) != NULL) {
		archivoValido = true;
	}

	return archivoValido;

}

