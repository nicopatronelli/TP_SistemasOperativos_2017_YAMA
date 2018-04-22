/*
 * File System.c
 *
 *  Created on: 3/9/2017
 *  Author    : Mandale Fruta
 *  Versión   : 1.0
 *
 */
#include "Globales.h"
#include "Directorios.h"
#include "Nodos.h"
#include "Consola.h"
#include "funcionesFILESYSTEM.h"
#include "Mensajes.h"

int main(int argc, char *argv[]) {

	// Variables generales
	estadoSeguro = false;
	estadoCargado = false;
	int estadoAnterior = false;
	int YAMAConectado = false;
	int socketYAMA = 0;
	int dataNodesConectados = 0;
	int workersConectados = 0;
	int i = 0;
	modoClean = false;

	// Variables para SELECT
    fd_set temporalFD;
    FD_ZERO(&maestroFD);
    FD_ZERO(&temporalFD);

    // Inicializo los semáforos
    sem_init(&set_bloque_OK, 0, 0);
    sem_init(&set_bloque_NO_OK, 0, 0);
    sem_init(&get_bloque_OK, 0, 0);
    sem_init(&get_bloque_NO_OK, 0, 0);

    // Estructuras para aceptar DataNodes, YAMA y Workers

    struct sockaddr_in direccionDataNode;
    struct sockaddr_in direccionYAMA;
    struct sockaddr_in direccionWorker;

    // Estructura para manejo de DataNodes y Workers conectados

    listaDataNodes = list_create();

    // Obtener los datos del archivo de configuración
    t_config* config = config_create("config.txt");
    archivoLog = log_create("File_System.log", "FILE SYSTEM", true, LOG_LEVEL_TRACE);
    int puertoEscuchaDataNodes = config_get_int_value(config, "PUERTO_ESCUCHA_DATANODE");
    int puertoEscuchaYAMA = config_get_int_value(config, "PUERTO_ESCUCHA_YAMA");
    int puertoEscuchaWorkers = config_get_int_value(config, "PUERTO_ESCUCHA_WORKERS");
    retardoOKBloques = config_get_int_value(config, "RETARDO_OK_BLOQUES");
    config_destroy(config);

    // Logueo los valores obtenidos del archivo de configuración

    log_info(archivoLog, "El puerto de escucha para las Data Nodes es el: %d.", puertoEscuchaDataNodes);
    log_info(archivoLog, "El puerto de escucha para YAMA es el: %d.", puertoEscuchaYAMA);
    log_info(archivoLog, "El puerto de escucha para Workers es el: %d.", puertoEscuchaWorkers);

	// Me fijo si debo ejecutar con el comando --clean o no
	for (i=1; i< argc; i++) {
		if ((strncmp(argv[1], "--clean", 7)) == 0) {
			log_trace(archivoLog, "Se ejecuta con parámetro para ignorar estado anterior.");
			 modoClean = true;
		}
	 }

    // Verifico si existe un estado anterior en caso de que el parámetro así lo indique
	if (modoClean == false) {
	    estadoAnterior = verificarEstadoAnterior();
	}

    // Si existe un estado anterior, lo cargo.
    // Si no, creo la Tabla de Directorios formateada.
    if (estadoAnterior == true) {
    	log_info(archivoLog, "Cargando estado anterior...");
    	estadoCargado = cargarEstadoAnterior();
        if (estadoCargado == true) {
        	log_info(archivoLog, "El File System se encuentra en un estado no seguro hasta tanto se conecten los nodos necesarios.");
        	log_trace(archivoLog, "Estado anterior cargado correctamente.");
        } else {
        	log_error(archivoLog, "Error al cargar el estado anterior.");
        	log_trace(archivoLog, "Se generan las estructuras formateadas.");
        	eliminarMetadata();
        	crearMetadata();
        	crearTablaDirectorios();
        	crearTablaArchivos();
        	crearTablaNodos();
        	estadoCargado = false;
        }
    } else {
    	log_info(archivoLog, "No existe o se ignora estado anterior.");
    	log_trace(archivoLog, "Se generan las estructuras formateadas.");
    	eliminarMetadata();
    	crearMetadata();
    	crearTablaDirectorios();
    	crearTablaArchivos();
    	crearTablaNodos();
    }

    // Hilo para estar a la espera de comandos desde la consola de File System
    pthread_t id_HiloComandos;

    // Tiro el hilo
    if(pthread_create(&id_HiloComandos, NULL, (void*)comandosFileSystem, NULL) < 0) {
    	log_error(archivoLog, "No se pudo crear hilo para leer comandos desde la consola.");
    	return ERROR;
    }

    // Enlazar mi dirección a la struct sockaddr_in con el puerto de escucha de Data Nodes

    struct sockaddr_in direccionFileSystem = crearDireccion(AF_INET, INADDR_ANY, puertoEscuchaDataNodes);

    // Me pongo a la Escucha de Data Nodes

    int socketEscuchaDataNodes = ponerseALaEscuchaEn(direccionFileSystem);

    if (socketEscuchaDataNodes > 0) {
        log_trace(archivoLog, "Esperando conexiones de DataNodes en socket %d...", socketEscuchaDataNodes);
        // Añadir socketEscuchaDataNodes al conjunto maestroFD
        FD_SET(socketEscuchaDataNodes, &maestroFD);
    } else {
    	log_error(archivoLog, "Error al ponerse a la escucha de DataNodes.");
    }

    // Ahora enlazo mi dirección con el puertoEscuchaYAMA

    direccionFileSystem.sin_port = htons(puertoEscuchaYAMA);

    // Me pongo a la escucha de YAMA

    int socketEscuchaYAMA = ponerseALaEscuchaEn(direccionFileSystem);

    if (socketEscuchaYAMA > 0) {
    	log_trace(archivoLog, "Esperando conexiones de YAMA en socket %d...", socketEscuchaYAMA);
        // Añadir socketEscuchaYAMA al conjunto maestroFD
        FD_SET(socketEscuchaYAMA, &maestroFD);
    } else {
    	log_error(archivoLog, "Error al ponerse a la escucha de YAMA.");
    }

    // Ahora enlazo mi dirección con el puertoEscuchaWorkers

    direccionFileSystem.sin_port = htons(puertoEscuchaWorkers);

    // Me pongo a la escucha de Workers

    int socketEscuchaWorkers = ponerseALaEscuchaEn(direccionFileSystem);

    if (socketEscuchaWorkers > 0) {
    	log_trace(archivoLog, "Esperando conexiones de Workers en socket %d...", socketEscuchaWorkers);
        // Añadir socketEscuchaWorkers al conjunto maestroFD
        FD_SET(socketEscuchaWorkers, &maestroFD);
    } else {
    	log_error(archivoLog, "Error al ponerse a la escucha de Workers.");
    }

    // Seteo los extremos del loopeo de FDs

    int minimoFD = socketEscuchaDataNodes;
    int maximoFD = socketEscuchaWorkers;
    t_protocolo* protocoloRecepcion;
    int retornoSelect;

    while(1) {

    	// Asigno el maestro de FD al temporal
    	temporalFD = maestroFD;



    	do { // Función SELECT
        	retornoSelect = select(maximoFD+1, &temporalFD, NULL, NULL, NULL);
    	} while (retornoSelect < 0);

        i = 0;

        // Scaneo el conjunto de FD hasta el máximo para ver si hay actividad
        for (i = minimoFD; i <= maximoFD; ++i) {
        	if (FD_ISSET(i, &temporalFD)) {
                // La actividad está en el socketEscuchaYAMA: se intenta conectar YAMA
                if (i == socketEscuchaYAMA) {
                	// Pregunto si estoy en estadoSeguro o no
                    // Si no tengo ningun YAMA Conectado, pregunto si estoy en estado estable
                    // Si estoy en estado estable, acepto la conexión de YAMA
                    t_cliente* nuevoYAMA = malloc(sizeof(t_cliente));
                    aceptarClienteEn(socketEscuchaYAMA, direccionYAMA, nuevoYAMA);
                    FD_SET(nuevoYAMA->socket_client, &maestroFD); // Añadir al maestroFD
                    if (nuevoYAMA->socket_client >= maximoFD) maximoFD = nuevoYAMA->socket_client;
                    log_info(archivoLog, "Nuevo YAMA solicitando conexión en el socket %d.", nuevoYAMA->socket_client);

                    // Si no tengo otro YAMA conectado, permito
                    if (YAMAConectado == false) {
						// Rechazo la conexión de YAMA porque no estoy en ESTADO SEGURO
						if (estadoSeguro == false) {
							enviar_mensaje(CONEXION_RECHAZADA, "1", nuevoYAMA->socket_client);
							close(nuevoYAMA->socket_client);
							FD_CLR(nuevoYAMA->socket_client, &maestroFD);
							log_error(archivoLog, "Se rechaza la conexión del YAMA en el socket %d porque el File System no está en Estado Seguro.", nuevoYAMA->socket_client);
							free(nuevoYAMA);
						} else {
							// Como estoy en estado estable, acepto la conexión
							enviar_mensaje(CONEXION_ACEPTADA, "1", nuevoYAMA->socket_client);
							YAMAConectado = true;
		                    socketYAMA = nuevoYAMA->socket_client;
		                    log_trace(archivoLog, "YAMA se ha conectado correctamente desde el socket %d.", socketYAMA);
						}
                    } else {
                    	// Rechazo la conexión de YAMA porque ya tengo otro YAMA conectado
                    	enviar_mensaje(CONEXION_RECHAZADA, "1", nuevoYAMA->socket_client);
                    	close(nuevoYAMA->socket_client);
                    	FD_CLR(nuevoYAMA->socket_client, &maestroFD);
                    	log_error(archivoLog, "Se rechaza la conexión del YAMA en el socket %d porque el File System ya tiene otro YAMA conectado.", nuevoYAMA->socket_client);
                    	free(nuevoYAMA);
                    }

                // Nuevo DataNode, lo doy de alta en la lista de DataNodes conectados
            	// Agrego el socket al maestro de FD
            	// Incremento la cantidad de dataNodesConectados
                } else if (i == socketEscuchaDataNodes) {
                    	t_datanode* nuevoDataNode = malloc(sizeof(t_datanode));
                    	aceptarDataNode(socketEscuchaDataNodes, direccionDataNode, nuevoDataNode);
                    	list_add(listaDataNodes, nuevoDataNode);
                    	FD_SET(nuevoDataNode->socket_client, &maestroFD); // Añadir al maestroFD
                    	if (nuevoDataNode->socket_client >= maximoFD) maximoFD = nuevoDataNode->socket_client;
                    	//log_info(archivoLog, "Nuevo nodo desde la IP %s en el socket %d solicitando conexión.", nuevoDataNode->ipNodo, nuevoDataNode->socket_client);
                    	dataNodesConectados++;
                // Nuevo Worker
                // Agrego el socket al maestro de FD
                // Incremento la cantidad de workersConectados
                } else if (i == socketEscuchaWorkers) {
                    	t_cliente* nuevoWorker = malloc(sizeof(t_cliente));
                    	aceptarClienteEn(socketEscuchaWorkers, direccionWorker, nuevoWorker);
                    	FD_SET(nuevoWorker->socket_client, &maestroFD); // Añadir al maestroFD
                    	if (nuevoWorker->socket_client >= maximoFD) maximoFD = nuevoWorker->socket_client;
                    	log_trace(archivoLog, "Worker conectado desde el socket %d.", nuevoWorker->socket_client);
                    	workersConectados++;
                    	free(nuevoWorker);
                		} else {
                			// Se me están desconectando, me fijo quién: YAMA, DataNode o Worker
							protocoloRecepcion = recibir_mensaje(i);
							if (protocoloRecepcion == CERRARON_SOCKET) {
								// Conexión cerrada desde el cliente
								// Se me desconectó YAMA
								if (i == socketYAMA) {
									log_error(archivoLog, "Se me desconectó YAMA en el socket %d.", socketYAMA);
									close(socketYAMA);
									FD_CLR(socketYAMA, &maestroFD); // Eliminar del conjunto maestroFD
									YAMAConectado = false;
								} else {
									// Me fijo si se me desconectó un DataNode
									int esSocketDataNode = false;
									char* nombreNodo = malloc(10);
									// Si tengo DataNodes conectados, itero en busca del socket
									if (list_size(listaDataNodes) > 0 ) {
										esSocketDataNode = buscarSocketEn(listaDataNodes, i, nombreNodo);
									}
									// Reconocí la desconexión de un Datanode
									if (esSocketDataNode > 0) {
										log_error(archivoLog, "Se me desconectó el nodo %s en el socket %d.", nombreNodo, i);
										eliminarDataNode(i, nombreNodo);
										dataNodesConectados--;
									} else {
										log_info(archivoLog, "El WORKER ENCARGADO finalizó el envío del archivo de REDUCCION GLOBAL desde el socket %d.", i);
									}
									// Libero
									free(nombreNodo);
									// Cierro el socket
									close(i);
									FD_CLR(i, &maestroFD); // Eliminar del conjunto maestroFD
								}
							} else if (protocoloRecepcion == NULL) {
								log_error(archivoLog, "Hubo un error al recibir un mensaje en el socket %d.", i);
								close(i); // Cierro el socket hay un fallo
								FD_CLR(i, &maestroFD); // Elimino el socket del conjunto master
						} else {
							// Recibí un mensaje de alguno de mis clientes (Nodos o YAMA)
							// Defino qué hacer en base al código de protocolo que recibí
							definirAccionHeader(protocoloRecepcion, i);
							eliminar_protocolo(protocoloRecepcion);
						}
                	} // Fin del else de sockets de escucha
            } // Fin del FD_ISSET()
        } // Fin del for()
    } // Fin del while(1)

    // Libero el archivo de log
    log_destroy(archivoLog);

    // Libero las listas
    list_destroy(listaDataNodes);

    // Libero el espacio mapeado a memoria del archivo
    munmap(arrayDirectorios, sizeof(struct t_directory) * CANTIDAD_DIRECTORIOS);

    return 0;

}
