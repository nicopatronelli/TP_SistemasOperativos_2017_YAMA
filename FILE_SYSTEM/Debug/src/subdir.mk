################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Consola.c \
../src/Directorios.c \
../src/File_System.c \
../src/Mensajes.c \
../src/Nodos.c \
../src/funcionesFILESYSTEM.c 

OBJS += \
./src/Consola.o \
./src/Directorios.o \
./src/File_System.o \
./src/Mensajes.o \
./src/Nodos.o \
./src/funcionesFILESYSTEM.o 

C_DEPS += \
./src/Consola.d \
./src/Directorios.d \
./src/File_System.d \
./src/Mensajes.d \
./src/Nodos.d \
./src/funcionesFILESYSTEM.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2017-2c-Mandale-Fruta/SHARED" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


