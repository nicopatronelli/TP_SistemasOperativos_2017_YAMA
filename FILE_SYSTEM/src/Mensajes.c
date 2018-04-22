#include "Mensajes.h"
#include "Globales.h"
#include "funcionesFILESYSTEM.h"
#include "Nodos.h"

// Cuando se conecta un Datanode, le envío un mensaje de conexión aceptada
int enviarConexionAceptada(int socket) {

 	// Creo el protocolo y buffer de Conexion Aceptada, con su tamaño
	int bytesEnviados = enviar_mensaje(DATANODE_ACEPTADO, "1", socket);

	return bytesEnviados;

}

// Cuando se conecta un Datanode, le envío un mensaje de conexión rechazada
int enviarConexionRechazada(int socket) {

 	// Creo el protocolo y buffer de Conexion Aceptada, con su tamaño
	int bytesEnviados = enviar_mensaje(DATANODE_RECHAZADO, "1", socket);

	return bytesEnviados;

}

void definirAccionHeader(t_protocolo *protocolo, uint32_t socket) {

	// Switch principal para ver los diferentes mensajes
	switch (protocolo->funcion){
        case ALMACENAR_ARCHIVO_REDUCCION_GLOBAL :

        	log_info(archivoLog, "Se ha recibido el mensaje ALMACENAR ARCHIVO REDUCCION GLOBAL desde el socket %d", socket);

        	// Con el primer mensaje recibo la ruta y el nombre del archivo a guardar en YAMA
        	char* rutaArchivoReduccionGlobal = malloc(protocolo->sizeMensaje + 1);
        	strcpy(rutaArchivoReduccionGlobal, protocolo->mensaje);

        	log_info(archivoLog, "La ruta para guardar el archivo en YAMAFS es '%s'.", rutaArchivoReduccionGlobal);

        	// Me quedo únicamente con la ruta del archivo para poder validarla
        	int sizeRutaArchivoReduccionGlobal = strlen(rutaArchivoReduccionGlobal);
        	int i = sizeRutaArchivoReduccionGlobal;
        	int retrocedi = 0;

        	while(rutaArchivoReduccionGlobal[i] != '/') {
        		i--;
        		retrocedi++;
        	}

        	int longitud = sizeRutaArchivoReduccionGlobal - retrocedi;

        	char* rutaDirectoriosArchivoReduccionGlobal = string_substring_until(rutaArchivoReduccionGlobal, longitud);

        	// Luego recibo un gran buffer con todos los datos que tiene el archivo
        	t_protocolo* mensaje = recibir_mensaje(socket);

        	// Si hay algun error salgo...
        	if ( mensaje == CERRARON_SOCKET ) {
        		log_error(archivoLog, "El WORKER cerro su conexión en el socket %d.", socket);
        		close(socket);
        		FD_CLR(socket, &maestroFD);
        		free(rutaArchivoReduccionGlobal);
        		free(rutaDirectoriosArchivoReduccionGlobal);
        		eliminar_protocolo(mensaje);
        	} else if ( mensaje == NULL) {
        		log_error(archivoLog, "Hubo un error al recibir un mensaje del WORKER en el socket %d.", socket);
        		close(socket);
        		FD_CLR(socket, &maestroFD);
        		free(rutaArchivoReduccionGlobal);
        		free(rutaDirectoriosArchivoReduccionGlobal);
        		eliminar_protocolo(mensaje);
        	} else { //... sino atiendo el mensaje ...

        		// Me envian el contenido del archivo de reduccion global para almacenarlo en YAMA FS
				if (mensaje->funcion == CONTENIDO_ARCHIVO_REDUCCION_GLOBAL) {
					// Aloco el buffer donde voy a copiar la información del archivo
					char* contenidoArchivoReduccionGlobal = calloc(mensaje->sizeMensaje + 1, 1);
					// Copio el contenido del archivo de reducción global en el buffer que aloque
					memcpy(contenidoArchivoReduccionGlobal, mensaje->mensaje, mensaje->sizeMensaje);
		        	// Me fijo si el directorio que me dieron para almacenar el archivo es válido
		        	int directorio = validarRutaDirectorios(rutaDirectoriosArchivoReduccionGlobal);
		        	// Si el directorio es válido, almaceno el archivo. Si no, doy error.
		        	if (directorio != -1) {
		        		almacenarArchivoReduccionGlobal(contenidoArchivoReduccionGlobal, rutaArchivoReduccionGlobal, directorio, rutaDirectoriosArchivoReduccionGlobal);
		        		// Si almaceno el archivo OK envio mensaje a WORKER ENCARGADO
		        		enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_OK, "mensaje vacio", socket);
		        		log_info(archivoLog, "Se informa al WORKER ENCARGADO que el archivo de Reducción Global ha sido almacenado correctamente.");
		        	} else {
		        		log_error(archivoLog, "La ruta de directorios '%s' no es válida.", rutaDirectoriosArchivoReduccionGlobal);
		        		// Si no almaceno el archivo, envio mensaje de error a WORKER ENCARGADO
		        		enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR, "mensaje vacio", socket);
		        		log_error(archivoLog, "Se informa al WORKER ENCARGADO que ha fallado el almacenamiento del archivo de Reducción Global.");
		        	}
					// Libero recursos
					free(contenidoArchivoReduccionGlobal);
					free(rutaArchivoReduccionGlobal);
					free(rutaDirectoriosArchivoReduccionGlobal);
					eliminar_protocolo(mensaje);
				} else {
					log_error(archivoLog, "Hubo un error al recibir el mensaje 'CONTENIDO_ARCHIVO_REDUCCION_GLOBAL'.");
					free(rutaArchivoReduccionGlobal);
					free(rutaDirectoriosArchivoReduccionGlobal);
	        		// Si no almaceno el archivo, envio mensaje de error a WORKER ENCARGADO
	        		enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR, "mensaje vacio", socket);
	        		close(socket);
	        		FD_CLR(socket, &maestroFD);
					eliminar_protocolo(mensaje);
				}
        	}
        	break;
        case RUTA_ARCHIVO_A_PROCESAR :
        	log_info(archivoLog, "Se ha recibido el mensaje RUTA_ARCHIVO_A_PROCESAR desde el socket %d", socket);
        	// En protocolo->mensaje tengo la ruta completa con el nombre del archivo
        	int archivoExiste = validarArchivoYama(protocolo->mensaje);
        	// Si el archivo existe en mi file system continuo, si no devuelvo error
        	if (archivoExiste) {
        		// Parseo la ruta completa para saber cual es el ultimo directorio
        		char** directorios = string_split(protocolo->mensaje, "/");
        		// Cuento la cantidad de directorios que vienen
        		int cantidadEtiquetasDirectorios = contarDirectorios(directorios, 1);
        		// Tengo en cuenta que la anteultima etiqueta es el directorio que debo buscar
        		int indiceDirectorio = buscarIndiceDirectorio(directorios[cantidadEtiquetasDirectorios-1]);
        		// Recupero la informacion de como esta compuesto el archivo (bloques, nodos, etc.)
        		if (recuperarYEnviarDatosArchivo(directorios[cantidadEtiquetasDirectorios], indiceDirectorio, socket)) {
        			log_trace(archivoLog, "Envié toda la info del archivo a YAMA.");
        		}
	        	// Libero directorios
        		int j = 0;
        		while(directorios[j] != NULL) {
        			free(directorios[j]);
        			j++;
        		}
        		free(directorios);
        	} else {
        		log_error(archivoLog, "La ruta y archivo %s no existen en YAMA File System.", protocolo->mensaje);
        		enviar_mensaje(ARCHIVO_NO_EXISTE, "1", socket);
        	}
        	break;
        case SET_BLOQUE_SUCCESS :
        	// Acá debo enviarle una señal al proceso de que el bloque que acaba de enviar es correcto
        	log_trace(archivoLog, "Se recibió OK de SET_BLOQUE.");
        	sem_post(&set_bloque_OK);
        	break;
        case SET_BLOQUE_FAILURE :
        	// Acá debo enviarle una señal al proceso de que el bloque que acaba de enviar fallo
        	log_error(archivoLog, "Se recibió NO OK de SET_BLOQUE.");
        	sem_post(&set_bloque_NO_OK);
        	break;
        case GET_BLOQUE_SUCCESS :
        	// Acá debo enviarle una señal al proceso de que el get del bloque que acaba de pedir es correcto
        	log_trace(archivoLog, "Se recibió OK de GET_BLOQUE.");
        	// Copio el buffer que recibi en el contenido del bloque para dejarselo disponible al hilo consola
            contenidoBloque = calloc(protocolo->sizeMensaje, 1);
        	memcpy(contenidoBloque, protocolo->mensaje, protocolo->sizeMensaje);
        	sem_post(&get_bloque_OK);
        	break;
        case GET_BLOQUE_FAILURE :
        	// Acá debo enviarle una señal al proceso de que el get del bloque que acaba de pedir falló
        	log_error(archivoLog, "Se recibió NO OK de GET_BLOQUE.");
        	sem_post(&get_bloque_NO_OK);
        	break;
        case DATANODE_CONECTADO :

        	log_info(archivoLog, "Se ha recibido el mensaje DATANODE_CONECTADO desde el socket %d", socket);

        	// Parseo el mensaje y obtengo el nombre del nodo y la cantidad de bloques
        	char** datosNodo = string_split(protocolo->mensaje, ";");

        	// Imprimo los valores que recibi del nodo
        	log_info(archivoLog, "El nombre del nodo es %s.", datosNodo[0]); // datosNodo[0] = nombreNodo
        	log_info(archivoLog, "La cantidad de bloques que posee es %d.", atoi(datosNodo[1])); // datosNodo[1] = cantidadBloques
        	log_info(archivoLog, "El WORKER de este nodo escuchará conexiones de MASTERs en el puerto %d.", atoi(datosNodo[2])); // datosNodo[2] = PUERTO_WORKER_ESCUCHA_MASTER
        	log_info(archivoLog, "La IP del nodo %s es %s.", datosNodo[0], datosNodo[3]);

        	// Imprimo mensaje de conexión del nodo
          	log_info(archivoLog, "Se está intentando conectar el DataNode %s", datosNodo[0]);

        	//Me fijo si es un nodo de un estado anterior en caso de que exista el estado anterior
          	if (estadoCargado == true) {

          		// Estoy intentando reconstruir un estado anterior
            	int esNodoEstadoAnterior = verificarNodoEstadoAnterior(datosNodo[0]);

            	if (esNodoEstadoAnterior) {

            		log_trace(archivoLog, "Nodo %s perteneciente a un estado anterior reconocido en socket %d.", datosNodo[0], socket);
            		log_info(archivoLog, "Los datos de los bloques se asumen como válidos.");
            		enviarConexionAceptada(socket);
            		log_info(archivoLog, "Se ha enviado un mensaje de conexión aceptada al nodo %s por el socket %d", datosNodo[0], socket);

            		// Actualizo el nombre del nodo y el puertoEscuchaMaster en la lista de nodos conectados
            		actualizarNombreYPuertoNodo(listaDataNodes, datosNodo[0], atoi(datosNodo[2]), socket, datosNodo[3]); // datosNodo[0] = nombreNodo / datosNodo[2] = puertoEscuchaMaster

            		// Si no estoy en Estado Seguro, me fijo si el datanode que se me conectó alcanza para pasar a un Estado Seguro
            		if (estadoSeguro == false) {
            			estadoSeguro = verificarEstadoSeguro();
                  		// Si me devuelve TRUE es que con este nodo pasé a un estado seguro (lo informo)
						if (estadoSeguro == true) {
							log_trace(archivoLog, "Con la conexión del nodo %s el File System pasa a un Estado Seguro.", datosNodo[0]);
						} else {
							log_info(archivoLog, "Con la conexión del nodo %s aun no alcanza para pasar a un Estado Seguro.", datosNodo[0]);
						}
            		}
            	} else {
            		log_error(archivoLog, "Se rechaza al nodo %s por no pertenecer al estado anterior.", datosNodo[0]);
            		enviarConexionRechazada(socket);
            		close(socket);
            		FD_CLR(socket, &maestroFD);
            		log_info(archivoLog, "Se ha enviado un mensaje de conexión rechazada al nodo %s por el socket %d", datosNodo[0], socket);
            	}
          	} else {
          		// Si no estoy en estado seguro puedo seguir aceptando DataNodes
          		if (estadoSeguro == false) {
					// Es un nuevoDataNode: se acepta la nueva conexión
					log_trace(archivoLog, "Se acepta la conexión de un nuevo nodo %s en el socket %d", datosNodo[0], socket);
					generarBitMapBloquesVacio(datosNodo[0], atoi(datosNodo[1])); // datosNodo[1] = cantidadBloques
					actualizarTablaNodos(datosNodo[0], atoi(datosNodo[1]), atoi(datosNodo[1]));
					enviarConexionAceptada(socket);
					log_info(archivoLog, "Se ha enviado un mensaje de conexión aceptada al nodo %s por el socket %d", datosNodo[0], socket);
					// Actualizo el nombre del nodo y el puertoEscuchaMaster en la lista de nodos conectados
					actualizarNombreYPuertoNodo(listaDataNodes, datosNodo[0], atoi(datosNodo[2]), socket, datosNodo[3]); // datosNodo[0] = nombreNodo / datosNodo[2] = puertoEscuchaMaster
          		} else {
          			// Si estoy en estado seguro no puedo aceptar más datanodes y debo rechazarlo. SOLO ACEPTO DATANODES INTENTANDO RECONECTARSE.
          			int nodoPerteneceEstadoSeguro = verificarNodoEstadoSeguro(datosNodo[0]);

          			if (nodoPerteneceEstadoSeguro) {
          				log_trace(archivoLog, "Se reconoce al nodo %s en el socket %d por formar parte de los nodos de Estado Seguro ", datosNodo[0], socket);
          				enviarConexionAceptada(socket);
          				log_info(archivoLog, "Se ha enviado un mensaje de conexión aceptada al nodo %s por el socket %d", datosNodo[0], socket);
          				//actualizarTablaNodos(datosNodo[0], atoi(datosNodo[1]), atoi(datosNodo[1]));
          				// Actualizo el nombre del nodo y el puertoEscuchaMaster en la lista de nodos conectados
          				actualizarNombreYPuertoNodo(listaDataNodes, datosNodo[0], atoi(datosNodo[2]), socket, datosNodo[3]); // datosNodo[0] = nombreNodo / datosNodo[2] = puertoEscuchaMaster
          			} else {
          				log_error(archivoLog, "Se rechaza al nodo %s porque el File System ya se encuentra formateado.");
          				enviarConexionRechazada(socket);
          				close(socket);
          				FD_CLR(socket, &maestroFD);
          			}
          		}
          	}

        	// Libero recursos
        	free(datosNodo[0]);
        	free(datosNodo[1]);
        	free(datosNodo[2]);
        	free(datosNodo[3]);
        	free(datosNodo);

        	break;
        default:
        	puts ("Protocolo desconocido, se rechaza la conexión.");
        	break;
    }
}
