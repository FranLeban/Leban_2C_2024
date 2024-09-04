/*! @mainpage Template
 *
 * @section genDesc General Description
 * 
 * Este ejercicio
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
 * | 04/09/2024 | Document creation		                         |
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
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_TECLAS 1000
#define CONFIG_BLINK_PERIOD_DISTANCIA 1000

/*==================[internal data definition]===============================*/
bool on = false;
bool hold = false;
uint16_t distancia = 0;
/*==================[internal functions declaration]=========================*/
static void MedirDistanciaTask(void *pvParameter)
{
	while(true)
	{
		if(on)
		{
			distancia=HcSr04ReadDistanceInCentimeters();
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_DISTANCIA/portTICK_PERIOD_MS);
	}	
}

static void TeclasTask(void *pvParameter)
{
	uint8_t teclas;
    while(true){
		teclas = SwitchesRead();
		switch(teclas)
		{
			case SWITCH_1://tecla1_presionada--> 01
				on=!on;
			break;
			case SWITCH_2://tecla2_presionada--> 10
				hold=!hold;
			break;
		}
		vTaskDelay(CONFIG_BLINK_PERIOD_TECLAS/portTICK_PERIOD_MS);
    }
}

static void MostrarDistanciaTask(void *pvParameter)
{
	while(true)
	{
		if(on)
		{
			if(distancia<10)
			{
				LedsOffAll();
			}
			else if(distancia >= 10 && distancia <= 20)
			{
				LedOn(LED_1);
			}
			else if(distancia >= 20 && distancia <= 30)
			{
				LedOn(LED_1);
				LedOn(LED_2);
			}
			else if(distancia>30)
			{
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
			}
		}
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	SwitchesInit();
    LedsInit();
    xTaskCreate(&MedirDistanciaTask, "LED_1", 512, NULL, 5, &led1_task_handle);//512 es la memoria a usar
    xTaskCreate(&TeclasTask, "LED_2", 512, NULL, 5, &led2_task_handle);//"&led2_task_handle" es un identificador para el sistema operativo para esa tarea 
    xTaskCreate(&MostrarTask, "LED_3", 512, NULL, 5, &led3_task_handle);
}