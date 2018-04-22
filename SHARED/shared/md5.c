#include "md5.h"
#include <stdio.h> // Para popen
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h> //Para usar el tipo de dato bool (true y false)

/*** TODO PROBADO: VALGRIND 0 ERRORES - 0 MEMORY LEAKS ***/

char* md5(char* ruta_archivo){

	int longitud_ruta_archivo = strlen(ruta_archivo);
	int longitud_md5sum = strlen("md5sum");
	char* param_popen = malloc(longitud_ruta_archivo + longitud_md5sum + 1 + 1); //+1 del espacio +1 del '\0'
	strcpy(param_popen, "md5sum ");
	strcat(param_popen, ruta_archivo);

	FILE* file = popen(param_popen, "r");
	free(param_popen);
	char* resultado_md5 = malloc(LONGITUD_HASH_MD5 + 1); //+1 del '\0'
	fscanf(file, "%s", resultado_md5); //fscanf deja de leer cuando encuentra un espacio... ¡justo lo que necesito!

	pclose(file);

	return resultado_md5;

}

void liberar_md5(char* resultado_md5){
	free(resultado_md5);
}

bool comparar_md5(char* md5_A, char* md5_B){
	int valor = strcmp(md5_A, md5_B);
	if(valor == 0){
		return true;
	}else{
		return false;
	}
}

