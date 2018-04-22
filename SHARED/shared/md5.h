/*
 * md5.h
 *
 *  Created on: 17/10/2017
 *      Author: utnso
 */

#ifndef SHARED_MD5_H_
#define SHARED_MD5_H_

#include <stdbool.h> //Para usar el tipo de dato bool (true y false)
#define LONGITUD_HASH_MD5 32 // Los hash producidos por el algoritmo MD5 tienen una longitud de 32 bytes

char* md5(char* ruta_archivo);
void liberar_md5(char* resultado_md5);
bool comparar_md5(char* md5_A, char* md5_B);

#endif /* SHARED_MD5_H_ */
