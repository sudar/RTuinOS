/**
 * @file tc05_setEvent.c
 *   Test case 05 or RTuinoOS. Several tasks of different priority are defined. Task
 * switches are partly controlled by manually posted events and counted and reported in the
 * idle task.
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
 *   task00_class00
 *   task01_class00
 *   task00_class01
 */

/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"
#include "rtos_assert.h"


/*
 * Defines
 */
 
/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13
 
/** Stack size of all the tasks. */
#define STACK_SIZE_TASK00_C0   256
#define STACK_SIZE_TASK01_C0   256
#define STACK_SIZE_TASK00_C1   256
 

/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
static void task00_class00(uint16_t postedEventVec);
static void task01_class00(uint16_t postedEventVec);
static void task00_class01(uint16_t postedEventVec);
 
 
/*
 * Data definitions
 */
 
static uint8_t _taskStack00_C0[STACK_SIZE_TASK00_C0]
             , _taskStack01_C0[STACK_SIZE_TASK01_C0]
             , _taskStack00_C1[STACK_SIZE_TASK00_C1];

rtos_task_t rtos_taskAry[RTOS_NO_TASKS+1] =
{ /* Task 0 of priority class 0 */
  { /* prioClass */	        0
  , /* taskFunction */	    task00_class00
  , /* taskFunctionParam */	0
  , /* timeDueAt */	        5
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack00_C0[0]   
  , /* stackSize */	        sizeof(_taskStack00_C0)
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  } /* End Task 1 */
  
, /* Task 1 of priority class 0 */
  { /* prioClass */	        0
  , /* taskFunction */	    task01_class00
  , /* taskFunctionParam */	0
  , /* timeDueAt */	        15
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack01_C0[0]   
  , /* stackSize */	        sizeof(_taskStack01_C0)
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  } /* End Task 1 */
  
, /* Task 0 of priority class 1 */
  { /* prioClass */	        1
  , /* taskFunction */	    task00_class01
  , /* taskFunctionParam */	10
  , /* timeDueAt */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack00_C1[0]   
  , /* stackSize */	        sizeof(_taskStack00_C1)
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
 
static volatile uint16_t noLoopsTask00_C0 = 0;
static volatile uint16_t noLoopsTask01_C0 = 0;
static volatile uint16_t noLoopsTask00_C1 = 0;
 
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
 * One of the low priority tasks in this test case.
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void task00_class00(uint16_t initCondition)

{
    for(;;)
    {
        ++ noLoopsTask00_C0;

        /* This tasks cycles with about 200ms. */
        rtos_delay(80);
        rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 100);
    }
} /* End of task00_class00 */





/**
 * Second task of low priority in this test case.
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void task01_class00(uint16_t initCondition)

{
    for(;;)
    {
        uint16_t u;
        
        ++ noLoopsTask01_C0;

        /* For test purpose only: This task consumes the CPU for most of the cycle time. */
        //delay(8 /*ms*/);
        
        /* Release high priority task for a single cycle. It should continue operation
           before we leave the suspend function here. Check it. */
        u = noLoopsTask00_C1;
        rtos_setEvent(/* eventVec */ RTOS_EVT_EVENT_00);
        ASSERT(u+1 == noLoopsTask00_C1)
        ASSERT(noLoopsTask01_C0 == noLoopsTask00_C1)
        
        /* This tasks cycles with about 10ms. */
        rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 1);
    }
} /* End of task01_class00 */





/**
 * Task of high priority.
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void task00_class01(uint16_t initCondition)

{
    /* At the moment all tasks are started via absolute timer event. This is a useless
       limitation and should become configurable for the application. Then we would wait
       for our event right from the beginning. */
//    ASSERT(initCondition == RTOS_EVT_EVENT_00)
    ASSERT(initCondition == RTOS_EVT_ABSOLUTE_TIMER)
    
    /* This tasks cycles once it is awaked by the event. */
    while(rtos_waitForEvent( /* eventMask */ RTOS_EVT_EVENT_00 | RTOS_EVT_DELAY_TIMER
                           , /* all */ false
                           , /* timeout */ 50+5 /* about 100 ms */
                           )
          == RTOS_EVT_EVENT_00
         )
    {
        /* As long as we stay in the loop we didn't see a timeout. */
        ++ noLoopsTask00_C1;
    }
    
    /* We must never get here. Otherwise the test case failed. In compilation mode
       PRODUCTION, when there's no assertion, we will see an immediate reset because we
       leave a task function. */
    ASSERT(false)

} /* End of task00_class01 */





/**
 * The initalization of the RTOS tasks and general board initialization.
 */ 

void setup(void)
{
    /* All tasks are set up by using a compile-time expression. */    
    
    /* Start serial port at 9600 bps. */
    Serial.begin(9600);
    Serial.println("\nRTuinOS starting up");

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
    Serial.println("RTuinOS is idle");
    Serial.print("noLoopsTask00_C0: "); Serial.println(noLoopsTask00_C0);
    Serial.print("noLoopsTask01_C0: "); Serial.println(noLoopsTask01_C0);
    Serial.print("noLoopsTask00_C1: "); Serial.println(noLoopsTask00_C1);
    blink(4);
    
} /* End of loop */




