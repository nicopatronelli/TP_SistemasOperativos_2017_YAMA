#include "Globales.h"
#include "Nodos.h"
#include "Directorios.h"

void config_remove_key(t_config* self, char* key) {
	dictionary_remove(self->properties, key);
}

int config_save_in_file(t_config *self, char* path) {
	FILE* file = fopen(path, "wb+");

	if (file == NULL) {
			return -1;
	}

	char* lines = string_new();
	void add_line(char* key, void* value) {
		string_append_with_format(&lines, "%s=%s\n", key, value);
	}

	dictionary_iterator(self->properties, add_line);
	int result = fwrite(lines, strlen(lines), 1, file);
	fclose(file);
	free(lines);
	return result;
}

void config_set_value(t_config *self, char *key, char *value) {
	t_dictionary* dictionary = self->properties;
	char* duplicate_value = string_duplicate(value);

	if(dictionary_has_key(dictionary, key)) {
		dictionary_remove_and_destroy(dictionary, key, free);
	}

	dictionary_put(self->properties, key, (void*)duplicate_value);
}

int config_save(t_config *self) {
	return config_save_in_file(self, self->path);
}

int cargarListaNodosEstadoSeguro() {

	/* Esta funcion carga la lista de Nodos Estado Seguro
	 * con lo que recupera de la tabla de nodos del estado anterior
	 */

	// Variables
	int i = 0;

	// Creo la lista
	listaNodosEstadoSeguro = list_create();

	// Cantidad y lista de Nodos
	int cantidadNodos = list_size(listaDataNodes);

	// Valido la cantidad de nodos del formateo
	if (cantidadNodos < 2) {
		log_error(archivoLog, "La cantidad de nodos (%d) no permite realizar el formateo.", cantidadNodos);
		log_error(archivoLog, "El sistema requiere al menos dos nodos.");
		log_error(archivoLog, "Se aborta el proceso de formateo del File System.");
		return false;
	} else {

		// Loopeo la lista de datanodes conectados y me voy quedando con el nombre de cada uno
		for (i = 0; i < cantidadNodos; ++i) {
			char* nombreNodo = malloc(10);
			t_datanode* nodoConectado = list_get(listaDataNodes, i);
			strcpy(nombreNodo, nodoConectado->nombreNodo);
			list_add(listaNodosEstadoSeguro, nombreNodo);
		}

		// Imprimo los nodos que forman parte del Estado Seguro
		log_info(archivoLog, "Los Nodos que forman parte del estado seguro son: ");

		printf("\n");

		for (i = 0; i < cantidadNodos; ++i) {
			char* nodo = list_get(listaNodosEstadoSeguro, i);
			log_info(archivoLog, "%s", nodo);
		}

		printf("\n");

	}

	return true;

}

int cargarListaNodosEstadoAnterior() {

	/* Esta funcion carga la lista de Nodos Estado Anterior
	 * con lo que recupera de la tabla de nodos del estado anterior
	 */

	// Variables
	int i = 0;

	// Creo la lista
	listaNodosEstadoAnterior = list_create();

	// Cantidad y lista de Nodos
	int cantidadNodos = recuperarCantidadNodos();

	// Valido la cantidad de nodos del estado anterior que me pasaron
	if (cantidadNodos < 2) {
		log_error(archivoLog, "La cantidad de nodos del estado anterior es %d.", cantidadNodos);
		log_error(archivoLog, "No se permite esa cantidad de nodos para un estado seguro.");
		log_error(archivoLog, "Se aborta el proceso de carga de estado anterior.");
		log_info(archivoLog, "El File System iniciará como si no existiera este estado anterior.");
		return false;
	} else {
		// Abro la tabla de nodos
		t_config* tablaNodos = config_create("metadata/nodos.bin");

		// Me quedo con la lista de Nodos de la tabla de nodos
		char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

		// Loopeo hasta quedarme con el nodo que tiene más espacio disponible
		for (i = 0; i < cantidadNodos; ++i) {
			char* nombreNodo = malloc(10);
			strcpy(nombreNodo, listaNodos[i]);
			list_add(listaNodosEstadoAnterior, nombreNodo);
			//free(nombreNodo);
		}

		// Libero los recursos
		config_destroy(tablaNodos);
		i = 0;
		while(listaNodos[i] != NULL) {
			free(listaNodos[i]);
			i++;
		}
		free(listaNodos);

		return true;
	}

}

int verificarEstadoSeguro() {

	/* Esta funcion verifica si el nodo que se me acaba de conectar es el ultimo nodo requerido
	 * para pasar a un estado seguro. En caso de que así sea, devuelve TRUE. Caso contario, devuelve FALSE.
	 *
	 */

	// Recupero la cantidad de nodos conectados
	int nodosConectados = list_size(listaDataNodes);

	// Si la cantidad de nodos es menor a 2, no es un estado seguro
	if (nodosConectados < 2) {
		return false;
	} else {

		// Verificar archivo por archivo si con la cantidad de nodos conectados puedo recontruirlos
		int archivosOK = verificarArchivos();

		// Si todos los archivos están OK, entonces paso a estado seguro
		if (archivosOK) {
			return true;
		} else {
			return false;
		}
	}
}

// Leer la tabla de archivos y por cada uno de los archivos llamar a "verificarComposicionArchivo"
int verificarArchivos() {

	// Variables
	int archivosOK = true;
	int i = 0;

	// Leo la Tabla de Archivos hasta el final y voy llamando a "verificarComposicionArchivo"
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		if (arrayArchivos[i].ocupado == 1) {
			if (verificarComposicionArchivo(arrayArchivos[i].ruta, arrayArchivos[i].nombre, listaDataNodes)) {
				log_info(archivoLog, "Con esta distribucion de Nodos puedo armar el archivo %s.", arrayArchivos[i].nombre);
			} else {
				log_error(archivoLog, "Con esta distribucion de Nodos NO puedo armar el archivo %s.", arrayArchivos[i].nombre);
				archivosOK = false;
				break;
			}
		}
	}

	return archivosOK;

}


bool _nombreNodoEstaEnLista(void* nodoLista) {

	int existe = strcmp(((t_datanode*)nodoLista)->nombreNodo, nombreNodoActual);

	if (existe == 0) {
		return true;
	} else {
		return false;
	}
}

int verificarComposicionArchivo(char* rutaArchivoYama, char* nombreArchivoYama, t_list* listaDatanodesConectados) {

	// Variables
	int i = 0;
	int j = 0; // Indice para liberar mallocs

	bool copia0OK = false;
	bool copia1OK = false;
	int composicionArchivoOK = true;

	// Me quedo unicamente con la ruta de directorios
	char** directorios = string_split(rutaArchivoYama, "/");

	// Me quedo con la cantidad de directorios
	int cantidadDirectorios = contarDirectorios(directorios, 0);

	// Buscar indice directorio
	int indiceDirectorio = validarRutaDirectorios(directorios[cantidadDirectorios-1]);

	// Indice del directorio
	char* indice = string_itoa(indiceDirectorio);

	// Armo la entrada para poder leer el archivo de detalle
	char* entradaArchivo = malloc(strlen("metadata/archivos/") + strlen(indice) + strlen("/") + strlen(nombreArchivoYama) + 1);
	strcpy(entradaArchivo, "metadata/archivos/");
	strcat(entradaArchivo, indice);
	strcat(entradaArchivo, "/");
	strcat(entradaArchivo, nombreArchivoYama);

	// Abrir archivo de detalle
	t_config* archivoDetalle = config_create(entradaArchivo);

	// Obtengo la cantidad de bloques que ocupa ese archivo
	int bloques = config_get_int_value(archivoDetalle, "BLOQUES");

	// Loopeo preguntando si con la cantidad de nodos que tengo conectados puedo armar el archivo
	for (i = 0; i < bloques; ++i) {

		// Paso a string el i
		char* iString = string_itoa(i);

		// Aloco las entradas para los bloques
		char* entradaBloqueCopia0 = malloc(strlen("BLOQUE999COPIA0") + 1);
		char* entradaBloqueCopia1 = malloc(strlen("BLOQUE999COPIA1") + 1);

		// Actualizo la info para el primer bloque
		strcpy(entradaBloqueCopia0, "BLOQUE");
		strcat(entradaBloqueCopia0, iString);
		strcat(entradaBloqueCopia0, "COPIA0");

		// Actualizo la info para el primer bloque
		strcpy(entradaBloqueCopia1, "BLOQUE");
		strcat(entradaBloqueCopia1, iString);
		strcat(entradaBloqueCopia1, "COPIA1");

		// Estoy trabajando a nivel bloque
		if (config_has_property(archivoDetalle, entradaBloqueCopia0)) {
			char** datosBloqueCopia0 = config_get_array_value(archivoDetalle, entradaBloqueCopia0);
			// Esto es para poder iterar en list_any_satisfy
			nombreNodoActual = malloc(strlen(datosBloqueCopia0[0]) + 1);
			strcpy(nombreNodoActual, datosBloqueCopia0[0]);
			// Me fijo si tengo la copia 0 del bloque en ALGUNO de los datanodes conectados...
			copia0OK = list_any_satisfy(listaDatanodesConectados, _nombreNodoEstaEnLista);

			// Libero lo que use
			free(nombreNodoActual);
			j = 0;
			while(datosBloqueCopia0[j] != NULL) {
				free(datosBloqueCopia0[j]);
				j++;
			}
			free(datosBloqueCopia0);

		}

		if (config_has_property(archivoDetalle, entradaBloqueCopia1)) {
			char** datosBloqueCopia1 = config_get_array_value(archivoDetalle, entradaBloqueCopia1);
			nombreNodoActual = malloc(strlen(datosBloqueCopia1[0]) + 1);
			strcpy(nombreNodoActual, datosBloqueCopia1[0]);
			copia1OK = list_any_satisfy(listaDatanodesConectados, _nombreNodoEstaEnLista);

			// Libero lo que use
			free(nombreNodoActual);
			j = 0;
			while(datosBloqueCopia1[j] != NULL) {
				free(datosBloqueCopia1[j]);
				j++;
			}
			free(datosBloqueCopia1);
		}

		// Libero los recursos
		free(iString);
		free(entradaBloqueCopia0);
		free(entradaBloqueCopia1);

		if (!copia0OK && !copia1OK) {
			composicionArchivoOK = false;
			break;
		}


	} // FIN FOR

	// Libero los recursos
	free(indice);
	free(entradaArchivo);

	// Libero el array de directorios
	i = 0;
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);

	// Libero el config
	config_destroy(archivoDetalle);

	return composicionArchivoOK;

}

int recuperarCantidadNodos() {

	// Variables
	int cantidadNodos = 0;
	int i = 0;

	// Abro la tabla de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Me quedo con la lista de Nodos de la tabla de nodos
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Itero contando la cantidad de nodos
	while (listaNodos[cantidadNodos] != NULL) {
		cantidadNodos++;
	}

	// Libero el espacio de memoria apuntado por listaNodos
	while(listaNodos[i] != NULL) {
		free(listaNodos[i]);
		i++;
	}
	free(listaNodos);

	// Libero recursos
	config_destroy(tablaNodos);

	return cantidadNodos;

}

int obtenerEspacioDisponibleNodo(char* nodo, t_config* tablaNodos) {

	// Variables
	int espacioDisponibleNodo = 0;

	// Aloco memoria para la entrada
	char* libre = malloc(strlen(nodo) + strlen("Libre") + 1);

	// Armo la entrada
	strcpy(libre, nodo);
	strcat(libre, "Libre");

	// Recupero el dato de la tabla de nodos
    espacioDisponibleNodo = config_get_int_value(tablaNodos, libre);

    // Libero los recursos
    free(libre);

    // Devuelvo el valor
	return espacioDisponibleNodo;

}

int obtenerIPYPuertoNodo(char* nodo, char* ipNodo) {

	// Variables
	int puertoEscuchaMaster = 0;
	int i = 0;
	int sizeLista = list_size(listaDataNodes);

	for (i = 0; i < sizeLista; ++i) {
		t_datanode* dataNode = list_get(listaDataNodes, i);
		if (strcmp(dataNode->nombreNodo, nodo) == 0) {
			strcpy(ipNodo, dataNode->ipNodo);
			puertoEscuchaMaster = dataNode->puertoEscuchaMaster;
			break;
		}
	}

	return puertoEscuchaMaster;
}

int obtenerSocketNodo(char* nodo) {

	// Variables
	int socketNodo = 0;
	int i = 0;
	int sizeLista = list_size(listaDataNodes);

	for (i = 0; i < sizeLista; ++i) {
		t_datanode* dataNode = list_get(listaDataNodes, i);
		if (strcmp(dataNode->nombreNodo, nodo) == 0) {
			socketNodo = dataNode->socket_client;
		}
	}

	return socketNodo;
}

int leerYLiberarBloquesArchivo(char* archivoDetalle) {

	// Variables
	int i = 0;

	// Paso las variables a tipo string
	char* copia0 = string_itoa(COPIA_0);
	char* copia1 = string_itoa(COPIA_1);

	// Abro el archivo de detalle
	t_config* archivo = config_create(archivoDetalle);

	// Recupero la cantidad de bloques para saber cuánto iterar
	int bloques = config_get_int_value(archivo, "BLOQUES");

	// Itero bloque por bloque
	for (i = 0; i < bloques; ++i) {

		// Convierto el i a String
		char* iString = string_itoa(i);

	    // Armo la entrada genérica para pedir el bloque copia 0
	    char* bloqueCopia0 = malloc(strlen("BLOQUE") + strlen(iString) + strlen("COPIA") + strlen(copia0) + 1);
	    strcpy(bloqueCopia0, "BLOQUE");
	    strcat(bloqueCopia0, iString);
	    strcat(bloqueCopia0, "COPIA");
	    strcat(bloqueCopia0, copia0);

	    // Armo la entrada genérica para pedir el bloque copia 0
	    char* bloqueCopia1 = malloc(strlen("BLOQUE") + strlen(iString) + strlen("COPIA") + strlen(copia1) + 1);
	    strcpy(bloqueCopia1, "BLOQUE");
	    strcat(bloqueCopia1, iString);
	    strcat(bloqueCopia1, "COPIA");
	    strcat(bloqueCopia1, copia1);

	    // Si existe la entrada para la copia 0, leo los datos y libero el espacio en ese nodo
	    if (config_has_property(archivo, bloqueCopia0)) {

	    	// Recupero los datos
	    	char** datosBloqueCopia0 = config_get_array_value(archivo, bloqueCopia0);

	    	// Actualizo el bitmaps de bloques de ese nodo (libero el bloque)
	    	actualizarBitmapBloques(datosBloqueCopia0[0], atoi(datosBloqueCopia0[1]), false);

	    	log_info(archivoLog, "El bloque %d del nodo %s ha sido liberado correctamente.", atoi(datosBloqueCopia0[1]), datosBloqueCopia0[0]);

	    	// Sumo uno al espacio libre en el nodo ya que se libera un bloque
	    	actualizarEspacioLibreNodo(datosBloqueCopia0[0], 1);

	    	free(datosBloqueCopia0[0]);
	    	free(datosBloqueCopia0[1]);
	    	free(datosBloqueCopia0);

	    }

	    // Si existe la entrada para la copia 0, leo los datos y libero el espacio en ese nodo
	    if (config_has_property(archivo, bloqueCopia1)) {

	    	// Recupero los datos
	    	char** datosBloqueCopia1 = config_get_array_value(archivo, bloqueCopia1);

	    	// Actualizo el bitmaps de bloques de ese nodo (libero el bloque)
	    	actualizarBitmapBloques(datosBloqueCopia1[0], atoi(datosBloqueCopia1[1]), false);

	    	log_info(archivoLog, "El bloque %d del nodo %s ha sido liberado correctamente.", atoi(datosBloqueCopia1[1]), datosBloqueCopia1[0]);

	    	// Sumo uno al espacio libre en el nodo ya que se libera un bloque
	    	actualizarEspacioLibreNodo(datosBloqueCopia1[0], 1);

	    	free(datosBloqueCopia1[0]);
	    	free(datosBloqueCopia1[1]);
	    	free(datosBloqueCopia1);

	    }

	    // Libero el i
	    free(iString);
	    free(bloqueCopia0);
	    free(bloqueCopia1);

	}

	// Libero recursos
	free(copia0);
	free(copia1);

	// Libero el archivo de detalle
	config_destroy(archivo);

	return true;

}

int recuperarNumeroBloqueLibre(char* nodo, int cantidadBloques) {

	// Variables
	int i = 0;
	int bloqueLibre = -1;
	int bufferBitmap[cantidadBloques];
	char* rutaBitmap = malloc(100);

	// Armo la ruta del bitmap que me piden
	strcpy(rutaBitmap, "metadata/bitmaps/");
	strcat(rutaBitmap, nodo);
	strcat(rutaBitmap, ".dat");

	// Abro el archivo bitmap de bloques del nodo indicado
	FILE* bitmap = fopen(rutaBitmap, "r");

	// Leo el contenido del bitmap en un buffer
	fread(bufferBitmap, sizeof(int), cantidadBloques, bitmap);

	// Loopeo hasta encontrar un bloque libre
	for (i = 0; i < cantidadBloques; i++) {
		if (bufferBitmap[i] == 0) {
			bloqueLibre = i;
			log_info(archivoLog, "Se ha encontrado el bloque libre %d en el nodo %s.", bloqueLibre, nodo);
			break;
		}
	}

	free(rutaBitmap);
	return bloqueLibre;

}

int recuperarCantidadBloques(char* nodo) {

	// Variables
	int cantidadBloques = -1;
	char* nodoTotal = string_new();

	// Armo la key que tengo que ir a buscar
	string_append(&nodoTotal, nodo);
	string_append(&nodoTotal, "Total");

	// Abro la tabla de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Me quedo con la lista de Nodos de la tabla de nodos
	cantidadBloques = config_get_int_value(tablaNodos, nodoTotal);

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	// Libero recursos
	free(nodoTotal);

	return cantidadBloques;
}

int buscarIndiceNodo(char* nodo, int cantidadNodos) {

	// Variables
	int i = 0;
	t_datanode* nodoTemporal = NULL;

	for (i = 0; i < cantidadNodos; i++) {
		nodoTemporal = list_get(listaDataNodes, i);
		if ((strcmp(nodoTemporal->nombreNodo, nodo)) == 0) {
			break;
		}
	}

	return i;

}

int buscarIndiceNodoTablaNodos(char* nodo, int cantidadNodos) {

	// Variables
	int i = 0; // Indice a recuperar
	int j = 0; // Para liberar la lista de nodos

	// Abro la tabla de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Recupero la lista de nodos
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Mientras no esté vacía, leo el nombre del nodo y voy verificando si existe su bitmap
	for (i = 0; i < cantidadNodos; ++i) {
		if ((strcmp(listaNodos[i], nodo)) == 0) break;
	}

	// Libero recursos
	j = 0;
	while(listaNodos[j] != NULL) {
		free(listaNodos[j]);
		j++;
	}
	free(listaNodos);

	// Si el i es mayor, no lo encontre
	if (i >= cantidadNodos) {
		log_error(archivoLog, "No encontré el nodo %s en la Tabla de Nodos.", nodo);
		config_destroy(tablaNodos);
		return -1;
	} else {
		log_info(archivoLog, "El nodo %s ha sido encontrado en la Tabla de nodos con el indice %d", nodo, i);
		config_destroy(tablaNodos);
		return i;
	}
}

int mostrarInformacionNodo(char* nombreNodo) {

	// Tabla de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Armo las entradas para obtener los valores del nodo
	char* entradaLibre = malloc(strlen(nombreNodo) + strlen("Libre") + 1);
	strcpy(entradaLibre, nombreNodo);
	strcat(entradaLibre, "Libre");

	char* entradaTotal = malloc(strlen(nombreNodo) + strlen("Total") + 1);
	strcpy(entradaTotal, nombreNodo);
	strcat(entradaTotal, "Total");

	// Obtengo los valores del config asociados a ese nodo
	int espacioLibre = config_get_int_value(tablaNodos, entradaLibre);
	int espacioTotal = config_get_int_value(tablaNodos, entradaTotal);

	// Imprimo los valores
	printf("ESPACIO LIBRE %s: %d\n", nombreNodo, espacioLibre);
	printf("ESPACIO TOTAL %s: %d\n", nombreNodo, espacioTotal);

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	// Libero recursos
	free(entradaLibre);
	free(entradaTotal);

	return true;

}

int obtenerNodoYBloqueDisponible(char* nodoDisponible, int indiceUltimoNodoUsado) {

	// Variables
	int i = 0;
	int espacioDisponibleNodo = 0;
	int mayorEspacioDisponible = 0;
	int bloqueDisponible = -1;
	int indiceAleatorio = -1;
	int nodoEncontrado = false;
	t_datanode* nodoTemporal = NULL;

	// Cantidad y lista de Nodos
	int cantidadNodos = list_size(listaDataNodes);

	// Si el indiceUltimoNodoUsado es igual a cantidadNodos - 1 quiere decir que el último nodo de la lista es el que usé.
	// En ese caso, le muevo 0 a indiceUltimoNodoUsado ya que debo volver a empezar.
	if (indiceUltimoNodoUsado == (cantidadNodos - 1)) {
		indiceUltimoNodoUsado = 0;
	}

	// Abro la tabla de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Si indiceUltimoNodoUsado = -1, entonces le mando al que maś espacio tiene.
	if (indiceUltimoNodoUsado == -1) {
		// Loopeo hasta quedarme con el nodo que tiene más espacio disponible
		for (i = 0; i < cantidadNodos; ++i) {
			nodoTemporal = list_get(listaDataNodes, i);
			espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
			if (espacioDisponibleNodo > 0) {
				if (espacioDisponibleNodo > mayorEspacioDisponible) {
					mayorEspacioDisponible = espacioDisponibleNodo;
					strcpy(nodoDisponible, nodoTemporal->nombreNodo);
					nodoEncontrado = true;
				}
			}
		}
	} else {
		// Intento mandar a un nodo aleatorio
		do {
			indiceAleatorio = rand() % cantidadNodos;
		} while (indiceAleatorio == indiceUltimoNodoUsado);
		nodoTemporal = list_get(listaDataNodes, indiceAleatorio);
		espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
		if (espacioDisponibleNodo > 0) {
			strcpy(nodoDisponible, nodoTemporal->nombreNodo);
			nodoEncontrado = true;
		} else {
			// Me fijo si el siguiente nodo al último usado tiene espacio. Si tiene lo elijo, si no, sigo buscando.
			nodoTemporal = list_get(listaDataNodes, indiceUltimoNodoUsado+1);
			espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
			if (espacioDisponibleNodo > 0) {
				strcpy(nodoDisponible, nodoTemporal->nombreNodo);
				nodoEncontrado = true;
			} else {
				// Loopeo hasta encontrar un nodo que tenga espacio, distinto del ultimo nodo usado.
				for (i = 0; i < cantidadNodos; ++i) {
					if (i != indiceUltimoNodoUsado) {
						nodoTemporal = list_get(listaDataNodes, i);
						espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
						if (espacioDisponibleNodo > 0) {
							strcpy(nodoDisponible, nodoTemporal->nombreNodo);
							nodoEncontrado = true;
							break;
						}
					}
				}
			}
		}
	}

	// No encontré ningún nodo para almacenar el bloque
	if (nodoEncontrado == false) {
		// En este caso, me fijo si el ultimoNodoUsado tiene espacio. Si no tiene, doy error.
		// Si tiene, no me queda otra que volver a usar este nodo.
		nodoTemporal = list_get(listaDataNodes, indiceUltimoNodoUsado);
		espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
		if (espacioDisponibleNodo > 0) {
			strcpy(nodoDisponible, nodoTemporal->nombreNodo);
			char* total = malloc(strlen(nodoDisponible) + strlen("Total") + 1);
			strcpy(total, nodoDisponible);
			strcat(total, "Total");
			int cantidadBloquesNodo = config_get_int_value(tablaNodos, total);
			bloqueDisponible = recuperarNumeroBloqueLibre(nodoDisponible, cantidadBloquesNodo);
			free(total);
		} else {
			log_error(archivoLog, "No hay espacio disponible para almacenar el bloque en ningún nodo.");
			return -1;
		}
	} else {
		char* total = malloc(strlen(nodoDisponible) + strlen("Total") + 1);
		strcpy(total, nodoDisponible);
		strcat(total, "Total");
		int cantidadBloquesNodo = config_get_int_value(tablaNodos, total);
		bloqueDisponible = recuperarNumeroBloqueLibre(nodoDisponible, cantidadBloquesNodo);
		free(total);
	}

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	return bloqueDisponible;
}

int obtenerNodoCopiaYBloqueDisponible(char* nodoDisponible, char* nodoCopia, int indiceUltimoNodoCopiaUsado) {

	// Variables
	int i = 0;
	int espacioDisponibleNodo = 0;
	int mayorEspacioDisponible = 0;
	int bloqueDisponible = -1;
	int indiceAleatorio = -1;
	int nodoEncontrado = false;
	t_datanode* nodoTemporal = NULL;

	// Cantidad y lista de Nodos
	int cantidadNodos = list_size(listaDataNodes);

	// Abro la tablaNodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Busco el indice del nodo que tengo que omitir (porque tiene la COPIA_0)
	int indiceNodoOmitido = buscarIndiceNodo(nodoDisponible, cantidadNodos);

	// Si indiceUltimoNodoCopiaUsado es -1, es la primera vez. Elijo el que más espacio tiene
	if (indiceUltimoNodoCopiaUsado == -1) {
		// Loopeo hasta quedarme con el nodo que tiene más espacio disponible
		for (i = 0; i < cantidadNodos; ++i) {
			if (i != indiceNodoOmitido) {
				nodoTemporal = list_get(listaDataNodes, i);
				espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
				if (espacioDisponibleNodo > 0) {
					if (espacioDisponibleNodo > mayorEspacioDisponible) {
						mayorEspacioDisponible = espacioDisponibleNodo;
						strcpy(nodoCopia, nodoTemporal->nombreNodo);
						nodoEncontrado = true;
					}
				}
			}
		}
	} else {
		// Busco un nodo aleatorio dentro de las opciones que me quedan
		if (cantidadNodos == 2) {
			do {
				indiceAleatorio = rand() % cantidadNodos;
			} while (indiceAleatorio == indiceNodoOmitido);
		} else {
			do {
				indiceAleatorio = rand() % cantidadNodos;
			} while ((indiceAleatorio == indiceNodoOmitido) || (indiceAleatorio == indiceUltimoNodoCopiaUsado));
		}

		nodoTemporal = list_get(listaDataNodes, indiceAleatorio);
		espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);

		if (espacioDisponibleNodo > 0) {
			strcpy(nodoCopia, nodoTemporal->nombreNodo);
			nodoEncontrado = true;
		} else {
			// Loopeo hasta encontrar un nodo distintos de todos los que ya probé
			for (i = 0; i < cantidadNodos; ++i) {
				if ((i != indiceNodoOmitido) && (i != indiceUltimoNodoCopiaUsado) && (i != indiceAleatorio)) {
					nodoTemporal = list_get(listaDataNodes, i);
					espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
					if (espacioDisponibleNodo > 0) {
						strcpy(nodoCopia, nodoTemporal->nombreNodo);
						nodoEncontrado = true;
						break;
					}
				}
			}
		}
	}

	// No encontré ningún nodo para almacenar el bloque
	if (nodoEncontrado == false) {
		// Si no encontré ninguno, no me queda otra que mandarle al que le mandé la última vez
		nodoTemporal = list_get(listaDataNodes, indiceUltimoNodoCopiaUsado);
		espacioDisponibleNodo = obtenerEspacioDisponibleNodo(nodoTemporal->nombreNodo, tablaNodos);
		if (espacioDisponibleNodo > 0) {
			strcpy(nodoCopia, nodoTemporal->nombreNodo);
			char* total = malloc(strlen(nodoCopia) + strlen("Total") + 1);
			strcpy(total, nodoCopia);
			strcat(total, "Total");
			int cantidadBloquesNodo = config_get_int_value(tablaNodos, total);
			bloqueDisponible = recuperarNumeroBloqueLibre(nodoCopia, cantidadBloquesNodo);
			free(total);
		} else {
			log_error(archivoLog, "No hay espacio disponible para almacenar el bloque en ningún nodo.");
			return -1;
		}
	} else {
		char* total = malloc(strlen(nodoCopia) + strlen("Total") + 1);
		strcpy(total, nodoCopia);
		strcat(total, "Total");
		int cantidadBloquesNodo = config_get_int_value(tablaNodos, total);
		bloqueDisponible = recuperarNumeroBloqueLibre(nodoCopia, cantidadBloquesNodo);
		free(total);
	}

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	return bloqueDisponible;

}

int buscarSocketEn(t_list* lista, int socket, char* nombreNodo) {

	// Variables
	int socketEncontrado = false;
	int i = 0;
	int sizeLista = list_size(lista);
	t_datanode* dataNode = NULL;

	// Loopeo en la lista
	for (i = 0; i < sizeLista; ++i) {
		dataNode = list_get(lista, i);
		if (dataNode->socket_client == socket) {
			socketEncontrado = dataNode->socket_client;
			strcpy(nombreNodo, dataNode->nombreNodo);
		}
	}

	// Si lo encontré, lo seteo
	if (socketEncontrado > 0) {
		log_trace(archivoLog, "El socket %d pertenece al DataNode %s en la lista de Clientes.", dataNode->socket_client, dataNode->nombreNodo);
	}

	return socketEncontrado;

}

int formatearTablaNodos() {
	// Variables
	int i;

	// Obtengo la lista de nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Obtengo la cantidad de nodos
	int cantidadNodos = sizeLista(listaNodos);

	// Si la cantidad es mayor a 0, formateo los Bitmap de Bloques de cada Nodo.
	if (cantidadNodos > 0) {
		for (i = 0; i < cantidadNodos; ++i) {
			if (formatearBitmapBloques(listaNodos[i]))
				log_trace(archivoLog, "El Bitmap de Bloques del Nodo %s ha sido formateado  correctamente.\n", listaNodos[i]);
			else
				log_error(archivoLog, "Error al formatear el Bitmap de Bloques del Nodo %s.\n", listaNodos[i]);
		}
	} else {
		log_info(archivoLog, "La lista de Nodos se encuentra vacía.\n");
	}

	// Libero la tabla de nodos
	config_destroy(tablaNodos);
	// Vuelvo a crear la Tabla de Nodos
	crearTablaNodos();
	return true;
}

int crearTablaNodos() {

	FILE* tablaNodos=fopen("metadata/nodos.bin","w+");

	if (tablaNodos == NULL) {
		log_error(archivoLog, "Error al crear la Tabla de Nodos.\n");
		return false;
	} else {
		txt_write_in_file(tablaNodos, "TAMANIO=0\n");
		txt_write_in_file(tablaNodos, "LIBRE=0\n");
		txt_write_in_file(tablaNodos, "NODOS=[]\n");
    	log_info(archivoLog, "La Tabla de Nodos se ha creado formateada.");
    	fclose(tablaNodos);
		return true;
	}
}

int eliminarNodoTablaNodos(char* nombreNodo) {

	// Variables
	int i = 0;
	char* nuevaListaNodos = NULL;

	// Recupero la cantidad de Nodos en la Tabla de Nodos
	int cantidadNodos = recuperarCantidadNodos();

	// Abro la tabla de Nodos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Armo entrada para recuperar el tamaño del nodo que se me desconecta
	char* entradaBloquesTotalesNodo = malloc(strlen(nombreNodo) + strlen("Total") + 1);
	strcpy(entradaBloquesTotalesNodo, nombreNodo);
	strcat(entradaBloquesTotalesNodo, "Total");

	// Armo entrada para borrar el total del nodo
	char* entradaBloquesLibresNodo = malloc(strlen(nombreNodo) + strlen("Libre") + 1);
	strcpy(entradaBloquesLibresNodo, nombreNodo);
	strcat(entradaBloquesLibresNodo, "Libre");

	// Recupero variables de la Tabla de Nodos
	int sizeFileSystem = config_get_int_value(tablaNodos, "TAMANIO");
	int sizeLibreFileSystem = config_get_int_value(tablaNodos, "LIBRE");
	int sizeNodo = config_get_int_value(tablaNodos, entradaBloquesTotalesNodo);
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Recalculo los nuevos valores
	int nuevoSizeFileSystem = sizeFileSystem - sizeNodo;
	int nuevoSizeLibreFileSystem = sizeLibreFileSystem - sizeNodo;

	// Convierto los valores de int a string
	char* nuevoSizeFileSystemString = string_itoa(nuevoSizeFileSystem);
	char* nuevoSizeLibreFileSystemString = string_itoa(nuevoSizeLibreFileSystem);

	// Recupero el indice de este nodo
	int indiceNodo = buscarIndiceNodoTablaNodos(nombreNodo, cantidadNodos);

	// Recorro la listaNodos, concatenandola para formar la nueva, siempre y cuando tenga más de un nodo
	if (cantidadNodos == 1) {
		// Si es el ultimo nodo el que me queda, entonces la nueva lista es "[]"
		nuevaListaNodos = malloc(strlen("[]") + 1);
		strcpy(nuevaListaNodos, "[]");
	} else {
		nuevaListaNodos = malloc(strlen("[]") + (strlen(", ") * cantidadNodos) + (strlen("NODOXX") * cantidadNodos) + 1);
		strcpy(nuevaListaNodos, "[");
		for (i = 0; i < cantidadNodos; ++i) {
			if (i != indiceNodo) {
				strcat(nuevaListaNodos, listaNodos[i]);
				if (i != (cantidadNodos - 1))
					strcat(nuevaListaNodos, ", ");
			}
		}
		// Cierro la nueva lista de nodos
		strcat(nuevaListaNodos, "]");
	}

	// Verifico si estoy en el ultimo nodo ya que me queda un ", " de mas
	if (cantidadNodos > 1) {
		if (indiceNodo == (cantidadNodos - 1)) {
			int sizeLista = strlen(nuevaListaNodos);
			nuevaListaNodos[sizeLista-3] = ']';
			nuevaListaNodos[sizeLista-2] = '\0';
		}
	}

	// Seteo los nuevos valores de la tabla de nodos
	config_set_value(tablaNodos, "NODOS", nuevaListaNodos);
	config_set_value(tablaNodos, "TAMANIO", nuevoSizeFileSystemString);
	config_set_value(tablaNodos, "LIBRE", nuevoSizeLibreFileSystemString);

	// Libero los recursos que ya use
	free(nuevoSizeFileSystemString);
	free(nuevoSizeLibreFileSystemString);

	// Elimino las key asociadas a ese Nodo
	config_remove_key(tablaNodos, entradaBloquesTotalesNodo);
	config_remove_key(tablaNodos, entradaBloquesLibresNodo);

	// Guardo la Tabla de Nodos
	config_save(tablaNodos);

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	// Libero recursos
	free(entradaBloquesTotalesNodo);
	free(entradaBloquesLibresNodo);
	free(nuevaListaNodos);

	// Libero la lista de nodos
	i = 0;
	while(listaNodos[i] != NULL) {
		free(listaNodos[i]);
		i++;
	}
	free(listaNodos);

	return true;

}

int eliminarDataNode(int socket, char* nombreNodo) {

	// Variables
	int i = 0;
	int sizeLista = list_size(listaDataNodes);

	// Loopeo en la lista y si lo encuentro lo elimino
	for (i = 0; i < sizeLista; ++i) {
		t_datanode* dataNode = list_get(listaDataNodes, i);
		if (dataNode->socket_client == socket) {
			// Lo elimino de la lista de nodos conectados y libero sus recursos
			list_remove_and_destroy_element(listaDataNodes, i, free);
			log_info(archivoLog, "El nodo %s ha sido eliminado correctamente de la lista de Nodos conectados.", nombreNodo);
			break;
		}
	}

	// Si vengo de un estado anterior no elimino la información del nodo
	if (estadoCargado == true || estadoSeguro == true) {
		log_info(archivoLog, "No se elimina la información del nodo %s de la Tabla de Nodos ya que se permite su reconexión.", nombreNodo);
	} else {
		// Elimino el Nodo de la Tabla de Nodos
		eliminarNodoTablaNodos(nombreNodo);
	}

	return true;

}

int verificarBitmap(char* nodo) {

	// Variables
	int bitmapOK = true;

	// Armo la ruta del bitmap del nodo dado
	char* rutaBitmapBloques = malloc(strlen("metadata/bitmaps/") + strlen(nodo) + strlen(".dat") + 1);
	strcpy(rutaBitmapBloques, "metadata/bitmaps/");
	strcat(rutaBitmapBloques, nodo);
	strcat(rutaBitmapBloques, ".dat");

	// Abro el archivo de bitmap del nodo
	FILE* bitmapNodo = fopen(rutaBitmapBloques, "r");

	// Si me da NULL es porque no existe o está corrupta
	if (bitmapNodo == NULL) {
		bitmapOK = false;
		log_error(archivoLog, "No existe el bitmap para el nodo %s.", nodo);
	}

	// Libero recursos
	free(rutaBitmapBloques);

	return bitmapOK;

}

// Esta funcion retorna TRUE si existen todos los bitmaps de todos los nodos en la lista
int verificarBitmapsEstadoAnterior() {

	// Variables
	int estadoOK = true;
	int i = 0;
	int existeBitmap = false;

	// Recupero la lista de Nodos
	int cantidadNodos = recuperarCantidadNodos();
	t_config* tablaNodos = config_create("metadata/nodos.bin");
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Mientras no esté vacía, leo el nombre del nodo y voy verificando si existe su bitmap
	for (i = 0; i < cantidadNodos; ++i) {
		existeBitmap = verificarBitmap(listaNodos[i]);
		if (!existeBitmap) {
			estadoOK = false;
		}
	}

	// Libero la tabla de nodos
	config_destroy(tablaNodos);
	i = 0;
	while(listaNodos[i] != NULL) {
		free(listaNodos[i]);
		i++;
	}
	free(listaNodos);

	return estadoOK;

}

int agregarNodo(char* nombreNodo, char** listaNodos) {

	// Variables
	int i = 0;
	int cantidadNodos = 0;

	/* Recorro hasta el final de la lista, donde es NULL
	*  En esa posicion, grabo el nuevo nodo.
	*/

	while(listaNodos[i] != NULL) {
		i++;
	}

	listaNodos[i] = malloc(10);
	strcpy(listaNodos[i], nombreNodo);

	cantidadNodos = i+1;

	return cantidadNodos;

}

int buscarNodo(char* nombreNodo) {
	// Variables
	int nodoEncontrado = false;
	int i = 0;
	int cantidadNodos = 0;

	// Recorro la lista de nodos hasta encontrarlo
	for (i = 0; i < cantidadNodos; ++i) {

		nodoEncontrado = true;
	}

	return nodoEncontrado;
}

int actualizarEspacioLibreNodo(char* nombreNodo, int sumarRestar) {

	// Esta funcion suma o resta un bloque a los bloques libres del nodo y al total del FS
	// Si sumarRestar = 1 (SUMA)
	// Si sumarRestar = -1 (RESTA)

	// Apertura de archivos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Recupero variables de la Tabla de Nodos
	int sizeLibreFileSystem = config_get_int_value(tablaNodos, "LIBRE");

	// Armo el detalle de bloques libres del nodo
	char* lineaLibre = malloc(strlen(nombreNodo) + strlen("Libre") + 1);
	strcpy(lineaLibre, nombreNodo);
	strcat(lineaLibre, "Libre");

	// Recupero el espacio libre del nodo
	int espacioLibreNodo = config_get_int_value(tablaNodos, lineaLibre);

	// Recalculo el nuevo Size Libre del FS
	int nuevoSizeLibre = sizeLibreFileSystem + sumarRestar;

	// Recalculo el nuevo Size Libre del nodo
	int nuevoEspacioNodo = espacioLibreNodo + sumarRestar;

	// Paso los valores a string
	char* nuevoSizeLibreString = string_itoa(nuevoSizeLibre);
	char* nuevoEspacioNodoString = string_itoa(nuevoEspacioNodo);

	// Actualizo la tabla con los nuevos valores
	config_set_value(tablaNodos, "LIBRE", nuevoSizeLibreString);
	config_set_value(tablaNodos, lineaLibre, nuevoEspacioNodoString);

	// Grabo los datos actualizados
	config_save(tablaNodos);

	// Libero recursos
	free(lineaLibre);
	free(nuevoSizeLibreString);
	free(nuevoEspacioNodoString);

	// Libero el config
	config_destroy(tablaNodos);


	return true;
}

int actualizarTablaNodos(char* nombreNodo, int totalBloques, int bloquesDisponibles) {

	// Variables
	int i = 0; // Para loopear

	// Apertura de archivos
	t_config* tablaNodos = config_create("metadata/nodos.bin");

	// Recupero variables de la Tabla de Nodos
	int sizeFileSystem = config_get_int_value(tablaNodos, "TAMANIO");
	int sizeLibreFileSystem = config_get_int_value(tablaNodos, "LIBRE");
	char** listaNodos = config_get_array_value(tablaNodos, "NODOS");

	// Recalculo valores
	int nuevoSize = sizeFileSystem + totalBloques;
	int nuevoSizeLibre = sizeLibreFileSystem + bloquesDisponibles;

	// Paso los valores a formato string
	char* nuevoSizeString = string_itoa(nuevoSize);
	char* nuevoSizeLibreString = string_itoa(nuevoSizeLibre);
	char* totalBloquesString = string_itoa(totalBloques);
	char* bloquesDisponiblesString = string_itoa(bloquesDisponibles);

	// Actualizo la tabla
	config_set_value(tablaNodos, "TAMANIO", nuevoSizeString);
	config_set_value(tablaNodos, "LIBRE", nuevoSizeLibreString);

	// Libero lo que ya use
	free(nuevoSizeString);
	free(nuevoSizeLibreString);

	// Agrego el nodo al final de la lista
	int cantidadNodos = agregarNodo(nombreNodo, listaNodos);

	// Genero la nuevaListaNodos
	char* nuevaListaNodos = malloc(strlen("[") + (strlen("NODOXX") * cantidadNodos) + (strlen(", ") * cantidadNodos) + strlen("]") + 1);
	strcpy(nuevaListaNodos, "[");

	// Recorro la listaNodos, concatenandola para formar la nueva
	for (i = 0; i < cantidadNodos; ++i) {
		strcat(nuevaListaNodos, listaNodos[i]);
		if (i != (cantidadNodos - 1))
			strcat(nuevaListaNodos, ", ");
	}

	// Cierro la nuevaListaNodos
	strcat(nuevaListaNodos, "]");

	// Seteo el nuevo valor de la nueva lista de nodos en la tabla de nodos
	config_set_value(tablaNodos, "NODOS", nuevaListaNodos);

	// Grabo la tabla
	config_save(tablaNodos);

	// Agrego el detalle de bloques totales del nodo
	char* lineaTotal = malloc(strlen("nombreNodo") + strlen("Total=") + strlen(totalBloquesString) + 1);
	strcpy(lineaTotal, nombreNodo);
	strcat(lineaTotal, "Total=");
	strcat(lineaTotal, totalBloquesString);

	// Agrego el detalle de bloques libres del nodo
	char* lineaLibre = malloc(strlen(nombreNodo) + strlen("Libre=") + strlen(bloquesDisponiblesString) + 1);
	strcpy(lineaLibre, nombreNodo);
	strcat(lineaLibre, "Libre=");
	strcat(lineaLibre, bloquesDisponiblesString);

	// Escribo en la tabla de nodos
	FILE* tablaNodosFILE = fopen("metadata/nodos.bin", "r+");

	fseek(tablaNodosFILE, 0, SEEK_END);

	txt_write_in_file(tablaNodosFILE, lineaTotal);
	txt_write_in_file(tablaNodosFILE, "\n");
	txt_write_in_file(tablaNodosFILE, lineaLibre);

	// Cierro archivos
	fclose(tablaNodosFILE);

	// Libero la tabla de nodos
	config_destroy(tablaNodos);

	// Libero recursos
	i = 0;
	while(listaNodos[i] != NULL) {
		free(listaNodos[i]);
		i++;
		if (i > (cantidadNodos - 1)) break;
	}
	free(listaNodos);
	free(bloquesDisponiblesString);
	free(totalBloquesString);
	free(lineaLibre);
	free(lineaTotal);
	free(nuevaListaNodos);

	return true;

}

int actualizarIPYPuerto(t_list* lista, int socket, char* IP, int puerto) {

	// Variables
	int i = 0;
	int sizeLista = list_size(lista);

	// Loopeo en la lista
	for (i = 0; i < sizeLista; ++i) {
		 t_datanode* dataNode = list_get(lista, i);
		if (dataNode->socket_client == socket) {
			strcpy(dataNode->ipNodo, IP);
			dataNode->puertoEscuchaMaster = puerto;
		}
	}

	return true;

}

int actualizarNombreYPuertoNodo(t_list* lista, char* nombreNodo, int puerto, int socket, char* ipNodo) {

	// Variables
	int i = 0;
	int sizeLista = list_size(lista);

	// Loopeo en la lista
	for (i = 0; i < sizeLista; ++i) {
		 t_datanode* dataNode = list_get(lista, i);
		if (dataNode->socket_client == socket) {
			strcpy(dataNode->nombreNodo, nombreNodo);
			strcpy(dataNode->ipNodo, ipNodo);
			dataNode->puertoEscuchaMaster = puerto;
		}
	}

	return true;

}

/*  Esta función recupera en "nodo" el nombre del dataNode que contiene el "numeroBloque"
 *  y la copia indicada en "numeroCopia". También deja en "bloqueDataBin" el numero de bloque en ese nodo.
 *  Devuelve true si el bloque existe en algún nodo y false si no existe en ninguno.
 */
int pedirBloque(int bloqueDataBin, int socket) {

	// Convierto el bloqueDataBin a string para poder mandarlo
	char* bloqueDataBinString = string_itoa(bloqueDataBin);

    // Envío el mensaje al nodo "nodo" de que solicito el bloque "bloqueDataBin"
    enviar_mensaje(GET_BLOQUE, bloqueDataBinString, socket);

    // Imprimo por pantalla para que parezca que estamos cargando el bloque
    log_info(archivoLog, "Procesando petición de bloque, por favor aguarde...");

    // Espero la respuesta del GET_BLOQUE, que llega a través del hilo principal del FS
    sleep(retardoOKBloques);
    int bloqueOK = sem_trywait(&get_bloque_OK);
    int bloqueNOOK = sem_trywait(&get_bloque_NO_OK);

    if (bloqueOK == 0) {
    	log_info(archivoLog, "Se recibió el mensaje de GET BLOQUE OK.");
    	free(bloqueDataBinString);
    	return true;
    } else {
    	if (bloqueNOOK == 0) {
    		log_error(archivoLog, "Se recibió el mensaje de GET BLOQUE NO OK.");
    	} else {
    		log_error(archivoLog, "No se recibió ni GET BLOQUE OK ni GET BLOQUE NO OK.");
    	}
    	free(bloqueDataBinString);
		return false;
    }
}

/*  Esta función recupera en "nodo" el nombre del dataNode que contiene el "numeroBloque"
 *  y la copia indicada en "numeroCopia". También deja en "bloqueDataBin" el numero de bloque en ese nodo.
 *  Devuelve true si el bloque existe en algún nodo y false si no existe en ninguno.
 */
int recuperarInfoBloque(t_config* archivo, int numeroBloque, int numeroCopia, char* nodo, int* bloqueDataBin, int* bytesOcupados) {

	// Variables
	int i = 0; // Para liberar los datos del bloque
	// Paso los valores numericos a String
	char* numeroBloqueString = string_itoa(numeroBloque);
	char* numeroCopiaString = string_itoa(numeroCopia);

    // Armo la entrada genérica para pedir los bloques
    char* entradaDatos = malloc(strlen("BLOQUE") + strlen(numeroBloqueString) + strlen("COPIA") + strlen(numeroCopiaString) + 1);
    strcpy(entradaDatos, "BLOQUE");
    strcat(entradaDatos, numeroBloqueString);
    strcat(entradaDatos, "COPIA");
    strcat(entradaDatos, numeroCopiaString);

	// Primero me fijo si existe la entrada para ese bloque
	// Si no existe, retorno false y salgo

    if (!config_has_property(archivo, entradaDatos)) {
    	log_error(archivoLog, "No existe la copia %d para el bloque %d.", numeroCopia, numeroBloque);
    	free(numeroBloqueString);
    	free(numeroCopiaString);
    	free(entradaDatos);
    	return false;
    }

    // Armo la entrada para pedir los bytes de ese bloque
    char* entradaBytes = malloc(strlen("BLOQUE") + strlen(numeroBloqueString) + strlen("BYTES") + 1);
    strcpy(entradaBytes, "BLOQUE");
    strcat(entradaBytes, numeroBloqueString);
    strcat(entradaBytes, "BYTES");

    // Recupero la data
    char** datosBloque = config_get_array_value(archivo, entradaDatos);

    if (datosBloque != NULL) {
    	strcpy(nodo, datosBloque[0]);
    	(*bloqueDataBin) = atoi(datosBloque[1]);
    	(*bytesOcupados) = config_get_int_value(archivo, entradaBytes);
    	// Libero los datos que pedi del bloque
    	i = 0;
    	while(datosBloque[i] != NULL){
    		free(datosBloque[i]);
    		i++;
    	}
    	free(datosBloque);
    	free(numeroBloqueString);
    	free(numeroCopiaString);
    	free(entradaDatos);
    	free(entradaBytes);
    	return true;
    } else {
    	log_error(archivoLog, "La copia %d del bloque %d no existe.", numeroCopia, numeroBloque);
    	// Libero los datos que pedi del bloque
    	i = 0;
    	while(datosBloque[i] != NULL){
    		free(datosBloque[i]);
    		i++;
    	}
    	free(datosBloque);
    	free(numeroBloqueString);
    	free(numeroCopiaString);
    	free(entradaDatos);
    	free(entradaBytes);
    	return false;
    }
}

int actualizarBitmapBloques(char* nombreNodo, int bloqueDisponible, int ocupoBloque) {

	// Obtener cantidad de bloques de ese nodo
	int cantidadBloques = recuperarCantidadBloques(nombreNodo);

	// Variables
	int bitmap[cantidadBloques];

    // Armo la ruta de bitmap de ese nodo
    char* rutaBitmap = malloc(strlen("metadata/bitmaps/") + strlen(nombreNodo) + strlen(".dat") + 1);
    strcpy(rutaBitmap, "metadata/bitmaps/");
    strcat(rutaBitmap, nombreNodo);
    strcat(rutaBitmap, ".dat");

    // Abro archivo bitmap de bloques
    FILE* bitmapBloquesAnterior = fopen(rutaBitmap, "r");

    // Leo archivo bitmap de bloques anterior
    fread(bitmap, sizeof(int), cantidadBloques, bitmapBloquesAnterior);

    // Lo cierro
    fclose(bitmapBloquesAnterior);

    // Actualizo el bloque que grabé o liberé
    if (ocupoBloque) {
    	bitmap[bloqueDisponible] = 1;
    } else {
    	bitmap[bloqueDisponible] = 0;
    }

    // Vuelvo a grabar el contenido del bitmap encima del ya existente
    FILE* bitmapBloquesNuevo = fopen(rutaBitmap, "w+");

    // Escribo el bitmap nuevamente
    fwrite(bitmap, sizeof(int), cantidadBloques, bitmapBloquesNuevo);

    // Lo cierro
    fclose(bitmapBloquesNuevo);

    // Libero la ruta del archivo
    free(rutaBitmap);

	return true;

}

int generarBitMapBloquesVacio(char* nombreNodo, int cantidadBloques) {

	/* El bitmap de bloques será un array de int, donde cada int indica si el bloque está
	 * vacío (0) o contiene datos (1).
	 *
	 */

	// Variables
	int bitmap[cantidadBloques];
	int i = 0;

	// Inicializo todos los bloques en 0
	for (i = 0; i < cantidadBloques; i++) {
		bitmap[i] = 0;
	}

    // Armo la ruta de bitmap de ese nodo
    char* rutaBitmap = malloc(100);
    strcpy(rutaBitmap, "metadata/bitmaps/");
    strcat(rutaBitmap, nombreNodo);
    strcat(rutaBitmap, ".dat");

    // Creo archivo bitmap de bloques
    FILE* bitmapBloques = fopen(rutaBitmap, "w+");

    // Escribo el contenido
    fwrite(bitmap, sizeof(bitmap), 1, bitmapBloques);

	// Libero recursos
	free(rutaBitmap);
	fclose(bitmapBloques);

	return true;

}

int verificarNodoEstadoSeguro(char* nombreNodo) {

	// Variables
	int i = 0;
	int esNodoEstadoSeguro = false;

	// Recuperar cantidad de nodos del estado anterior
	int cantidadNodos = list_size(listaNodosEstadoSeguro);

	// Loopeo preguntando si el nodo que se me conectó se encuentra en la lista
	for (i = 0; i < cantidadNodos; i++) {
		char* nodo = list_get(listaNodosEstadoSeguro, i);
		if (strcmp(nodo, nombreNodo) == 0) {
			esNodoEstadoSeguro = true;
			break;
		}
	}

	return esNodoEstadoSeguro;

}

int verificarNodoEstadoAnterior(char* nombreNodo) {

	// Variables
	int i = 0;
	int esNodoEstadoAnterior = false;

	// Recuperar cantidad de nodos del estado anterior
	int cantidadNodos = list_size(listaNodosEstadoAnterior);

	// Loopeo preguntando si el nodo que se me conectó se encuentra en la lista
	for (i = 0; i < cantidadNodos; i++) {
		char* nodo = list_get(listaNodosEstadoAnterior, i);
		if (strcmp(nodo, nombreNodo) == 0) {
			esNodoEstadoAnterior = true;
			break;
		}
	}

	return esNodoEstadoAnterior;

}

int sizeLista(char** array) {
	int i = 0;
	// Mientras tenga elementos en la lista, voy sumando 1.
	while (array[i] != NULL) {
		i = i + 1;
	}
	return i;
}

int cantidadBloquesBitmap(char* rutaBitmap) {

	// Variables
	int cantidadBloques = 0;

	cantidadBloques = (tamanioArchivo(rutaBitmap) / sizeof(int));

	return cantidadBloques;

}

int formatearBitmapBloques(char* nodo) {
	char* rutaBitmapBloques = string_from_format("metadata/bitmaps/%s.dat", nodo);
	int cantidadBloques = cantidadBloquesBitmap(rutaBitmapBloques);
	if ((remove(rutaBitmapBloques)) == 0) {
		generarBitMapBloquesVacio(nodo, cantidadBloques);
		return true;
	} else {
		return false;
	}
}

