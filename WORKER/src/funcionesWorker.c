/*
 * funcionesWorker.c
 *
 *  Created on: 2/11/2017
 *      Author: utnso
 */

#include "funcionesWorker.h"

t_log* log_WORKER; // Definimos la variable log_MASTER como GLOBAL para poder acceder a el archivo de log desde cualquier función
int numero_script_trans;
int numero_script_reduc;

t_config* configuracion_WORKER(char* ruta_archivo_configuracion, char** RUTA_DATABIN, int* PUERTO_ESCUCHA_MASTER, int* PUERTO_FS, char** IP_FS){

	t_config* config = config_create(ruta_archivo_configuracion);
	if ( config == NULL){
		log_error(log_WORKER, "No se pudo leer el archivo de configuración");
		return NULL;
	}
	else{
		*RUTA_DATABIN = config_get_string_value(config, "RUTA_DATABIN"); // Recupero la la RUTA del data.bin del Nodo de config.txt
		*PUERTO_ESCUCHA_MASTER = config_get_int_value(config,"PUERTO_WORKER_ESCUCHA_MASTER"); // Recupero el PUERTO de escucha de conexiones de MASTER del config.txt
		*PUERTO_FS = config_get_int_value(config,"PUERTO_FILESYSTEM"); // Recupero el PUERTO donde FILE_SYSTEM escucha conexiones de WORKER
		*IP_FS = config_get_string_value(config, "IP_FILESYSTEM"); // Recupero la la RUTA del data.bin del Nodo de config.txt

		log_info(log_WORKER,"El puerto de escucha para MASTER es: %d", *PUERTO_ESCUCHA_MASTER);
		log_info(log_WORKER,"La ruta del archivo data.bin del Nodo es %s", *RUTA_DATABIN);

		return config;
		}

} // Fin configuracion WORKER


int aplicar_transformacion(t_info_trans* info_trans_actual, void* databin_mapeado){

	// Recupero el contenido (REAL) del bloque desde el data.bin mapeado
	void* contenido_bloque = get_bloque_ocupado(info_trans_actual->nro_bloque_databin, info_trans_actual->bytes_ocupados, databin_mapeado);

	// Persisto el contenido del bloque en un archivo auxiliar
	char* ruta_bloque_archivo = bloque_a_archivo(contenido_bloque, info_trans_actual->nro_bloque_databin);

	/* Tenemos que armar el siguiente string:
	 *
	 * 		"cat bloque_n.bin.csv | ./script_trans_0.py | sort > resul_trans"
	 *
	 * Para luego llamar a system con dicho string como parámetro:
	 *
	 *		system("cat archivo_a_procesar.csv | ./transformador.py | sort > resul_trans");
	 *
	 * NOTA: system() NO forma parte del string en realidad.
	 */

	char* comando_system = malloc( strlen("cat ") + strlen(ruta_bloque_archivo) + strlen(" | ./") + strlen(info_trans_actual->ruta_script_trans) + strlen(" | ") + strlen("sort") + strlen(" > ") + strlen(info_trans_actual->nombre_temp_trans) + 1); // + 1 del '\0'
	strcpy(comando_system, "cat ");
	strcat(comando_system, ruta_bloque_archivo);
	strcat(comando_system, " | ./");
	strcat(comando_system, info_trans_actual->ruta_script_trans);
	strcat(comando_system, " | ");
	strcat(comando_system, "sort");
	strcat(comando_system, " > ");
	strcat(comando_system, info_trans_actual->nombre_temp_trans);

	// Llamo a system utilizando el comando que arme
	int resultado_system = system(comando_system);

	liberar_bloque(contenido_bloque);
	free(ruta_bloque_archivo);
	free(comando_system);

	// system retorna 0 en caso de éxito
	return resultado_system;

} // FIN aplicar_transformacion


int aplicar_reduccion_local(t_info_reduc_local* info_reduc_local){

	/* Antes de aplicar el script reductor local tengo que unificar todos los archivos temporales
	 * transformados en uno sólo apareandolos. Para ello, usamos el comando sort (y NO sort -m).
	 *
	 * Por lo tanto, tenemos que armar el siguiente string:
	 *
	 *     "sort tmp_1 tmp_2 tmp_3 ... tmp_n | ./script_reduc_0.py > resul_reduc_local"
	 *
	 * Para luego llamar a system() con dicho string como parámetro:
	 *
	 *		system("sort tmp_1 tmp_2 tmp_3 ... tmp_n | ./script_reduc_0.py > resul_reduc_local");
	 *
	 * NOTA1: system() NO forma parte del string en realidad.
	 *
	 * NOTA2: sort arma un archivo unificado (apareado) y ordenado, que es lo que buscamos.
	 *
	 */

	// Variables auxiliares
	t_temp_trans* t_tmp;

	// Para no tener que calcular la memoria a utilizar estimo que con 500 Bytes me alcanzará
	char* comando_system = malloc(500);
	strcpy(comando_system, "sort");

	// Voy concatenando todos los temporales a reducir
	int nro_tmp_a_reducir;
	for( nro_tmp_a_reducir = 0; nro_tmp_a_reducir < list_size(info_reduc_local->lista_temp_trans); nro_tmp_a_reducir++ ){

		strcat(comando_system, " "); // Espacio antes de tmp_i
		t_tmp = list_get(info_reduc_local->lista_temp_trans, nro_tmp_a_reducir);
		strcat(comando_system, t_tmp->nombre_temp_trans);

	} // FIN for

	// Concateno lo que me falta
	strcat(comando_system, " | ");
	strcat(comando_system, "./");
	strcat(comando_system, info_reduc_local->ruta_script_reduc_local);
	strcat(comando_system, " > ");
	strcat(comando_system, info_reduc_local->nombre_temp_reduc_local);

	// Llamo a system utilizando el comando que arme
	int resultado_system = system(comando_system);

	free(comando_system); // Libero la memoria que aloque para el string del comando

	// system retorna 0 en caso de éxito
	return resultado_system;

} // FIN aplicar_reduccion_local


int aplicar_reduccion_global(t_list* lista_temporales, char* ruta_script, char* ruta_archivo_resultado){

	/* Antes de aplicar el script reductor global tengo que unificar todos los archivos temporales
	 * reducidos localmente apareandolos. Para ello, usamos el comando sort (y NO sort -m).
	 *
	 * Por lo tanto, tenemos que armar el siguiente string:
	 *
	 *     "sort tmp_1 tmp_2 tmp_3 ... tmp_n | ./script_reduc_0.py > resul_reduc_local"
	 *
	 * Para luego llamar a system() con dicho string como parámetro:
	 *
	 *		system("sort tmp_1 tmp_2 tmp_3 ... tmp_n | ./script_reduc_0.py > resul_reduc_local");
	 *
	 * NOTA1: system() NO forma parte del string en realidad.
	 *
	 * NOTA2: sort arma un archivo unificado (apareado) y ordenado, que es lo que buscamos.
	 *
	 */

	// Variables auxiliares
	t_nombre_temporal_reducido* t_tmp;

	// Para no tener que calcular la memoria a utilizar estimo que con 500 Bytes me alcanzará
	char* comando_system = malloc(500);
	strcpy(comando_system, "sort");

	// Voy concatenando todos los temporales a reducir
	int nro_tmp_a_reducir;
	for( nro_tmp_a_reducir = 0; nro_tmp_a_reducir < list_size(lista_temporales); nro_tmp_a_reducir++ ){

		strcat(comando_system, " "); // Espacio antes de tmp_i
		t_tmp = list_get(lista_temporales, nro_tmp_a_reducir);
		strcat(comando_system, t_tmp->nombre_temporal_reducido);

	} // FIN for

	// Concateno lo que me falta
	strcat(comando_system, " | ");
	strcat(comando_system, "./");
	strcat(comando_system, ruta_script);
	strcat(comando_system, " > ");
	strcat(comando_system, ruta_archivo_resultado);
	printf("->>>>> El comando system de la reducción global es %s\n: ", comando_system);

	int resultado_system = system(comando_system);

	free(comando_system);

	// system retorna 0 en caso de éxito
	return resultado_system;

} // FIN aplicar_reduccion_global


char* script_trans_a_archivo(void* buffer_script_trans){

	/* Uso la VARIABLE_GLOBAL numero_script_trans para crear un nombre único para el nombre del archivo
	 * donde voy a almacenar el buffer con el script transformador. Tengo que hacerlo, pues WORKER
	 * recibe solicitudes de distintos MASTERs que inician diferentes Jobs (y recordar que cada Job
	 * tiene su propio script transformador).
	 */

	// Convierto numero_script_trans de int a char*
	char* numero_trans_string = string_itoa(numero_script_trans);

	/* Reservo memoria para nombre_archivo (ruta del archivo donde se va a guardar el script transformador)
	 * Ejemplo: script_trans_2.py
	 */
	char* nombre_archivo = malloc(strlen("script_trans_") + strlen(numero_trans_string) + strlen(".py") + 1); // + 1 del '\0'
	strcpy(nombre_archivo, "script_trans_");
	strcat(nombre_archivo, numero_trans_string);
	strcat(nombre_archivo, ".py");

	numero_script_trans++; // Incremento el numero de script transformador para no repetir el nombre en la siguiente invocación
	free(numero_trans_string);

	int confirmacion = persistir_buffer_en_archivo(nombre_archivo, buffer_script_trans);

	if ( confirmacion == OK){
		return nombre_archivo; // Retorno la RUTA donde se guardo el archivo con el script transformador para que WORKER sepa donde está
	}else{
		return NULL;
	}

} // FIN script_trans_a_archivo


char* script_reduc_a_archivo(void* buffer_script_reduc){

	/* Uso la VARIABLE_GLOBAL numero_script_reduc para crear un nombre único para el nombre del archivo
	 * donde voy a almacenar el buffer con el script reductor. Tengo que hacerlo, pues WORKER
	 * recibe solicitudes de distintos MASTERs que inician diferentes Jobs (y recordar que cada Job
	 * tiene su propio script reductor).
	 */

	// Convierto numero_script_reduc de int a char*
	char* numero_reduc_string = string_itoa(numero_script_reduc);

	/* Reservo memoria para nombre_archivo (ruta del archivo donde se va a guardar el script reductor)
	 * Ejemplo: script_reduc_2.py
	 */
	char* nombre_archivo = malloc(strlen("script_reduc_") + strlen(numero_reduc_string) + strlen(".py") + 1); // + 1 del '\0'
	strcpy(nombre_archivo, "script_reduc_");
	strcat(nombre_archivo, numero_reduc_string);
	strcat(nombre_archivo, ".py");

	numero_script_reduc++; // Incremento el numero de script reductor para no repetir el nombre en la siguiente invocación
	free(numero_reduc_string);

	int confirmacion = persistir_buffer_en_archivo(nombre_archivo, buffer_script_reduc);

	if ( confirmacion == OK){
		return nombre_archivo; // Retorno la RUTA donde se guardo el archivo con el script reductor para que WORKER sepa donde está
	}else{
		return NULL;
	}

} // FIN script_reduc_a_archivo

// FUNCIONES DE LIBERACION DE LISTAS

void liberar_lista_temp_trans(t_list* lista_temp_trans) {

	// Variables
	int indice_elemento;
	t_temp_trans* elemento;

	// Calculo el tamaño de la lista
	int cantidad_elementos = list_size(lista_temp_trans);

	for (indice_elemento = 0; indice_elemento < cantidad_elementos; ++indice_elemento) {
		elemento = list_get(lista_temp_trans, indice_elemento);
		free(elemento->nombre_temp_trans);
	}

	list_iterate(lista_temp_trans, free);
	list_destroy(lista_temp_trans);


} // FIN liberar_lista_temp_trans

void liberar_lista_nombres_temporales(t_list* lista_nombres_temporales) {

	// Variables
	int indice_elemento;
	t_nombre_temporal_reducido* elemento;

	// Calculo el tamaño de la lista
	int cantidad_elementos = list_size(lista_nombres_temporales);

	for (indice_elemento = 0; indice_elemento < cantidad_elementos; ++indice_elemento) {
		elemento = list_get(lista_nombres_temporales, indice_elemento);
		free(elemento->nombre_temporal_reducido);
	}

	list_iterate(lista_nombres_temporales, free);
	list_destroy(lista_nombres_temporales);


} // FIN liberar_lista_temp_trans

// FIN FUNCIONES DE LIBERACION DE LISTAS
