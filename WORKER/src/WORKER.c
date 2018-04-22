/*
 ============================================================================
 Name        : WORKER.c
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
#include <sys/mman.h> // Para mmap
#include <sys/stat.h>
#include <signal.h>
#include <commons/config.h> //Necesaria para manipular el archivo de configuracion (config.txt)
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include <commons/string.h> //Biblioteca para el manejo de strings creada por la cátedra
#include <shared/sockets.h>
#include <shared/protocolo.h>
#include <sys/wait.h>
#include <shared/funcionesAuxiliares.h> //Necesaria para tamanioArchivo()
#include <shared/mensajes.h> // Biblioteca exclusiva para la gestión de mensajes
#include <shared/archivos.h> // Biblioteca para mapeo de archivos data.bin, get_bloque, set_bloque, etc...
#include "funcionesWorker.h"


/***** INICIO MAIN *****/
int main(int argc, char* argv[]) {

		/*
		===================================
		INICIO CONFIGURACION WORKER
		===================================
		*/

		//Lo primero es crear el archivo de log (WORKER.log) para registrar la traza de ejecución del proceso WORKER
		log_WORKER = log_create("WORKER.log","WORKER", true, LOG_LEVEL_TRACE);

		// WORKER (N) espera conexiones de MASTER (N), por lo que debe tener un puerto de escucha
		int PUERTO_ESCUCHA_MASTER;//PUERTO_WORKER_ESCUCHA_MASTER
		char* RUTA_DATABIN;

		// WORKER (N) se conecta a FILE_SYSTEM para enviar el archivo final de la ejecución de un JOB, así que debe conocer su ip y puerto
		int PUERTO_FILE_SYSTEM;
		char* IP_FILE_SYSTEM;

		// Levanto el archivo de configuración (config.txt) y cargo la RUTA del archivo data.bin y el PUERTO de escucha para procesos MASTER
		t_config* config = configuracion_WORKER("config.txt", &RUTA_DATABIN, &PUERTO_ESCUCHA_MASTER, &PUERTO_FILE_SYSTEM, &IP_FILE_SYSTEM);

		/* Mapeo el archivo data.bin del Nodo en memoria, así ya lo tengo cargado para posteriores consultas.
		 * IMPORTANTE: Si soy un WORKER y el archivo data.bin no existe NO puedo crearlo (eso le corresponde al DATANODE).
		 */
		void* databin_mapeado = map_databin(RUTA_DATABIN, 0, WORKER, log_WORKER);
		if (databin_mapeado == NULL){
			return EXIT_FAILURE;
		}

		// Inicializo las variables globales para los nombres temporales
		numero_script_trans = 0;
		numero_script_reduc = 0;

		// Variable para recibir el primer mensaje de cada MASTER
		t_protocolo* protocolo_Recepcion;

		// Variables para el forkeo
		int pid;

		// Creo la carpeta TEMPORALES donde voy a guardar archivos temporales, scripts, resultados de transformaciones, etc...
		//mkdir("temporales", S_IRWXU);

		// Variables para case ETAPA_TRANSFORMACION
		char* ruta_script_transformador;

		// Variables para case ETAPA_REDUCCION_LOCAL
		char* ruta_script_reductor_local;
		int cantidad_temporales_trans;

		// Variables para case NODO_AYUDANTE

		// Variables para case NODO_ENCARGADO
		void* buffer_contenido_archivo_reduccion_global;

		// Variables para ENVIO ARCHIVO FINAL A FILE_SYSTEM
		t_direccion direccion_FILE_SYSTEM;


		/*
		--------------------------------------------------
		FIN CONFIGURACION WORKER
		--------------------------------------------------
		*/


		/*
		=============================================
			INICIO PROCESAMIENTO SOLICITUDES
		=============================================
		*/

		struct sockaddr_in direccion_WORKER = nuevaDireccion(PUERTO_ESCUCHA_MASTER, "INADDR_ANY");
		int socket_escucha_MASTER = ponerseALaEscuchaEn(direccion_WORKER);

		struct sockaddr_in direccion_master;

		while(true){ // INICIO while(true)

			// Gestiono conexiones de nuevos clientes (nuevos procesos MASTER)
			log_trace(log_WORKER, "WORKER esta a la escucha de solicitudes.");
			int socket_cliente = aceptar_clienteEn(socket_escucha_MASTER, direccion_master);
			if ( socket_cliente < 0 ){
				log_error(log_WORKER, "Hubo un ERROR al aceptar una conexión entrante.");
			}else{

				/*
				=============================================
						INICIO - WORKER RECIBE PEDIDOS
				=============================================
				*/


				// Sino hubo un error en el accept se acepto la conexión de un NUEVO MASTER
				log_trace(log_WORKER, "Se acepto un nuevo cliente el socket %d", socket_cliente);

				/*
				 * RECIBO EL PRIMER MENSAJE DENTRO DEL PADRE.
				 */
 				protocolo_Recepcion = recibir_mensaje(socket_cliente);

				switch (protocolo_Recepcion->funcion){

						/*
						=============================================
								INICIO - ETAPA TRANSFORMACION
						=============================================
						*/

						case ETAPA_TRANSFORMACION:

							// Me llego un nuevo pedido de MASTER así que me forkeo para atenderlo
							pid = fork();

							if( pid == 0){ 	//****************** INICIO - ESTOY EN EL HIJO ******************

								// INICIO - Recibo la información del bloque a transformar y la persisto en una estructura del tipo t_info_trans

								t_protocolo* protocolo_Recepcion_HIJO;
								t_info_trans* info_trans_actual = malloc(sizeof(t_info_trans));

								// 1° Recibo el script transformador
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == SCRIPT_TRANSFORMADOR){

									// Primero me guardo el contenido del script transformador en un archivo y me quedo con la ruta del mismo
									ruta_script_transformador = script_trans_a_archivo(protocolo_Recepcion_HIJO->mensaje);

									// Ahora copia la ruta donde se almaceno el script transformador localmente
									info_trans_actual->ruta_script_trans = malloc(strlen(ruta_script_transformador) + 1); // + 1 del '\0'
									strcpy(info_trans_actual->ruta_script_trans, ruta_script_transformador);

									// Libero la memoria alocada para la ruta del script transformador pues ya la guarde
									free(ruta_script_transformador);

								}else{

									log_error(log_WORKER, "Error al recibir el script transformador en una solicitud del MASTER del socket %d", socket_cliente);

									// Le aviso a MASTER que la transformación falló
									enviar_mensaje(TRANSFORMACION_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo

								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje


								// 2° Recibo el número de bloque del archivo data.bin a transformar
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == BLOQUE_DATABIN_A_TRANSFORMAR){
									info_trans_actual->nro_bloque_databin = atoi(protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir el numero del bloque en una solicitud del MASTER del socket %d", socket_cliente);

									// Le aviso a MASTER que la transformación falló
									enviar_mensaje(TRANSFORMACION_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje


								// 3° Recibo los bytes ocupados por dicho bloque (cuanto del 1MiB posible)
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == BYTES_OCUPADOS_BLOQUE_A_TRANSFORMAR){
									info_trans_actual->bytes_ocupados = atoi(protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir los bytes ocupados en una solicitud del MASTER del socket %d", socket_cliente);

									// Le aviso a MASTER que la transformación falló
									enviar_mensaje(TRANSFORMACION_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje


								// 4° Recibo el nombre temporal para el resultado de transformar el bloque
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_ARCHIVO_TEMPORAL_TRANSFORMACION){
									info_trans_actual->nombre_temp_trans = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '0\'
									strcpy( info_trans_actual->nombre_temp_trans, protocolo_Recepcion_HIJO->mensaje );
								}else{
									log_error(log_WORKER, "Error al recibir el nombre del archivo temporal en una solicitud del MASTER del socket %d", socket_cliente);

									// Le aviso a MASTER que la transformación falló
									enviar_mensaje(TRANSFORMACION_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje

								// FIN - Recibo la información del bloque a transformar

								// Aplico la transformación sobre el archivo con el bloque
								int resultado_transformacion = aplicar_transformacion(info_trans_actual, databin_mapeado);
								log_info(log_WORKER,"El valor devuelto por system es %d", resultado_transformacion);

								// Libero memoria
								free(info_trans_actual->ruta_script_trans);
								free(info_trans_actual->nombre_temp_trans);
								free(info_trans_actual);

								// Informo a MASTER del resultado de la transformación
								if ( resultado_transformacion == 0){
									log_trace(log_WORKER, "Se concreto una transformación en forma exitosa.");
									enviar_mensaje(TRANSFORMACION_OK, "Mensaje vacio", socket_cliente);
								}else{
									log_error(log_WORKER, "Fallo una transformación.");
									enviar_mensaje(TRANSFORMACION_ERROR, "Mensaje vacio", socket_cliente);
								}

								// PRUEBA
								//close(socket_cliente);

								exit(0); // Mato al proceso hijo porque ya termino su procesamiento

							} //****************** FIN - ESTOY EN EL HIJO ******************

							break;

						/*
						--------------------------------------------------
									FIN - ETAPA TRANSFORMACION
						--------------------------------------------------
						*/


						/*
						=============================================
								INICIO - ETAPA REDUCCION LOCAL
						=============================================
						*/

						case ETAPA_REDUCCION_LOCAL:

							// Me llego un nuevo pedido de MASTER así que me forkeo para atenderlo
							pid = fork();

							if( pid == 0){ 	//****************** INICIO - ESTOY EN EL HIJO ******************

								// INICIO - Recibo la información del Nodo a reducir localmente y la guardo en una estructura t_info_reduc_local

								t_protocolo* protocolo_Recepcion_HIJO;
								t_info_reduc_local* info_reduc_local = malloc(sizeof(t_info_reduc_local));

								// 1° Recibo el script reductor

								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == SCRIPT_REDUCTOR_LOCAL){

									// Primero me guardo el contenido del script reductor en un archivo y me quedo con la ruta del mismo
									ruta_script_reductor_local = script_reduc_a_archivo(protocolo_Recepcion_HIJO->mensaje);

									// Ahora copio la ruta donde se almaceno el script reductor localmente
									info_reduc_local->ruta_script_reduc_local = malloc(strlen(ruta_script_reductor_local) + 1); // + 1 del '\0'
									strcpy(info_reduc_local->ruta_script_reduc_local, ruta_script_reductor_local);

									// Libero la memoria alocada para la ruta del script reductor pues ya la guarde
									free(ruta_script_reductor_local);

								}else{
									log_error(log_WORKER, "Error al recibir el script reductor (local) en una solicitud del MASTER del socket %d", socket_cliente);

									// Le aviso a MASTER que la reducción local falló
									enviar_mensaje(REDUCCION_LOCAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje


								// 2° Recibo la lista de nombres temporales a reducir localmente

								// 2a - Primer recibo la cantidad de temporales a reducir localmente
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == CANTIDAD_TEMPORALES_TRANS ){
									cantidad_temporales_trans = atoi(protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir la cantidad de temporales a reducir del MASTER en el socket %d", socket_cliente);

									// Le aviso a MASTER que la reducción local falló
									enviar_mensaje(REDUCCION_LOCAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO); // Libero el protocolo para recibir el siguiente mensaje

								// 2b - Recibo, uno a uno, los nombres de los temporales a reducir localmente

								info_reduc_local->lista_temp_trans = list_create(); // Creo la lista

								int i;
								for(i=0; i < cantidad_temporales_trans; i++){

									protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);

									// Creo una estructura del tipo t_temp_trans donde voy a guardar el nombre del temporal a reducir localmente
									t_temp_trans* nuevo_temp_trans = malloc(sizeof(t_temp_trans));
									nuevo_temp_trans->nombre_temp_trans = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
									strcpy(nuevo_temp_trans->nombre_temp_trans, protocolo_Recepcion_HIJO->mensaje);

									// Añado el nuevo nombre temporal a reducir localmente a la lista
									list_add(info_reduc_local->lista_temp_trans, nuevo_temp_trans);

									// Libero el protocolo para la siguiente iteración del for o para el siguiente mensaje
									eliminar_protocolo(protocolo_Recepcion_HIJO);

								} // FIN for (recibir la lista de temporales)

								// 3 - Recibo el nombre temporal con el que se va a guardar el resultado de la reduccion local
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_TEMPORAL_REDUCCION_LOCAL){
									info_reduc_local->nombre_temp_reduc_local = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
									strcpy(info_reduc_local->nombre_temp_reduc_local, protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir el nombre para guardar el resultado de una reduccion local del MASTER en el socket %d", socket_cliente);

									// Le aviso a MASTER que la reducción local falló
									enviar_mensaje(REDUCCION_LOCAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}

								// FIN - Recibo la información del Nodo a reducir localmente

								// Aplico la reduccion local sobre los temporales del Nodo
								int resultado_reduccion_local = aplicar_reduccion_local(info_reduc_local);
								log_info(log_WORKER,"El valor devuelto por system es %d", resultado_reduccion_local);

								// Libero memoria
								free(info_reduc_local->ruta_script_reduc_local);
								free(info_reduc_local->nombre_temp_reduc_local);
								liberar_lista_temp_trans(info_reduc_local->lista_temp_trans);
								free(info_reduc_local);


								// Informo a MASTER del resultado de la transformación
								if ( resultado_reduccion_local == 0){
									log_trace(log_WORKER, "Se concreto una reducción local en forma exitosa.");
									enviar_mensaje(REDUCCION_LOCAL_OK, "Mensaje vacio", socket_cliente);
								}else{
									// Le aviso a MASTER que la reducción local falló
									enviar_mensaje(REDUCCION_LOCAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0); // Mato al proceso hijo
								}

								exit(0); // Mato al proceso hijo porque ya termino su procesamiento

							} //****************** FIN - ESTOY EN EL HIJO ******************

							break;


						/*
						--------------------------------------------------
									FIN - ETAPA REDUCCION_LOCAL
						--------------------------------------------------
						*/


						/*
						===============================================================
								INICIO - WORKER AYUDANTE (ETAPA REDUCCION GLOBAL
						===============================================================
						*/

						case WORKER_AYUDANTE: // Me eligieron para ser un WORKER AYUDANTE

							// Me llego un nuevo pedido así que me forkeo para atenderlo
							pid = fork();

							if ( pid == 0 ){ //****************** INICIO - ESTOY EN EL HIJO ******************

								// Declaro variables temporales
								t_protocolo* protocolo_Recepcion_HIJO;
								char* nombre_archivo_temporal;
								void* buffer_contenido_archivo_temporal;

								// 1° - Recibo el nombre del archivo temporal (de la reducción local)
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_TMP_NODO_AYUDANTE ){
									nombre_archivo_temporal = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // +1 del barra '\0'
									strcpy(nombre_archivo_temporal, protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir el nombre del archivo temporal a enviar al WORKER encargado.");
									enviar_mensaje(NOMBRE_TMP_NODO_AYUDANTE_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);

								// 2° - Abro el archivo temporal y cargo su contenido en un buffer
								buffer_contenido_archivo_temporal = map_archivo(nombre_archivo_temporal);

								// 4° - Enviar el buffer al WORKER encargado
								enviar_mensaje(CONTENIDO_TMP_NODO_AYUDANTE, buffer_contenido_archivo_temporal, socket_cliente);

								// Libero recursos
								unmap_archivo(nombre_archivo_temporal, buffer_contenido_archivo_temporal, log_WORKER);
								free(nombre_archivo_temporal);

								exit(0); // Mato al proceso hijo porque ya termino su procesamiento

							} //****************** FIN - ESTOY EN EL HIJO ******************

							break;

						/*
						-------------------------------------------------------------
									FIN - WORKER AYUDANTE (ETAPA REDUCCION GLOBAL
						-------------------------------------------------------------
						*/


						/*
						=============================================================
								INICIO - WORKER ENCARGADO (ETAPA REDUCCION GLOBAL)
						=============================================================
						*/

						case WORKER_ENCARGADO: // Eligieron a este WORKER como ENCARGADO de la REDUCCION GLOBAL

							// Me llego un nuevo pedido así que me forkeo para atenderlo
							pid = fork();

							if( pid == 0){ 	//****************** INICIO - ESTOY EN EL HIJO ******************

								// INICIO - Recibo la información para realizar la REDUCCIÓN GLOBAL

								// Declaro variables temporales para conectarme a los Nodos ayudantes
								t_protocolo* protocolo_Recepcion_HIJO; // Protocolo para recibir mensajes del hijo
								t_info_nodo_ayudante* info_nodo_ayudante; // Estructura para guardar info de cada Nodo ayudante
								t_direccion direccion_NODO_AYUDANTE; // Direccion del Nodo ayudante al cual me voy a conectar
								int socket_WORKER_ENCARGADO_AYUDANTE;
								int cantidad_nodos_ayudantes;
								t_list* lista_nombres_temporales = list_create();
								char* ruta_script_reduccion_global;
								char* ruta_nombre_resultado_reduccion_global;
								char* ruta_archivo_final;

								// 1° - Recibo la cantidad de Nodos ayudantes
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == CANTIDAD_NODOS_AYUDANTES ){
									cantidad_nodos_ayudantes = atoi(protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir la cantidad de nodos ayudantes de la reducción global.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);

								// 2° - Recibo, uno a uno, la información de cada Nodo ayudante
								int indice_nodo_ayudante;
								for(indice_nodo_ayudante = 0; indice_nodo_ayudante < cantidad_nodos_ayudantes; indice_nodo_ayudante++){

									// Reservo memoria para un t_info_nodo_ayudante
									info_nodo_ayudante = malloc(sizeof(t_info_nodo_ayudante));

									// Recibo la IP del Nodo ayudante
									protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
									if ( protocolo_Recepcion_HIJO->funcion == IP_NODO_AYUDANTE ){
										info_nodo_ayudante->ip = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
										strcpy(info_nodo_ayudante->ip, protocolo_Recepcion_HIJO->mensaje);
									}else{
										log_error(log_WORKER, "Error al recibir la IP de un Nodo ayudante.");
										enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
										exit(0);
									}
									eliminar_protocolo(protocolo_Recepcion_HIJO);

									// Recibo el PUERTO del Nodo ayudante
									protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
									if ( protocolo_Recepcion_HIJO->funcion == PUERTO_NODO_AYUDANTE ){
										info_nodo_ayudante->puerto_worker = atoi(protocolo_Recepcion_HIJO->mensaje);
									}else{
										log_error(log_WORKER, "Error al recibir  el PUERTO de un Nodo ayudante.");
										enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
										exit(0);
									}
									eliminar_protocolo(protocolo_Recepcion_HIJO);

									// Recibo el nombre del archivo temporal de la reducción local
									protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
									if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_REDUCCION_LOCAL_NODO_AYUDANTE ){
										info_nodo_ayudante->nombre_temporal_reduccion = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
										strcpy(info_nodo_ayudante->nombre_temporal_reduccion, protocolo_Recepcion_HIJO->mensaje);

										// Además, consisto el nombre del archivo temporal en una lista para la REDUCCION GLOBAL
										t_nombre_temporal_reducido* t_nombre_temporal_actual = malloc(sizeof(t_nombre_temporal_reducido));
										t_nombre_temporal_actual->nombre_temporal_reducido = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
										strcpy(t_nombre_temporal_actual->nombre_temporal_reducido, protocolo_Recepcion_HIJO->mensaje);

										list_add(lista_nombres_temporales, t_nombre_temporal_actual);

									}else{
										log_error(log_WORKER, "Error al recibir el PUERTO de un Nodo ayudante.");
										enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
										exit(0);
									}
									eliminar_protocolo(protocolo_Recepcion_HIJO);

									// 3° - Me conecto al Nodo ayudante
									direccion_NODO_AYUDANTE = nuevaDireccion(info_nodo_ayudante->puerto_worker, info_nodo_ayudante->ip);

									socket_WORKER_ENCARGADO_AYUDANTE = conectarseA(direccion_NODO_AYUDANTE);
									if ( socket_WORKER_ENCARGADO_AYUDANTE > 0 ) {
										log_trace(log_WORKER, "Se estableció conexión con un WORKER AYUDANTE en el socket %d.", socket_WORKER_ENCARGADO_AYUDANTE);
									}else{
										log_error(log_WORKER, "No se pudo establecer conexión con un WORKER AYUDANTE en el socket %d.", socket_WORKER_ENCARGADO_AYUDANTE);
										enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
										exit(0); // Mato al proceso hijo porque hay un error irrecuperable
									}

									// 4° - Envio al Nodo ayudante el mensaje de que inicie su cooperación para la etapa de reducción global
									enviar_mensaje(WORKER_AYUDANTE, "Mensaje vacio", socket_WORKER_ENCARGADO_AYUDANTE);

									// 5° - Envio al Nodo ayudante el nombre del archivo temporal de la reducción local
									enviar_mensaje(NOMBRE_TMP_NODO_AYUDANTE, info_nodo_ayudante->nombre_temporal_reduccion, socket_WORKER_ENCARGADO_AYUDANTE);

									// 6° - Espero la respuesta del Nodo ayudante, que tiene que ser el contenido del archivo temporal de la reducción local
									protocolo_Recepcion_HIJO = recibir_mensaje(socket_WORKER_ENCARGADO_AYUDANTE);

									// 7° - Grabo el contenido del archivo reducido en un un archivo temporal con el mismo nombre
									if ( protocolo_Recepcion_HIJO->funcion == CONTENIDO_TMP_NODO_AYUDANTE ){
										persistir_buffer_en_archivo(info_nodo_ayudante->nombre_temporal_reduccion, protocolo_Recepcion_HIJO->mensaje);
									}else{
										log_error(log_WORKER, "Error al recibir el contenido del archivo de reducción local de un Nodo ayudante.");
										enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
										exit(0);
									}

									// Libero recursos para la siguiente iteración
									free(info_nodo_ayudante->ip);
									free(info_nodo_ayudante->nombre_temporal_reduccion);
									free(info_nodo_ayudante);
									eliminar_protocolo(protocolo_Recepcion_HIJO);

								} // FIN for - recibir info de cada Nodo ayudante

								// 8° - Recibo el nombre del archivo temporal de reducción local PROPIO (YA QUE SOY EL WORKER ENCARGADO) y lo agrego a la lista de temporales
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_REDUCCION_LOCAL_NODO_ENCARGADO ){
									t_nombre_temporal_reducido* t_nombre_archivo_temporal_encargado = malloc(sizeof(t_nombre_archivo_temporal_encargado));
									t_nombre_archivo_temporal_encargado->nombre_temporal_reducido = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // + 1 del '\0'
									strcpy(t_nombre_archivo_temporal_encargado->nombre_temporal_reducido, protocolo_Recepcion_HIJO->mensaje);
									list_add(lista_nombres_temporales, t_nombre_archivo_temporal_encargado);
								}else{
									log_error(log_WORKER, "Error al recibir el nombre del archivo temporal de reducción local del WORKER ENCARGADO.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);

								// 9° - Recibo el script de reducción global
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == SCRIPT_REDUCCION_GLOBAL ){
									ruta_script_reduccion_global = script_reduc_a_archivo(protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir el script de reducción global.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);

								// 10° - Recibo la ruta donde debo almacenar el archivo resultado de la reducción global
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == NOMBRE_RESULTADO_REDUCCION_GLOBAL ){
									ruta_nombre_resultado_reduccion_global = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // +1 del '\0'
									strcpy(ruta_nombre_resultado_reduccion_global, protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir el nombre del archivo resultado de la reducción global.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);

								// 11° - Recibo de MASTER la RUTA FINAL con la que se guardará el resultado de la reducción global en FILE_SYSTEM
								protocolo_Recepcion_HIJO = recibir_mensaje(socket_cliente);
								if ( protocolo_Recepcion_HIJO->funcion == RUTA_ARCHIVO_FINAL_FS ){
									ruta_archivo_final = malloc(strlen(protocolo_Recepcion_HIJO->mensaje) + 1); // +1 del '\0'
									strcpy(ruta_archivo_final, protocolo_Recepcion_HIJO->mensaje);
								}else{
									log_error(log_WORKER, "Error al recibir la ruta donde se guardará el archivo final en yamafs.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
									exit(0);
								}
								eliminar_protocolo(protocolo_Recepcion_HIJO);


								// FIN - Recibo la información para realizar la reducción global

								/* APLICO LA REDUCCION GLOBAL: Llegado a este punto cuento con toda la info necesaria para poder
								 * aplicar la reducción global del job.
								 */
								int resultado_reduccion_global = aplicar_reduccion_global(lista_nombres_temporales, ruta_script_reduccion_global, ruta_nombre_resultado_reduccion_global);
								log_info(log_WORKER,"El valor devuelto por system es %d", resultado_reduccion_global);

								/* Si la REDUCCIÓN GLOBAL salió bien le informo a MASTER y también envió el archivo
								 * final a FILE_SYSTEM para su almacenamiento.
								 */
								if ( resultado_reduccion_global == 0){

									log_trace(log_WORKER, "Se concreto una reducción global en forma exitosa.");

									// Le aviso a MASTER que la reducción global se hizo correctamente
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_OK, "Mensaje vacio", socket_cliente);

									/*
									===========================================================================
											INICIO - WORKER ENCARGADO ENVIA ARCHIVO FINAL A FILE_SYSTEM
									===========================================================================
									*/


									// Me conecto a FILE_SYSTEM
									direccion_FILE_SYSTEM = nuevaDireccion(PUERTO_FILE_SYSTEM, IP_FILE_SYSTEM);

									int socket_WORKER_FS = conectarseA(direccion_FILE_SYSTEM);
									if ( socket_WORKER_FS > 0 ) {
										log_trace(log_WORKER, "Se estaleció conexión con FILE_SYSTEM en el socket %d", socket_WORKER_FS);
									}
									else{
										log_error(log_WORKER, "No se pudo establecer conexión con FILE_SYSTEM.");
										log_info(log_WORKER, "Se aborta el envío del archivo final a FILE_SYSTEM");
										enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR, "Mensaje vacio", socket_cliente);
										exit(0); // Salgo del proceso hijo
									}

									// Le envio la ruta donde quiero guardar el resultado final en yamafs (NOMBRE FINAL DEL ARCHIVO)
									enviar_mensaje(ALMACENAR_ARCHIVO_REDUCCION_GLOBAL, ruta_archivo_final, socket_WORKER_FS);

									// Le envio el contenido del archivo final
									buffer_contenido_archivo_reduccion_global = map_archivo(ruta_nombre_resultado_reduccion_global);
									enviar_mensaje(CONTENIDO_ARCHIVO_REDUCCION_GLOBAL, buffer_contenido_archivo_reduccion_global, socket_WORKER_FS);
									//unmap_archivo(ruta_nombre_resultado_reduccion_global, buffer_contenido_archivo_reduccion_global, log_WORKER);

									// Espero la confirmación de FILE_SYSTEM para avisarle a MASTER que la etapa de ALMACENAMIENTO FINAL se completo correctamente
									protocolo_Recepcion_HIJO = recibir_mensaje(socket_WORKER_FS);
									if ( protocolo_Recepcion_HIJO->funcion == ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_OK ){

										// Le aviso a MASTER que el almacenamiento final se realizo con éxito
										enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_OK, "Mensaje vacio", socket_cliente);
										log_info(log_WORKER, "Le comunico a MASTER que el archivo resultado de la reducción global se ha persistido correctamente en FILE_SYSTEM");

									}else{

										// Le aviso a MASTER que el almcenamiento final fallo
										enviar_mensaje(ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR, "Mensaje vacio", socket_cliente);
										log_info(log_WORKER, "Le comunico a MASTER que el archivo resultado de la reducción global NO se ha podido guardar en FILE_SYSTEM.");

									}
									eliminar_protocolo(protocolo_Recepcion_HIJO);

									/*
									---------------------------------------------------------------------------------
												FIN - WORKER ENCARGADO ENVIA ARCHIVO FINAL A FILE_SYSTEM
									---------------------------------------------------------------------------------
									*/

								}else{ // Si la REDUCCIÓN GLOBAL falló le aviso a MASTER

									log_trace(log_WORKER, "Fallo una etapa de reducción global.");
									enviar_mensaje(ETAPA_REDUCCION_GLOBAL_ERROR, "Mensaje vacio", socket_cliente);
								}

								/* OBSERVACION: Por supuesto, si la etapa de reducción global falla se aborta el job
								 * y el WORKER encargado NO llega a conectarse a FILE_SYSTEM para persistir el archivo
								 * definitivo.
								 */

								// Libero memoria
								free(ruta_script_reduccion_global);
								free(ruta_nombre_resultado_reduccion_global);

								// Libero la lista de nombres de archivos temporales
								liberar_lista_nombres_temporales(lista_nombres_temporales);

								exit(0); // Mato al proceso hijo porque ya termino su procesamiento

							} //****************** FIN - ESTOY EN EL HIJO ******************

							break;

						/*
						------------------------------------------------------------------
									FIN - WORKER ENCARGADO (ETAPA REDUCCION GLOBAL)
						------------------------------------------------------------------
						*/


						default: // Me manda un mensaje desconocido

								log_error(log_WORKER, "Se desconoce el mensaje enviado por el MASTER del socket %d.", socket_cliente);
								break;

						} // FIN switch

				eliminar_protocolo(protocolo_Recepcion); // Libero la memoria del protocolo_Recepcion del padre para la siguiente vuelta del while

				/*
				--------------------------------------------------
							FIN - WORKER RECIBE PEDIDOS
				--------------------------------------------------
				*/

				} // FIN else - se acepto la conexión de un NUEVO MASTER

		} // FIN while(true)

		/*
		--------------------------------------------------
			FIN - PROCESAMIENTO SOLICITUDES
		--------------------------------------------------
		*/


		/*
		==================================================
		INICIO CIERRE Y LIBERACION DE RECURSOS GENERALES
		==================================================
		*/

		// Espero por la culminación de TODOS los procesos hijos que tenga
		waitpid(-1, NULL, 0);

		unmap_databin(RUTA_DATABIN, databin_mapeado, log_WORKER); // Hacemos el unmap del archivo data.bin
		config_destroy(config); // Libero la memoria utilizada por las funciones del archivo de configuración
		log_destroy(log_WORKER); // Libero la memoria utilizada por las funciones de log

		/*
		--------------------------------------------------
		FIN CIERRE Y LIBERACION DE RECURSOS GENERALES
		--------------------------------------------------
		*/

		return EXIT_SUCCESS;

}
/***** FIN MAIN *****/

