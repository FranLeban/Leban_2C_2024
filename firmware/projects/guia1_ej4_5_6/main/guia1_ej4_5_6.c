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
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 14/08/2024 | Document creation		                         |
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
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/
//Para ejercicio 4
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

	// Ejemplo de uso: Mostrar el dígito 2
    uint8_t digitoBCD = 8;
    cambiar_estado(digitoBCD, arreglo);
	//Finaliza ejercicio 5
}
/*==================[end of file]============================================*/