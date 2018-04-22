/*
 * Directorios.h
 *
 *  Created on: 21/10/2017
 *      Author: utnso
 */

#ifndef DIRECTORIOS_H_
#define DIRECTORIOS_H_

#include "funcionesFILESYSTEM.h"
#include "Globales.h"

int archivoEnDirectorio(char* rutaArchivoYama, int indiceDirectorio);
int directorioTieneArchivos(int indiceDirectorio);
int grabarDirectorio(char* nombreDirectorio, int indiceDirectorio, int padreDirectorio);
int inicializarTablaDirectorios(int tablaDirectorios) ;
int formatearTablaDirectorios() ;
int buscarEspacioLibre();
int crearDirectorio(char* nombre, int indiceActual);
int parsearNombreDirectorio(char * nombreDirectorio, char* nombreParseado);
int buscarIndiceDirectorio(char* directorio);
int buscarIndiceDirectorioPadre(char* directorio);
int contarDirectorios(char** etiquetas, int tieneNombreArchivo);
int validarRutaDirectorios(char* ruta);
int validarRutaDirectoriosConNombreArchivo(char* rutaConNombreArchivo);
char* armarRutaDirectorioPadre(char* rutaDirectorioActual);
int buscarDirectorioEn(char* nombreDirectorio, int indiceDirectorioActual);
int buscarDirectorio(char* nombre);
int eliminarDirectorio(char* directorio, int indice);
int cargarArrayDirectorios(struct t_directory* directorio, int indiceDirectorio, struct t_directory* arrayDirectorios);
int cargarTablaDirectorios();
int renombrarDirectorio(int indiceDirectorio, char* nuevoNombreDirectorio) ;
int obtenerSizeDirectorio(t_directory* directorio);
int listarArchivosDirectorio(int indiceDirectorio);
int listarDirectoriosDirectorio(int indiceDirectorioActual);

#endif /* DIRECTORIOS_H_ */
