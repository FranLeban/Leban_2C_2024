/*! @mainpage Examen
 *
 * @section genDesc General Description
 *
 * LED 1 - Verde
 * LED 2 - Amarillo
 * LED 3 - Rojo
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO    	| 	GPIO_3	    |
 * | 	TRIGGER   	| 	GPIO_2	    |
 * | 	BUZZER 	 	| 	GPIO_10 	|
 * | 	+5V 	 	| 	+5V 		|
 * | 	GND 	 	| 	GND 		|
 * 
 * 
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * |  4/11/2023 | Document creation		                         |
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
#include "hc_sr04.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "buzzer.h"
#include "gpio.h"
/*==================[macros and definitions]=================================*/

#define CONFIG_BLINK_PERIOD_DISTANCIA 500 // 2 veces por segundo = cada 500 ms
#define CONFIG_BLINK_PERIOD_MOSTRADO 100
#define CONFIG_BLINK_PERIOD_ALARMA_PRECAUCION 1000
#define CONFIG_BLINK_PERIOD_ALARMA_PELIGRO 500
#define CONFIG_BLINK_PERIOD_ACELEROMETRO 10

#define DELAY 100

/*==================[internal data definition]===============================*/
/**
 * @var on
 * @brief booleano que indica si el sistema está activo
*/
bool on = false;
/**
 * @var distancia
 * @brief variable que almacena el valor de distancia
*/
uint16_t distancia = 0;

/**
 * @var sumatoria
 * @brief variable que almacena el valor de sumatoria
*/
uint16_t sumatoria = 0;

/*==================[internal functions declaration]=========================*/
/** 
* @brief mide la distancia que marca el sensor en centimetros
* @param[in] pvParameter puntero tipo void.
*/
static void Medir_Distancia(void *pvParameter)
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
* @brief muestra la distancia ilustrada en los leds que prenden de acuerdo al rango de distancia
* @param[in] pvParameter puntero tipo void.
*/
static void Ver_Distancia(void *pvParameter)
{
	while(true)
	{
		if(on)
		{
			/*
			500 cm = 5 metros   ---   300 cm = 3 metros

			Los LEDs se encienden según los siguientes rangos de distancia:
				* mayores a 500: LED 1 encendido -verde
				* menores a 500 y mayores a 300: LEDs 1 y 2 encendidos -verde amarillo
				* menores a 300: LEDs 1, 2 y 3 encendidos -verde amarillo rojo
			*/
			if(distancia >= 500)
			{
				LedsOffAll();
				LedOn(LED_1);
			}
			else if(distancia >= 300 && distancia < 500)
			{
				LedsOffAll();
				LedOn(LED_1);
				LedOn(LED_2);
			}
			else if(distancia < 300)
			{
				LedsOffAll();
				LedOn(LED_1);
				LedOn(LED_2);
				LedOn(LED_3);
			}
        }
		else
		{
			LedsOffAll();
		}

		//EsperO un poco antes de leer la distancia de nuevo
        vTaskDelay(CONFIG_BLINK_PERIOD_MOSTRADO/portTICK_PERIOD_MS);
	}
}

/** 
* @brief activa la alarma de acuerdo a los rangos de distancia
* @param[in] pvParameter puntero tipo void.
*/
static void Sonido_Alarma (void *pvParameter)
{
	while(true)
	{
		/*
		Los LEDs se encienden según los siguientes rangos de distancia:
			* mayores a 500: Alarma no suena
			* menores a 500 y mayores a 300: Alarma suena en modo precaución, a una frecuencia de cada 1 seg
			* menores a 300: Alarma suena en modo peligro, a una frecuencia de cada 0.5 seg
		*/
		if(distancia >= 300 && distancia < 500)
		{
			//buzzer en PRECAUCION
			
			/*GPIOOn(GPIO_10);//GPIO_10 en alto
			BuzzerOn;//el buzzer se enciende
			BuzzerPlayTone(220,1000);//se reproduce la alarma cada 1 segundo
			vTaskDelay(25 / portTICK_PERIOD_MS);//espero
			GPIOOff(GPIO_10);//GPIO_10 en bajo
			BuzzerOff;//el buzzer se apaga
			vTaskDelay(25 / portTICK_PERIOD_MS);//espero*/

			//forma con GPIO
			GPIOOn(GPIO_10);//GPIO_10 en alto
			vTaskDelay( (CONFIG_BLINK_PERIOD_ALARMA_PRECAUCION - DELAY) / portTICK_PERIOD_MS);//espero
			GPIOOff(GPIO_10);//GPIO_10 en bajo
		}
		else if(distancia < 300)
		{
			//buzzer en PELIGRO
			
			/*GPIOOn(GPIO_10);//GPIO_10 en alto
			BuzzerOn;//el buzzer se enciende
			BuzzerPlayTone(220,500);//se reproduce la alarma cada 0.5 segundo
			vTaskDelay(25 / portTICK_PERIOD_MS);//espero
			GPIOOff(GPIO_10);//GPIO_10 en bajo
			BuzzerOff;//el buzzer se apaga
			vTaskDelay(25 / portTICK_PERIOD_MS);//espero*/

			//forma con GPIO
			GPIOOn(GPIO_10);//GPIO_10 en alto
			vTaskDelay( (CONFIG_BLINK_PERIOD_ALARMA_PELIGRO - DELAY) / portTICK_PERIOD_MS);//espero
			GPIOOff(GPIO_10);//GPIO_10 en bajo
		}

        vTaskDelay(DELAY/portTICK_PERIOD_MS);
	}
}

/** 
* @brief controla el envío de datos por la UART acerca de las distancias y/o golpes/caidas
* @param[in] pvParameter puntero tipo void 
*/
static void Envio_Notificacion (void *pvParameter)
{
	while(true)
	{
		if(distancia >= 300 && distancia < 500)
		{
			UartSendString(UART_CONNECTOR, "Precaución, vehículo cerca \r\n")
		}
		else if(distancia < 300)
		{
			UartSendString(UART_CONNECTOR, "Peligro, vehículo cerca \r\n")
		}
		else if (sumatoria > 4)
		{
			UartSendString(UART_CONNECTOR, "Caida detectada \r\n")
		}

        vTaskDelay(CONFIG_BLINK_PERIOD_DISTANCIA/portTICK_PERIOD_MS);//VER que valor de CONFIG poner
	}
}

/** 
* @brief detecta si existen o no golpes y caidas de acuerdo al acelerometro
* @param[in] pvParameter puntero tipo void
*/
static void Detectar_golpesCaidas(void *pvParameter)
{
	uint16_t aceleracion_ejex = 0;
	uint16_t aceleracion_ejey = 0;
	uint16_t aceleracion_ejez = 0;

	while(true)
	{
		//acelerómetro analógico triaxial, 3 canales --> CH1, CH2 y CH3
		AnalogInputReadSingle(CH1, &aceleracion_ejex);
		AnalogInputReadSingle(CH2, &aceleracion_ejey);
		AnalogInputReadSingle(CH3, &aceleracion_ejez);

		//conversiones a partir de regla de 3
		aceleracion_ejex = (aceleracion_ejex * 5.5) / 3.3;
		aceleracion_ejey = (aceleracion_ejey * 5.5) / 3.3;
		aceleracion_ejez = (aceleracion_ejez * 5.5) / 3.3;

		//sumatoria escalar
		sumatoria = aceleracion_ejex + aceleracion_ejey +aceleracion_ejez;

		//como la frecuencia de muestreo del acelerometro es de 100HZ, estoy en un tiempo de 10 ms, y como
		//Delay admite como límite 10 ms, aunque estoy en el límite puedo utilizar Delay igualmente
		vTaskDelay(CONFIG_BLINK_PERIOD_ACELEROMETRO/portTICK_PERIOD_MS);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
	//Inicializaciones
	LedsInit();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2);

	GPIOInit(GPIO_10, GPIO_OUTPUT); // inicializo el gpio del buzzer como de salida
	
	//Inicializo la UART
	serial_config_t serial_port1 = {
		.port = UART_CONNECTOR,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};
	UartInit(&serial_port1);

	serial_config_t serial_port2 = {
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,
		.param_p = NULL
	};
	UartInit(&serial_port2);

	//Inicializo los ADC
	analog_input_config_t config_ADC1 = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&config_ADC1);

	analog_input_config_t config_ADC2 = {
		.input = CH2,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&config_ADC2);

	analog_input_config_t config_ADC3 = {
		.input = CH3,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = 0
	};
	AnalogInputInit(&config_ADC3);

	//Creación de tareas
	xTaskCreate(&Medir_Distancia, "Medir_Distancia", 1024, NULL, 5, NULL);
	xTaskCreate(&Ver_Distancia, "Ver_Distancia", 1024, NULL, 5, NULL);
	xTaskCreate(&Sonido_Alarma, "Sonido_Alarma", 1024, NULL, 5, NULL);
	xTaskCreate(&Envio_Notificacion, "Envio_Notificacion", 1024, NULL, 5, NULL);
	xTaskCreate(&Detectar_golpesCaidas, "Detectar_golpesCaidas", 1024, NULL, 5, NULL);
}
/*==================[end of file]============================================*/