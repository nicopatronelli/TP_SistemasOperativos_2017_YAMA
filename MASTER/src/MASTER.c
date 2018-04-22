/*
 ============================================================================
 Name        : MASTER.c
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
#include <pthread.h> //Necesaria para el manejo de hilos (threads)
#include <commons/config.h> //Necesaria para manipular el archivo de configuracion (config.txt)
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include <commons/string.h> //Biblioteca para el manejo de strings creada por la cátedra
#include <shared/sockets.h>
#include <shared/protocolo.h>
#include <shared/funcionesAuxiliares.h> //Necesaria para tamanioArchivo()
#include <shared/mensajes.h> // Biblioteca exclusiva para la gestión de mensajes
#include <shared/md5.h> // Para la función md5()
#include "funcionesMaster.h" // Biblioteca exclusiva para funciones de MASTER

// CONTADOR DE HILOS (Variable global)
int indice_hilo_actual; // Voy a ir aumentando este valor

/***** INICIO MAIN *****/
int main(int argc, char* argv[]) {

		/*
		===========================
		INICIO CONFIGURACION MASTER
		===========================
		*/

		//Lo primero es crear el archivo de log (MASTER.log) para registrar la traza de ejecución del proceso MASTER
		log_MASTER = log_create("MASTER.log","MASTER", true, LOG_LEVEL_TRACE);

		// MASTER se conecta a YAMA, así que necesita conocer su IP y PUERTO por archivo de configuración
		char* YAMA_IP;
		int YAMA_PUERTO;

		/* argc es la cantidad de argumentos que recibe main. Por defecto (sino se le pasa ningun argumento al llamar al proceso)
		 * vale 1, pues el propio nombre del programa es el primer argumento. Como nosotros vamos a pasar 4 argumentos, entonces en
		 * total debe haber 1+4=5 argumentos.
		 */
		const int cantidad_argumentos_recibidos = argc;
		const int cantidad_argumentos_esperados = 5;

		// Compruebo que MASTER reciba la cantidad de parámetros correcta; sino aborto la ejecución
		if( cantidad_argumentos_recibidos != cantidad_argumentos_esperados ){
			log_error(log_MASTER, "La cantidad de argumentos pasados a MASTER no es la correcta (4)");
			return EXIT_FAILURE;
		}

		// Guardo las rutas de los argumentos
		char* ruta_transformador = argv[1];
		char* ruta_reductor = argv[2];
		char* ruta_archivoAProcesar = argv[3];
		ruta_resultadoFinal = argv[4];

		log_info(log_MASTER, "La ruta del script transformador es %s\n", ruta_transformador);
		log_info(log_MASTER, "La ruta del script reductor es %s\n", ruta_reductor);
		log_info(log_MASTER, "La ruta del archivo a procesar es %s\n", ruta_archivoAProcesar);
		log_info(log_MASTER, "La ruta del almacenamiento final es %s\n", ruta_resultadoFinal);

		//Levanto el archivo de configuración (config.txt) y cargo YAMA_IP y YAMA_PUERTO
		t_config* config = configuracion_MASTER("config.txt", &YAMA_IP, &YAMA_PUERTO);

		// Declaro el protocolo donde voy a recibir los mensajes que me envien YAMA y WORKER
		t_protocolo* protocolo_Recepcion;
		t_info_bloque_a_transformar* info_bloque_actual;

		/* Declaro una cierta cantidad de estructuras de tipo hilo. En tiempo de ejecución voy a crear,
		 * efectivamente, hasta CANTIDAD_HILOS. Luego, voy a realizar pthread_join de aquellos hilos
		 * que solamente cree.
		 */
		pthread_t un_hilo[CANTIDAD_HILOS];
		indice_hilo_actual = -1; // Inicializo el contador de hilos en 0


		/*
		--------------------------------------------------
		FIN CONFIGURACION MASTER
		--------------------------------------------------
		*/


		/*
		=========================================================
		INICIO CARGA DE SCRIPTS - TRANSFORMADOR Y REDUCTOR
		=========================================================
		*/

		buffer_script_transformador = _guardar_script(ruta_transformador); // Me guardo el script transformador
		if (buffer_script_transformador == NULL)
		{
			log_error(log_MASTER, "Hubo un error al cargar el script transformador.");
			return EXIT_FAILURE;
		}

		buffer_script_reductor = _guardar_script(ruta_reductor); // Me guardo el script reductor
		if (buffer_script_reductor == NULL)
		{
			log_error(log_MASTER, "Hubo un error al cargar el script reductor");
			return EXIT_FAILURE;
		}


		/*
		--------------------------------------------------
		FIN CARGA DE SCRIPTS - TRANSFORMADOR Y REDUCTOR
		--------------------------------------------------
		*/

		/*
		=========================================================
		INICIO ESTABLECIMIENTO CONEXION MASTER A YAMA
		=========================================================
		*/

		t_direccion direccion_YAMA = nuevaDireccion(YAMA_PUERTO, YAMA_IP);

		socket_MASTER_YAMA = conectarseA(direccion_YAMA); // El socket de MASTER se conecta a YAMA
		if ( socket_MASTER_YAMA > 0 ) {
			log_trace(log_MASTER, "Se estableció conexión con YAMA en el socket %d.", socket_MASTER_YAMA);
		}
		else{
			log_error(log_MASTER, "No se pudo establecer conexión con YAMA.");
			return EXIT_FAILURE;
		}

		/*
		--------------------------------------------------------------------------------------------------
		FIN ESTABLECIMIENTO CONEXION MASTER A YAMA (En este punto MASTER ya está contectado con YAMA)
		--------------------------------------------------------------------------------------------------
		*/


		/*
		=================================================================================================
		INICIO - MASTER LE ENVIA UN MENSAJE A YAMA INDICALE LA RUTA DEL ARCHIVO CON EL QUE DESEA OPERAR
		=================================================================================================
		*/

		int bytes_enviados = enviar_mensaje(RUTA_ARCHIVO_A_PROCESAR, ruta_archivoAProcesar, socket_MASTER_YAMA);

		if ( bytes_enviados > 0 ) {
					log_info(log_MASTER, "La ruta del archivo a procesar se envio correctamente a YAMA.");
			} else {
					log_error(log_MASTER, "La ruta del archivo a procesar NO se pudo enviar a YAMA.");
					return EXIT_FAILURE;
		}


		/*
		----------------------------------------------------------------------------------------
		FIN - MASTER LE ENVIA UN MENSAJE A YAMA INDICALE EL ARCHIVO CON EL QUE DESEA OPERAR
		----------------------------------------------------------------------------------------
		*/


		/*
		===================================================================================================
		INICIO - MASTER se pone a la escucha de pedidos de YAMA y los atiende (procesa) según corresponda
		===================================================================================================
		*/

		// Inicializo el flag_job_finalizado para recibir pedidos hasta que se ponga en true
		bool flag_job_finalizado = false;

		// Recibo la respuesta al envio de la RUTA_ARCHIVO_A_PROCESAR que le envie a YAMA
		protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);

		// Mientras flag_job_finalizado sea false atiendo pedidos de YAMA
		while( flag_job_finalizado == false ){

			switch (protocolo_Recepcion->funcion){

			/* IMPORTANTE: Cada case debe finalizar llamando a eliminar_protocolo(), es decir, debe dejar
			 * protocolo_Recepcion liberado.
			 */

				/*
				===================================================================================================
				INICIO - case TRANSFORMACION
				===================================================================================================
				*/

				case TRANSFORMACION:

					/* Primero recibo toda la info del bloque a transformar y me la guardo en una estructura
					 * del tipo t_info_bloque_a_transformar .
					 */

					info_bloque_actual = malloc(sizeof(t_info_bloque_a_transformar));

					// 0° - Me guardo el numero de bloque del archivo lógico
					info_bloque_actual->numero_bloque_archivo = atoi(protocolo_Recepcion->mensaje);
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensjae

					// 1° - Recibo el NOMBRE DEL NODO que tiene el bloque a procesar
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NOMBRE_NODO ){
						info_bloque_actual->nombre_nodo = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
						strcpy(info_bloque_actual->nombre_nodo, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el nombre del Nodo de un bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);


					// 2° - Recibo la IP del WORKER que tiene el bloque a procesar
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == IP_WORKER ){
						info_bloque_actual->ip_nodo = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
						strcpy(info_bloque_actual->ip_nodo, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir la ip del Nodo de un bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);


					// 3° - Recibo el PUERTO DE ESCUCHA del WORKER
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == PUERTO_WORKER ){
						info_bloque_actual->puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el puerto de escucha del WORKER de un bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);


					// 4° - Recibo el NUMERO DE BLOQUE del databin donde está el bloque a procesar
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NUMERO_BLOQUE_DATABIN ){
						info_bloque_actual->numero_bloque_databin = atoi(protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el numero de bloque del archivo data.bin donde se almacena el bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);


					// 5° - Recibo los BYTES OCUPADOS en dicho bloque
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == BYTES_OCUPADOS_BLOQUE_DATABIN ){
						info_bloque_actual->bytes_ocupados = atoi(protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir los bytes ocupados del bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);


					// 6° - Recibo el NOMBRE DEL ARCHIVO TEMPORAL con el que se va a guardar el resultado de la transformación
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NOMBRE_ARCHIVO_TEMPORAL_TRANS ){
						info_bloque_actual->nombre_archivo_temporal = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
						strcpy(info_bloque_actual->nombre_archivo_temporal, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el nombre del archivo temporal de la transformación del bloque a procesar.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion);

					/* Una vez que tengo toda la info necesaria del bloque lanzo el hilo para conectarme al WORKER
					 * correspondiente e indicarle que realice la transformación.
					 */

					// Incremento el numero de hilo
					indice_hilo_actual++;

					if ( pthread_create( &un_hilo[indice_hilo_actual], NULL, (void*)enviar_info_bloque_transformar_a_worker, info_bloque_actual) != 0 ){
						log_error(log_MASTER, "NO se pudo crear el hilo %d.", indice_hilo_actual);
						break;
					}

					break;

				/*
				--------------------------------------------------------------------------------------------------
				FIN - case TRANSFORMACION
				--------------------------------------------------------------------------------------------------
				*/


				/*
				===================================================================================================
				INICIO - case REDUCCION LOCAL
				===================================================================================================
				*/

				case REDUCCION_LOCAL:

					log_trace(log_MASTER, "YAMA me solicita iniciar una REDUCCION LOCAL.");
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

					/* Recibo la info necesaria para iniciar la reduccion local. Para ello, primero creo una
					 * estructura del tipo t_info_nodo_a_reducir donde voy a ir guardando toda la información
					 * que debo enviarle al WORKER que va a realizar la reducción local.
					 */

					t_info_nodo_a_reducir* nodo_a_reducir = malloc(sizeof(t_info_nodo_a_reducir));


					/* 1° - Recibo los nombres de los archivos temporales de la transformación del Nodo que voy
					 * a reducir.
					 */

					// Recibo la cantidad de temporales trans
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == CANTIDAD_TEMPORALES ){
						nodo_a_reducir->cantidad_temporales_a_reducir = atoi(protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir la cantidad de archivos temporales a reducir.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir un nuevo mensaje

					// Creo la lista donde voy a guardar los nombres de los temporales trans
					nodo_a_reducir->lista_temporales_a_reducir = list_create();

					int nro_temporal_trans;
					for(nro_temporal_trans = 0; nro_temporal_trans < nodo_a_reducir->cantidad_temporales_a_reducir; nro_temporal_trans++){

						// Recibo los nombres temporales de la transformación, uno a uno
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == NOMBRE_TEMPORAL_A_REDUCIR){

							// Me guardo cada nombre temporal a reducir en una estructura t_nombre_temporal_a_reducir

							t_nombre_temporal_a_reducir* nuevo_temporal = malloc(sizeof(t_nombre_temporal_a_reducir));
							nuevo_temporal->nombre_temporal_a_reducir = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
							strcpy(nuevo_temporal->nombre_temporal_a_reducir, protocolo_Recepcion->mensaje);

							// Añado el elemento a la lista
							list_add(nodo_a_reducir->lista_temporales_a_reducir, nuevo_temporal);
						}

						// Libero el protocolo para recibir el siguiente nombre temporal a reducir o el proximo mensaje
						eliminar_protocolo(protocolo_Recepcion);

					} // FIN for (recibo la lista de nombres temporales a reducir)


					/* 2° - Recibo la info (nombre, ip y puerto) del Nodo (WORKER) que va a hacer la
					 * reduccion local.
					 */

					// 2a - Recibo el NOMBRE del Nodo
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NOMBRE_NODO_REDUCCION){
						nodo_a_reducir->nombre_nodo = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
						strcpy(nodo_a_reducir->nombre_nodo, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el nombre del Nodo donde se va a realizar la reduccion local.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir un nuevo mensaje

					// 2b - Recibo la IP del Nodo
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == IP_NODO_REDUCCION){
						nodo_a_reducir->ip_nodo = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
						strcpy(nodo_a_reducir->ip_nodo, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir la ip del Nodo donde se va a realizar la reduccion local.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir un nuevo mensaje

					// 2c - Recibo el PUERTO de escucha del WORKER
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == PUERTO_WORKER_ESCUCHA_MASTER_REDUCCION){
						nodo_a_reducir->puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el puerto del Nodo donde se va a realizar la reduccion local.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir un nuevo mensaje


					/* 3° - Recibo el nombre del archivo temporal con el que WORKER va a guardar el resultado de
					 * la reduccion local
					 */
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NOMBRE_RESULTADO_REDUCCION_LOCAL ){
						nodo_a_reducir->nombre_temporal_reduccion = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
						strcpy(nodo_a_reducir->nombre_temporal_reduccion, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el nombre con el que se va a guardar el resultado de una reduccion local.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir un nuevo mensaje

					 /* 4° - Una vez que recibí TODA la información para iniciar la REDUCCIÓN LOCAL lanzo un
					  * hilo y se la envio al WORKER correspondiente.
					  */

					indice_hilo_actual++; // Incremento el numero de hilo antes de lanzar uno nuevo

					if ( pthread_create( &un_hilo[indice_hilo_actual], NULL, (void*)enviar_info_nodo_reduccion_local_a_worker, nodo_a_reducir) != 0 ){
						log_error(log_MASTER, "NO se pudo crear el hilo %d.", indice_hilo_actual);
						break;
					}

					break;

				/*
				--------------------------------------------------------------------------------------------------
				FIN - case REDUCCION LOCAL
				--------------------------------------------------------------------------------------------------
				*/


				/*
				===================================================================================================
				INICIO - case NODO_ENCARGADO (REDUCCION GLOBAL)
				===================================================================================================
				*/

				case NODO_ENCARGADO: // YAMA pasa la información del Nodo encargado de realizar una REDUCCION GLOBAL

					log_trace(log_MASTER, "YAMA me solicita iniciar la etapa de REDUCCION GLOBAL.");

					/* Recibo la info necesaria para iniciar la reduccion global. Para ello, primero creo una
					 * estructura del tipo t_info_redu_global donde voy a ir guardando toda la información
					 * que debo enviarle al Nodo (WORKER) ENCARGADO que va a realizar la reducción global.
					 */

					// Me guardo la cantidad de Nodos participantes en el job
					int cant_nodos_participantes = atoi(protocolo_Recepcion->mensaje);

					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

					t_list* lista_nodos_participantes = list_create();

					// REcibo, uno a uno, la info de cada Nodo participante del job actual y me la guardo en una lista
					int nro_nodo;
					for(nro_nodo = 0; nro_nodo < cant_nodos_participantes; nro_nodo++){

						// Creo un nuevo elemento del tipo t_info_nodo_redu_global donde voy a recibir toda la info del Nodo participante actual
						t_info_nodo_redu_global* nodo_participante_actual = malloc(sizeof(t_info_nodo_redu_global));

						// 1° - Recibo el NOMBRE del Nodo participante actual
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == NOMBRE_NODO_PARTICIPANTE ){
							nodo_participante_actual->nombre_nodo = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
							strcpy(nodo_participante_actual->nombre_nodo, protocolo_Recepcion->mensaje);
						}else{
							log_error(log_MASTER, "Error al recibir el nombre de un nodo participante para la reducción global.");
							return EXIT_FAILURE;
						}
						eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

						// 2° - Recibo la IP del Nodo participante actual
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == IP_NODO_PARTICIPANTE ){
							nodo_participante_actual->ip = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
							strcpy(nodo_participante_actual->ip, protocolo_Recepcion->mensaje);
						}else{
							log_error(log_MASTER, "Error al recibir la ip de un nodo participante para la reducción global.");
							return EXIT_FAILURE;
						}
						eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

						// 3° - Recibo el PUERTO del Nodo participante actual
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == PUERTO_NODO_PARTICIPANTE ){
							nodo_participante_actual->puerto_worker = atoi(protocolo_Recepcion->mensaje);
						}else{
							log_error(log_MASTER, "Error al recibir el puerto un nodo participante para la reducción global.");
							return EXIT_FAILURE;
						}
						eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

						// 4° - Recibo el nombre temporal de la reducción local del Nodo participante actual
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == NOMBRE_REDUC_LOCAL_NODO_PARTICIPANTE ){
							nodo_participante_actual->nombre_temporal_reduccion = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // + 1 del '\0'
							strcpy(nodo_participante_actual->nombre_temporal_reduccion, protocolo_Recepcion->mensaje);
						}else{
							log_error(log_MASTER, "Error al recibir el nombre temporal de la reduccion local un nodo participante para la reducción global.");
							return EXIT_FAILURE;
						}
						eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

						// 5° - Recibo si el Nodo participante actual es ENCARGADO o no
						protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
						if ( protocolo_Recepcion->funcion == ES_NODO_ENCARGADO ){
							nodo_participante_actual->encargado = atoi(protocolo_Recepcion->mensaje);
						}else{
							log_error(log_MASTER, "Error al recibir si un Nodo participante es el encargado o no para la reducción global.");
							return EXIT_FAILURE;
						}
						eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

						// Añado la info del nodo_participante_actual a mi lista de nodos participantes
						list_add(lista_nodos_participantes, nodo_participante_actual);

					} // FIN for

					// Por último, recibo el nombre del archivo TEMPORAL con el que se va a guardar el resultado de la reducción global
					protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);
					if ( protocolo_Recepcion->funcion == NOMBRE_RESULTADO_REDUCCION_GLOBAL){
						nombre_resultado_reduccion_global = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
						strcpy(nombre_resultado_reduccion_global, protocolo_Recepcion->mensaje);
					}else{
						log_error(log_MASTER, "Error al recibir el nombre donde se guardara el resultado de la reducción global.");
						return EXIT_FAILURE;
					}
					eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir el siguiente mensaje

					/* Una vez que tengo toda la info necesaria para iniciar la etapa de REDUCCION GLOBAL lanzo el hilo
					 * y le paso dicha información al Nodo (WORKER) seleccionado como encargado (esto se hace dentro
					 * del hilo).
					 */

					// Incremento el numero de hilo
					indice_hilo_actual++;

					if ( pthread_create( &un_hilo[indice_hilo_actual], NULL, (void*)enviar_info_nodo_encargado, lista_nodos_participantes) != 0 ){
						log_error(log_MASTER, "NO se pudo crear el hilo %d en la etapa de reducción global.", indice_hilo_actual);
						break;
					}

					break;

				/*
				--------------------------------------------------------------------------------------------------
				FIN - case NODO_ENCARGADO (REDUCCION GLOBAL)
				--------------------------------------------------------------------------------------------------
				*/

				case FIN_JOB_EXITO:

					log_trace(log_MASTER, "El JOB actual finalizó CORRECTAMENTE.");
					flag_job_finalizado = true; // Marco el flag_job_finalizado en true para salir del while
					break;

				case ABORTAR_JOB:

					log_error(log_MASTER, "YAMA aborta el JOB actual.");
					flag_job_finalizado = true; // Marco el flag_job_finalizado en true para salir del while
					break;

			} // FIN switch

			// Recibo el mensaje para la siguiente iteración del while
			protocolo_Recepcion = recibir_mensaje(socket_MASTER_YAMA);

		} // FIN while( flag_job_finalizado == false )


		/*
		==========================================================================================
		INICIO CIERRE Y LIBERACION DE RECURSOS GENERALES
		==========================================================================================
		*/

		// Espero por todos los hilos que cree durante la ejecución de MASTER
		int i;
		for(i= 0; i < indice_hilo_actual; i++ ){

			pthread_join(un_hilo[i], NULL);

		} // FIN for

		// Libero la memoria utilizada para el guardar el nombre del resultado de la reducción global (variable global)
		free(nombre_resultado_reduccion_global);

		liberar_buffer_script(buffer_script_transformador); // Libero la memoria utilizada por el buffer del script transformador
		liberar_buffer_script(buffer_script_reductor); // Libero la memoria utilizada por el buffer del script reductor

		close(socket_MASTER_YAMA); // Cierro el socket_MASTER_YAMA y libero sus recursos
		config_destroy(config); // Libero la memoria utilizada por las funciones del archivo de configuración
		log_info(log_MASTER, "Proceso MASTER terminado satisfactoriamente.");
		log_destroy(log_MASTER); // Libero la memoria utilizada por las funciones de log


		/*
		-----------------------------------------------
		FIN CIERRE Y LIBERACION DE RECURSOS GENERALES
		-----------------------------------------------
		*/

		return EXIT_SUCCESS;
}
/***** FIN MAIN *****/



