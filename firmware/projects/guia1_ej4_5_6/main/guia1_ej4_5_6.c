/*! @mainpage Template
 *
 * @section genDesc General Description
 * 
 * Ejercicio 4: En este ejercicio escribi una función que recibe un dato de 32 bits, la cantidad de
 * dígitos de salida y un puntero a un arreglo donde se almacenen los n dígitos, esta función convierte
 * el dato recibido a BCD, guardando cada uno de los dígitos de salida en el arreglo que pase como puntero.
 * 
 * Ejercicio 5: En este ejercicio escribi una función que recibe como parámetro un dígito BCD y un vector de
 * estructuras del tipo gpioConf_t; lo que hace la función es cambiar el estado de cada GPIO a 0 o a 1,
 * dependiendo del bit correspondiente en el BCD ingresado. En el main definí un vector que mapee los bits.
 * Por medio de un display se visualiza el número con el que trabajé.
 * 
 * Ejercicio 6: En este ejercicio escribi una función que recibe como parámetros un dato de 32 bits, la cantidad
 * de dígitos de salida y dos vectores de estructuras del tipo gpioConf_t; lo que hace la función es mostrar por
 * display el valor que le doy, reutilizando las funciones creadas en el punto 4 y 5. En el main defini un nuevo
 * vector que mapea los bits nuevos y también utilice la defincion del vector que mapea los bits del ejercicio 5.
 * Por medio de un display conectado a la ESP32 se visualiza el número con el que trabajé, que en este caso es
 * con el ejemplo del número 138.
 * 
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	D1	    	| 	GPIO_20		|
 * | 	D2	    	| 	GPIO_21		|
 * | 	D3	 	    | 	GPIO_22		|
 * | 	D4  	 	| 	GPIO_23		|
 * | 	SEL_1   	| 	GPIO_19		|
 * | 	SEL_2	 	| 	GPIO_18		|
 * | 	SEL_3	 	| 	GPIO_9		|
 * | 	+5V 	 	| 	+5V 		|
 * | 	GND 	 	| 	GND 		| 
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 28/08/2024 | Document creation		                         |
 *
 * @author Leban Francesca (francheleban123@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "gpio_mcu.h"

/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
//Para ejercicio 5

/**
 * @brief Estructura que permite la configuracion del pin y la direccion (salida/entrada) de cada GPIO
 * 
 */
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
//Para ejercicio 4

/**
 * @brief Convierte el dato recibido a BCD
 * 
 * @param data Dato de 32 bits; de tipo entero sin signo
 * @param digits Cantidad de digitos de salida; de tipo entero, sin signo, de 8 bits
 * @param bcd_number Puntero al arreglo donde se almacenan los dígitos; de tipo entero, sin signo, de 8 bits
 */
void convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for (uint32_t i = 0; i < digits; i++)
	{
		bcd_number [digits - 1 - i] = data % 10;
		//"digits - 1 - i" calcula la posición correcta en el arreglo bcd_number para almacenar el dígito
		//actual, digits - 1 apunta a la última posición del arreglo. Al restar i, se va llenando el arreglo
		//desde la derecha hacia la izquierda (de los dígitos menos significativos a los más significativos)
		//El operador % es el operador de módulo, que devuelve el resto de la división entre data y 10.
		data /= 10;//elimina el último dígito procesado
	}
}

//Para ejercicio 5

/**
 * @brief Cambia el estado de cada GPIO, a ‘0’ o a ‘1’
 * 
 * @param digitoBCD Dígito BCD, de tipo entero, sin signo, de 8 bits
 * @param arreglo Puntero a vector de estructuras del tipo gpioConf_t que permite configurar el pin y la dirección de los distintos GPIO
 */
void cambiar_estado (uint8_t digitoBCD, gpioConf_t *arreglo)
{
	for (uint8_t i = 0; i < 4; i++)
	{
		if (digitoBCD & (1 << i))
		{
			GPIOOn(arreglo[i].pin);
		}
		else
		{
			GPIOOff(arreglo[i].pin);
		}
	}
}

//Para ejercicio 6

/**
 * @brief Muestra por display el valor que recibe
 * 
 * @param dato Dato de 32 bits; de tipo entero sin signo
 * @param digitos_salida Cantidad de digitos de salida; de tipo entero, sin signo, de 8 bits
 * @param arreglo1 Puntero a vector de estructuras del tipo gpioConf_t que, al igual que el anterior, permite configurar el pin y la dirección de los distintos GPIO
 * @param arreglo_mapeo Puntero a vector de estructuras del tipo gpioConf_t, que mapea los puertos con el dígito del LCD a donde mostrar un dato
 */
void mostrarXdisplay (uint32_t dato, uint8_t digitos_salida, gpioConf_t *arreglo1, gpioConf_t *arreglo_mapeo)
{
	uint8_t arreglo_aux [digitos_salida];
	convertToBcdArray(dato, digitos_salida, arreglo_aux);

	for (uint8_t i = 0; i < 3; i++)
	{
		cambiar_estado(arreglo_aux[i],arreglo1);
		//para lograr dar el pulso
		GPIOOn(arreglo_mapeo[i].pin);
		GPIOOff(arreglo_mapeo[i].pin);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	//Inicio de ejercicio 4
	uint32_t data = 245;
	uint8_t digits = 3;
	uint8_t bcd_number [3];

	convertToBcdArray(data, digits, bcd_number);//llamo a la función
	
	for (uint8_t i = 0; i < 3; i++)
	{
		printf ("%d", bcd_number[i]);	
	}
	//Finaliza ejercicio 4

	//Comienzo ejercicio 5
	//el siguiente arreglo y definicion de los GPIO también se utilizara en el ejercicio 6
	gpioConf_t arreglo[] =//vector (también llamado arreglo) que mapea los bits
	{
		{GPIO_20, GPIO_OUTPUT},
		{GPIO_21, GPIO_OUTPUT},
		{GPIO_22, GPIO_OUTPUT},
		{GPIO_23, GPIO_OUTPUT}
	};

	for (uint8_t i = 0; i < 4; i++)//uso numero magico 4 como excepcion porque son 4 los GPIO
	{
		GPIOInit(arreglo[i].pin, arreglo[i].dir);
	}

	//Ejemplo de uso: Mostrar el dígito 2
    //uint8_t digitoBCD = 8;
    //cambiar_estado(digitoBCD, arreglo);
	//Finaliza ejercicio 5

	//Inicia ejercicio 6
	//tambien se utilizara la definicion de los anteriores GPIO del ejercicio 5
	gpioConf_t arreglo_mapeo[] =//vector (también llamado arreglo) que mapea los bits
	{
		{GPIO_19, GPIO_OUTPUT},
		{GPIO_18, GPIO_OUTPUT},
		{GPIO_9, GPIO_OUTPUT}
	};

	for (uint8_t i = 0; i < 3; i++)//uso numero magico 3 como excepcion porque son 3 los GPIO
	{
		GPIOInit(arreglo_mapeo[i].pin, arreglo_mapeo[i].dir);
	}

	//Ejemplo de uso: Mostrar el 138
    mostrarXdisplay(138, 3, arreglo, arreglo_mapeo);
	//Finaliza ejercicio 6
}
/*==================[end of file]============================================*/