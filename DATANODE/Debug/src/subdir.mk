################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/DATANODE.c \
../src/funcionesDataNode.c 

OBJS += \
./src/DATANODE.o \
./src/funcionesDataNode.o 

C_DEPS += \
./src/DATANODE.d \
./src/funcionesDataNode.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2017-2c-Mandale-Fruta/SHARED" -I/commons -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


