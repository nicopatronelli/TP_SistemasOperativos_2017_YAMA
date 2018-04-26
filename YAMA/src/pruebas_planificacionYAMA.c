/*
									*** INICIO PRUEBAS PLANIFICACION ***


									tabla_global_workers[1].carga_actual = 3;
									tabla_global_workers[2].carga_actual = 2;
									tabla_global_workers[3].carga_actual = 2;

									tabla_global_workers[1].carga_historica = 3;
									tabla_global_workers[2].carga_historica = 2;
									tabla_global_workers[3].carga_historica = 2;


									int _cantidad_bloques_archivo = 7; // Del 0 al 6
									t_list* _lista_bloques_archivo = list_create();

									// bloque_0
									t_Bloque* bloque0 = malloc(sizeof(t_Bloque));
									bloque0->copia0.existe_copia = true;
									strcpy(bloque0->copia0.nombre_nodo, "NODO1");
									bloque0->copia1.existe_copia = true;
									strcpy(bloque0->copia1.nombre_nodo, "NODO2");
									list_add(_lista_bloques_archivo, bloque0);

									// bloque_1
									t_Bloque* bloque1 = malloc(sizeof(t_Bloque));
									bloque1->copia0.existe_copia = true;
									strcpy(bloque1->copia0.nombre_nodo, "NODO1");
									bloque1->copia1.existe_copia = true;
									strcpy(bloque1->copia1.nombre_nodo, "NODO3");
									list_add(_lista_bloques_archivo, bloque1);

									// bloque_2
									t_Bloque* bloque2 = malloc(sizeof(t_Bloque));
									bloque2->copia0.existe_copia = true;
									strcpy(bloque2->copia0.nombre_nodo, "NODO2");
									bloque2->copia1.existe_copia = true;
									strcpy(bloque2->copia1.nombre_nodo, "NODO3");
									list_add(_lista_bloques_archivo, bloque2);

									// bloque_3
									t_Bloque* bloque3 = malloc(sizeof(t_Bloque));
									bloque3->copia0.existe_copia = true;
									strcpy(bloque3->copia0.nombre_nodo, "NODO1");
									bloque3->copia1.existe_copia = true;
									strcpy(bloque3->copia1.nombre_nodo, "NODO2");
									list_add(_lista_bloques_archivo, bloque3);

									// bloque_4
									t_Bloque* bloque4 = malloc(sizeof(t_Bloque));
									bloque4->copia0.existe_copia = true;
									strcpy(bloque4->copia0.nombre_nodo, "NODO1");
									bloque4->copia1.existe_copia = true;
									strcpy(bloque4->copia1.nombre_nodo, "NODO3");
									list_add(_lista_bloques_archivo, bloque4);

									// bloque_5
									t_Bloque* bloque5 = malloc(sizeof(t_Bloque));
									bloque5->copia0.existe_copia = true;
									strcpy(bloque5->copia0.nombre_nodo, "NODO2");
									bloque5->copia1.existe_copia = true;
									strcpy(bloque5->copia1.nombre_nodo, "NODO3");
									list_add(_lista_bloques_archivo, bloque5);

									// bloque_6
									t_Bloque* bloque6 = malloc(sizeof(t_Bloque));
									bloque6->copia0.existe_copia = true;
									strcpy(bloque6->copia0.nombre_nodo, "NODO1");
									bloque6->copia1.existe_copia = true;
									strcpy(bloque6->copia1.nombre_nodo, "NODO2");
									list_add(_lista_bloques_archivo, bloque6);

									t_list* tabla_algoritmo_clock = planificacion(_lista_bloques_archivo, _cantidad_bloques_archivo, ALGORITMO_BALANCEO, DISPONIBILIDAD_BASE);

									// IMPRIMO EL PLAN DE EJECUCIÓN ACTUAL

									t_entrada_algoritmo* entrada_auxiliar;
									t_bloque_a_procesar* bloque_asignado;

									// 1er entrada de la tabla del algoritmo
									puts("Bloques asignados al Nodo de la primer entrada de la tabla.");
									entrada_auxiliar = list_get(tabla_algoritmo_clock, 0);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 0);
									printf("%d\n", bloque_asignado->nro_bloque);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 1);
									printf("%d\n", bloque_asignado->nro_bloque);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 2);
									printf("%d\n", bloque_asignado->nro_bloque);

									// 2da entrada de la tabla del algoritmo
									puts("Bloques asignados al Nodo de la segunda entrada de la tabla.");
									entrada_auxiliar = list_get(tabla_algoritmo_clock, 1);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 0);
									printf("%d\n", bloque_asignado->nro_bloque);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 1);
									printf("%d\n", bloque_asignado->nro_bloque);

									// 3er entrada de la tabla del algoritmo
									puts("Bloques asignados al Nodo de la tercer entrada de la tabla.");
									entrada_auxiliar = list_get(tabla_algoritmo_clock, 2);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 0);
									printf("%d\n", bloque_asignado->nro_bloque);
									bloque_asignado = list_get(entrada_auxiliar->lista_bloques_asignados, 1);
									printf("%d\n", bloque_asignado->nro_bloque);

									printf("La carga actual del WORKER 1 es %d\n", tabla_global_workers[1].carga_actual);
									printf("La carga histórica del WORKER 1 es %d\n", tabla_global_workers[1].carga_historica);
									printf("La carga actual del WORKER 2 es %d\n", tabla_global_workers[2].carga_actual);
									printf("La carga histórica del WORKER 2 es %d\n", tabla_global_workers[2].carga_historica);
									printf("La carga actual del WORKER 3 es %d\n", tabla_global_workers[3].carga_actual);
									printf("La carga histórica del WORKER 1 es %d\n", tabla_global_workers[3].carga_actual);

									pause();

									*** FIN PRUEBAS PLANIFICACION ***
*/
