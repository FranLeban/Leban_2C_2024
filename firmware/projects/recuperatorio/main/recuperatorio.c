/*! @mainpage Recuperatorio
 *
 * @section genDesc General Description
 * 
 * Se desea implementar un sistema de pesaje de camiones basado en la placa ESP-EDU
 * 
 * Para ello, el sistema debe verificar primero la velocidad de ingreso de los vehículos a la
 * balanza. Se cuenta con un sensor de distancia HC-SR04, ubicado de frente a la
 * dirección de ingreso a la balanza. El sistema debe realizar mediciones de distancia a
 * razón de 10 muestras por segundo y, una vez detectado un vehículo a una distancia
 * menor a 10m, utilizar dichas mediciones para calcular la velocidad de los vehículos en m/s 
 * (la dirección de avance de los vehículos es siempre hacia el sensor). Se debe
 * además utilizar los LEDs de la ESP-EDU como señales de advertencia de velocidad:
 * velocidad mayor a 8m/s: LED3, velocidad entre 0m/s y 8m/s: LED2, vehículo detenido:LED1.
 * 
 * Cuando se detecte que el vehículo está detenido, se debe proceder a pesarlo. La
 * balanza está equipada con dos galgas, las cuales cuentan con circuitos de
 * acondicionamiento que brindan una salida analógica de 0 a 3.3V para el rango de
 * pesos de cada galga que es de 0 a 20.000kg. La medición de peso se debe realizar a
 * una tasa de 200 muestras por segundo, y para determinar el peso total del vehículo se
 * deberán promediar 50 mediciones de cada galga, y sumar ambos promedios.
 * Una vez pesado el vehículo, se debe informar a la PC del operario (a través de la
 * UART correspondiente) el valor de peso y la velocidad máxima del vehículo con el
 * siguiente formato:
 * “Peso: 15.000kg”
 * “Velocidad máxima: 10 m/s”
 * El operario también debe poder controlar desde la PC la apertura y cierre de una
 * barrera, utilizando para esto también la UART. Ante el envío de un carácter 'o' se
 * deberá abrir la barrera y con una 'c', cerrarla. La barrera se controlará utilizando un
 * GPIO ('1': barrera abierta, '0': barrera cerrada).
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral	    |   ESP32   	|
 * |:------------------:|:--------------|
 * | 	ECHO    		| 	GPIO_3	    |
 * | 	TRIGGER   		| 	GPIO_2	    |
 * | 	+5V 	 		| 	+5V 		|
 * | 	GND 	 		| 	GND 		|
 * | 	BARRERA 		| 	GPIO_10	    |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 11/11/2024 | Document creation		                         |
 *
 * @author Leban Francesca (francheleban123@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "uart_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hc_sr04.h"
#include "led.h"
#include "analog_io_mcu.h"
#include "gpio_mcu.h"
#include "timer_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_DISTANCIA
 * @brief tiempo en milisegundos del delay usado en distancia
*/
#define CONFIG_BLINK_PERIOD_DISTANCIA 100 //10 muestras por segundo = cada 100 ms

/** @def CONFIG_BLINK_PERIOD_BALANZA_US
 * @brief tiempo en micro segundos del TIMER_A
*/
#define CONFIG_BLINK_PERIOD_BALANZA_US 5000//5 ms en microsegundos

/*==================[internal data definition]===============================*/

TaskHandle_t Obtener_Peso_VehiculoTask_handle = NULL;

/**
 * @var distancia
 * @brief variable que almacena el valor de distancia
*/
uint16_t distancia = 0;

/**
 * @var distancia
 * @brief variable que almacena el valor de velocidad
*/
uint16_t velocidad = 0;

/**
 * @var peso_galga1_voltios
 * @brief variable que almacena el valor de peso_galga1_voltios
*/
uint16_t peso_galga1_voltios = 0;

/**
 * @var peso_galga2_voltios
 * @brief variable que almacena el valor de peso_galga2_voltios
*/
uint16_t peso_galga2_voltios = 0;

/**
 * @var peso_total
 * @brief variable que almacena el valor de peso_total
*/
uint16_t peso_total = 0;

/**
 * @var on
 * @brief booleano que indica si el sistema está activo
*/
bool on = false;

/**
 * @var off
 * @brief booleano que indica si el sistema no está activo
*/
bool off = false;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Mide la distancia en centimetros con el sensor HC-SR04
 * 
 * @param pvParameter puntero a parametros no utilizado
 * 
 * La tarea lee la distancia cada CONFIG_BLINK_PERIOD_DISTANCIA milisegundos
 * y la guarda en la variable distancia.
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
 * @brief Obtiene la velocidad del vehículo en m/s
 * 
 * @param pvParameter puntero a parametros no utilizado
 * 
 * La tarea compara las distancias para luego utilizarlas en el calculo de la velocidad
 */
static void Velocidad_Vehiculo(void *pvParameter)
{
	uint16_t distancia_anterior = 0;
	while(true)
	{
		if (distancia < 1000)//1000 cm = 10 metros
		{
			//calculo la velocidad (velocidad = distancia/tiempo) de los vehículos en m/s
			for (int i = 0; i < 10; i++)
			{
				distancia_anterior = distancia;
			}

			velocidad = ( (distancia - distancia_anterior)/100 ) / ( (CONFIG_BLINK_PERIOD_DISTANCIA/portTICK_PERIOD_MS) / 0.001);//velocidad en m/s
		}
	}
}

/**
 * @brief Advierte mediante leds el rango de velocidad
 * 
 * @param pvParameter puntero a parametros no utilizado
 * 
 * La tarea activa leds en base a si la velocidad del vehículo es mayor a 8, esta entre 0 y 8 o 
 * el vehículo está detenido
 */
static void Advertencia_Velocidad(void *pvParameter)
{
	while(true)
	{
		if (velocidad > 8)
		{
			//prendo LED3
			LedOn(LED_3);
		}
		else if (velocidad > 0 && velocidad <= 8)
		{
			//prendo LED2
			LedOn(LED_2);
		}
		else//vehículo detenido
		{
			//prendo LED1
			LedOn(LED_1);
		}
	}
}

/**
 * @brief Tarea para obtener el peso del vehículo cuando este se encuentra detenido.
 *
 * La tarea se encarga de medir el peso del vehículo a una tasa de 200 muestras por segundo,
 * y para determinar el peso total del vehículo se deberán promediar 50 mediciones de cada galga, y sumar 
 * ambos promedios.
 *
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 *
 * @return Ninguno
 */
static void Obtener_Peso_Vehiculo(void *pvParameter)
{
    while(true)
    {
        if (on)
        {
            if (velocidad == 0)
            {
				uint16_t peso_galga1_kg = 0;
                uint16_t peso_galga2_kg = 0;
				
				/*
				En peso_galga1_kg y peso_galga2_kg lo que utilizo es la formula y tabla de interpolacion:
				Tabla interpolación --> |      X0=0V       |    Y0=0kg         |
										|:----------------:|:------------------|
								        |	  X=sensor     |   Y=incognita     |
										|:----------------:|:------------------|
										|	  X1=3.3V      | 	 Y1=20000kg    |
				Formula interpolación --> Y = Y0 + ( (Y1 - Y0) / (X1 - X0) ) * (X - X0)*/
				
                for (int i = 0; i < 50; i++)
                {
					AnalogInputReadSingle(CH1, &peso_galga1_voltios);
					// conversion a kg
					peso_galga1_kg = (0) + ((20000 - (0)) / (3300 - 0)) * (peso_galga1_voltios - 0); // 3.3v = 3300mV

					AnalogInputReadSingle(CH2, &peso_galga2_voltios);
					// conversion a kg
					peso_galga2_kg = (0) + ((20000 - (0)) / (3300 - 0)) * (peso_galga2_voltios - 0); // 3.3v = 3300mV
                }

                peso_galga1_kg /= 50;//50 muestras
                peso_galga2_kg /= 50;//50 muestras

                peso_total = peso_galga1_kg + peso_galga2_kg;
            }
        }
		//utilizo timers porque 200 muestras por segundo significa cada 5 ms y delay es con HASTA 10 ms
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

/**
 * @brief Tarea para enviar por UART la información del peso y velocidad del vehículo.
 * 
 * La tarea se encarga de enviar por UART la información del peso y velocidad del vehículo,
 * cada que se llama a la función.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void Envio_Notificacion (void *pvParameter)
{
	while(true)
	{
		UartSendString(UART_PC, "Peso: ");
		UartSendString(UART_PC, peso_total);
		UartSendString(UART_PC, "kg");

		UartSendString(UART_PC, "\r\n");

		UartSendString(UART_PC, "Velocidad maxima: ");
		UartSendString(UART_PC, velocidad);
		UartSendString(UART_PC, "m/s");

		UartSendString(UART_PC, "\r\n");
	}
}

/**
 * @brief Envía una notificación a la tarea asociada a Obtener_Peso_Vehiculo cada 5ms 
 * para que esta pueda obtener el peso del vehículo.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void Funcion_TimerBalanza_A(void *pvParameter)
{
    vTaskNotifyGiveFromISR(Obtener_Peso_VehiculoTask_handle, pdFALSE);
    /*Envía una notificación a la tarea asociada a Obtener_Peso_Vehiculo*/
}

/**
 * @brief Lee un byte desde la UART y controla las banderas de 'on' y 'hold' según el valor leído.
 * 
 * Esta función se utiliza para leer un byte desde la UART y actualizar las banderas de 'on' y 'hold' 
 * según el valor leído. No reemplaza el funcionamiento de las teclas 1 y 2 de la placa, sino que 
 * proporciona una forma adicional de controlar estas banderas desde la PC, con "O" y "C".
 * 
 * @param param Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void FuncionUartLeer(void* param)
{
	uint8_t dato;
	UartReadByte(UART_PC, &dato);

	//para controlar las banderas de on y off por medio de la PC con O y C
	switch(dato)
	{
		case 'O':
			on = !on;
			GPIOOn(GPIO_10);
			break;
		case 'C':
			off = !off;
			GPIOOff(GPIO_10);
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
	HcSr04Init(GPIO_3, GPIO_2);

	// Inicializar timers
    timer_config_t timer_pesar = {
        .timer = TIMER_A,
        .period = CONFIG_BLINK_PERIOD_BALANZA_US,
        .func_p = Funcion_TimerBalanza_A,
        .param_p = NULL};
    TimerInit(&timer_pesar);

	// Encender timers (START)
    TimerStart(timer_pesar.timer);

	//configuro la UART para comunicacion bidireccional
	serial_config_t uart_config_leer_enviar = {//serial_config_t es el struct
		.port = UART_PC,
		.baud_rate = 9600,
		.func_p = FuncionUartLeer,
		.param_p = NULL
	};
	UartInit(&uart_config_leer_enviar);//inicializo la UART

	//Creo las tareas
	xTaskCreate(&Medir_Distancia, "Medir_Distancia", 1024, NULL, 5, NULL);
	xTaskCreate(&Velocidad_Vehiculo, "Velocidad_Vehiculo", 1024, NULL, 5, NULL);
	xTaskCreate(&Advertencia_Velocidad, "Advertencia_Velocidad", 1024, NULL, 5, NULL);
	xTaskCreate(&Obtener_Peso_Vehiculo, "Obtener_Peso_Vehiculo", 1024, NULL, 5, NULL);
	xTaskCreate(&Envio_Notificacion, "Envio_Notificacion", 1024, NULL, 5, NULL);
}
/*==================[end of file]============================================*/