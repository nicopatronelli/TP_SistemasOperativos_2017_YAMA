/*
 * Directorios.c
 *
 *  Created on: 11/11/2017
 *      Author: utnso
 */

#include "Directorios.h"
#include "Globales.h"


int grabarDirectorio(char* nombreDirectorio, int indiceDirectorio, int padreDirectorio) {

	strcpy(arrayDirectorios[indiceDirectorio].nombre, nombreDirectorio);
	arrayDirectorios[indiceDirectorio].padre = padreDirectorio;
	arrayDirectorios[indiceDirectorio].index = indiceDirectorio;
	return true;

}

int crearCarpetaDirectorio(int indiceDirectorio) {

	// Armo la ruta del directorio que quiero crear
	char* ruta = string_new();
	char* indiceDirectorioString = string_itoa(indiceDirectorio);
	string_append(&ruta, "metadata/archivos/");
	string_append(&ruta, indiceDirectorioString);
	mkdir(ruta, S_IRWXU);

	// Libero la memoria
	free(ruta);
	free (indiceDirectorioString);

	return true;

}

int inicializarTablaDirectorios(int tablaDirectorios) {

	// Variables
	int indiceDirectorio = 0;

	// Mapeo el arrayDirectorios a memoria y lo consisto en archivo
	arrayDirectorios =  (struct t_directory*) mmap(NULL, sizeof(struct t_directory) * CANTIDAD_DIRECTORIOS, PROT_READ | PROT_WRITE, MAP_SHARED, tablaDirectorios, 0);

	if (arrayDirectorios == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
	}

	grabarDirectorio("base", indiceDirectorio, -1);
	crearCarpetaDirectorio(indiceDirectorio);

	for (indiceDirectorio = 1; indiceDirectorio < 100; ++indiceDirectorio) {
		grabarDirectorio("", indiceDirectorio, -1);
	}

	msync(arrayDirectorios, sizeof(struct t_directory) * CANTIDAD_DIRECTORIOS, MS_SYNC);

	log_info(archivoLog, "La Tabla de Directorios se ha creado formateada.");
	close(tablaDirectorios);
	return true;

}

int formatearTablaDirectorios() {
	remove("metadata/directorios.dat");
	crearTablaDirectorios();
	return true;
}

int buscarEspacioLibre() {

	// Variables
	int i = 1;
	int espacioLibre = -1;
	struct t_directory* directorioLeido = malloc(sizeof(t_directory));

	// Abro la Tabla de Directorios para buscar el directorio
	FILE* tablaDirectorios = fopen("metadata/directorios.dat", "r");

	// Descarto el primer directorio porque ahí no puedo grabar
	fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);

	// Lectura anticipada para entrar al while
	fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);

	// Loopeo hasta encontrar el primer directorio que tiene de padre al -1
	while(directorioLeido->padre != -1) {
		fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);
		i++;
		if (i > CANTIDAD_DIRECTORIOS - 1) break;
	}

	// Si iteré 100 veces es porque no encontré lugar
	if (i > CANTIDAD_DIRECTORIOS - 1) {
		log_error(archivoLog, "No se encontró espacio libre para guardar el directorio.");
	} else {
		// Si no, encontré lugar para guardar el directorio
		espacioLibre = directorioLeido->index;
	}

	// Libero recursos
	free(directorioLeido);

	return espacioLibre;

}

int buscarDirectorio(char* nombre) {

	/* Busca el directorio "nombre" en la Tabla de Directorios.
	 * Devuelve true si lo encuentra y false si no.
	 */

	// Variables
	int directorioEncontrado = false;
	int i = 0;

	// Loopeo hasta encontrar donde está ese directorio
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		if ((strcmp(arrayDirectorios[i].nombre, nombre)) == 0) {
			directorioEncontrado = true;
			break;
		}
	}

	return directorioEncontrado;

}

int crearDirectorio(char* nombre, int indiceActual) {

	// Variablaes
	int indiceNuevoDirectorio = -1;

	// Me fijo si ese nombre de directorio ya existe en el sistema
	// Si existe, no permito que se cree el directorio
	int directorioYaExiste = buscarDirectorio(nombre);

	if (directorioYaExiste) {
		log_error(archivoLog, "No se pudo crear el directorio '%s' porque ya existe uno con ese nombre.", nombre);
		return -1;
	}

	// Busco espacio libre para guardar el directorio
	indiceNuevoDirectorio = buscarEspacioLibre();

	// Si no lo encuentro, lo informo.
	if (indiceNuevoDirectorio == -1) {
		log_error(archivoLog, "No hay espacio para crear un nuevo directorio.");
	} else {
		// Grabo el directorio en el lugar que encontré
		grabarDirectorio(nombre, indiceNuevoDirectorio, indiceActual);
		crearCarpetaDirectorio(indiceNuevoDirectorio);
	}

	return indiceNuevoDirectorio;

}

int parsearNombreDirectorio(char * nombreDirectorio, char* nombreParseado) {
	/* Parseo la ruta completa con el nombre del directorio buscado "/".
	*  Me quedo con la anteúltima etiqueta, ya que la última es NULL.
	*/
	char** split = string_split(nombreDirectorio, "/");
	int i = 0;

	// Itero hasta encontrar el NULL
	while(split[i] != NULL)
		i++;

	// Copio el nombreDirectorio al nombreParseado
	strcpy(nombreParseado, split[i-1]);

	return true;

}

int buscarIndiceDirectorio(char* directorio) {

	// Variables
	int i = 0;

	// Loopeo hasta encontrar donde está ese directorio
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		if ((strcmp(arrayDirectorios[i].nombre, directorio)) == 0) {
			break;
		}
	}

	// Si i es mayor a 99 es porque no lo encontré
	if (i > CANTIDAD_DIRECTORIOS - 1) {
		return -1;
	} else {
		return i;
	}
}

int buscarIndiceDirectorioPadre(char* directorio) {

	// Variables
	int indiceDirectorioPadre = -1;
	int indiceDirectorio = 0;

	// Busco en qué lugar de la tabla está el directorio del que estoy buscando el padre
	indiceDirectorio = buscarIndiceDirectorio(directorio);

	// Si lo encontré, actualizo la info si no fallo
	if (indiceDirectorio != -1) {
		// Actualizo la informacion del directorio padre para esa entrada de la tabla
		indiceDirectorioPadre = arrayDirectorios[indiceDirectorio].padre;
		return indiceDirectorioPadre;
	} else {
		log_error(archivoLog, "El directorio %s no existe.", directorio);
		return -2;
	}
}

int contarDirectorios(char** etiquetas, int tieneNombreArchivo) {
	// Variables
	int i = 0;
	// Itero hasta llegar al NULL y se que donde termino -1 es la última etiqueta con datos
	while (etiquetas[i] != NULL) {
		i++;
	}

	// Pregunto si tiene archivo devuelvo el resultado -1
	if (tieneNombreArchivo) {
		return i-1;
	} else {
		return i;
	}
}

int validarRutaDirectoriosConNombreArchivo(char* rutaConNombreArchivo) {

	// Variables
	int indiceDirectorioActual = 0;
	int i = 0; // Para liberar
	int j = 0;
	int cantidadEtiquetasDirectorios = 0;
	int indiceDirectorioPadre = -1;
	char** directorios = string_split(rutaConNombreArchivo, "/");

	// Me guardo la cantidad de directorios que tengo que buscar
	cantidadEtiquetasDirectorios = contarDirectorios(directorios, 1);

	/* Voy buscando los padres de cada directorio (comenzando desde el ultimo),
	 * hasta llegar al directorio raiz. Si no llego nunca, termino porque no es valida la ruta
	 * Si llego, la ruta es valida
	 */

	j = cantidadEtiquetasDirectorios - 1;

	while ( j >= 0 ) {
		indiceDirectorioPadre = buscarIndiceDirectorioPadre(directorios[j]);
		if (indiceDirectorioPadre != -2) {
			if (j == cantidadEtiquetasDirectorios - 1) {
				indiceDirectorioActual = buscarIndiceDirectorio(directorios[j]);
			}
			j--;
		} else {
			// Libero recursos
			i = 0;
			while(directorios[i] != NULL) {
				free(directorios[i]);
				i++;
			}
			free(directorios);
			return -1;
		}
	}

	// Libero recursos
	i = 0;
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);

	return indiceDirectorioActual;

}

int validarRutaDirectorios(char* ruta) {

	// Variables
	int indiceDirectorioActual = 0;
	int j = 0;
	int cantidadEtiquetasDirectorios = 0;
	int indiceDirectorioPadre = -1;
	char** directorios = string_split(ruta, "/");

	// Me guardo la cantidad de directorios que tengo que buscar
	cantidadEtiquetasDirectorios = contarDirectorios(directorios, 0);

	/* Voy buscando los padres de cada directorio (comenzando desde el ultimo),
	 * hasta llegar al directorio raiz. Si no llego nunca, termino porque no es valida la ruta
	 * Si llego, la ruta es valida
	 */

	j = cantidadEtiquetasDirectorios - 1;

	while ( j >= 0 ) {
		indiceDirectorioPadre = buscarIndiceDirectorioPadre(directorios[j]);
		if (indiceDirectorioPadre != -2) {
			if (j == cantidadEtiquetasDirectorios - 1) {
				indiceDirectorioActual = buscarIndiceDirectorio(directorios[j]);
			}
			j--;
		} else {
			// Libero recursos
			j = 0;
			while(directorios[j] != NULL) {
				free(directorios[j]);
				j++;
			}
			free(directorios);
			return -1;
		}
	}

	// Libero recursos
	j = 0;
	while(directorios[j] != NULL) {
		free(directorios[j]);
		j++;
	}
	free(directorios);

	return indiceDirectorioActual;

}

char* armarRutaDirectorioPadre(char* rutaDirectorioActual) {

	/* Esta funcion recibe una ruta de directorio actual y elimina la ultima entrada
	 * que tiene, quedando la ruta del directorio padre en rutaDirectorioPadre
	 */

	// Variables
	char** directorios = string_split(rutaDirectorioActual, "/");
	char* rutaDirectorioPadre = malloc(strlen(rutaDirectorioActual) + 1);
	int i = 0;
	int cantidadEtiquetasDirectorios = 0;
	strcpy(rutaDirectorioPadre, "/");

	cantidadEtiquetasDirectorios = contarDirectorios(directorios, 0);

	// Recorro etiquetas y voy concatenando hasta llegar al anteúltimo directorio
	for (i = 0; i < cantidadEtiquetasDirectorios - 1; ++i) {
		strcat(rutaDirectorioPadre, directorios[i]);
		if (i != cantidadEtiquetasDirectorios - 2) {
			strcat(rutaDirectorioPadre, "/");
		}
	}

	// Libero recursos
	i = 0;
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);

	return rutaDirectorioPadre;

}

int buscarDirectorioEn(char* nombreDirectorio, int indiceDirectorioActual) {

	// Variables
	int i = 0;
	int indiceDirectorio = -1;
	struct t_directory* directorioLeido = malloc(sizeof(t_directory));

	// Abro la Tabla de Directorios para buscar el directorio
	FILE* tablaDirectorios = fopen("metadata/directorios.dat", "r");

	// Realizo lectura anticipada para entrar al while
	fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);
	i++;

	// Loopeo hasta encontrar el directorio buscado
	while(((strcmp(directorioLeido->nombre, nombreDirectorio)) != 0)) {
		fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);
		// Si llegué al final de la tabla, salgo
		++i;
		if (i > CANTIDAD_DIRECTORIOS-1) break;
	}

	// Si iteré mas de 99 veces es porque no lo encontre
	if (i > CANTIDAD_DIRECTORIOS - 1) {
		log_error(archivoLog, "El directorio %s no existe.", nombreDirectorio);
	} else {
		// Si lo encontré pero el padre no es en el que estoy parado, fallo
		if (directorioLeido->padre == indiceDirectorioActual) {
			indiceDirectorio = directorioLeido->index;
		} else {
			log_error(archivoLog, "El directorio %s no existe.", nombreDirectorio);
		}
	}

	// Cierro la tabla de directorios
	fclose(tablaDirectorios);

	// Libero recursos
	free(directorioLeido);

	return indiceDirectorio;

}

int archivoEnDirectorio(char* rutaArchivoYama, int indiceDirectorio) {

	// Variables
	int archivoEnDirectorio = false;
	int i = 0;

	// Parseo los directorios
	char** directorios = string_split(rutaArchivoYama, "/");

	// Me quedo con el directorio
	int cantidadDirectorios = contarDirectorios(directorios, 0);

	// Busco el indice del directorio del archivo
	int indiceDirectorioArchivo = buscarIndiceDirectorio(directorios[cantidadDirectorios - 1]);

	// Si el indice del directorio es igual al que tengo, entonces el archivo está en ese directorio
	if (indiceDirectorioArchivo == indiceDirectorio) {
		archivoEnDirectorio = true;
	}

	// Libero recursos
	while(directorios[i] != NULL) {
		free(directorios[i]);
		i++;
	}
	free(directorios);

	return archivoEnDirectorio;

}


int directorioTieneArchivos(int indiceDirectorio) {

	// Variables
	int tieneArchivos = false;
	int i = 0;

	// Itero la tabla hasta llegar al final
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		// Primero pregunto si está ocupado, si no lo descarto
		if (arrayArchivos[i].ocupado == 1) {
			if (archivoEnDirectorio(arrayArchivos[i].ruta, indiceDirectorio)) {
				tieneArchivos = true;
				break;
			}
		}
	}

	return tieneArchivos;

}

int eliminarDirectorio(char* directorio, int indice) {

	// Para poder eliminar un directorio, el mismo no debe tener archivos dentro
	int directorioConDatos = directorioTieneArchivos(indice);

	// El directorio esta vacio, lo puedo eliminar
	if (!directorioConDatos) {
		strcpy(arrayDirectorios[indice].nombre, "");
		arrayDirectorios[indice].padre = -1;
		log_trace(archivoLog, "El directorio '%s' ha sido eliminado correctamente de la posicion %d.", directorio, indice);
		return true;
	} else {
		log_error(archivoLog, "No se puede eliminar el directorio %s ya que posee archivos en él.", directorio);
		return false;
	}
}

int cargarArrayDirectorios(struct t_directory* directorio, int indiceDirectorio, struct t_directory* arrayDirectorios) {

	arrayDirectorios[indiceDirectorio].index = directorio->index;
	strcpy(arrayDirectorios[indiceDirectorio].nombre, directorio->nombre);
	arrayDirectorios[indiceDirectorio].padre = directorio->padre;

	return true;

}

int cargarTablaDirectorios() {

	// Variables
	int indiceDirectorio = 0;
	int estadoCargado = false;

	// Abro la tabla de directorios
	FILE* tablaDirectorios = fopen("metadata/directorios.dat", "r+");
	int tablaDirectoriosFD = fileno(tablaDirectorios);

	// Mapeo el archivo a memoria
	arrayDirectorios =  (struct t_directory*) mmap(NULL, sizeof(struct t_directory) * CANTIDAD_DIRECTORIOS, PROT_READ | PROT_WRITE, MAP_SHARED, tablaDirectoriosFD, 0);

	if (arrayDirectorios == MAP_FAILED) {
	    log_error(archivoLog, "Error al mapear la Tabla de Directorios.");
	    return estadoCargado;
	} else {
		// Declaro el Registro de Directorio que me va a servir para leer
		t_directory* directorioLeido = malloc(sizeof(t_directory));
		for (indiceDirectorio = 0; indiceDirectorio < CANTIDAD_DIRECTORIOS; ++indiceDirectorio) {
			// Leer Registro de Directorio
			fread(directorioLeido, sizeof(t_directory), 1, tablaDirectorios);
			cargarArrayDirectorios(directorioLeido, indiceDirectorio, arrayDirectorios);
		}
		free(directorioLeido);
		estadoCargado = true;
		return estadoCargado;
	}
}

int renombrarDirectorio(int indiceDirectorio,  char* nuevoNombreDirectorio) {

	// Si ya existe un directorio con ese nombre, no lo puedo renombrar
	int directorioYaExiste = buscarDirectorio(nuevoNombreDirectorio);

	if (!directorioYaExiste) {
		log_trace(archivoLog, "El directorio '%s' ha sido renombrado a '%s' correctamente.", arrayDirectorios[indiceDirectorio].nombre, nuevoNombreDirectorio);
		strcpy(arrayDirectorios[indiceDirectorio].nombre, nuevoNombreDirectorio);

	} else {
		log_error(archivoLog, "No se puede renombrar el directorio con el nombre '%s' porque ya existe.", nuevoNombreDirectorio);
	}

	return true;
}

int listarArchivosDirectorio(int indiceDirectorio) {

	// Variables
	char* indiceString = string_itoa(indiceDirectorio);

	// Comando 'ls'
	char* comando = malloc(strlen("ls ") + strlen("metadata/archivos/") + strlen(indiceString) + 1);
	strcpy(comando, "ls ");
	strcat(comando, "metadata/archivos/");
	strcat(comando, indiceString);

	// Hago el comando ls con la direccion metada/archivos/'indiceDirectorio'
	log_trace(archivoLog, "Los archivos que contiene el directorio actual son: ");
	printf("\n");
	system(comando);
	printf("\n");

	// Libero recursos
	free(comando);
	free(indiceString);

	return true;
}

int listarDirectoriosDirectorio(int indiceDirectorio) {

	// Variables
	int i = 0;

	// Comando 'ls'
	log_trace(archivoLog, "Los directorios que contiene el directorio actual son: ");
	printf("\n");

	// Recorro el array de directorios y si el padre de alguno es el directorio en el que estoy, lo imprimo
	for (i = 0; i < CANTIDAD_DIRECTORIOS; ++i) {
		if (arrayDirectorios[i].padre == indiceDirectorio) {
			printf("%s\n", arrayDirectorios[i].nombre);
		}
	}

	printf("\n");

	return true;

}
