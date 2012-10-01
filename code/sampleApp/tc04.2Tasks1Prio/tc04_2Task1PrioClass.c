/**
 * @file tc04_2Task1PrioClass.c
 *   Test case 03 of RTuinOS. Two tasks of same priority class are defined besides the idle
 * task.
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
static void task02_class00(uint16_t taskParam);
 
 
/*
 * Data definitions
 */
 
static uint8_t _taskStack1[STACK_SIZE_TASK00];
static uint8_t _taskStack2[STACK_SIZE_TASK00];

rtos_task_t rtos_taskAry[RTOS_NO_TASKS+1] =
{ /* Task 1 */
  { /* prioClass */	        0
  , /* taskFunction */	    task01_class00
  , /* taskFunctionParam */	0
  , /* timeDueAt */	        5
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack1[0]   
  , /* stackSize */	        STACK_SIZE_TASK00
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  , /* fillToPowerOfTwoSize */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  } /* End Task 1 */
  
, /* Task 2 */
  { /* prioClass */	        0
  , /* taskFunction */	    task02_class00
  , /* taskFunctionParam */	0
  , /* timeDueAt */	        250
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* timeRoundRobin */	0
#endif
  , /* pStackArea */	    &_taskStack2[0]   
  , /* stackSize */	        STACK_SIZE_TASK00
  , /* cntDelay */	        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
  , /* cntRoundRobin */	    0
#endif
  , /* postedEventVec */	0
  , /* eventMask */	        0
  , /* waitForAnyEvent */	0
  , /* stackPointer */	    0
  , /* fillToPowerOfTwoSize */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  } /* End Task 2 */
  
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
  , /* fillToPowerOfTwoSize */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
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


volatile uint16_t t1=0, t2=0, id=0;


/**
 * The only task in this test case (besides idle).
 *   @param initParam
 * The task gets an initialization parameter for whatever configuration purpose.
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void task01_class00(uint16_t taskParam) __attribute((used, noinline));
static void task01_class00(uint16_t taskParam)

{
    uint16_t u;
    uint16_t sp1, sp2, spId;
    uint32_t ti0, ti1;
    
    Serial.print("task01_class00: Activated by 0x");
    Serial.println(taskParam, HEX);

//    for(u=0; u<1; ++u)
//        blink(2);
    
    for(;;)
    {
        uint8_t noSusT, susTIdAry[RTOS_NO_TASKS+5], noDue=9;
        extern volatile uint8_t _noSuspendedTasks, _suspendedTaskIdAry[RTOS_NO_TASKS], _noDueTasksAry[RTOS_NO_PRIO_CLASSES];
        
        ++ t1;
        Serial.print("t1: "); Serial.print(t1);
        Serial.print(", t2: "); Serial.print(t2);
        Serial.print(", id: "); Serial.println(id);
        
#if 1
        {
            cli();
            sp1  = rtos_taskAry[0].stackPointer;
            sp2  = rtos_taskAry[1].stackPointer;
            spId = rtos_taskAry[2].stackPointer;
            sei();
            Serial.print("sp1: 0x"); Serial.print(sp1, HEX);
            Serial.print(", sp1: 0x"); Serial.print(sp2, HEX);
            Serial.print(", spId: 0x"); Serial.println(spId, HEX);
        }            
#endif
        Serial.println("task01_class00: rtos_delay(20)");
        ti0 = millis();
        rtos_delay(20);
        ti1 = millis();
        Serial.print("task01_class00: Back from delay after ");
        Serial.println(ti1-ti0);
        
#if 0        
        cli();
        noSusT = _noSuspendedTasks;
        noDue  = _noDueTasksAry[0];
        sei();
        Serial.print("noSusT: "); Serial.println(noSusT); 
        Serial.print("noDue: "); Serial.println(noDue); 
        cli();
        for(sp1=0; sp1<RTOS_NO_TASKS; ++sp1)
            susTIdAry[sp1] = _suspendedTaskIdAry[sp1];
        sei();
        for(sp1=0; sp1<RTOS_NO_TASKS; ++sp1)
        {
            Serial.print("susTId: "); Serial.println(susTIdAry[sp1]); 
        }
#endif
        Serial.print("task01_class00: Suspending at ");
        Serial.println(millis());

        u = rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 125);
        
        Serial.print("task01_class00: Released at ");
        Serial.println(millis());
        //Serial.println(u, HEX);
    }
} /* End of task01_class00 */




static void task02_class00(uint16_t taskParam) __attribute((used, noinline));
static void task02_class00(uint16_t taskParam)

{
    uint16_t u;
    
    for(;;)
    {
        volatile uint8_t v = 0;
        
        ++ t2;
        
//        delay(1000);
//        for(u=0; u<1000; ++u)
//            v = 2*v;

        u = rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 100);
    }
} /* End of task02_class00 */





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
    uint8_t u;    
    bool ok = true;
    
    for(u=0; u<10; ++u)
    {
        if(_taskStack1[u] != 0x29  ||  _taskStack2[u] != 0x29)
        {
            ok = false;
            break;
        }
    }
    
    if(ok)
        blink(2);
    else
        blink(3);

    ++ id;    
    
} /* End of loop */




