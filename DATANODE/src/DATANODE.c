/*
 ============================================================================
 Name        : DATANODE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h> //Biblioteca estandard para entrada y salida
#include <stdlib.h> //Manejo de memoria dinamica y otros
#include <string.h> //Biblioteca estandard para el manejo de strings
#include <stdbool.h> //Para incluir el tipo de dato bool, y usar true y false
#include <sys/socket.h> //Necesaria para socket() e inet_addr()
#include <netinet/in.h> //Necesaria para inet_addr()
#include <arpa/inet.h> //Necesaria para inet_addr()
#include <sys/types.h> //Necesaria para socket() y stat()
#include <unistd.h> //Necesaria para close() -> Para cerrar un socket
#include <fcntl.h>
#include <sys/stat.h>
#include <commons/config.h> //Necesaria para manipular el archivo de configuracion (config.txt)
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include <commons/string.h> //Biblioteca para el manejo de strings creada por la cátedra
#include <shared/protocolo.h>
#include <shared/sockets.h>
#include <shared/funcionesAuxiliares.h> //Necesaria para tamanioArchivo()
#include <shared/mensajes.h> // Biblioteca exclusiva para la gestión de mensajes
#include <shared/archivos.h> // Biblioteca para mapeo de archivos data.bin, get_bloque, set_bloque, etc...
#include <sys/mman.h> // Función mmap()
#include "funcionesDataNode.h"

/* Para crear el data.bin hay que hacer lo siguiente (por linea de comando)
 *
 * 1) truncate --size=10M data.bin
 * Esto genera un archivo de exactamente 10 MB
 *
 */


int main(void){

		/*
		===============================
		INICIO CONFIGURACION DATANODE
		===============================
		*/

		// Es la única variable t_protocolo* que usamos para recibir mensajes de FILESYSTEN
		t_protocolo* protocolo_Recepcion;

		// Variables auxiliares para trabajar con los bloques
		int numero_bloque;
		void* contenido_bloque;

		//Lo primero es crear el archivo de log (DATANODE.log) para registrar la traza de ejecución del proceso DATANODE
		log_DATANODE = log_create("DATANODE.log","DATANODE", true, LOG_LEVEL_TRACE);

		// DATANODE se conecta a FILE_SYSTEM, así que necesita conocer su IP y PUERTO por archivo de configuración
		int FILESYSTEM_PUERTO;
		char* FILESYSTEM_IP;

		/* Cada Nodo tiene un nombre (NOMBRE_NODO) y un archivo data.bin (RUTA_DATABIN), al cual acceden tanto el
		 * proceso DATANODE (en modo lectura/escritura) como el proceso WORKER (sólo en modo lectura). Además,
		 * necesito saber el tamaño del data.bin
		 */
		char* NODO;
		char* RUTA_DATABIN;
		char* IP_NODO;
		int TAMANIO_DATABIN;

		// Puerto en el cual el proceso WORKER correspondiente a este nodo escuchará conexiones de procesos MASTERs
		int PUERTO_WORKER_ESCUCHA_MASTER;

		//Levanto el archivo de configuración (config.txt) y cargo YAMA_IP y YAMA_PUERTO
		t_config* config = configuracion_DATANODE("config.txt", &FILESYSTEM_IP, &FILESYSTEM_PUERTO, &NODO, &PUERTO_WORKER_ESCUCHA_MASTER, &RUTA_DATABIN, &TAMANIO_DATABIN, &IP_NODO);

		/* Mapeo el archivo data.bin del Nodo en memoria, así ya lo tengo cargado para posteriores consultas.
		 * Como estamos en DATANODE, si el archivo data.bin NO existe entonces se crea.
		 */
		void* databin_mapeado = map_databin(RUTA_DATABIN, TAMANIO_DATABIN, DATANODE, log_DATANODE);

		// Calculo la cantidad de bloques que conforman el data.bin
		int tamanio_databin = tamanioArchivo(RUTA_DATABIN);
		int cantidad_bloques_databin = tamanio_databin/TAMANIO_BLOQUE_DATABIN;

		/*
		--------------------------------------------------
		FIN CONFIGURACION DATANODE
		--------------------------------------------------
		*/


		/*
		===============================================================
		INICIO ESTABLECIMIENTO DE CONEXION DE DATANODE A FILESYSTEM
		===============================================================
		*/

		t_direccion direccion_FILESYSTEM = nuevaDireccion(FILESYSTEM_PUERTO, FILESYSTEM_IP);
		int socket_DATANODE_FILESYSTEM = conectarseA(direccion_FILESYSTEM); // El socket de DATANODE se conecta a FILESYSTEM

		if ( socket_DATANODE_FILESYSTEM > 0 ) {

			log_info(log_DATANODE, "Se inicia la conexión con FILESYSTEM en el socket %d.", socket_DATANODE_FILESYSTEM);

			// Envio a FILESYSTEM la informacion de mi Nodo
			enviarInformacionNodo(socket_DATANODE_FILESYSTEM, DATANODE_CONECTADO, NODO, cantidad_bloques_databin, PUERTO_WORKER_ESCUCHA_MASTER, IP_NODO);

			// Espero la respuesta del FILESYSTEM de que fui aceptado
			protocolo_Recepcion = recibir_mensaje(socket_DATANODE_FILESYSTEM);

			// Si hay un error al recibir el header aborto
			if ( protocolo_Recepcion == NULL || protocolo_Recepcion == CERRARON_SOCKET){
				log_error(log_DATANODE, "Error al recibir el mensaje de aceptación de FILESYSTEM en el socket %d", socket_DATANODE_FILESYSTEM);
				close(socket_DATANODE_FILESYSTEM); // Cierro el socket pues hay un fallo
				return EXIT_FAILURE;
			}

			// Conexión establecida con FILESYSTEM
			if ( protocolo_Recepcion->funcion == DATANODE_ACEPTADO ) {
				log_trace(log_DATANODE, "Conexión establecida con FILESYSTEM en el socket %d.", socket_DATANODE_FILESYSTEM);
			} else {//if ( protocolo_Recepcion->funcion = DATANODE_RECHAZADO ) {
				log_error(log_DATANODE, "No se pudo establecer conexión con FILESYSTEM en el socket %d. FILESYSTEM rechazo la conexión.", socket_DATANODE_FILESYSTEM);
				close(socket_DATANODE_FILESYSTEM); // Cierro el socket pues hay un fallo
				return EXIT_FAILURE;
			}

			// Libero los recursos del protocolo recibido
			eliminar_protocolo(protocolo_Recepcion);
		}
		else{ // Si conectarseA devuelve un socket de valor negativo...
			log_error(log_DATANODE, "No se pudo enviar el mensaje de conexión a FILESYSTEM debido a un problema en este DATANODE.");
			return EXIT_FAILURE;
		}

		/*
		------------------------------------------------------------------------------------------------------------------------
		FIN ESTABLECIMIENTO DE CONEXION DE DATANODE A FILESYSTEM (En este punto DATANODE ya está conectado al FILESYSTEM)
		------------------------------------------------------------------------------------------------------------------------
		*/


		/*
		===========================================================================================================
		INICIO DATANODE PROCESA SOLICITUDES DE FILESYSTEM

		El FILESYSTEM podrá solicitar las siguientes DOS operaciones:
			- getBloque(numero): Devolverá el contenido del bloque solicitado almacenado en el Espacio de Datos.
			- setBloque(numero, [datos]): Grabará los datos enviados en el bloque solicitado del Espacio de Datos

		===========================================================================================================
		*/

		/* Si bien habrán N DATANODES, el FILESYSTEM es único, por lo tanto, para cada DATANODE la comunicación con el FILESYSTEM
		 * es una relación de 1 a 1 (NO es un servidor).
		 */

		while(true) {

			/* ATIENDO PEDIDOS DEL FILESYSTEM */

			protocolo_Recepcion = recibir_mensaje(socket_DATANODE_FILESYSTEM); // Se bloquea hasta recibir un mensaje de FILESYSTEM

			if ( protocolo_Recepcion == CERRARON_SOCKET){
				log_error(log_DATANODE, "FILESYSTEM cerro la conexión en el socket %d", socket_DATANODE_FILESYSTEM);
				break;
			}else if ( protocolo_Recepcion == NULL){
				log_error(log_DATANODE, "Hubo un error al recibir un mensaje enviado por FILESYSTEM en el socket %d", socket_DATANODE_FILESYSTEM);
			}else{ // INICIO else switch

					// INICIO SWITCH
				switch (protocolo_Recepcion->funcion){

					/*
					===================
					INICIO GET_BLOQUE
					===================
					*/
					case GET_BLOQUE: // FILESYSTEM me solicita leer el contenido de un determinado bloque

						/* El contenido del mensaje que envia FILESYSTEM a DATANODE para get_bloque es el número de
						 * bloque que quiere leer (1).
						 */
						log_info(log_DATANODE, "Recibí el mensaje GET_BLOQUE desde el socket %d", socket_DATANODE_FILESYSTEM);
						numero_bloque = atoi((char*)protocolo_Recepcion->mensaje);

						// Obtengo el contenido del bloque del archivo data.bin mapeado
						contenido_bloque = get_bloque(databin_mapeado, numero_bloque);

						// Compruebo errores al obtener el bloque
						if ( contenido_bloque == NULL){
							enviar_mensaje(GET_BLOQUE_FAILURE, "MENSAJE VACIO", socket_DATANODE_FILESYSTEM);
						}else{
							// Si lei el bloque correctamente, entonces envio su contenido al FILESYSTEM
							enviar_mensaje(GET_BLOQUE_SUCCESS, (char*)contenido_bloque, socket_DATANODE_FILESYSTEM);
							log_trace(log_DATANODE, "Acabo de enviar el contenido del bloque solicitado."); // Para pruebas - SE PUEDE BORRAR
						}

						// Devuelva NULL o el contenido del bloque, get_bloque hace malloc(TAMANIO_BLOQUE_DATABIN), así que debemos hacer el free correspondiente
						liberar_bloque(contenido_bloque);

					break;

					/*
					----------------
					FIN GET_BLOQUE
					----------------
					*/

					/*
					===================
					INICIO SET_BLOQUE
					===================
					*/

					// FILESYSTEM me solicita grabar contenido (que me envia) en un determinado bloque

					case SET_BLOQUE_NUMERO: // Primero recibo el número de bloque

						/* El contenido del mensaje que envia FILESYSTEM a DATANODE para set_bloque es el contenido a grabar y el
						 * número de bloque donde guardarlo => Necesito que FILESYSTEM me envie (2) mensajes: El primero con el número
						 * de bloque y el segundo con el contenido a grabar en dicho bloque.
						 */

						/* En el primer mensaje tengo el número de bloque donde guardar los datos. Como el campo mensaje es un void*
						 * lo casteo a char* y luego lo convierto a tipo int.
						 */
						numero_bloque = atoi((char*)protocolo_Recepcion->mensaje);

						log_info(log_DATANODE, "Recibí SET_BLOQUE_NUMERO para el bloque %d en el socket %d", numero_bloque, socket_DATANODE_FILESYSTEM);
						// Libero la memoria asignada al primer protocolo para recibir el segundo mensaje
						eliminar_protocolo(protocolo_Recepcion);

						// Recibo el segundo mensaje, donde FILESYSTEM me envia el contenido a grabar en el bloque numero_bloque
						protocolo_Recepcion = recibir_mensaje(socket_DATANODE_FILESYSTEM);

						// Compruebo errores al recibir el contenido del bloque
						if ( protocolo_Recepcion == NULL || protocolo_Recepcion == CERRARON_SOCKET){
							enviar_mensaje(SET_BLOQUE_FAILURE, "MENSAJE VACIO", socket_DATANODE_FILESYSTEM);
						}else if ( protocolo_Recepcion->funcion == SET_BLOQUE_CONTENIDO ) { // AHora recibo el contenido del bloque
							// Si recibi correctamente el mensaje con el contenido del bloque entonces lo guardo y envio el mensaje de confirmación a FILESYSTEM
							log_info(log_DATANODE, "Recibí SET_BLOQUE_CONTENIDO para el bloque %d en el socket %d", numero_bloque, socket_DATANODE_FILESYSTEM);
							if ( set_bloque(databin_mapeado, numero_bloque, protocolo_Recepcion->mensaje) == OK ){
								enviar_mensaje(SET_BLOQUE_SUCCESS, "MENSAJE VACIO", socket_DATANODE_FILESYSTEM);
								log_trace(log_DATANODE, "Pude grabar el bloque correctamente."); // Para pruebas - SE PUEDE BORRAR
							}
						}

					break;

					/*
					----------------
					FIN SET_BLOQUE
					----------------
					*/

					default:
						break;


					} // FIN switch

			} // FIN else switch

			// Libero los recursos del protocolo recibido
			eliminar_protocolo(protocolo_Recepcion);

		} // Fin while(true)

		/*
		---------------------------------------------------
		FIN DATANODE PROCESA SOLICITUDES DEL FILESYSTEM
		---------------------------------------------------
		*/


		/*
		===================================================
		INICIO CIERRE Y LIBERACION DE RECURSOS GENERALES
		===================================================
		*/

		unmap_databin(RUTA_DATABIN, databin_mapeado, log_DATANODE); // Libero el mapeo del data.bin
		close(socket_DATANODE_FILESYSTEM); // Cierro el socket_DATANODE_FILESYSTEM y libero sus recursos
		config_destroy(config); // Libero la memoria utilizada por las funciones del archivo de configuración
		log_destroy(log_DATANODE); // Libero la memoria utilizada por las funciones de log

		/*
		-----------------------------------------------
		FIN CIERRE Y LIBERACION DE RECURSOS GENERALES
		-----------------------------------------------
		*/

		return EXIT_SUCCESS;

}
