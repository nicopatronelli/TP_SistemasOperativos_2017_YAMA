/*Name        : YAMA.c
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
#include <errno.h>
#include <signal.h>
#include <commons/config.h> //Necesaria para manipular el archivo de configuracion (config.txt)
#include <commons/log.h> //Biblioteca para el manejo de archivos de log creada por la cátedra
#include <commons/string.h> //Biblioteca para el manejo de strings creada por la cátedra
#include <commons/temporal.h> //Biblioteca necesaria temporal_get_string_time()
#include <commons/collections/list.h> //Biblliteca para el manejo de listas
#include <shared/sockets.h>
#include <shared/protocolo.h>
#include <shared/funcionesAuxiliares.h> //Necesaria para tamanioArchivo()
#include <shared/mensajes.h>
#include <pthread.h> //Necesaria para el manejo de hilos (threads)
#include "funcionesYAMA.h" // Biblioteca exclusiva para funciones de YAMA
#include "planificacionYAMA.h" // Biblioteca exclusiva para las funciones de PLANIFICACION de YAMA


// Variables globales config
int RETARDO_PLANIFICACION;
int DISPONIBILIDAD_BASE;
char* ALGORITMO_BALANCEO;

// Variable global para select
int retorno_select;

// Funcion sig_handler para recargar la configuración
static void recargar_configuracion(int signal){

	t_config* config = config_create("config.txt");

	if ( config == NULL){
		log_error(log_YAMA, "No se pudo leer el archivo de configuración de YAMA.");
	}
	else{
		RETARDO_PLANIFICACION = config_get_int_value(config,"RETARDO_PLANIFICACION");
		ALGORITMO_BALANCEO = config_get_string_value(config,"ALGORITMO_BALANCEO");
		DISPONIBILIDAD_BASE = config_get_int_value(config, "DISPONIBILIDAD_BASE");
	}

	printf("El RETARDO ahora es %d.\n", RETARDO_PLANIFICACION );
	printf("La DISPONIBILIDAD ahora es %d.\n", DISPONIBILIDAD_BASE);
	printf("El ALGORITMO ahora es %s.\n", ALGORITMO_BALANCEO);

}


/***** INICIO MAIN *****/
int main(void) {

		/*
		=========================
		INICIO CONFIGURACION YAMA
		=========================
		 */

		//Lo primero es crear el archivo de log (YAMA.log) para registrar la traza de ejecución del proceso YAMA
		log_YAMA = log_create("YAMA.log","YAMA", true, LOG_LEVEL_TRACE);

		// YAMA se conecta a FILE_SYSTEM, así que necesita conocer su IP y PUERTO por archivo de configuración
		char* FILESYSTEM_IP = NULL;
		int FILESYSTEM_PUERTO;

		// YAMA se pone a la escucha de conexiones de MASTER, por lo que debe tener un puerto asignado para eso
		int PUERTO_ESCUCHA_MASTER;

		//Levanto el archivo de configuración (config.txt) y cargo YAMA_IP y YAMA_PUERTO
		t_config* config = configuracion_YAMA("config.txt", &FILESYSTEM_IP, &FILESYSTEM_PUERTO, &PUERTO_ESCUCHA_MASTER, &RETARDO_PLANIFICACION, &ALGORITMO_BALANCEO, &DISPONIBILIDAD_BASE);

		// Variables para recibir los mensajes que envie MASTER y FILE_SYSTEM
		t_protocolo* protocolo_Recepcion = NULL;

		// Creo la lista donde voy a ir guardando la información (el socket) de los distintos MASTERs que se conecten a YAMA
		lista_MASTERs = list_create(); // El indice de la lista es el número de MASTER de dicho MASTER
		int numero_master;

		// Variables para manejar la tabla de estados (lista de solicitudes)
		tabla_estados = list_create();
		nro_job = 0; // Inicializo nro_job en 0
		int nro_entrada_tabla_estados = 0; // ¿No debería ser global?
		inicializar_lista_nodos(); // Creo la lista de nodos y le cargo un nodo nulo (para que su size sea y poder entrar al for de actualizar_lista_nodos)

		// Variables y configuración previa para la tabla (array) global de workers
		inicializar_tabla_global_workers(tabla_global_workers); // Inicializo la tabla global de workers ( carga_actual = 0 y carga_historica = 0)

		// Variable para generar los nombres temporales unicos
		indice_nombre_temporal = 0;

		// Manejo de señal SIGUSR1
		signal(SIGUSR1, recargar_configuracion);

		// Variables para RUTA_ARCHIVO_A_PROCESAR
		int cantidad_bloques_archivo;
		int bloque;
		t_Bloque* bloque_actual;
		t_list* lista_bloques_archivo = list_create(); // Lista con la información de cada bloque (ambas copias de existir) que conforma un archivo a procesar
		bool existen_todos_los_bloques = true; // Asumo que existe al menos una copia de cada bloque

		// Variables para BLOQUE_TRANSFORMADO_OK
		int numero_nodo_actual;
		bool inicio_reduccion_local;
		char* nombre_nodo_actual; // También se utiliza en los otros case
		char* nombre_temporal_reduccion_local;
		t_list* lista_temporales_trans;
		int cantidad_temporales;
		char* cantidad_temporales_string;
		int nro_temporal;
		t_nombre_temporal* temporal_actual;
		char* puerto_worker_reduccion_string;

		// Variables para REDUCCION_LOCAL_OK
		bool inicio_reduccion_global;
		char* nombre_temporal_reduccion_global;
		int indice_nodo; // Para el for donde se envia la info de cada Nodo participante en el Job a MASTER
		t_nodo_job* nodo_job_actual; // Para el for donde se envia la info de cada Nodo participante en el Job a MASTER
		int cantidad_nodos_participantes;
		char* cantidad_nodos_participantes_string;
		char* puerto_worker_nodo_participante_string;
		char* es_nodo_encargado_string;
		/*
		--------------------------------------------------
		FIN CONFIGURACION YAMA
		--------------------------------------------------
		 */

		/*
		=======================================================
		INICIO ESTABLECIMIENTO CONEXION YAMA A FILE_SYSTEM
		=======================================================
		 */

		t_direccion direccion_FILESYSTEM = nuevaDireccion(FILESYSTEM_PUERTO, FILESYSTEM_IP);

		int socket_YAMA_FILESYSTEM = conectarseA(direccion_FILESYSTEM); // El socket de YAMA se conecta a FILESYSTEM
		if ( socket_YAMA_FILESYSTEM > 0 ) {
			log_info(log_YAMA, "Se inicia conexión con FILESYSTEM en el socket %d.", socket_YAMA_FILESYSTEM);
			protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);
			if ( protocolo_Recepcion->funcion == CONEXION_RECHAZADA ){
				log_error(log_YAMA, "... pero FILESYSTEM no acepta la conexión porque NO está en un estado estable.");
				eliminar_protocolo(protocolo_Recepcion);
				close(socket_YAMA_FILESYSTEM);
				return EXIT_FAILURE;
			}else if ( protocolo_Recepcion->funcion == CONEXION_ACEPTADA ){
				log_trace(log_YAMA, "FILESYSTEM acepta la conexión de YAMA.");
			}else{
				log_error(log_YAMA, "FILE_SYSTEM envio un protocolo desconocido. YAMA se cierra.");
				eliminar_protocolo(protocolo_Recepcion);
				return EXIT_FAILURE;
			}

		}else{ // Fallo la función connect
			log_error(log_YAMA, "No se pudo establecer conexión con FILESYSTEM.");
			return EXIT_FAILURE;
		}

		/*
		--------------------------------------------------------------------------------------------------------------
		FIN ESTABLECIMIENTO CONEXION YAMA A FILE_SYSTEM (En este punto YAMA y FILESYSTEM ya están conectados).
		--------------------------------------------------------------------------------------------------------------
		 */


		/*
		=======================================================
		INICIO - YAMA SE PONE A LA ESCUCHA DE MASTERs
		=======================================================
		 */

		// YAMA se pone a la escucha de nuevas conexiones de procesos MASTER
		struct sockaddr_in direccionYama = nuevaDireccion(PUERTO_ESCUCHA_MASTER, "INADDR_ANY");
		int socket_escucha_MASTER = ponerseALaEscuchaEn(direccionYama);

		struct sockaddr_in direccion_cliente; // dirección del cliente (proceso MASTER que se conecta a YAMA)

		/*
		--------------------------------------------------------------------------------------------------------------
		FIN - YAMA SE PONE A LA ESCUCHA DE MASTERs (En este momento YAMA está escuchando conexiones de MASTERs
		--------------------------------------------------------------------------------------------------------------
		 */

		/*
		======================================
		INICIO - SELECT
		======================================
		 */

		// NOTA: fd = file descriptor (descriptor de archivo)
		int fdmax; // El valor del fd más alto
		int fdmin; // El valor del fd más bajo
		int newfd; // fd para nuevos clientes
		int fd_i; // fd auxiliar

		fd_set read_fds; // Conjunto de fd de donde voy a leer
		fd_set master; // Conjunto maestro de fd

		// limpio (inicializo) los conjuntos maestro y temporal
		FD_ZERO(&master);
		FD_ZERO(&read_fds);

		// Añado el socket de escucha de MASTER al conjunto master de fds
		FD_SET(socket_escucha_MASTER, &master);

		// Sigo la pista del fd mayor
		fdmax = socket_escucha_MASTER; // Como por ahora el único que tengo es el de escucha, entonces es el máximo

		/* Como el único fd que tengo es el de escucha, entonces también es el mínimo, ¿y lo será siempre?
		 * Los fd 0, 1 y 2 están reservados para STD_IN, STD_OUT_ STD_ERROR. Por eso, el SO empieza a asignar fd a partir del 3,
		 * y como los fds son valores positivos, los siguientes fd serán 4, 5, 6...
		 * Sin embargo, si por alguna razón el SO le da el fd 7 al socket de escucha y luego el 5 a un fd cliente, no estaríamos
		 * teniéndolo en cuenta en el barrido del for. Por lo tanto, para las pruebas está bien poner fdmin = socket_escucha_MASTER
		 * por una cuestión de velocidad en el debug, pero para las entregas o pruebas formales conviene (por seguridad)
		 * poner fd_i=0 (que barra desde el fd 0)
		 */
		fdmin = socket_escucha_MASTER;

		/* Si el select falla verificar que no cerremos la conexión de un cliente MASTER desde el propio proceso MASTER con close.
		 * Al hacer esto, estamos haciendo un send del valor 0, por lo que el select va a detectar que hay actividad en dicho socket.
		 * ¡Cuidado!
		 */

		while(true) { /*** INICIO BUCLE PRINCIPAL***/

			read_fds = master; // El conjunto de fds de lectura ahora tiene el conjunto de fds maestro

			/* Hago el select(). Solo me interesa el conjunto de fds de lectura, por lo que el de escritura (2do par) y excepciones (3er par) los paso
			 * en NULL. También paso en NULL el tiempo (4to par) de espera del select().
			 */

			retorno_select = select(fdmax+1, &read_fds, NULL, NULL, NULL);
			if ( retorno_select == -1 ) {
				if ( errno == EINTR ){
					/* El error es producido por la señal de recarga de configuración,
					 * así que lo ignoro (corro select de vuelta).
					 */
					retorno_select = select(fdmax+1, &read_fds, NULL, NULL, NULL);
				}else{
					/* El error es por una cuestión desconocida, así que finalizo YAMA.
					 */
					log_error(log_YAMA, "Hubo un error en el select.");
					return EXIT_FAILURE;
				}
			}


			/* Exploro conexiones existentes en busca de datos que leer. Para eso, barro desde el fd 0 (o fdmin) hasta
			 * el fdmax, que es el mayor fd en el que puedo tener actividad.
			 */
			for(fd_i = fdmin; fd_i <= fdmax; fd_i++) { // INICIO for recorrer todos los fds

				if (FD_ISSET(fd_i, &read_fds)) { // Si fd_i está en el conjunto read_fds, entonces tiene datos para leer (select se encargo de ponerlo en read_fds)
					if (fd_i == socket_escucha_MASTER) { // A su vez, si fd_i es el socket de escucha, entonces hay nuevas conexiones para aceptar

						// Gestiono conexiones de nuevos clientes (nuevos procesos MASTER)
						newfd = aceptar_clienteEn(socket_escucha_MASTER, direccion_cliente);
						if ( newfd < 0 ){
							log_error(log_YAMA, "Hubo un error en el accept dentro del select.");
							break;
						}
						else // Sino hubo un error en el accept la conexión del cliente se acepto
						{
							FD_SET(newfd, &master); // Añado al conjunto master de fds el fd perteneciente al nuevo cliente (nuevo MASTER)
							if (newfd > fdmax) { // Actualizo el fdmax de ser necesario
								fdmax = newfd;
							}
							agregar_master(newfd, lista_MASTERs); // Agrego el nuevo MASTER a la lista de MASTERs
							numero_master = master_actual(lista_MASTERs, newfd);
							log_trace(log_YAMA, "Nuevo MASTER conectado en el socket %d. Lo identificamos como el MASTER Nro %d.", newfd, numero_master);
						}
					} // fin  if (fd_i == socket_escucha_MASTER)
					else
					{ // Si fd_i NO es el socket de escucha, entonces es un socket de un cliente (un MASTER) ya aceptado, y por lo tanto, debo gestionar su pedido

						/*
						==================================================
						INICIO - CENTRO DE GESTIÓN DE MENSAJES DEL SELECT
						==================================================
						 */

						// Identifico el MASTER que me esta hablando:
						numero_master = master_actual(lista_MASTERs, fd_i);

						/*
						 * RECIBO EL MENSAJE PRINCIPAL PARA ENTRAR A UN DETERMINADO CASE
						 */
						protocolo_Recepcion = recibir_mensaje(fd_i);

						// Si hay algun error salgo...
						if ( protocolo_Recepcion == CERRARON_SOCKET ){
							log_trace(log_YAMA, "El MASTER Nro %d cerro su conexión en el socket %d.", numero_master, fd_i);
							desconectar_master(numero_master, fd_i, &master);
							//eliminar_protocolo(protocolo_Recepcion);
						}else if ( protocolo_Recepcion == NULL){
							log_error(log_YAMA, "Hubo un error al recibir un mensaje del MASTER NRO %d en el socket %d.", numero_master, fd_i);
							desconectar_master(numero_master, fd_i, &master);
							//eliminar_protocolo(protocolo_Recepcion);
						}else{ //... sino atiendo el mensaje ...

							switch(protocolo_Recepcion->funcion){ // INICIO SWITCH

							/*
							==================================================
							INICIO - case RUTA_ARCHIVO_A_PROCESAR (NUEVO JOB)
							==================================================
							 */

							case RUTA_ARCHIVO_A_PROCESAR: //Es el PRIMER mensaje que me manda cada MASTER nuevo (da origen a un nuevo JOB)

								log_trace(log_YAMA, "[SE INICIA UN NUEVO JOB] - Se lo identifica como JOB %d", nro_job);
								log_info(log_YAMA, "Recibi una nueva RUTA DE ARCHIVO A PROCESAR. La misma es %s.", protocolo_Recepcion->mensaje);


								// PASO 1 - Envio a FILE_SYSTEM la ruta del archivo a procesar y quedo a la espera de su respuesta
								enviar_mensaje(RUTA_ARCHIVO_A_PROCESAR, protocolo_Recepcion->mensaje, socket_YAMA_FILESYSTEM);

								// 2 - FILE_SYSTEM me contesta
								eliminar_protocolo(protocolo_Recepcion); // Libero los recursos del protocolo actual para recibir un nuevo mensaje
								protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

								// 2.A - Si el archivo no existe en yamafs, FILE_SYSTEM me avisa, lo informo y hago un break, pues no tengo nada más que hacer por este job (lo aborto)
								if ( protocolo_Recepcion->funcion == ARCHIVO_NO_EXISTE ){

									log_error(log_YAMA, "El archivo que MASTER solicito procesar NO existe en yamafs.");
									log_error(log_YAMA, "Se aborta la ejecución del JOB %d.", nro_job);

									// Le informo a MASTER que no se puede iniciar el JOB
									enviar_mensaje(ABORTAR_JOB, "Se aborta el JOB porque NO existe el archivo a procesar.", fd_i);
									enviar_mensaje(MENSAJE_VACIO, "Mensaje vacio", fd_i);

									// IMPORTANTE: Al final de cada CASE liberamos el protocolo para que se pueda recibir el proximo mensaje
									eliminar_protocolo(protocolo_Recepcion);

									break; // Salgo del case

								}

								// 2.B - Si el archivo existe en yamafs, entonces el primer mensaje que me manda FILE_SYSTEM es la cantidad de bloques que lo componen

								if ( protocolo_Recepcion->funcion == CANTIDAD_DE_BLOQUES){ // INICIO if ( protocolo_Recepcion->funcion == CANTIDAD_DE_BLOQUES)

									cantidad_bloques_archivo = atoi(protocolo_Recepcion->mensaje);
									log_info(log_YAMA, "La cantidad de bloques del archivo es %d", cantidad_bloques_archivo);

									// Recibo los datos por CADA BLOQUE del archivo
									for(bloque=0; bloque<cantidad_bloques_archivo; bloque++){ // INICIO for recorro todos los bloques del archivo

										eliminar_protocolo(protocolo_Recepcion); // Libero los recursos del protocolo actual para recibir un nuevo mensaje
										protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM); // Recibo BLOQUE_NO_EXISTE o NUMERO_DE_BLOQUE

										if ( protocolo_Recepcion->funcion == BLOQUE_NO_EXISTE){

											log_error(log_YAMA, "No existen ninguna de las 2(dos) copias del bloque %d del archivo a procesar.", atoi(protocolo_Recepcion->mensaje) );
											log_error(log_YAMA, "Se aborta la ejecución del JOB %d.", nro_job);

											// Le informo a MASTER que no se puede iniciar el JOB
											enviar_mensaje(ABORTAR_JOB, "Se aborta el JOB por falta de consistencia.", fd_i);
											existen_todos_los_bloques = false;

											// IMPORTANTE: Al final de cada CASE liberamos el protocolo para que se pueda recibir el proximo mensaje
											eliminar_protocolo(protocolo_Recepcion);

											break; // Salgo del for

										}else{ // Si existe AL MENOS UNA COPIA del bloque...

											//----> Si llegamos a este punto significa que existe, por lo menos, una copia del bloque (puede que ambas)

											// En esta estructura me voy a guardar todos los datos de cada copia del bloque actual
											bloque_actual = malloc(sizeof(t_Bloque));

											/*=== INICIO - RECIBO COPIA0 ===*/

											// PASO 1 - RECIBO LA COPIA0 (Si existe)

											if ( protocolo_Recepcion->funcion == COPIA0_NO_EXISTE){
												bloque_actual->copia0.existe_copia = false;
												log_info(log_YAMA, "La COPIA 0 del bloque %d del archivo a procesar NO EXISTE.", bloque);
											}else{ // Si la COPIA0 existe entonces recibo su información

												bloque_actual->copia0.existe_copia = true;
												log_info(log_YAMA, "EXISTE la COPIA 0 del bloque %d del archivo a procesar.", bloque);

												// PASO 2 - GUARDO EL NOMBRE_NODO DONDE ESTA EL BLOQUE (Ej: NODO1)
												strcpy(bloque_actual->copia0.nombre_nodo, protocolo_Recepcion->mensaje);

												// PASO 3 - RECIBO LA IP_NODO
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												strcpy(bloque_actual->copia0.ip_nodo, protocolo_Recepcion->mensaje);

												// PASO 4 - RECIBO EL PUERTO_WORKER_ESCUCHA_MASTER
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia0.puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);

												// PASO 5 - RECIBO EL NUMERO DEL BLOQUE_DATABIN DONDE SE GUARDA EL CONTENIDO DEL BLOQUE
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia0.numero_bloque_databin = atoi(protocolo_Recepcion->mensaje);

												// PASO 6 - RECIBO LOS BYTES_OCUPADOS DENTRO DEL BLOQUE DEL DATA.BIN (CUANTO DEL 1MiB)
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia0.bytes_ocupados = atoi(protocolo_Recepcion->mensaje);

											} // Fin else recibo la COPIA0

											/*** FIN - RECIBO COPIA0 ***/

											/*=== INICIO - RECIBO COPIA1 ===*/

											// PASO 1 - RECIBO LA COPIA1 (Si existe)

											eliminar_protocolo(protocolo_Recepcion);
											protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

											if ( protocolo_Recepcion->funcion == COPIA1_NO_EXISTE){
												bloque_actual->copia1.existe_copia = false;
												log_info(log_YAMA, "La COPIA1 del bloque %d del archivo a procesar NO EXISTE.", bloque);
											}else{ // Si la COPIA1 existe entonces recibo su información

												bloque_actual->copia1.existe_copia = true;
												log_info(log_YAMA, "EXISTE la COPIA 1 del bloque %d del archivo a procesar.", bloque);

												// PASO 2 - GUARDO EL NOMBRE_NODO DONDE ESTA EL BLOQUE (NODON)
												strcpy(bloque_actual->copia1.nombre_nodo, protocolo_Recepcion->mensaje);

												// PASO 3 - RECIBO LA IP_NODO
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												strcpy(bloque_actual->copia1.ip_nodo, protocolo_Recepcion->mensaje);

												// PASO 4 - RECIBO EL PUERTO_WORKER_ESCUCHA_MASTER
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia1.puerto_worker_escucha_master = atoi(protocolo_Recepcion->mensaje);

												// PASO 5 - RECIBO EL NUMERO DEL BLOQUE_DATABIN DONDE SE GUARDA EL CONTENIDO DEL BLOQUE DENTRO DEL DATA.BIN DEL NODO NOMBRE_NODO
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia1.numero_bloque_databin = atoi(protocolo_Recepcion->mensaje);

												// PASO 6 - RECIBO LOS BYTES_OCUPADOS DENTRO DEL BLOQUE (CUANTO DEL 1MiB)
												eliminar_protocolo(protocolo_Recepcion);
												protocolo_Recepcion = recibir_mensaje(socket_YAMA_FILESYSTEM);

												bloque_actual->copia1.bytes_ocupados = atoi(protocolo_Recepcion->mensaje);

											} // Fin else recibo la COPIA1*/

											/*** FIN - RECIBO COPIA1 ***/

											// Agrego el bloque actual (que contiene la información de sus dos copias) a la lista de bloques
											list_add(lista_bloques_archivo, bloque_actual);

											// Actualizo la lista de Nodos agregando aquellos que no tenga y cargando su ip y puerto
											actualizar_lista_nodos(lista_bloques_archivo);

										} // FIN else recibo las dos copias del bloque actual

									} // FIN for recibo TODOS los bloques del archivo perteneciente al job actual

								} // FIN if ( protocolo_Recepcion->funcion == CANTIDAD_DE_BLOQUES)

								// ---> En este punto voy a tener la lista_bloque_archivo cargada con toda la información de las dos copias de cada bloque del archivo a procesar (del job actual)

								/*
								===============================================================================================
								INICIO - existen_todos_los_bloques == TRUE (Se puede iniciar la TRANSFORMACION del JOB actual
								===============================================================================================
								 */

								if ( existen_todos_los_bloques == true ){

									/* IMPORTANTE: La planificación de un job NO PUEDE FALLAR. Si se llego a este punto significa que existe, al menos,
									 * una copia de cada bloque del archivo. Por lo tanto, la planificación se ejecutará correctamente.
									 */

									// Aplico el RETARDO de la planificación
									usleep(RETARDO_PLANIFICACION);

									// PLANIFICO: Obtengo el plan de ejecución
									t_list* tabla_algoritmo_clock = planificacion(lista_bloques_archivo, cantidad_bloques_archivo, ALGORITMO_BALANCEO, DISPONIBILIDAD_BASE);

									// Actualizo la TABLA DE ESTADOS con la info del JOB actual
									actualizar_tabla_estados(cantidad_bloques_archivo, nro_job, numero_master, tabla_algoritmo_clock, lista_bloques_archivo);

									// Como se inicia un nuevo job, grabo la TABLA DE ESTADOS en un archivo para que pueda ser consultada
									grabar_tabla_estados_en_archivo();

									// Envio la información de cada bloque a procesar (seleccionado por la planificación) a MASTER
									enviar_info_bloque_a_transformar(fd_i, cantidad_bloques_archivo, lista_bloques_archivo);

								} // FIN if ( existen_todos_los_bloques == true )


								/*
								----------------------------------------------------------------------------------------
								FIN - existen_todos_los_bloques == TRUE (Se puede iniciar la TRANSFORMACION del JOB actual
								----------------------------------------------------------------------------------------
								 */

								nro_job++; // VARIABLE GLOBAL: Incremento el numero de Job para asignarle al proximo

								// flag = false; // Descomentar para valgrind

								break;

								/*
								--------------------------------------------------
								FIN - case RUTA_ARCHIVO_A_PROCESAR (NUEVO JOB)
								--------------------------------------------------
								 */


								/*
								=====================================================================
								INICIO - MASTER me avisa que un bloque se transformo correctamente
								=====================================================================
								 */

								case BLOQUE_TRANSFORMADO_OK:

									/* 1° Reconozco el MASTER que me esta enviando el mensaje:
									 *
									 * Cada Job tiene su propio Nro de MASTER asignado. Inclusive, aunque un JOB
									 * termine, el Nro de MASTER siempre va en incremento, así que por más que dos
									 * JOB distintos tengan el mismo fd_i su Nro de MASTER dentro de YAMA va a ser
									 * diferente y los distinguirá.
									 */

									nro_bloque_archivo_transformado_ok = atoi(protocolo_Recepcion->mensaje);
									nro_job_actual = obtener_nro_job_para_master(fd_i);
									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									log_trace(log_YAMA, "El bloque %d perteneciente al JOB %d se ha transformado correctamente.", nro_bloque_archivo_transformado_ok, nro_job_actual);

									// 2° Marco como "OK" el estado del bloque que se transformo en forma correcta.
									actualizar_tabla_estados_bloque_ok(nro_job_actual, nro_bloque_archivo_transformado_ok);
									grabar_tabla_estados_en_archivo(); // Me bajo las novedades al archivo de la tabla de estados

									// 3° Recibo el Nombre del Nodo donde se efectuo correctamente la transformación
									protocolo_Recepcion = recibir_mensaje(fd_i);
									if ( protocolo_Recepcion->funcion == NOMBRE_NODO_BLOQUE_TRANSFORMADO_OK){
										nombre_nodo_actual = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
										strcpy(nombre_nodo_actual, protocolo_Recepcion->mensaje);
									}else{
										log_error(log_YAMA, "Error al recibir el nombre del Nodo donde bloque %d se transformo correctamente.", nro_bloque_archivo_transformado_ok);
										return EXIT_FAILURE;
									}
									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// 4° Calculo el numero de Nodo actual y luego disminuyo la carga del WORKER asociado a dicho Nodo en una unidad
									numero_nodo_actual = obtener_numero_nodo(nombre_nodo_actual);
									tabla_global_workers[numero_nodo_actual].carga_actual--;

									/*
									=====================================================================
									INICIO - Chequeo si puedo iniciar una etapa de REDUCCION_LOCAL
									=====================================================================
									 */

									/* Cada vez que un bloque se transforma correctamente tengo que chequear si no es el
									 * último bloque transformado ok de dicho Nodo. De ser así, tengo que iniciar la etapa
									 * de reducción local en ese Nodo.
									 */

									inicio_reduccion_local = chequear_nodo_iniciar_reduccion_local(nombre_nodo_actual, nro_job_actual);

									if ( inicio_reduccion_local == true ){

										// Le indico a MASTER que inicie la REDUCCION_LOCAL en el Nodo nombre_nodo_actual
										enviar_mensaje(REDUCCION_LOCAL, "Mensaje vacio", fd_i);

										// 1° - Envio a MASTER la lista de nombres temporales del Nodo a reducir
										lista_temporales_trans = lista_nombres_temporales_trans_nodo_reduccion_local(nombre_nodo_actual, nro_job_actual);

										// 1a - Primero le envío la cantidad de nombres temporales
										cantidad_temporales = list_size(lista_temporales_trans);
										cantidad_temporales_string = string_itoa(cantidad_temporales);
										enviar_mensaje(CANTIDAD_TEMPORALES, cantidad_temporales_string, fd_i);
										free(cantidad_temporales_string); // Libero la memoria usada por string_itoa

										// 1b - Ahora si le envío, uno a uno, los nombres de los temporales

										for(nro_temporal = 0; nro_temporal < cantidad_temporales; nro_temporal++){

											// Envio, uno a uno, todos los nombres temporales a reducir
											temporal_actual = list_get(lista_temporales_trans, nro_temporal);
											enviar_mensaje(NOMBRE_TEMPORAL_A_REDUCIR, temporal_actual->nombre_temporal, fd_i);

										} // FIN for

										// Ya no necesito la lista de temporales para este Job, así que libero su memoria
										list_iterate(lista_temporales_trans, free);
										list_destroy(lista_temporales_trans);

										// 2° - Envio a MASTER la info (nombre, ip y puerto) del Nodo (WORKER) donde se va a hacer la reduccion local
										t_nodo* info_nodo_actual = recuperar_info_nodo(nombre_nodo_actual);

										// 2a - Envio el nombre del Nodo
										enviar_mensaje(NOMBRE_NODO_REDUCCION, info_nodo_actual->nombre_nodo, fd_i);

										// 2b - Envio la ip del Nodo
										enviar_mensaje(IP_NODO_REDUCCION, info_nodo_actual->ip, fd_i);

										// 2c - Envio el puerto donde el WORKER va a escuchar conexiones de MASTERs
										puerto_worker_reduccion_string = string_itoa(info_nodo_actual->puerto_worker_escucha_master);
										enviar_mensaje(PUERTO_WORKER_ESCUCHA_MASTER_REDUCCION, puerto_worker_reduccion_string, fd_i);
										free(puerto_worker_reduccion_string); // LIbero la memoria usada por string_itoa

										/*** ACTUALIZO LA TABLA DE ESTADOS ***/

										// Agrego la nueva entrada donde se indica el inicio de la correspondiente REDUCCION LOCAL a la tabla de estados
										nombre_temporal_reduccion_local = agregar_entrada_reduccion_local_tabla_estados(nro_job_actual, fd_i, nombre_nodo_actual);

										// Como actualice la tabla de estados es un buen momento para bajar la última versión al archivo
										grabar_tabla_estados_en_archivo();

										// 3° - Por último, envio a MASTER el nombre temporal con el que guardar el resultado de la reducción local
										enviar_mensaje(NOMBRE_RESULTADO_REDUCCION_LOCAL, nombre_temporal_reduccion_local, fd_i);

									} // FIN ( inicio_reduccion_local == true )

									free(nombre_nodo_actual); // Libero nombre_nodo_actual para que la pueda usar otro case

									/*
									---------------------------------------------------------------------
									FIN - Chequeo si puedo iniciar una etapa de REDUCCION_LOCAL
									---------------------------------------------------------------------
									 */

									break;

								/*
								---------------------------------------------------------------------
								FIN - MASTER me avisa que un bloque se transformo correctamente
								---------------------------------------------------------------------
								 */


								/*
								====================================================================
								INICIO - MASTER me informa que fallo la transformación de un bloque
								====================================================================
								 */

								case BLOQUE_TRANSFORMADO_ERROR:

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									nro_bloque_archivo_transformado_error = atoi(protocolo_Recepcion->mensaje);
									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									log_error(log_YAMA, "El bloque número %d del archivo perteneciente al JOB nro %d NO se ha podido transformar", nro_bloque_archivo_transformado_error, nro_job_actual);
									log_info(log_YAMA, "Se aborta el JOB nro %d", nro_job_actual);

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									enviar_mensaje(ABORTAR_JOB, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									break;


								/*
								---------------------------------------------------------------------
								FIN - MASTER me informa que fallo la transformación de un bloque
								---------------------------------------------------------------------
								 */


								/*
								==============================================================================
								INICIO - ETAPA_REDUCCION_LOCAL_OK
								==============================================================================
								 */

								case ETAPA_REDUCCION_LOCAL_OK:

									// Recibo el nombre del Nodo donde la reducción local se completo exitosamente
									nombre_nodo_actual = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
									strcpy(nombre_nodo_actual, protocolo_Recepcion->mensaje);
									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									log_trace(log_YAMA, "Se ha concluido exitosamente la etapa REDUCCION LOCAL para el JOB %d en el %s",nro_job_actual, nombre_nodo_actual);

									// Marco como "OK" el Nodo del Job donde se concreto bien la reducción local
									actualizar_tabla_estados_nodo_reduccion_local(nombre_nodo_actual, nro_job_actual, "OK");
									grabar_tabla_estados_en_archivo(); // Me bajo las novedades al archivo de la tabla de estados


									/*
									=====================================================================
									INICIO - Chequeo si puedo iniciar una etapa de REDUCCION_GLOBAL
									=====================================================================
									 */

									inicio_reduccion_global = chequear_job_iniciar_reduccion_global(nro_job_actual);

									// SI inicio_reduccion_local es true entonces inicio la REDUCCION GLOBAL el el job nro_job_actual
									if ( inicio_reduccion_global == true ){

										log_trace(log_YAMA, "Se inicia la etapa de REDUCCION GLOBAL para el JOB nro %d", nro_job_actual);

										// 1° - Elijo el Nodo (WORKER) que va a ser el ENCARGADO de la reducción global
										/* Será el WORKER que menor carga actual WL(w) tenga - ver tabla global de workers -
										 * de los que hayan PARTICIPADO en el JOB.
										 */
										t_list* lista_nodos_participantes = generar_lista_nodos_job_actual(nro_job_actual);
										elegir_nodo_encargado(lista_nodos_participantes);

										// 2° - Le indico a MASTER que quiero iniciar una reducción global enviándole el mensaje NODO_ENCARGADO
										/* Para ello, en el contenido del mensaje le envio la cantidad de Nodos que participaron en el
										 * presente job, de manera que MASTER sepa la cantidad de mensajes a recibir.
										 */
										cantidad_nodos_participantes = list_size(lista_nodos_participantes);
										cantidad_nodos_participantes_string = string_itoa(cantidad_nodos_participantes);
										enviar_mensaje(NODO_ENCARGADO, cantidad_nodos_participantes_string, fd_i);
										free(cantidad_nodos_participantes_string);


										// 3° - Le envio a MASTER la lista de nodos participantes, uno a uno
										for(indice_nodo= 0; indice_nodo < list_size(lista_nodos_participantes); indice_nodo++){

											// Envio, uno a uno, la info de cada Nodo participante del job actual
											nodo_job_actual = list_get(lista_nodos_participantes, indice_nodo);

											// 1° Envio el nombre del Nodo actual
											enviar_mensaje(NOMBRE_NODO_PARTICIPANTE, nodo_job_actual->nombre_nodo, fd_i);

											// 2° Envio la ip del Nodo actual
											enviar_mensaje(IP_NODO_PARTICIPANTE, nodo_job_actual->ip, fd_i);

											// 3° Envio el puerto del Nodo actual
											puerto_worker_nodo_participante_string = string_itoa(nodo_job_actual->puerto_worker);
											enviar_mensaje(PUERTO_NODO_PARTICIPANTE, puerto_worker_nodo_participante_string, fd_i);
											free(puerto_worker_nodo_participante_string);

											// 4° Envio el nombre temporal de la reducción local en dicho Nodo
											enviar_mensaje(NOMBRE_REDUC_LOCAL_NODO_PARTICIPANTE, nodo_job_actual->nombre_temporal_reduccion, fd_i);

											// 4° Envio el nombre temporal de la reducción local en dicho Nodo
											es_nodo_encargado_string = string_itoa(nodo_job_actual->encargado);
											enviar_mensaje(ES_NODO_ENCARGADO, es_nodo_encargado_string, fd_i);
											free(es_nodo_encargado_string);

										} // FIN for


										/*** ACTUALIZO LA TABLA DE ESTADOS ***/

										// Agrego la nueva entrada donde se indica el inicio de una nueva REDUCCION_GLOBAL en estado "En proceso" para el job actual
										nombre_temporal_reduccion_global = agregar_entrada_reduccion_global_tabla_estados(nro_job_actual, lista_nodos_participantes, fd_i);

										// Como actualice la tabla de estados es un buen momento para bajar la última versión al archivo
										grabar_tabla_estados_en_archivo();

										// Por último, envio a MASTER el nombre temporal con el que guardar el resultado de la reducción global
										enviar_mensaje(NOMBRE_RESULTADO_REDUCCION_GLOBAL, nombre_temporal_reduccion_global, fd_i);


									} // FIN if ( inicio_reduccion_global == true )



									/*
									---------------------------------------------------------------------
									FIN - Chequeo si puedo iniciar una etapa de REDUCCION_GLOBAL
									---------------------------------------------------------------------
									 */

									free(nombre_nodo_actual); // Libero nombre_nodo_actual para que la pueda usar otro case

									break;

								/*
								---------------------------------------------------------------------------
								FIN - ETAPA_REDUCCION_LOCAL_OK
								---------------------------------------------------------------------------
								 */


								/*
								==============================================================================
								INICIO - ETAPA_REDUCCION_LOCAL_ERROR
								==============================================================================
								 */

								case ETAPA_REDUCCION_LOCAL_ERROR:

									// Recibo el nombre del Nodo donde la reducción local fallo
									nombre_nodo_actual = malloc(strlen(protocolo_Recepcion->mensaje) + 1); // +1 del '\0'
									strcpy(nombre_nodo_actual, protocolo_Recepcion->mensaje);
									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									log_error(log_YAMA, "La etapa de REDUCCION LOCAL en el Nodo %s para el JOB nro %d falló.", nombre_nodo_actual, nro_job_actual);
									log_info(log_YAMA, "Se aborta el JOB nro %d", nro_job_actual);

									// Marco como "ERROR" la entrada del Job donde fallo la reducción global
									actualizar_tabla_estados_nodo_reduccion_local(nombre_nodo_actual, nro_job_actual, "ERROR");;

									// Me bajo las novedades al archivo de la tabla de estados
									grabar_tabla_estados_en_archivo();

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									enviar_mensaje(ABORTAR_JOB, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									log_info(log_YAMA, "Le aviso a MASTER que el JOB se ha abortado.");

									break;

								/*
								---------------------------------------------------------------------------
								FIN - ETAPA_REDUCCION_LOCAL_ERROR
								---------------------------------------------------------------------------
								 */


								/*
								==============================================================================
								INICIO - ETAPA_REDUCCION_GLOBAL_OK
								==============================================================================
								 */

								case ETAPA_REDUCCION_GLOBAL_OK:

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									log_trace(log_YAMA, "Ha concluido con éxito la etapa de REDUCCION GLOBAL en el JOB nro %d.", nro_job_actual);

									// Marco como "OK" la entrada del Job donde se concreto bien la reducción global
									actualizar_tabla_estados_job_reduccion_global(nro_job_actual, "OK");

									// Me bajo las novedades al archivo de la tabla de estados
									grabar_tabla_estados_en_archivo();

									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									//enviar_mensaje(FIN_JOB_EXITO, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									//enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									break;

								/*
								---------------------------------------------------------------------------
								FIN - ETAPA_REDUCCION_GLOBAL_OK
								---------------------------------------------------------------------------
								 */


								/*
								==============================================================================
								INICIO - ETAPA_REDUCCION_GLOBAL_ERROR
								==============================================================================
								 */

								case ETAPA_REDUCCION_GLOBAL_ERROR:

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									log_error(log_YAMA, "Falló la etapa de REDUCCION GLOBAL en el JOB nro %d.", nro_job_actual);
									log_info(log_YAMA, "Se aborta el JOB nro %d", nro_job_actual);

									// Marco como "ERROR" la entrada del Job donde fallo la reducción global
									actualizar_tabla_estados_job_reduccion_global(nro_job_actual, "ERROR");

									// Me bajo las novedades al archivo de la tabla de estados
									grabar_tabla_estados_en_archivo();

									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									enviar_mensaje(ABORTAR_JOB, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									log_info(log_YAMA, "Le indico a MASTER que el JOB no se ha podido completar.");

									break;

								/*
								---------------------------------------------------------------------------
								FIN - ETAPA_REDUCCION_GLOBAL_ERROR
								---------------------------------------------------------------------------
								 */


								case ETAPA_ALMACENAMIENTO_FINAL_OK:

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									/* NO hace falta agregarlo a la tabla de estados, simplemente imprimimos por pantalla
									 * que el almacenamiento final se hizo correctamente
									 */
									log_trace(log_YAMA, "La etapa de ALMACENAMIENTO FINAL para el JOB nro %d se concreto EXITOSAMENTE.", nro_job_actual);

									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									enviar_mensaje(FIN_JOB_EXITO, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									log_info(log_YAMA, "Le indico a MASTER que el job ha finalizado correctamente.");

									break;


								case ETAPA_ALMACENAMIENTO_FINAL_ERROR:

									// Puedo saber el Nro de Job en función del fd del MASTER que me está hablando
									nro_job_actual = obtener_nro_job_para_master(fd_i);

									/* NO hace falta agregarlo a la tabla de estados, simplemente imprimimos por pantalla
									 * que el almacenamiento final falló.
									 */
									log_error(log_YAMA, "Falló la etapa de ALMACENAMIENTO FINAL en el JOB nro %d.", nro_job_actual);
									log_info(log_YAMA, "Se aborta el JOB nro %d", nro_job_actual);

									eliminar_protocolo(protocolo_Recepcion); // Libero el protocolo para recibir otro mensaje

									// Le aviso a MASTER que el job actual se aborto para que finalice su ejecución
									enviar_mensaje(ABORTAR_JOB, "Mensaje vacio", fd_i);

									// Envio un mensaje vacio para respetar el formato del while de MASTER
									enviar_mensaje(MENSAJE_VACIO, "Mensaje_vacio", fd_i);

									log_info(log_YAMA, "Le indico a MASTER que el JOB ha sido abortado.");

									break;


							default:
								break;

							} // FIN SWITCH

							/*
							--------------------------------------------------
							FIN - CENTRO DE GESTIÓN DE MENSAJES DEL SELECT
							--------------------------------------------------
							 */

						} // FIN else recibir contenido mensaje

						/******** FIN - CENTRO DE GESTIÓN DE MENSAJES DEL SELECT ******/


					} // fin else fd_i es un socket de cliente

				} // fin if fd_i está en el conjunto read_fds

			} // FIN for recorrer todos los fds

		} /*** FIN BUCLE PRINCIPAL while(true) ***/

		/*
		--------------------------------------------------
		FIN SELECT
		--------------------------------------------------
		 */


		/*
		==========================================================================================
		INICIO CIERRE Y LIBERACION DE RECURSOS GENERALES
		==========================================================================================
		 */

		list_iterate(lista_MASTERs, free); // Libero la memoria utilizada por cada nodo de la lista_MASTERs
		list_destroy(lista_MASTERs); // Libero la memoria alocada por lista_MASTERs

		list_iterate(tabla_estados, free); // Libero la memoria utilizada por cada nodo de la lista_solicitudes
		list_destroy(tabla_estados); // Libero la memoria alocada por lista_solicitudes

		close(socket_escucha_MASTER);
		close(socket_YAMA_FILESYSTEM);
		config_destroy(config);
		log_destroy(log_YAMA);

		/*
		-----------------------------------------------
		FIN CIERRE Y LIBERACION DE RECURSOS GENERALES
		-----------------------------------------------
		 */

		return EXIT_SUCCESS;

} /***** FIN MAIN *****/
