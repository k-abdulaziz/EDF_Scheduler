/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/* Task Handlers */
TaskHandle_t Button_1_Monitor_Handle = NULL;
TaskHandle_t Button_2_Monitor_Handle = NULL;
TaskHandle_t Periodic_Transmitter_Handle = NULL;
TaskHandle_t UART_Receiver_Handle = NULL;
TaskHandle_t Load_1_Simulation_Handle = NULL;
TaskHandle_t Load_2_Simulation_Handle = NULL;

/*Tasks Prototypes*/
void Button_1_Monitor( void * pvParameters );
void Button_2_Monitor( void * pvParameters );
void Periodic_Transmitter (void * pvParameters );
void UART_Receiver (void * pvParameters );
void Load_1_Simulation ( void * pvParameters );
void Load_2_Simulation ( void * pvParameters );

/* Execution Time Variables */
uint32_t B1_inTime, B2_inTime;
uint32_t L1_inTime, L2_inTime;
uint32_t Tx_inTime, Rx_inTime;
float Total_ExecTime;
float CPU_Load;

/* RunTime Stats Buffer*/
char Run_Time_Stats[300];

/* Queue Handlers */
QueueHandle_t Queue1 = NULL;
QueueHandle_t Queue2 = NULL;
QueueHandle_t Queue3 = NULL;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );

volatile int misses = 0;
/*-----------------------------------------------------------*/

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
		/* Setup the hardware for use with the Keil demo board. */
		prvSetupHardware();
	
		/* Message Queues Creation */
		Queue1 = xQueueCreate(1, sizeof(char));
		Queue2 = xQueueCreate(1, sizeof(char));
		Queue3 = xQueueCreate(15, sizeof(char));
	
   /* Create Tasks here */
	 /* Create the task, storing the handle. */
   xTaskPeriodicCreate(
                    Button_1_Monitor,       /* Function that implements the task. */
                    "B1",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Button_1_Monitor_Handle, /* Used to pass out the created task's handle. */
										50 );      /* Task deadline. */
										
   xTaskPeriodicCreate(
                    Button_2_Monitor,       /* Function that implements the task. */
                    "B2",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Button_2_Monitor_Handle, /* Used to pass out the created task's handle. */
										50 );      /* Task deadline. */										

   xTaskPeriodicCreate(
                    Periodic_Transmitter,       /* Function that implements the task. */
                    "Tx",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Periodic_Transmitter_Handle, /* Used to pass out the created task's handle. */
										100 );      /* Task deadline. */

   xTaskPeriodicCreate(
                    UART_Receiver,       /* Function that implements the task. */
                    "Rx",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &UART_Receiver_Handle, /* Used to pass out the created task's handle. */
										20 );      /* Task deadline. */

   xTaskPeriodicCreate(
                    Load_1_Simulation,       /* Function that implements the task. */
                    "L1",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Load_1_Simulation_Handle, /* Used to pass out the created task's handle. */
										10 );      /* Task deadline. */

   xTaskPeriodicCreate(
                    Load_2_Simulation,       /* Function that implements the task. */
                    "L2",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Load_2_Simulation_Handle, /* Used to pass out the created task's handle. */
										100 );      /* Task deadline. */
										
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}

/*-----------------------------------------------------------*/

/* Tasks to be created. */

void Button_1_Monitor(void* pvParameters)
{
	pinState_t Button1_CurrState;
	pinState_t Button1_PrevState = GPIO_read(PORT_1, PIN0);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint8_t Edge_Flag = 0;

	for( ;; )
	{
		/* Read current state for button 1 */
		Button1_CurrState = GPIO_read(PORT_1, PIN0);
		
		/* Detect the edge */
		if ( ( Button1_CurrState == PIN_IS_HIGH ) && ( Button1_PrevState == PIN_IS_LOW ) )
		{
			/*Rising edge */
			Edge_Flag = '+';
		}
		else if ( ( Button1_CurrState == PIN_IS_LOW ) && ( Button1_PrevState == PIN_IS_HIGH ) )
		{
			/*Falling edge */
			Edge_Flag = '-';
		}
		else
		{
			/*No event*/
			Edge_Flag = '=';
		}
		
		/* Send the new data to the consumer */
		xQueueOverwrite(Queue1, &Edge_Flag);

		/* Update the previous (Reference) State */
		Button1_PrevState = Button1_CurrState;
				
		/* priodicity = 50 ms */
		vTaskDelayUntil(&xLastWakeTime, 50);
	}
}

void Button_2_Monitor(void* pvParameters)
{
	pinState_t Button2_CurrState;
	pinState_t Button2_PrevState = GPIO_read(PORT_1, PIN1);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint8_t Edge_Flag = 0;

	/* Warning: I ignored debaunce effect as we run on simulation not real hardware */
	for( ;; )
	{
		/* Read current state for button 1 */
		Button2_CurrState = GPIO_read(PORT_1, PIN1);
		
		/* Detect the edge */
		if ( ( Button2_CurrState == PIN_IS_HIGH ) && ( Button2_PrevState == PIN_IS_LOW ) )
		{
			/* Rising edge  */
			Edge_Flag = '+';
		}
		else if ( ( Button2_CurrState == PIN_IS_LOW ) && ( Button2_PrevState == PIN_IS_HIGH ) )
		{
			/*Falling edge  */
			Edge_Flag = '-';
		}
		else
		{
			/* No edge is detected */
			Edge_Flag = '=';
		}
			
		/* Send the new data to the consumer */
		xQueueOverwrite(Queue2, &Edge_Flag );

		/* Update the previous (Reference) State */
		Button2_PrevState = Button2_CurrState;

		/* priodicity = 50 ms */
		vTaskDelayUntil(&xLastWakeTime, 50);
	}
}

void Periodic_Transmitter(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint8_t index = 0;
	char strTx[15];
	strcpy(strTx, "\n100 ms");
	
	for( ;; )
	{
		/* Send the string to UART_Receiver_Task character by character */
		for (index = 0; index < 15; index++)
		{
			xQueueSend(Queue3, strTx + index, 100);
		}
		
		/* priodicity = 100 ms */
		vTaskDelayUntil(&xLastWakeTime, 100);
	}
}

void UART_Receiver(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	int8_t B1_Data;
	int8_t B2_Data;
	uint8_t index = 0;
	char strRx[15];

	for( ;; )
	{
		index = 0;
		/* Button 1  */
		if ( xQueueReceive(Queue1, &B1_Data, 0) && ( B1_Data != '=' ) )
		{
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('1');
			xSerialPutChar(':');
			xSerialPutChar(B1_Data);
		}
		else
		{	
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
		}
		
		index = 0;
		/* Button 2  */
		if ( xQueueReceive(Queue2, &B2_Data, 0) && ( B2_Data != '=' ) )
		{
			xSerialPutChar('\n');		
			xSerialPutChar('B');
			xSerialPutChar('2');
			xSerialPutChar(':');
			xSerialPutChar(B2_Data);
		}
		else
		{
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
			xSerialPutChar(' ');
		}
		
		/* Periodic_Transmitter_Task String */
		if ( uxQueueMessagesWaiting(Queue3) != 0)
		{
			for(index = 0; index < 15; index++)
			{
				xQueueReceive(Queue3, strRx + index, 0);
			}
			/* Send the name of button character by character */
			vSerialPutString( (int8_t *) strRx, strlen(strRx));
			xQueueReset(Queue3);
		}
		#if(GetRunTimeStats == 1)	
		   	xSerialPutChar('\n');		
			vTaskGetRunTimeStats(Run_Time_Stats);
   		    vSerialPutString( (int8_t *) Run_Time_Stats, strlen(Run_Time_Stats));
		#endif

		/*Periodicity = 20 ms*/
		vTaskDelayUntil(&xLastWakeTime, 20);
	}
}

void Load_1_Simulation(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint32_t counter = 0;
	uint32_t x = 12000 * 5; /* (XTAL / 1000U)*time_in_ms  */
	for( ;; )
	{
		for( counter =0 ; counter <= x; counter++)
		{
			/* 5 ms delay */
		}
		/*Periodicity = 10 ms*/ 
		vTaskDelayUntil( &xLastWakeTime , 10); 
	}
}

void Load_2_Simulation(void* pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	uint32_t counter = 0;
	uint32_t x = 12000 * 12; /* (XTAL / 1000U)*time_in_ms  */
	for( ;; )
	{
		for( counter =0 ; counter <= x; counter++)
		{
			/* 12 ms delay */
		}
		/*Periodicity = 100 ms*/ 
		vTaskDelayUntil( &xLastWakeTime , 100); 
	}
}

/*-----------------------------------------------------------*/

/* Tick Hook Implementation */
void vApplicationTickHook( void )
{
	/* Function code goes here. */
	GPIO_write(PORT_0, PIN0, PIN_IS_HIGH);
	GPIO_write(PORT_0, PIN0, PIN_IS_LOW);
}

/* Tick Hook Implementation */
void vApplicationIdleHook( void )
{
	/* Function code goes here. */
}

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}

/*-----------------------------------------------------------*/
