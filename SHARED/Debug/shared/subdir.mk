################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../shared/archivos.c \
../shared/estructuras.c \
../shared/funcionesAuxiliares.c \
../shared/md5.c \
../shared/mensajes.c \
../shared/protocolo.c \
../shared/sockets.c 

OBJS += \
./shared/archivos.o \
./shared/estructuras.o \
./shared/funcionesAuxiliares.o \
./shared/md5.o \
./shared/mensajes.o \
./shared/protocolo.o \
./shared/sockets.o 

C_DEPS += \
./shared/archivos.d \
./shared/estructuras.d \
./shared/funcionesAuxiliares.d \
./shared/md5.d \
./shared/mensajes.d \
./shared/protocolo.d \
./shared/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
shared/%.o: ../shared/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


