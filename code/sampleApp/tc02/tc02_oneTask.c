/**
 * @file tc01_idleTaskOnly.c
 *   Test case 01 or RTuinoOS. No task is defined, only the idle task is running. System
 * behaves like an ordinary Arduino sketch.
 *
 * Copyright (C) 2012 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Module interface
 *   setup
 *   loop
 * Local functions
 *   blink
 *   task01_class00
 */

/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"


/*
 * Defines
 */
 
/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13
 
/** Stack size of task. */
#define STACK_SIZE_TASK00   256
 

/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
static void task01_class00(uint16_t taskParam);
 
 
/*
 * Data definitions
 */
 
static uint8_t _taskStack[STACK_SIZE_TASK00];

rtos_task_t rtos_taskAry[RTOS_NO_TASKS+1] =
{ /* Task 1 */
  { /* prioClass */	        0
  , /* taskFunction */	    task01_class00
  , /* taskFunctionParam */	10
  , /* timeDueAt */	        5
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack[0]   
  , /* stackSize */	        STACK_SIZE_TASK00
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  } /* End Task 1 */
  
, /* Idle Task */
  { /* prioClass */	        0
  , /* taskFunction */      NULL
  , /* taskFunctionParam */	0
  , /* timeDueAt */	        255
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    NULL
  , /* stackSize */	        0
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  } /* End Task 1 */
  
}; /* End of initialization of task array. */
 
 
/*
 * Function implementation
 */

/**
 * Trivial routine that flashes the LED a number of times to give simple feedback. The
 * routine is blocking.
 *   @param noFlashes
 * The number of times the LED is lit.
 */
 
static void blink(uint8_t noFlashes)
{
#define TI_FLASH 150

    while(noFlashes-- > 0)
    {
        digitalWrite(LED, HIGH);  /* Turn the LED on. (HIGH is the voltage level.) */
        delay(TI_FLASH);          /* The flash time. */
        digitalWrite(LED, LOW);   /* Turn the LED off by making the voltage LOW. */
        delay(TI_FLASH);          /* Time between flashes. */
    }                              
    delay(1000-TI_FLASH);         /* Wait for a second after the last flash - this command
                                     could easily be invoked immediately again and the
                                     bursts need to be separated. */
#undef TI_FLASH
}



/**
 * The only task in this test case (besides idle).
 *   @param initParam
 * The task gets an initialization parameter for whatever configuration purpose.
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void task01_class00(uint16_t taskParam)

{
    uint16_t u;
    
    Serial.print("task01_class00: Activated by 0x");
    Serial.println(taskParam, HEX);

//    for(u=0; u<3; ++u)
//        blink(2);
    
    for(;;)
    {
        Serial.println("task01_class00: rtos_delay...");
        u = rtos_delay(255);
        Serial.print("task01_class00: Released with ");
        Serial.println(u, HEX);
        
        Serial.println("task01_class00: Suspending...");
        u = rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 125);
        Serial.print("task01_class00: Released with ");
        Serial.println(u, HEX);
    }
} /* End of task01_class00 */





/**
 * The initalization of the RTOS tasks and general board initialization.
 */ 

void setup(void)
{
    /* All tasks are set up by using a compile-time expression. */    
    
    /* Start serial port at 9600 bps. */
    Serial.begin(9600);
    Serial.println("RTuinOS starting up");

    /* Initialize the digital pin as an output. The LED is used for most basic feedback about
       operability of code. */
    pinMode(LED, OUTPUT);
    
    Serial.print("sizeof(rtos_task_t): "); 
    Serial.println(sizeof(rtos_task_t)); 
    
} /* End of setup */




/**
 * The application owned part of the idle task. This routine is repeatedly called whenever
 * there's some execution time left. It's interrupted by any other task when it becomes
 * due.
 *   @remark
 * Different to all other tasks, the idle task routine may and should termninate. This has
 * been designed in accordance with the meaning of the original Arduino loop function.
 */ 

void loop(void)
{
    extern uintTime_t _time;
    extern uint16_t _postEv;
    extern uint16_t _makeDue;
    extern uint8_t _activeTaskId;

//    Serial.println("RTuinOS is idle");
    delay(3000);
//    Serial.print("time: "); Serial.println(_time);
//    Serial.print("postEv: "); Serial.println(_postEv);
//    Serial.print("makeDue: "); Serial.println(_makeDue);
//    Serial.print("activeTaskId: "); Serial.println(_activeTaskId);
    blink(4);
    
} /* End of loop */




