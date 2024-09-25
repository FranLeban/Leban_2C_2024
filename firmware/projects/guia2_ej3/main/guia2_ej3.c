/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * Este ejercicio utiliza un sistema de medición de distancia, por ultrasonido (HC-SR04), para
 * encender LEDs según el rango de distancia y mostrar la distancia en un display LCD, con la dinamica
 * de los ejercicios anteriores. Se agrega el puerto serie, envía la distancia por UART a una conexión
 * externa, para poder observarlos en un terminal en la PC, siguiendo un formato específico. La tarea
 * puede ser controlada mediante 'O' y 'H', que activa o desactiva la medición y visualización de la
 * distancia.
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
 * | 25/09/2024 | Document creation		                         |
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
#include "timer_mcu.h"
#include "led.h"
#include "switch.h"
#include "lcditse0803.h"
#include "hc_sr04.h"
#include "uart_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_MOSTRADO_US 1000*1000
#define CONFIG_BLINK_PERIOD_DISTANCIA_US 1000*1000
/*==================[internal data definition]===============================*/
bool on = false;
bool hold = false;
uint16_t distancia = 0;
/*==================[internal functions declaration]=========================*/
TaskHandle_t MostrarDistanciaTask_handle = NULL;
TaskHandle_t MedirDistanciaTask_handle = NULL;

/**
 * @brief Tarea para medir la distancia utilizando el sensor HC-SR04.
 * 
 * Esta función mide la distancia cuando la variable 'on' es verdadera y almacena el valor en la
 * variable 'distancia'. Se ejecuta indefinidamente hasta recibir una notificación.
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
		
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    
        /*La tarea espera en este punto hasta recibir una notificación*/
	}	
}

/**
 * @brief Alterna el estado de la variable 'on'.
 * 
 * Esta función invierte el estado actual de la variable 'on'.
 * 
 * @param Ninguno
 * 
 * @return Ninguno
 */
void FuncTecla1()
{
	on = !on;
}

/**
 * @brief Alterna el estado de la variable 'hold'.
 * 
 * Esta función invierte el estado actual de la variable 'hold'.
 * 
 * @param Ninguno
 * 
 * @return Ninguno
 */
void FuncTecla2()
{
	hold = !hold;
}

/**
 * @brief Notifica a la tarea MedirDistancia para que realice su operación.
 * 
 * Esta función envía una notificación a la tarea MedirDistancia desde una rutina de servicio
 * de interrupción (ISR).
 * 
 * @param Ninguno
 * 
 * @return Ninguno
 */
void TimerMedir()
{
	vTaskNotifyGiveFromISR(MedirDistanciaTask_handle, pdFALSE);    
    //Envía una notificación a la tarea asociada a MedirDistancia

	//vTaskNotifyGiveFromISR(MostrarDistanciaTask_handle, pdFALSE);    
    //Envía una notificación a la tarea asociada a MostrarDistancia
}

/**
 * @brief Notifica a la tarea MostrarDistancia para que realice su operación.
 * 
 * Esta función envía una notificación a la tarea MostrarDistancia desde una rutina de servicio
 * de interrupción (ISR).
 * 
 * @param Ninguno
 * 
 * @return Ninguno
 */
void TimerMostrar()
{
	vTaskNotifyGiveFromISR(MostrarDistanciaTask_handle, pdFALSE);    
    //Envía una notificación a la tarea asociada a MostrarDistancia
}

/**
 * @brief Tarea para mostrar la distancia en el display LCD, encender LEDs según el rango de distancia y enviar la distancia por UART.
 * 
 * Esta función se ejecuta indefinidamente y muestra la distancia en el display LCD cuando la variable
 * 'on' es verdadera. Los LEDs se encienden según los siguientes rangos de distancia:
 *  * 0-9: Todos los LEDs apagados
 *  * 10-20: LED 1 encendido
 *  * 21-30: LEDs 1 y 2 encendidos
 *  * 31+: LEDs 1, 2 y 3 encendidos
 * Además, envía la distancia por UART al puerto UART_PC.
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

        	if(hold==false)
			{
				//---Escribir el arreglo de bytes en el display LCD---
        		LcdItsE0803Write(distancia);
				/*esta funcion se utiliza para enviar los datos al display*/
			}

			//UART
			//mando el valor de distancia desde la uart
			UartSendString(UART_PC, (char*) UartItoa(distancia, 10));//10 es la base de la conversion, en este caso 10 por ser decimal
			UartSendString(UART_PC, " cm\r\n");
		}
		else
		{
			LedsOffAll();
			LcdItsE0803Off();
		}

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		/*La tarea espera en este punto hasta recibir una notificación*/
	}
}

/**
 * @brief Lee un byte desde la UART y controla las banderas de 'on' y 'hold' según el valor leído.
 * 
 * Esta función se utiliza para leer un byte desde la UART y actualizar las banderas de 'on' y 'hold' 
 * según el valor leído. No reemplaza el funcionamiento de las teclas 1 y 2 de la placa, sino que 
 * proporciona una forma adicional de controlar estas banderas desde la PC.
 * 
 * @param param Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void FuncionUartLeer(void* param)
{
	uint8_t dato;
	UartReadByte(UART_PC, &dato);

	//para controlar las banderas de on y hold por medio de la PC con O y H
	//no reemplaza a las teclas 1 y 2 de la placa, hace lo mismo que ellas
	switch(dato)
	{
		case 'O':
			on = !on;
			break;
		case 'H':
			hold = !hold;
			break;
		default:
			break;
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	//Inicializaciones
	LedsInit();
	SwitchesInit();
	LcdItsE0803Init();	
	HcSr04Init(GPIO_3, GPIO_2);
	
	//Interrupciones Switches
	SwitchActivInt(SWITCH_1,FuncTecla1,NULL);
	SwitchActivInt(SWITCH_2,FuncTecla2,NULL);

	//configuro la UART para comunicacion bidireccional
	serial_config_t uart_config_leer_enviar = {//serial_config_t es el struct
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = FuncionUartLeer,//poner funcion para lee, previamente creada
		.param_p = NULL
	};
	UartInit(&uart_config_leer_enviar);//inicializo la UART

	//Inicialización de timers
    timer_config_t timer_medir = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_DISTANCIA_US,
        .func_p = TimerMedir,
        .param_p = NULL
    };
    TimerInit(&timer_medir);

    timer_config_t timer_mostrar = {
        .timer = TIMER_B,
        .period = CONFIG_BLINK_PERIOD_MOSTRADO_US,
        .func_p = TimerMostrar,
        .param_p = NULL
    };
    TimerInit(&timer_mostrar);

    //Creación de tareas
    xTaskCreate(&MedirDistanciaTask, "MedirDistancia", 512, NULL, 5, &MedirDistanciaTask_handle);
	xTaskCreate(&MostrarDistanciaTask, "MostrarDistancia", 512, NULL, 5, &MostrarDistanciaTask_handle);

    //Inicialización del conteo de timers
    TimerStart(timer_medir.timer);
    TimerStart(timer_mostrar.timer);

	/*Como CONFIG_BLINK_PERIOD_MOSTRADO_US y CONFIG_BLINK_PERIOD_DISTANCIA_US tienen el mismo valor
	podria unificar los dos timer y por ejemplo borrar el "timer_mostrar",  y directamente unirlo con
	el  "timer_medir", lo mismo con la funcion, es decir, lo que codifique en "TimerMostrar" pasarlo
	directamente a "TimerMedir", dejando solo un timer y una funcion. Lo probe y anduvo pero decidi
	dejarlo de la otra forma por una cuestion de entendimiento propio :)*/
}
/*==================[end of file]============================================*/