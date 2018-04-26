/*

 * funcionesDiversas.h
 *
 *  Created on: 26/11/2017
 *      Author: utnso


#ifndef FUNCIONESDIVERSAS_H_
#define FUNCIONESDIVERSAS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <signal.h>
#include <commons/temporal.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <stdbool.h>


 //EStructura de la Tabla de Estados
 typedef enum{
 	TRANSFOR, REDUC_LOCAL, REDUC_GLOBAL
 }t_etapa;

 typedef enum{
 	EN_PROCESO,FIN_OK,FIN_ERROR
 }t_estado;

 typedef struct {
 	uint32_t job;
 	uint32_t master;
 	uint32_t nodo;
 	//t_bloque bloque;
 	t_etapa etapa;
 	char* archivoTemporal;
 	t_estado estado;
 } t_TablaEstados;

 typedef struct{
	 int idJob;
	 int socket;
	 t_list *transformaciones;
	 t_list *reducciones;
	 t_list *reduccionesGlobales;
 } t_job;

 //Variables Temporales
  uint32_t jobTemp;
  uint32_t masterTemp;
  uint32_t nodoTemp;
 // t_bloque bloqueTemp;
  t_etapa etapaTemp;
  char* archivoTemporalTemp;
  t_estado estadoTemp;

#endif  FUNCIONESDIVERSAS_H_
*/
