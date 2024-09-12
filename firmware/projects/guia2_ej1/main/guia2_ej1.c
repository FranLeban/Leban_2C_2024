/*! @mainpage Template
 *
 * @section genDesc General Description
 * 
 * En este ejercicio se diseño un firmware que mide distancia por ultrasonido (HC-SR04) y enciende LEDs,
 * dependiendo del caso del que se trate, y muestra el valor de los cm de distancia en un display. Para esto,
 * se realizaron distintas tareas encargadas de la lectura del sensor, encendido y apagado del/los LED/s y la
 * escritura del valor de distancia.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO    	| 	GPIO_3	    |
 * | 	TRIGGER   	| 	GPIO_2	    |
 * | 	+5V 	 	| 	+5V 		|
 * | 	GND 	 	| 	GND 		|
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
#include "lcditse0803.h"
#include "hc_sr04.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_TECLAS 200
#define CONFIG_BLINK_PERIOD_DISTANCIA 1000
#define CONFIG_BLINK_PERIOD_MOSTRADO 100

/*==================[internal data definition]===============================*/
bool on = false;
bool hold = false;
uint16_t distancia = 0;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Tarea para medir la distancia.
 * 
 * Esta función mide la distancia utilizando el sensor HC-SR04 y almacena el valor en la variable
 * 'distancia'. Se ejecuta indefinidamente y mide la distancia cuando la variable 'on' es verdadera.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
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

/**
 * @brief Tarea para manejar la lectura de switches y actualizar los estados on y hold.
 * 
 * Esta función lee el estado de los switches y actualiza las variables on y hold según corresponda.
 * Se ejecuta indefinidamente y se demora un período de CONFIG_BLINK_PERIOD_TECLAS (1000) entre cada
 * lectura de switch.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void TeclasTask(void *pvParameter)
{
	uint8_t teclas;
    while(true)
	{
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

/**
 * @brief Tarea para mostrar la distancia medida.
 *  
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void MostrarDistanciaTask(void *pvParameter)
{
	while(true)
	{
		if(on)
		{
			/*Los LEDs se encienden según los siguientes rangos de distancia:
				* 0-9: Todos los LEDs apagados
				* 10-20: LED 1 encendido
				* 21-30: LEDs 1 y 2 encendidos
				* 31+: LEDs 1, 2 y 3 encendidos*/
			if(distancia<10)
			{
				LedsOffAll();
			}
			else if(distancia >= 10 && distancia < 20)//tendria que ir <= pero si lo pongo asi, me lo
			//va a considerar en esta condición y en la siguiente, y habra un problema con ello, por
			//lo que directamente en una de las condiciones, en este caso en esta, no incluyo a 20
			{
				LedsOffAll();
				LedOn(LED_1);
			}
			else if(distancia >= 20 && distancia <= 30)
			{
				LedsOffAll();
				LedOn(LED_1);
				LedOn(LED_2);
			}
			else if(distancia>30)
			{
				LedsOffAll();
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
			}

			if(hold==false)
			{
				//---Escribir el arreglo de bytes en el display LCD---
        		LcdItsE0803Write(distancia);
				/*esta funcion se utiliza para enviar los datos al display*/
			}
        }
		else
		{
			LedsOffAll();
			LcdItsE0803Off();
		}

		//---Esperar un poco antes de leer la distancia de nuevo---
        vTaskDelay(CONFIG_BLINK_PERIOD_MOSTRADO/portTICK_PERIOD_MS);
		/*llamo a la función vTaskDelay para pausar la ejecución de la tarea durante un período de
		tiempo*/
		
	}
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();	
	HcSr04Init(GPIO_3, GPIO_2);
	/* Creación de tareas */
	xTaskCreate(&MedirDistanciaTask, "MedirDistancia", 512, NULL, 5, NULL);
	xTaskCreate(&TeclasTask, "Teclas", 512, NULL, 5, NULL);
	xTaskCreate(&MostrarDistanciaTask, "MostrarDistancia", 512, NULL, 5, NULL);
}