/**
 * @file tc05_setEvent.c
 *   Test case 05 of RTuinoOS. Several tasks of different priority are defined. Task
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
 *   rtos_enableIRQTimerTic
 *   loop
 * Local functions
 *   blink
 *   subRoutine
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
 * A sub routine which has the only meaning of consuming stack - in order to test the stack
 * usage computation.
 *   @param nestedCalls
 * The routine will call itself recursively \a nestedCalls-1 times. In total the stack will
 * be burdened by \a nestedCalls calls of this routine.
 *   @remark
 * The optimizer removes the recursion completely. The stack-use effect of the sub-routine
 * is very limited, but still apparent the first time it is called.
 */ 

static volatile uint8_t _touchedBySubRoutine; /* To discard removal of recursion by
                                                 optimization. */
static RTOS_TRUE_FCT void subRoutine(uint8_t);
static void subRoutine(uint8_t nestedCalls)
{
    volatile uint8_t stackUsage[43];
    if(nestedCalls > 1)
    {
        _touchedBySubRoutine += 2;
        stackUsage[0] = 
        stackUsage[sizeof(stackUsage)-1] = 0;
        subRoutine(nestedCalls-1);    
    }
    else
    {
        ++ _touchedBySubRoutine;
        stackUsage[0] = 
        stackUsage[sizeof(stackUsage)-1] = nestedCalls;
    }
} /* End of subRoutine */





/**
 * Test of redefining the central interrupt of RTuinOS. The default implementation of the
 * interrupt configuration function is overridden by redefining the same function.\n
 */ 

void rtos_enableIRQTimerTic(void)
{
    Serial.println("Overloaded interrupt initialization rtos_enableIRQTimerTic in " __FILE__);
    
#ifdef __AVR_ATmega2560__
    /* Initialization of the system timer: Arduino (wiring.c, init()) has initialized
       timer2 to count up and down (phase correct PWM mode) with prescaler 64 and no TOP
       value (i.e. it counts from 0 till MAX=255). This leads to a call frequency of
       16e6Hz/64/510 = 490.1961 Hz, thus about 2 ms period time.
         Here, we found on this setting (in order to not disturb any PWM related libraries)
       and just enable the overflow interrupt. */
    TIMSK2 |= _BV(TOIE2);
#else
# error Modifcation of code for other AVR CPU required
#endif
    
} /* End of rtos_enableIRQTimerTic */






/**
 * One of the low priority tasks in this test case.
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static uint16_t _task00_C0_cntWaitTimeout = 0;

static void task00_class00(uint16_t initCondition)

{
    for(;;)
    {
        ++ noLoopsTask00_C0;

        /* To see the stack reserve computation working we invoke a nested sub-routine
           after a while. */
        if(millis() > 20000ul)
            subRoutine(1);
        if(millis() > 30000ul)
            subRoutine(2);
        if(millis() > 40000ul)
            subRoutine(3);
        
        /* Wait for an event from the idle task. The idle task is asynchrounous and its
           speed depends on the system load. The behavior is thus not perfectly
           predictable. Let's have a look on the overrrun counter for this task. */
        if(rtos_waitForEvent( /* eventMask */ RTOS_EVT_EVENT_03 //| RTOS_EVT_DELAY_TIMER
                            , /* all */ false
                            , /* timeout */ 40 /* about 80 ms */
                            )
           == RTOS_EVT_DELAY_TIMER
          )
        {
            ++ _task00_C0_cntWaitTimeout;
        }
        
        //rtos_delay(80);
        /* This tasks cycles with about 500ms. */
        rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ 0);
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
    ASSERT(initCondition == RTOS_EVT_EVENT_00)
    
    /* This tasks cycles once it is awaked by the event. */
    do
    {
        /* As long as we stay in the loop we didn't see a timeout. */
        ++ noLoopsTask00_C1;
    }
    while(rtos_waitForEvent( /* eventMask */ RTOS_EVT_EVENT_00 | RTOS_EVT_DELAY_TIMER
                           , /* all */ false
                           , /* timeout */ 50+5 /* about 100 ms */
                           )
          == RTOS_EVT_EVENT_00
         );
    
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
    
    /* Task 0 of priority class 0 */
    rtos_initializeTask( /* idxTask */          0
                       , /* taskFunction */     task00_class00
                       , /* prioClass */        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
                       , /* timeRoundRobin */
#endif
                       , /* pStackArea */       &_taskStack00_C0[0]
                       , /* stackSize */        sizeof(_taskStack00_C0)
                       , /* startEventMask */   RTOS_EVT_DELAY_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );

    /* Task 1 of priority class 0 */
    rtos_initializeTask( /* idxTask */          1
                       , /* taskFunction */     task01_class00
                       , /* prioClass */        0
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
                       , /* timeRoundRobin */
#endif
                       , /* pStackArea */       &_taskStack01_C0[0]
                       , /* stackSize */        sizeof(_taskStack01_C0)
                       , /* startEventMask */   RTOS_EVT_DELAY_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     15
                       );

    /* Task 0 of priority class 1 */
    rtos_initializeTask( /* idxTask */          2
                       , /* taskFunction */     task00_class01
                       , /* prioClass */        1
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
                       , /* timeRoundRobin */
#endif
                       , /* pStackArea */       &_taskStack00_C1[0]
                       , /* stackSize */        sizeof(_taskStack00_C1)
                       , /* startEventMask */   RTOS_EVT_EVENT_00
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );

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
    uint8_t idxStack;
    
    /* An event can be posted even if nobody is listening for it. */
    rtos_setEvent(/* eventVec */ RTOS_EVT_EVENT_04);

    /* This event will release task 0 of class 0. However we do not get here again fast
       enough to avoid all timeouts in that task. */
    rtos_setEvent(/* eventVec */ RTOS_EVT_EVENT_03);

    Serial.println("RTuinOS is idle");
    Serial.print("noLoopsTask00_C0: "); Serial.println(noLoopsTask00_C0);
    Serial.print("_task00_C0_cntWaitTimeout: "); Serial.println(_task00_C0_cntWaitTimeout);
    Serial.print("noLoopsTask01_C0: "); Serial.println(noLoopsTask01_C0);
    Serial.print("noLoopsTask00_C1: "); Serial.println(noLoopsTask00_C1);
    
    /* Look for the stack usage. */
    for(idxStack=0; idxStack<RTOS_NO_TASKS; ++idxStack)
    {
        Serial.print("Stack reserve of task");
        Serial.print(idxStack);
        Serial.print(": ");
        Serial.print(rtos_getStackReserve(idxStack));
        Serial.print(", task overrun: ");
        Serial.println(rtos_getTaskOverrunCounter(idxStack, /* doReset */ false));
    }
    
    blink(2);
    
} /* End of loop */




