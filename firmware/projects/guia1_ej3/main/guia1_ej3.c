/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * En este ejercicio implementamos una función que recibe un puntero a una estructura LED, usando
 * como guía para la implementación el diagrama de flujo que se encuentra en el enunciado. Luego
 * configuramos la función en modo Toggle, 10 ciclos, 500 milisegundos y verificamos el funcionamiento
 * utilizando el Osciloscopio.
 * El programa, en pocas palabras, hace titilar el LED 1 cinco veces debido al período de tiempo elegido
 * en la implementación 
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
 * | 07/08/2024 | Document creation		                         |
 *
 * @author Francesca Leban (francheleban123@gmail.com)
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

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 100

//defino los modos en variables enteras porque mode es un entero (sin signo)
#define ON 1
#define OFF 0
#define TOGGLE 2

/*==================[internal data definition]===============================*/
struct leds
{
    uint8_t mode;         //ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;     //indica la cantidad de ciclos de encendido/apagado
	//enteros sin signo de 8 bits

	uint16_t periodo;     //indica el tiempo de cada ciclo
	//entero sin signo de 16 bits
} my_leds;

/*==================[internal functions declaration]=========================*/
void funcion1(struct leds *variablep1)
{
	switch (variablep1->mode)
	{
		case ON:
			switch (variablep1->n_led)
			{
				case 1:
					LedOn(LED_1);
					break;
				case 2:
					LedOn(LED_2);
					break;
				case 3:
					LedOn(LED_3);
					break;
				default:
					break;
			}
		case OFF:
			switch (variablep1->n_led)
			{
				case 1:
					LedOff(LED_1);
					break;
				case 2:
					LedOff(LED_2);
					break;
				case 3:
					LedOff(LED_3);
					break;
				default:
					break;
			}
		case TOGGLE:
			for (uint8_t i = 0; i < variablep1->n_ciclos; i++)
			{
				switch (variablep1->n_led)
				{
					case 1:
						LedToggle(LED_1);
						break;
					case 2:
						LedToggle(LED_2);
						break;
					case 3:
						LedToggle(LED_3);
						break;
					default:
						break;
				}
				for (uint8_t j = 0; j < variablep1->periodo/CONFIG_BLINK_PERIOD; j++)
				{
					vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS); //divido para que me de en ms
				}
				
			}		
	}
}
/*==================[external functions definition]==========================*/

void app_main(void)
{
	my_leds.periodo=500;
	my_leds.n_ciclos=10;
	my_leds.mode=TOGGLE;
	my_leds.n_led=1;

	LedsInit();
	SwitchesInit();
	
	funcion1(&my_leds);
}
/*==================[end of file]============================================*/