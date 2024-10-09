/*! @mainpage guia2_ej4
 *
 * @section genDesc General Description
 * 
 * Este ejercicio consiste en el desarrollo de una aplicación que, por un lado, permite la convesión analógica-digital
 * de una señal que ingresa por los pines destinados para este fin (CH1 en este caso), luego esta señal se envía
 * mediante la UART y se puede visualizar en un osciloscopio que visualiza los datos del puerto serie. A su vez,
 * también es posible realizar una conversión digital-analógica (por medio del CH0) de una señal mediante sus valores
 * muestreados, y visualizarla también en la misma interfaz.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |     ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN1-POT 	|	   CH0		|
 * | 	PIN2-POT 	|	   CH1		|
 * | 	PIN3-POT 	|	   GND		|
 *
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
/** @def BUFFER_SIZE
 * @brief indica el tamaño del buffer del vector ecg
*/
#define BUFFER_SIZE 231

/** @def CONFIG_SENSOR_TIMER_ADC
 * @brief tiempo en micro segundos del timer ADC
*/
#define CONFIG_SENSOR_TIMER_ADC 2000//500Hz de frecuencia de muestreo que se piden en el enunciado

/** @def CONFIG_SENSOR_TIMER_DAC
 * @brief tiempo en micro segundos del timer DAC
*/
#define CONFIG_SENSOR_TIMER_DAC 4000//4 ms de retraso del tiempo para tomar cada valor de la señal a convertir

/** @def BASE
 * @brief base numérica del número que se va a convertir a ASCII, es la base de la conversión
*/
#define BASE 10

/*==================[internal data definition]===============================*/
TaskHandle_t main_task_handle = NULL;
TaskHandle_t ADC_Conversion_Task_handle = NULL;
TaskHandle_t DAC_Conversion_Task_handle = NULL;
TaskHandle_t UartEnviar_Task_handle = NULL;

/** @var  value
 *  @brief variable donde se almacena el valor analógico leído.
*/
volatile uint16_t value = 5;//con un valor inicial de 0 no anda, pero con 5 si lo toma. Peña en la consulta me dijo que lo deje así :)

/** @var contador
 *  @brief variable que se utiliza para recorrer el vector de ECG.
*/
volatile uint8_t contador = 0;

/** @const ecg[]
 *  @brief vector ecg que simula un ecg.
*/
const char ecg[BUFFER_SIZE] = {
    76, 77, 78, 77, 79, 86, 81, 76, 84, 93, 85, 80,
    89, 95, 89, 85, 93, 98, 94, 88, 98, 105, 96, 91,
    99, 105, 101, 96, 102, 106, 101, 96, 100, 107, 101,
    94, 100, 104, 100, 91, 99, 103, 98, 91, 96, 105, 95,
    88, 95, 100, 94, 85, 93, 99, 92, 84, 91, 96, 87, 80,
    83, 92, 86, 78, 84, 89, 79, 73, 81, 83, 78, 70, 80, 82,
    79, 69, 80, 82, 81, 70, 75, 81, 77, 74, 79, 83, 82, 72,
    80, 87, 79, 76, 85, 95, 87, 81, 88, 93, 88, 84, 87, 94,
    86, 82, 85, 94, 85, 82, 85, 95, 86, 83, 92, 99, 91, 88,
    94, 98, 95, 90, 97, 105, 104, 94, 98, 114, 117, 124, 144,
    180, 210, 236, 253, 227, 171, 99, 49, 34, 29, 43, 69, 89,
    89, 90, 98, 107, 104, 98, 104, 110, 102, 98, 103, 111, 101,
    94, 103, 108, 102, 95, 97, 106, 100, 92, 101, 103, 100, 94, 98,
    103, 96, 90, 98, 103, 97, 90, 99, 104, 95, 90, 99, 104, 100, 93,
    100, 106, 101, 93, 101, 105, 103, 96, 105, 112, 105, 99, 103, 108,
    99, 96, 102, 106, 99, 90, 92, 100, 87, 80, 82, 88, 77, 69, 75, 79,
    74, 67, 71, 78, 72, 67, 73, 81, 77, 71, 75, 84, 79, 77, 77, 76, 76,
};

/*==================[internal functions declaration]=========================*/

/**
 * @brief Tarea para enviar datos por UART.
 * 
 * Esta tarea se encarga de enviar el valor actual de la variable "value" por UART a la PC.
 * Se ejecuta indefinidamente hasta recibir una notificación.
 * 
 * @param param Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void UartEnviar(void* param)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /*La tarea espera en este punto hasta recibir una notificación*/

        UartSendString(UART_PC, (char*) UartItoa(value, BASE));
        /* >>> UartItoa toma un número entero como entrada, lo convierte a una cadena de caracteres y luego
               envía esa cadena a través de la interfaz UART. En este caso, se envía el valor value
               convertido a una cadena de caracteres en decimal a través de la interfaz UART_PC
		   >>> (char*) hace un casting de la cadena de caracteres resultante a un puntero a char (char*),
               que es el tipo de dato esperado por la función UartSendString
        */
        UartSendString(UART_PC, "\r");
    }	
}

/**
 * @brief Timer para la tarea de conversión ADC y la tarea de envío de datos por UART.
 * 
 * Esta función se utiliza como callback para la interrupción del timer ADC.
 * Envía una notificación a la tarea ADC_Conversion para que realice su operación.
 * Envía una notificación a la tarea FuncionUartEnviar para que realice su operación.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void TimerADC(void *pvParameter)
{
    vTaskNotifyGiveFromISR(ADC_Conversion_Task_handle, pdFALSE);
    vTaskNotifyGiveFromISR(UartEnviar_Task_handle, pdFALSE);
}

/**
 * @brief Timer para la tarea de conversión DAC.
 * 
 * Esta función se utiliza como callback para la interrupción del timer DAC.
 * Envía una notificación a la tarea DAC_Conversion para que realice su operación.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
void TimerDAC(void *pvParameter)
{
    vTaskNotifyGiveFromISR(DAC_Conversion_Task_handle, pdFALSE);
}

/**
 * @brief Tarea para la conversión analógica digital.
 * 
 * Esta tarea se encarga de leer el valor analógico del canal CH1 y guardarlo en la variable "value". Se
 * ejecuta indefinidamente hasta recibir una notificación.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void ADC_Conversion(void *pvParameter)//ADC: Analogic Digital Converter
{
    //uint16_t value = 0;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /*La tarea espera en este punto hasta recibir una notificación*/
        AnalogInputReadSingle(CH1, &value);//Leo el valor analógico del canal CH1 y lo guardo en la variable "value"
        
        //UartSendString(UART_PC, (char*) UartItoa(value, BASE));
        /* >>> UartItoa toma un número entero como entrada, lo convierte a una cadena de caracteres y luego
               envía esa cadena a través de la interfaz UART. En este caso, se envía el valor value
               convertido a una cadena de caracteres en decimal a través de la interfaz UART_PC
		   >>> (char*) hace un casting de la cadena de caracteres resultante a un puntero a char (char*),
               que es el tipo de dato esperado por la función UartSendString
        */
        //UartSendString(UART_PC, "\r");
    }
}

/**
 * @brief Tarea para la conversión digital analógica.
 * 
 * Esta tarea se encarga de enviar los valores del arreglo ecg a la función AnalogOutputWrite en un bucle,
 * actualizando el contador para avanzar a través del arreglo. Cuando se llega al final del arreglo, el
 * contador se restablece a 0 para comenzar de nuevo. Se ejecuta indefinidamente hasta recibir una notificación.
 * 
 * @param pvParameter Puntero void a un parámetro, no utilizado en esta función.
 * 
 * @return Ninguno
 */
static void DAC_Conversion(void *pvParameter)//DAC: Digital Analogic Converter
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /*La tarea espera en este punto hasta recibir una notificación*/

        //Envío los valores del arreglo ecg a la función AnalogOutputWrite en un bucle, actualizando
        //el contador para avanzar a través del arreglo. Cuando se llega al final del arreglo, el contador
        //se restablece a 0 para comenzar de nuevo.
        AnalogOutputWrite(ecg[contador]);
        if (contador < BUFFER_SIZE - 1)
        {
            contador++;//Se incrementa el valor de contador en 1. Esto hace que el siguiente valor del
            //arreglo ecg sea enviado a la función AnalogOutputWrite en la próxima iteración.
        }
        else
        {
            contador = 0;//Se restablece el valor de contador a 0. Esto hace que el bucle comience de
            //nuevo desde el principio del arreglo ecg.
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    //Inicialización de timers
    timer_config_t timer_ADC = {
        .timer = TIMER_A,
        .period = CONFIG_SENSOR_TIMER_ADC,
        .func_p = TimerADC,
        .param_p = NULL
    };
    TimerInit(&timer_ADC);

    timer_config_t timer_DAC = {
        .timer = TIMER_B,
        .period = CONFIG_SENSOR_TIMER_DAC,
        .func_p = TimerDAC,
        .param_p = NULL
    };
    TimerInit(&timer_DAC);

    analog_input_config_t ADC_config = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0 //es 0 porque toma valores solo para lectura continua, y yo le puse single read no continuos
    };
    
    AnalogInputInit(&ADC_config);

    //Configuro la UART para comunicacion bidireccional
	serial_config_t uart_config = {//serial_config_t es el struct
		.port = UART_PC,
		.baud_rate = 115200,
		.func_p = NULL,//poner funcion para leer, en este caso no hay
		.param_p = NULL
	};
	UartInit(&uart_config);//inicializo la UART

    AnalogOutputInit();

    //Creación de tareas
    xTaskCreate(&ADC_Conversion, "ConversionADC", 2048, NULL, 4, &ADC_Conversion_Task_handle);
	xTaskCreate(&DAC_Conversion, "ConversionDAC", 2048, NULL, 5, &DAC_Conversion_Task_handle);
    xTaskCreate(&UartEnviar, "Envio de datos a UART", 2048, NULL, 4, &UartEnviar_Task_handle);

    //Inicialización del conteo de timers
    TimerStart(timer_ADC.timer);
    TimerStart(timer_DAC.timer);
}
/*==================[end of file]============================================*/