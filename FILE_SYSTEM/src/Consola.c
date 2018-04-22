/*
 * Consola.c

 *
 *  Created on: 11/11/2017
 *      Author: utnso
 */
#include "Consola.h"
#include "Directorios.h"
#include "Globales.h"
#include "funcionesFILESYSTEM.h"

void* comandosFileSystem (void) {

	// Detacho el hilo que estoy ejecutando
	pthread_detach(pthread_self());

	// Imprimo bienvenida
	puts("********* BIENVENIDO A LA CONSOLA DEL FILE SYSTEM *********");

	// Variables necesarias
	almacenoCopias = true;
	nodosSuficientes = false;
	int comandoReconocido = false;
	int comandoReconocidoCD = false;

	// Seteo el directorio actual
	char* directorioActual = malloc(100);
	int indiceDirectorioActual = 0;
	int padreDirectorioActual = -1;
	strcpy(directorioActual, "/base");

	// Armo la linea de comando con el directorio actual y el simbolo
	char* lineaComando = malloc(100);
	strcpy(lineaComando, "/base");
	strcat(lineaComando, ">");

	// Loopeo a la espera de comandos desde consola
	while(true) {

		// Leo desde línea de comando lo que ingresa el usuario.
		char* linea = readline(lineaComando);

		// El comando todavía no es conocido
		comandoReconocido = false;
		comandoReconocidoCD = false;

		// COMANDO "cd .."
		if ((strncmp("cd ..", linea, 5)) == 0) {
			comandoReconocidoCD = true;
			comandoReconocido = true;
			log_info(archivoLog, "El comando ingresado es 'cd ..'.");
			// Quiero ir al directorio padre de donde estoy
			if (padreDirectorioActual == -1) {
				log_error(archivoLog, "El directorio raiz no tiene directorio padre.");
			} else {
				// Seteo al directorio padre de donde estoy como directorio actual
				indiceDirectorioActual = padreDirectorioActual;
				char* rutaDirectorioPadre = armarRutaDirectorioPadre(directorioActual);
				strcpy(lineaComando, rutaDirectorioPadre);
				strcat(lineaComando, ">");
				padreDirectorioActual = arrayDirectorios[indiceDirectorioActual].padre;
				strcpy(directorioActual, rutaDirectorioPadre);
				free(rutaDirectorioPadre);
			}
		}

		// COMANDO "cd"
		if (!comandoReconocidoCD) {
			if ((strncmp("cd ", linea, 3)) == 0) {
				log_info(archivoLog, "El comando ingresado es 'cd'.");
				comandoReconocido = true;
				// Quiero ir al directorio especificado
				char* nombreDirectorio = string_substring_from(linea, 3);
				indiceDirectorioActual = buscarDirectorioEn(nombreDirectorio, indiceDirectorioActual);
				// Si indiceDirectorioActual me devuelve -1 es porque en el lugar donde estoy parado no existe ese directorio
				if (indiceDirectorioActual != -1) {
					strcat(directorioActual, "/");
					strcat(directorioActual, arrayDirectorios[indiceDirectorioActual].nombre);
					strcpy(lineaComando, directorioActual);
					strcat(lineaComando, ">");
					padreDirectorioActual = arrayDirectorios[indiceDirectorioActual].padre;
				} else {
					log_error(archivoLog, "El directorio especificado no existe.");
				}
				// Libero recursos
				free(nombreDirectorio);
			}
		}

		// COMANDO "set ALMACENO COPIAS"
		if ((strncmp("set almacenoCopias ", linea, 19)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'set almacenoCopias'.");
			comandoReconocido = true;
			// Quiero setear el valor de almacenoCopias (true/false)
			char* valor = string_substring_from(linea, 19);
			almacenoCopias = atoi(valor);
			free(valor);
		}

		// COMANDO "set RETARDO"
		if ((strncmp("set retardo ", linea, 12)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'set retardo'.");
			comandoReconocido = true;
			// Quiero setear el valor de RETARDO
			char* valor = string_substring_from(linea, 12);
			retardoOKBloques = atoi(valor);
			log_trace(archivoLog, "El RETARDO ha sido modificado a %d.", retardoOKBloques);
			free(valor);
		}

		// COMANDO "show RETARDO"
		if ((strncmp("show retardo", linea, 12)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'show retardo'.");
			comandoReconocido = true;
			// Quiero ver el valor de RETARDO
			printf("RETARDO=%d\n", retardoOKBloques);
		}

		// COMANDO "ver"
		if ((strncmp("ver ", linea, 4)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'ver'.");
			comandoReconocido = true;
			// Ver información del nodo
			char* nombreNodo = string_substring_from(linea, 4);
			mostrarInformacionNodo(nombreNodo);
			free(nombreNodo);
		}

		// COMANDO "mostrar ALMACENO COPIAS"
		if ((strncmp("mostrar almacenoCopias", linea, 22)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'mostrar almacenoCopias'.");
			comandoReconocido = true;
			// Quiero ver el valor de almacenoCopias
			printf("almacenoCopias = %d\n", almacenoCopias);
		}

		// COMANDO "mostrar DIRECTORIO ACTUAL"
		if ((strncmp("mostrar directorioActual", linea, 24)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'mostrar directorioActual'.");
			comandoReconocido = true;
			// Quiero ver el valor de directorioActual
			printf("directorioActual = %s\n", directorioActual);
		}

		// COMANDO "mostrar INDICE DIRECTORIO ACTUAL"
		if ((strncmp("mostrar indiceDirectorioActual", linea, 30)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'mostrar indiceDirectorioActual'.");
			comandoReconocido = true;
			// Quiero ver el valor de directorioActual
			printf("indiceDirectorioActual = %d\n", indiceDirectorioActual);
		}

		// COMANDO "mostrar PADRE DIRECTORIO ACTUAL"
		if ((strncmp("mostrar padreDirectorioActual", linea, 29)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'mostrar padreDirectorioActual'.");
			comandoReconocido = true;
			// Quiero ver el valor de padreDirectorioActual
			printf("padreDirectorioActual = %d\n", padreDirectorioActual);
			free(linea);
		}

		// COMANDO "mkdir"
		if ((strncmp("mkdir ", linea, 6)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'mkdir'.");
			comandoReconocido = true;
			// Quieren crear un directorio, me quedo con el nombre
			char* nombreDirectorio = string_substring_from(linea, 6);
			int indiceDirectorioCreado = crearDirectorio(nombreDirectorio, indiceDirectorioActual);
			// Pregunto si el directorio se creo o no
			if (indiceDirectorioCreado == -1) {
				log_error(archivoLog, "Error al crear el directorio %s.", nombreDirectorio);
			} else {
				log_trace(archivoLog, "El directorio %s se ha creado correctamente en el indice %d.", nombreDirectorio, indiceDirectorioCreado);
			}
			free(nombreDirectorio);
		}

		// COMANDO "cpfrom -b" (ARCHIVO BINARIO)
		if ((strncmp("cpfrom -b ", linea, 10)) == 0 ) {
			comandoReconocido = true;
			if (estadoSeguro == true) {
				log_info(archivoLog, "El comando ingresado es 'cpfrom -b'.");

				char* archivoOrigenYRutaYamaB = string_substring_from(linea, 10);
				char** parametrosCPFROMB = string_split(archivoOrigenYRutaYamaB, " ");
				char* nombreArchivoLocalB = malloc(strlen(parametrosCPFROMB[0]) + 1);
				char* rutaYamaB = malloc(strlen(parametrosCPFROMB[1]) + 1);

				// Copio la info
				strcpy(nombreArchivoLocalB, parametrosCPFROMB[0]);
				strcpy(rutaYamaB, parametrosCPFROMB[1]);

				// Libero lo que ya use
				free(parametrosCPFROMB[0]);
				free(parametrosCPFROMB[1]);
				free(parametrosCPFROMB);
				free(archivoOrigenYRutaYamaB);

				// Me fijo si el archivo que me pasaron existe en mi FS local
				if ((validarArchivo(nombreArchivoLocalB)) == false) {
				// Si el archivo no es válido, doy mensaje de error
					log_error(archivoLog, "El archivo %s no existe.", nombreArchivoLocalB);
				} else {
					// Si el archivo es válido, me fijo si la ruta destino es valida
					int indiceDirectorioB = -1;
					indiceDirectorioB = validarRutaDirectorios(rutaYamaB);
					// Si indiceDirectorio es -1 no es una ruta válida
					// Si indiceDirectorio es distinto de -1, es una ruta valida y tengo el indice del directorio donde grabar
					if (indiceDirectorioB == -1) {
						log_error(archivoLog, "La ruta %s no existe.", rutaYamaB);
					} else {
						almacenarArchivoBinario(nombreArchivoLocalB, rutaYamaB, indiceDirectorioB);
					}
				}
				// Libero Recursos
				free(nombreArchivoLocalB);
				free(rutaYamaB);
			} else {
				log_error(archivoLog, "Este comando no se puede realizar porque el FS no está en Estado Seguro.");
			}
		}

		// COMANDO "cpfrom -t" (ARCHIVO TEXTO)
		if ((strncmp("cpfrom -t ", linea, 10)) == 0 ) {
			comandoReconocido = true;
			if (estadoSeguro == true) {
				log_info(archivoLog, "El comando ingresado es 'cpfrom -t'.");
				char* archivoOrigenYRutaYamaT = string_substring_from(linea, 10);
				char** parametrosCPFROMT = string_split(archivoOrigenYRutaYamaT, " ");
				char* nombreArchivoLocalT = malloc(strlen(parametrosCPFROMT[0]) + 1);
				char* rutaYamaT = malloc(strlen(parametrosCPFROMT[1]) + 1);

				// Copio la info
				strcpy(nombreArchivoLocalT, parametrosCPFROMT[0]);
				strcpy(rutaYamaT, parametrosCPFROMT[1]);

				// Me fijo si el archivo que me pasaron existe en mi FS local
				if ((validarArchivo(nombreArchivoLocalT)) == false) {
				// Si el archivo no es válido, doy mensaje de error
					log_error(archivoLog, "El archivo %s no existe.", nombreArchivoLocalT);
				} else {
					// Si el archivo es válido, me fijo si la ruta destino es valida
					int indiceDirectorioT = -1;
					indiceDirectorioT = validarRutaDirectorios(rutaYamaT);
					// Si indiceDirectorio es -1 no es una ruta válida
					// Si indiceDirectorio es distinto de -1, es una ruta valida y tengo el indice del directorio donde grabar
					if (indiceDirectorioT == -1) {
						log_error(archivoLog, "La ruta %s no existe.", rutaYamaT);
					} else {
						almacenarArchivoTexto(nombreArchivoLocalT, rutaYamaT, indiceDirectorioT);
					}
				}

				// Libero Recursos
				free(nombreArchivoLocalT);
				free(archivoOrigenYRutaYamaT);
				int i = 0;
				while(parametrosCPFROMT[i] != NULL) {
					free(parametrosCPFROMT[i]);
					i++;
				}
				free(parametrosCPFROMT);
				free(rutaYamaT);

			} else {
				log_error(archivoLog, "Este comando no se puede realizar porque el FS no está en Estado Seguro.");
			}
		}

		// COMANDO "rename -d" (Renombra un directorio del YAMA FS)
		if((strncmp("rename -d ",linea, 10)) == 0) {
			log_info(archivoLog,"El comando ingresado es 'rename -d'.");
			comandoReconocido = true;

			// Me guardo el path original del directorio y el nuevo nombre
			char* pathDirectorioYNuevoNombre = string_substring_from(linea, 10);

			// Ahora los divido
			char** parametrosRenameDirectorio = string_split(pathDirectorioYNuevoNombre, " ");

			// En parametrosRenameArchivo[0] tengo el pathOriginal del directorio a renombrar
			// En parametrosRenameArchivo[1] tengo el nuevo nombre del directorio

			// Valido que el directorio que me ingresaron existe
			int directorioValido = validarRutaDirectorios(parametrosRenameDirectorio[0]);

			// Si la ruta que me ingresaron es válida, lo renombro
			if (directorioValido != -1) {
				renombrarDirectorio(directorioValido, parametrosRenameDirectorio[1]);
			} else {
				log_error(archivoLog, "El directorio %s no existe en Yama FS.", parametrosRenameDirectorio[0]);
			}

			// Libero recursos
			free(pathDirectorioYNuevoNombre);

			// Libero los parametros
			int i = 0;
			while(parametrosRenameDirectorio[i] != NULL) {
				free(parametrosRenameDirectorio[i]);
				i++;
			}
			free(parametrosRenameDirectorio);
		}

		// COMANDO "format"
		if ((strncmp("format", linea, 6)) == 0) {
			comandoReconocido = true;
			if (estadoCargado == false) {
				log_info(archivoLog, "El comando ingresado es 'format'.");
				formatearFileSystem();
			} else {
				log_error(archivoLog, "No se puede formatear el File System ya que se está recuperando de un estado anterior.");
			}
		}

		// COMANDO "rm -d"
		if((strncmp("rm -d ",linea, 6)) == 0) {
			comandoReconocido = true;
			//Funcion eliminar directorio
			log_info(archivoLog, "El comando ingresado es 'rm -d'.");

			// Me quedo con la ruta del directorio
			char* pathDirectorio = string_substring_from(linea, 6);

			// La parseo para saber exactamente cuál es el directorio que quieren borrar
			char** nombreDirectorio = string_split(pathDirectorio,"/");

			int i = 0;

			// Recorro hasta el nombre y ahi se lo paso a buscarIndiceDirectorio
			while (nombreDirectorio[i] != NULL) {
				i++;
			}

			// Me quedo con el indice del directorio que quiero eliminar
			int indiceDirectorioEliminar = buscarIndiceDirectorio(nombreDirectorio[i-1]);

			// Si el indiceDirectorioEliminar es igual a 0 es el base
			if((indiceDirectorioEliminar == 0)) {
				log_error(archivoLog, "El directorio 'base' no puede eliminarse.");
			} else {
				if (indiceDirectorioEliminar == indiceDirectorioActual) {
					log_error(archivoLog, "No se puede eliminar el directorio actual.");
				} else {
					// Si el directorio a eliminar tiene hijos, no puede eliminarse
					if(tieneHijos(indiceDirectorioEliminar)) {
						log_error(archivoLog, "El directorio %s no puede eliminarse ya que posee subdirectorios.", nombreDirectorio[i-1]);
					} else {
						eliminarDirectorio(pathDirectorio, indiceDirectorioEliminar);
					}
				}

			}

			// Libero recursos
			free(pathDirectorio);
			i = 0;
			while(nombreDirectorio[i] != NULL) {
				free(nombreDirectorio[i]);
				i++;
			}
			free(nombreDirectorio);
		}

		// COMANDO "rm "
		if((strncmp("rm -a ",linea, 6)) == 0) {
			// Funcion eliminar Archivo
			log_info(archivoLog, "El comando ingresado es 'rm -a'.");
			comandoReconocido = true;
			// Quiero ir a buscar el nombre del archivo
			char* pathArchivo = string_substring_from(linea, 6);
			char** nombreArchivo = string_split(pathArchivo,"/");

			// Libero el nombre del archivo
			int i = 0;
			while (nombreArchivo[i] != NULL) {
				i++;
			}

			// Si el archivo es válido, lo elimino. Si no, doy error.
			if (validarArchivoYama(pathArchivo)) {
				int indiceDirectorioArchivoEliminar = buscarIndiceDirectorio(nombreArchivo[i-2]);
				borrarArchivoYama(nombreArchivo[i-1], indiceDirectorioArchivoEliminar, pathArchivo);
			} else {
				log_error(archivoLog, "El archivo %s no puede eliminarse ya que no existe en YAMA FS.", pathArchivo);
			}

			// Libero recursos
			free(pathArchivo);
			i = 0;
			while(nombreArchivo[i] != NULL) {
				free(nombreArchivo[i]);
				i++;
			}
			free(nombreArchivo);
		}

		// COMANDO "ls"
		if((strncmp("ls",linea,2)) == 0) {
			//imprime los nombres de los archivos en el directorio actual
			log_info(archivoLog,"El comando ingresado es 'ls' .");
			comandoReconocido = true;
			listarArchivosDirectorio(indiceDirectorioActual);
			listarDirectoriosDirectorio(indiceDirectorioActual);
		}

		// COMANDO "md5"
		if((strncmp("md5 ",linea,4)) == 0) {
			comandoReconocido = true;
			if (estadoSeguro == true) {
				//Calcula el md5 del archivo dado. El archivo debe existir en el YAMA File System
				log_info(archivoLog,"El comando ingresado es 'md5'.");
				char* rutaArchivoTemporalMD5 = malloc(200);
				char** rutaArchivoMD5 = string_split(linea, " ");
				// Armo el archivo. Si no falla, la función retorna la ruta del archivo temporal grabado
				if (validarArchivoYama(rutaArchivoMD5[1])) {
					armarArchivo(rutaArchivoMD5[1], rutaArchivoTemporalMD5);
					// Valido la respuesta de armarArchivo
					if (rutaArchivoTemporalMD5 != NULL) {
						// Tengo una ruta, le calculo el MD5 a ese archivo
						char* resultadoMD5 = md5(rutaArchivoTemporalMD5);
						log_trace(archivoLog, "El MD5 del archivo %s es %s.", rutaArchivoMD5[1], resultadoMD5);
						free(resultadoMD5);
					} else {
						// Falló el armado del archivo, informo el error
						log_error(archivoLog, "No se pudo calcular el MD5 del archivo %s.", rutaArchivoMD5[1]);
					}
				} else {
					log_error(archivoLog, "El archivo %s no es un archivo válido del YAMA FS.", rutaArchivoMD5[1]);
				}

				// Libero recursos
				free(rutaArchivoMD5[0]);
				free(rutaArchivoMD5[1]);
				free(rutaArchivoMD5);
				free(rutaArchivoTemporalMD5);
			} else {
				log_error(archivoLog, "Este comando no se puede realizar porque el FS no está en Estado Seguro.");
			}
		}

		// COMANDO "mv -a"
		if((strncmp("mv -a ", linea, 6)) == 0) {
			// Puede mover un archivo
			log_info(archivoLog,"El comando ingresado es 'mv' .");
			comandoReconocido = true;

			// Variables
			char* pathInicialYFinal = string_substring_from(linea, 6);
			char** parametrosMVA = string_split(pathInicialYFinal, " ");

			// Me quedo con la informacion que me importa
			char* pathOriginal = malloc(strlen(parametrosMVA[0]) + 1);
			strcpy(pathOriginal, parametrosMVA[0]);

			char* pathFinal = malloc(strlen(parametrosMVA[1]) + 1);
			strcpy(pathFinal, parametrosMVA[1]);

			// Ahora me fijo de qué directorio viene el archivo y a cuál lo quieren mover
			char** directorioOrigen = string_split(pathOriginal,"/");
			char** directorioDestino = string_split(pathFinal,"/");

			// Avanzo para conocer el ultimo directorio antes del nombre
			int i = 0;
			while (directorioOrigen[i] != NULL) {
				i++;
			}

			// Avanzo para conocer el ultimo directorio antes del nombre
			int j = 0;
			while (directorioDestino[j] != NULL) {
				j++;
			}

			// Del path original me quedo unicamente con la ruta del archivo
			int sizePathOriginal = strlen(pathOriginal);
			int k = sizePathOriginal;
			int retrocedi = 0;

			while(pathOriginal[k] != '/') {
				k--;
				retrocedi++;
			}

			int longitud = sizePathOriginal - retrocedi;

			char* rutaArchivoYama = string_substring_until(pathOriginal, longitud);

			// Una vez que tengo la informacion, busco los directorios para ver si existen
			if (buscarDirectorio(directorioOrigen[i-2])) {
				if (buscarDirectorio(directorioDestino[i-1])) {
					// Luego de esto, busco el indice del directorioOrigen y el del directorioDestino
					int indiceDirectorioOrigen = buscarIndiceDirectorio(directorioOrigen[i-2]);
					int indiceDirectorioDestino = buscarIndiceDirectorio(directorioDestino[i-1]);
					moverArchivo(rutaArchivoYama, directorioOrigen[i-1], indiceDirectorioOrigen, indiceDirectorioDestino, pathFinal);
				} else {
					log_error(archivoLog, "La ruta destino no es válida.");
				}
			} else {
				log_error(archivoLog, "La ruta origen no es válida.");
			}

			// Libero recursos
			free(parametrosMVA[0]);
			free(parametrosMVA[1]);
			free(parametrosMVA);
			free(pathOriginal);
			free(pathFinal);
			free(pathInicialYFinal);

			// Libero directorios
			i = 0;
			while(directorioOrigen[i] != NULL) {
				free(directorioOrigen[i]);
				i++;
			}
			free(directorioOrigen);
			j = 0;
			while(directorioDestino[j] != NULL) {
				free(directorioDestino[j]);
				j++;
			}
			free(directorioDestino);
		}

		// COMANDO "cat"
		if ((strncmp("cat ", linea, 4)) == 0) {
			// Leer el contenido de un archivo de YAMA File System
			log_info(archivoLog, "El comando ingresado es 'cat'.");
			comandoReconocido = true;
			char* rutaArchivoTemporal = malloc(200);
			char** ruta = string_split(linea, " ");

			// Armo el archivo. Si no falla, la función retorna la ruta del archivo temporal grabado
			if (validarArchivoYama(ruta[1])) {
				if (armarArchivo(ruta[1], rutaArchivoTemporal)) {
					// Valido la respuesta de armarArchivo
					if (rutaArchivoTemporal != NULL) {
						// Tengo una ruta, le hago cat a ese archivo para ver su contenido
						char* comando = malloc(strlen("cat ") + strlen(ruta[1]) + 1);
						strcpy(comando, "cat ");
						strcat(comando, rutaArchivoTemporal);
						log_trace(archivoLog, "El contenido del archivo %s es: ", ruta[1]);
						system(comando);
						free(comando);
						// Borro el archivo temporal que genere
						remove(rutaArchivoTemporal);
					} else {
						// Falló el armado del archivo, informo el error
						log_error(archivoLog, "El archivo %s no se puede mostrar.", ruta[1]);
					}
				} else {
					log_error(archivoLog, "Hubo un problema al recuperar el archivo '%s'.", ruta[1]);
				}
			} else {
				log_error(archivoLog, "La ruta '%s' ingresada no es una ruta válida.", ruta[1]);
			}

			// Libero los recursos
			free(rutaArchivoTemporal);
			int i = 0;
			while(ruta[i] != NULL) {
				free(ruta[i]);
				i++;
			}
			free(ruta);
		}

		// COMANDO "info"
		if (strncmp("info ", linea, 5) == 0) {
			log_info(archivoLog, "El comando ingresado es 'info'.");
			comandoReconocido = true;
			char* rutaArchivoInfo = string_substring_from(linea, 5);

			int archivoYamaValido = validarArchivoYama(rutaArchivoInfo);

			// Si el archivo es valido, recupero su informacion
			if (archivoYamaValido) {
				recuperarInfoArchivo(rutaArchivoInfo);
			} else {
				log_error(archivoLog, "El archivo %s no existe en Yama FS.", rutaArchivoInfo);
			}

			// Libero recursos
			free(rutaArchivoInfo);
		}

		// COMANDO "cpto"
		if ((strncmp("cpto ", linea, 5)) == 0) {
			comandoReconocido = true;
			if (estadoSeguro == true) {
				//copiar un archivo de YAMA al directorio local
				log_info(archivoLog, "El comando ingresado es 'cpto'.");
				char* pathYAMAFSYpathFSLocal = string_substring_from(linea, 6);
				char** parametrosCPTO = string_split(pathYAMAFSYpathFSLocal, " ");
				char* pathYAMAFS = malloc(strlen(parametrosCPTO[0]) + 1);
				char* pathFSLocal = malloc(strlen(parametrosCPTO[1]) + 1);
				strcpy(pathYAMAFS, parametrosCPTO[0]);
				strcpy(pathFSLocal, parametrosCPTO[1]);

				// Valido el archivo que me piden
				if ((validarArchivoYama(pathYAMAFS)) == false) {
					// Si el archivo no es válido, doy mensaje de error
					log_error(archivoLog, "El archivo %s no existe.", pathYAMAFS);
				} else {
					// Si el archivo es válido paso el mismo al directorio local
					log_info(archivoLog, "Se inicia la copia del archivo %s en la ruta %s", pathYAMAFS, pathFSLocal);
					copiarArchivoAFSLocal(pathYAMAFS, pathFSLocal);
				}

				// Libero los recursos
				free(pathYAMAFSYpathFSLocal);
				free(pathYAMAFS);
				free(pathFSLocal);

				// Libero los parametros
				int i = 0;
				while(parametrosCPTO[i] != NULL) {
					free(parametrosCPTO[i]);
					i++;
				}
				free(parametrosCPTO);

			} else {
				log_error(archivoLog, "Este comando no se puede realizar porque el FS no está en Estado Seguro.");
			}
		}

		// COMANDO "rename -a" (Renombra un archivo guardado en YAMA FS) (FALTA ACTUALIZAR TABLA ARCHIVOS)
		if((strncmp("rename -a ",linea, 10)) == 0) {
			comandoReconocido = true;
			log_info(archivoLog,"El comando ingresado es 'rename -a'.");

			// Me guardo el path original del archivo y el nuevo nombre
			char* pathOriginalYNuevoNombre = string_substring_from(linea, 10);

			// Ahora los divido
			char** parametrosRenameArchivo = string_split(pathOriginalYNuevoNombre, " ");

			// Me quedo con la cantidad de bytes que ocupan la ruta y el nombre
			int sizeRutaYNombre = strlen(parametrosRenameArchivo[0]);

			// Desde la última posición empiezo a sumar hasta llegar a la "/"
			int i = sizeRutaYNombre;
			int retrocedi = 0;
			while(parametrosRenameArchivo[0][i] != '/') {
				i--;
				retrocedi++;
			}

			int longitud = sizeRutaYNombre - retrocedi;

			// Me quedo con la ruta del archivo únicamente
			char* rutaArchivoARenombrar = string_substring_until(parametrosRenameArchivo[0], longitud);

			// Me quedo con el nombre del archivo unicamente
			char* nombreArchivoARenombrar = string_substring(parametrosRenameArchivo[0], longitud+1, sizeRutaYNombre-longitud);

			// En parametrosRenameArchivo[0] tengo el pathOriginal con el nombre del archivo a renombrar
			// En parametrosRenameArchivo[1] tengo el nuevo nombre a poner en el archivo
			// Valido que el archivo que me ingresaron existe
			int archivoValido = validarArchivoYama(parametrosRenameArchivo[0]);

			if (archivoValido) {
				if ( (renombrarArchivoYama(parametrosRenameArchivo[0], parametrosRenameArchivo[1])) ) {
					actualizarNombreArchivoEnTablaArchivos(rutaArchivoARenombrar, nombreArchivoARenombrar, parametrosRenameArchivo[1]);
				} else {
					log_error(archivoLog, "Error al renombrar el archivo %s.", parametrosRenameArchivo[0]);
				}
			} else {
				log_error(archivoLog, "El archivo %s no existe en Yama FS.", parametrosRenameArchivo[0]);
			}

			// Libero recursos
			free(pathOriginalYNuevoNombre);
			free(rutaArchivoARenombrar);
			free(nombreArchivoARenombrar);

			// Libero parametros
			int j = 0;
			while(parametrosRenameArchivo[j] != NULL) {
				free(parametrosRenameArchivo[j]);
				j++;
			}
			free(parametrosRenameArchivo);
		}

		// El comando que se ha ingresado no es reconocido.
		if (!comandoReconocido) {
			printf("Comando desconocido.\n");
		}

		// COMANDO "exit"
		if ((strncmp("exit", linea, 4)) == 0) {
			log_info(archivoLog, "El comando ingresado es 'exit'.");
			comandoReconocido = true;
			// Libero lo que me quedo pendiente
			free(lineaComando);
			free(directorioActual);
			exit(0);
		}

		// Libero la linea porque la vuelvo a pedir arriba
		free(linea);

	} // Fin while(true)

	return 0;

} // Fin función
