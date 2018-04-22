/*
 * protocolo.h
 *
 *  Created on: 23/4/2017
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include "estructuras.h"
#include <sys/socket.h> //Necesaria para socket() e inet_addr()

#ifndef SHARED_PROTOCOLO_H_
#define SHARED_PROTOCOLO_H_

// Mensajes para Handshake
#define HANDSHAKE_FS 101
#define HANDSHAKE_DATANODE 201
#define HANDSHAKE_WORKER 301
#define HANDSHAKE_MASTER 401
#define HANDSHAKE_YAMA 501
#define DATANODE_CONECTADO 601
#define DATANODE_ACEPTADO 602
#define DATANODE_RECHAZADO 603
#define HEADER_SIZE 8

// Mensajes de File System a YAMA (RUTA_ARCHIVO_A_PROCESAR)
#define ARCHIVO_NO_EXISTE 700
#define CANTIDAD_DE_BLOQUES 701
#define BLOQUE_NO_EXISTE 702
#define NUMERO_DE_BLOQUE 703
#define NUMERO_COPIA 704
#define NOMBRE_DE_NODO 705
#define IP_DE_NODO 706
#define PUERTO_ESCUCHA_MASTER_NODO 707
#define BLOQUE_DATABIN 708
#define BYTES_OCUPADOS 709
#define COPIA0_NO_EXISTE 710
#define COPIA1_NO_EXISTE 711
#define CONEXION_RECHAZADA 712
#define CONEXION_ACEPTADA 713

// Mensajes para File System
#define ALMACENAR_ARCHIVO_REDUCCION_GLOBAL 101
#define ALMACENAR_ARCHIVO 102
#define LEER_ARCHIVO 103
#define GET_BLOQUE 104
#define SET_BLOQUE_NUMERO 105
#define SET_BLOQUE_CONTENIDO 106
#define CANTIDAD_BLOQUES 107
#define BLOQUE_SOLICITADO 108
#define CONTENIDO_ARCHIVO_REDUCCION_GLOBAL 109

// Mensajes de DATANODE a FILESYSTEM
#define GET_BLOQUE_FAILURE 151
#define GET_BLOQUE_SUCCESS 152
#define SET_BLOQUE_FAILURE 153
#define SET_BLOQUE_SUCCESS 154

// Mensaje de WORKER a MASTER
#define TRANSFORMACION_OK 2001
#define TRANSFORMACION_ERROR 2002
#define REDUCCION_LOCAL_OK 2003
#define REDUCCION_LOCAL_ERROR 2004

// Mensajes MASTER a YAMA
#define RUTA_ARCHIVO_A_PROCESAR 901
#define BLOQUE_TRANSFORMADO_OK 902
#define NOMBRE_NODO_BLOQUE_TRANSFORMADO_OK 908
#define BLOQUE_TRANSFORMADO_ERROR 903
#define NOMBRE_NODO_BLOQUE_TRANSFORMADO_ERROR 909
#define ETAPA_REDUCCION_LOCAL_OK 904
#define ETAPA_REDUCCION_LOCAL_ERROR 905
#define ETAPA_REDUCCION_GLOBAL_OK 906
#define ETAPA_REDUCCION_GLOBAL_ERROR 907
#define ETAPA_ALMACENAMIENTO_FINAL_OK 911
#define ETAPA_ALMACENAMIENTO_FINAL_ERROR 912
#define MENSAJE_VACIO 910

// Mensajes MASTER a WORKER (TRANSFORMACION)
#define SCRIPT_TRANSFORMADOR 801
#define SCRIPT_TRANSFORMADOR_OK 803
#define BLOQUE_DATABIN_A_TRANSFORMAR 804
#define BYTES_OCUPADOS_BLOQUE_A_TRANSFORMAR 805
#define NOMBRE_ARCHIVO_TEMPORAL_TRANSFORMACION 806
// Etapas que MASTER le indica ejecutar a WORKER
#define ETAPA_TRANSFORMACION 850
#define ETAPA_REDUCCION_LOCAL 851
#define ETAPA_REDUCCION_GLOBAL 852 // Los WORKERs que participen en una REDUCCION GLOBAL pero NO sean el encargado recibirán este mensaje
#define WORKER_ENCARGADO 853 // El WORKER encargado de realizar la REDUCCION GLOBAL recibirá este mensaje
#define ETAPA_ALMACENAMIENTO_FINAL 854 // Podria estar incluida al final de la reduccion global


// Mensajes MASTER a WORKER (REDUCCION_LOCAL)
#define SCRIPT_REDUCTOR_LOCAL 1400
#define CANTIDAD_TEMPORALES_TRANS 1401
#define NOMBRE_TEMPORAL_TRANS 1402
#define NOMBRE_TEMPORAL_REDUCCION_LOCAL 1403


// Mensajes MASTER a WORKER (REDUCCION GLOBAL)
#define CANTIDAD_NODOS_AYUDANTES 1600
#define IP_NODO_AYUDANTE 1601
#define PUERTO_NODO_AYUDANTE 1602
#define NOMBRE_REDUCCION_LOCAL_NODO_AYUDANTE 1603
#define NOMBRE_REDUCCION_LOCAL_NODO_ENCARGADO 1604
#define SCRIPT_REDUCCION_GLOBAL 1605
#define RUTA_ARCHIVO_FINAL_FS 1606

// Mensajes de FILE SYSTEM a WORKER ENCARGADO (REDUCCION GLOBAL)
#define ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_OK 1900
#define ARCHIVO_REDUCCION_GLOBAL_ALMACENADO_ERROR 1901

// Mensajes de YAMA a MASTER
#define TRANSFORMACION 1500
#define REDUCCION_LOCAL 1501
#define REDUCCION_GLOBAL 1502
#define ALMACENAMIENTO_FINAL 1503
#define FIN_JOB_EXITO 1504
#define ABORTAR_JOB 505
#define NOMBRE_NODO 506
#define CANTIDAD_BLOQUES_ARCHIVO 1507
#define IP_WORKER 1508
#define PUERTO_WORKER 1509
#define NUMERO_BLOQUE_DATABIN 1510
#define BYTES_OCUPADOS_BLOQUE_DATABIN 1511
#define NOMBRE_ARCHIVO_TEMPORAL_TRANS 1512
#define INICIAR_REDUCCION_LOCAL 1513
#define CANTIDAD_TEMPORALES 1514
#define NOMBRE_TEMPORAL_A_REDUCIR 1515
#define NOMBRE_NODO_REDUCCION 1516
#define IP_NODO_REDUCCION 1517
#define PUERTO_WORKER_ESCUCHA_MASTER_REDUCCION 1518
#define NOMBRE_RESULTADO_REDUCCION_LOCAL 1519
// Mensajes para iniciar la etapa de REDUCCION_GLOBAL
#define NODO_ENCARGADO 1520
#define NOMBRE_NODO_PARTICIPANTE 1521
#define IP_NODO_PARTICIPANTE 1522
#define PUERTO_NODO_PARTICIPANTE 1523
#define NOMBRE_REDUC_LOCAL_NODO_PARTICIPANTE 1524
#define ES_NODO_ENCARGADO 1525
#define NOMBRE_RESULTADO_REDUCCION_GLOBAL 1526

// Mensajes de WORKER ENCARGADO a WORKER AYUDANTE
#define WORKER_AYUDANTE 1700
#define NOMBRE_TMP_NODO_AYUDANTE 1701

// Mensajes de WORKER AYUDANTE a WORKER ENCARGADO
#define CONTENIDO_TMP_NODO_AYUDANTE 1800
#define NOMBRE_TMP_NODO_AYUDANTE_ERROR 1801

#define OK 0
#define ERROR -1


// Estructura para envío de mensajes
typedef struct {
    int funcion;
	int sizeMensaje;
    char* mensaje;
} __attribute__((packed)) t_protocolo;

typedef struct {
    uint32_t funcion;
	uint32_t sizeMensaje;
} __attribute__((packed)) t_handshake;


/* @DESC: Crea una estructura de tipo t_handshake y la inicializa con el header y tamanioMensaje
 * pasados por parámetro. Retorna dicha estructura.
 */
t_handshake *crearHandshake (int header ,int tamanioMensaje);
t_protocolo *crearProtocolo (int header ,int tamanioMensaje, char* mensajeProtocolo );

/* @DESC: Retorna el tamaño del handkshake. */
unsigned int size_handshake(t_handshake *handshake);
int size_protocolo(t_protocolo* protocolo);

void serializar_handshake(unsigned char *buffer, t_handshake *handshake);

void serializar_protocolo(void* buffer, t_protocolo* protocolo) ;

int enviarMensajeHandshake (int socket_cliente, int HANDSHAKE);

int serializar_int(int nro, void *buffer);
int serializar_string(char* string, void* buffer, int tamanio_receptor);

void eliminar_handshake(t_handshake *handshake);
void eliminar_protocolo(t_protocolo* protocolo);

void deserializar_handshake(unsigned char *buffer, t_handshake *handshake);
void deserializar_protocolo(unsigned char *buffer, t_protocolo *protocolo);
void deserializar_header (void* buffer, t_protocolo* protocolo);

int deserializar_int(int* nro, void* buffer);
int deserializar_string(char* string, void* buffer, int longitud_string);


/* INICIO Funciones enmascaradas por mati - devuelven int en lugar de void */

int enviarBuffer(unsigned char* buffer, int socket, int size);
int deserializarHeader (unsigned char* buffer, t_protocolo* protocolo);
int recibirMensaje(int socket_recepcion, t_protocolo* protocoloRecepcion);
int serializarProtocolo(unsigned char* buffer, t_protocolo* protocolo);
int deserializarProtocolo(unsigned char* buffer, t_protocolo* protocolo);
int liberarProtocolo(t_protocolo* protocolo);

/* FIN Funciones enmascaradas por mati - devuelven int en lugar de void */

#endif /* SHARED_PROTOCOLO_H_ */
