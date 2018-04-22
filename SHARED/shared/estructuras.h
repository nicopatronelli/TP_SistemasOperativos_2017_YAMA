#ifndef SHARED_ESTRUCTURAS_H_   /* Include guard */
#define SHARED_ESTRUCTURAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

// Estructura para el manejo de la Tabla de Directorios

typedef struct t_directory {
	int index;
	char nombre[255];
	int padre;
} t_directory;

// Estructura para el manejo de archivos
typedef struct t_archivo {
	int index;
	char ruta[255];
	char nombre[100];
	int ocupado;
} t_archivo;

// Estructura para el manejo de bloques de un archivo
typedef struct t_bloque {
	int numero;
	char nodo[30];
	int bloqueDataBin;
	int espacioOcupado;
	char ipNodo[16];
	int puertoEscuchaMaster;
} t_bloque;

#endif // SHARED_ESTRUCTURAS_H_
