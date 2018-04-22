/*
 * pcb.c
 *
 *  Created on: 4/5/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "estructuras.h"

int PID=0;
/*

t_pcb *crearPcb(uint32_t idProceso,uint32_t programCounter,uint32_t cantidadPaginas,t_intructions *indiceDeCodigo,char *indiceDeEtiquetas,t_queue *indiceDelStack, uint32_t exit_code){

	t_pcb *proceso = malloc(sizeof(t_pcb));

	proceso->idProceso = idProceso;
	proceso->programCounter = programCounter;
	proceso->cantidadPaginas = cantidadPaginas;
	proceso->indiceDeCodigo = indiceDeCodigo;
	proceso->indiceDeEtiquetas = indiceDeEtiquetas;
	proceso->indiceDelStack = indiceDelStack;
	proceso->exit_code = exit_code;

	return proceso;
}


t_pcb *inicializarPCB (int pidProceso) {
	t_pcb *nuevoPCB = malloc(sizeof(t_pcb));
	nuevoPCB->idProceso = pidProceso+1;
	nuevoPCB->programCounter = 0;
	nuevoPCB->cantidadPaginas = 0;
	nuevoPCB->cantidadInstrucciones = 0;
	nuevoPCB->cantidadEtiquetas =0;
	nuevoPCB->estadoProceso = string_new();
	strcpy(nuevoPCB->estadoProceso, "NUEVO\0");
	nuevoPCB->cantidadInstrucciones=0;
	nuevoPCB->indiceDeCodigo =  malloc(sizeof (t_intructions));
	nuevoPCB->indiceDeEtiquetas = string_new();
	nuevoPCB->indiceDelStack =malloc(sizeof (t_pila));
	nuevoPCB->exit_code = 0;

    return nuevoPCB;

}


*/
