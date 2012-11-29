/**
 * @file tc10_systemLoad.c
 *   Test case 10 of RTuinoOS. Three tasks with known CPU consumption are scheduled. A
 * simple system load estimation is compared to the the known CPU consumption of the
 * tasks.\n
 *   The system load estimation routine can be used in many other RTuinOS applications. The
 * prerequiste is, that the idle task is not used in the application. Or at maximum
 * for very low frequently done things. Most of the idle time is consumed by the system
 * load estimation.\n
 *   To reuse the load estimation in your application copy the files gsl_systemLoad.c/h to
 * your application and see the idle task implementation here to find out how to apply
 * the code.
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
#include "rtos_assert.h"
#include "gsl_systemLoad.h"


/*
 * Defines
 */
 
/** The number of system timer tics required to implement the time span given in Milli
    seconds. Consider to use an expression like \a TIME_IN_MS(10) as argument to the time
    related RTuinOS API functions in order to get readable code.
      @param tiInMs
      The desired time in Milli seconds. The argument must not exceed 127 as we use the 8
    Bit system timer.
      @remark
    The double operations are limited to the compile time if the argument of the macro is a
    literal. No double operation is then found in the machine code. Never use this macro
    with runtime expressions! */
#define TIME_IN_MS(tiInMs) ((uintTime_t)((double)(tiInMs)/RTOS_TIC_MS+0.5))



/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13
 
/** Stack size of task. */
#define STACK_SIZE   256
 

/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
static void taskT0C0(uint16_t taskParam);
static void taskT0C1(uint16_t taskParam);
static void taskT0C2(uint16_t taskParam);

 
/*
 * Data definitions
 */
 
static uint8_t _taskStackT0C0[STACK_SIZE]
             , _taskStackT0C1[STACK_SIZE]
             , _taskStackT0C2[STACK_SIZE];


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
 * A load producing task.
 *   @param initCondition
 * The task gets an initialization parameter for whatever configuration purpose.
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void taskT0C0(uint16_t taskCondition)

{
#define TI_CYCLE_MS 250

    uint16_t cnt=0;
    uint32_t ti = millis()
           , tiCycle;
    
    for(;;)
    {
        rtos_delay(35);
      
        delayMicroseconds(/* tiDelayInuS */ 15 * 1000u); /* 15 of 250 ms, i.e. 6% load. */
        if(++cnt >= 40)
        {
            delayMicroseconds(/* tiDelayInuS */ 15 * 1000u); /* 15 of 250 ms, i.e. 6% load. */
            delayMicroseconds(/* tiDelayInuS */ 15 * 1000u); /* 15 of 250 ms, i.e. 6% load. */
            
            if(cnt >= 80)
            {
                cnt = 0;
            }
        }

        rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ TIME_IN_MS(TI_CYCLE_MS));
        tiCycle = millis();
        float tiCycleRel = (tiCycle-ti) * (1.0/1000.0 / (TIME_IN_MS(TI_CYCLE_MS)/490.1961));
        ASSERT(tiCycleRel >= 0.9  &&  tiCycleRel <= 1.1);
        ti = tiCycle;
    }
#undef TI_CYCLE_MS
} /* End of taskT0C0 */





/**
 * A load producing task.
 *   @param initCondition
 * The task gets an initialization parameter for whatever configuration purpose.
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void taskT0C1(uint16_t taskCondition)

{
#define TI_CYCLE_MS 30

    uint32_t ti = millis()
           , tiCycle;
    
    while(rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ TIME_IN_MS(TI_CYCLE_MS)))
    {
        tiCycle = millis();
        float tiCycleRel = (tiCycle-ti) * (1.0/1000.0 / (TIME_IN_MS(TI_CYCLE_MS)/490.1961));
        ASSERT(tiCycleRel >= 0.9  &&  tiCycleRel <= 1.1);

        rtos_delay(TIME_IN_MS(3));  /* Delay without load. */
        delayMicroseconds(/* tiDelayInuS */ 7 * 1000u); /* 7 of 30 ms, i.e. 23% load. */
        rtos_delay(TIME_IN_MS(7));  /* Delay without load. */

        ti = tiCycle;
    }
    
#undef TI_CYCLE_MS
} /* End of taskT0C1 */





/**
 * A load producing task.
 *   @param initCondition
 * The task gets an initialization parameter for whatever configuration purpose.
 *   @remark
 * A task function must never return; this would cause a reset.
 */ 

static void taskT0C2(uint16_t taskCondition)

{
#define TI_CYCLE_MS 10

    uint32_t ti = millis()
           , tiCycle;
    
    while(rtos_suspendTaskTillTime(/* deltaTimeTillRelease */ TIME_IN_MS(TI_CYCLE_MS)))
    {
        tiCycle = millis();
        float tiCycleRel = (tiCycle-ti) * (1.0/1000.0 / (TIME_IN_MS(TI_CYCLE_MS)/490.1961));
        
        /* The boundaries for the test need to be wider here; we have a resolution of
           millis() of 1 ms in relation to the cycle time of 10 ms, the basic accuracy of
           the computation itself is thus only 10%. */
        ASSERT(tiCycleRel >= 0.8  &&  tiCycleRel <= 1.2);

        delayMicroseconds(/* tiDelayInuS */ 2 * 1000u); /* 2 of 10 ms, i.e. 20% load. */
        rtos_delay(TIME_IN_MS(2));  /* Delay without load. */

        ti = tiCycle;
    }
    
#undef TI_CYCLE_MS
} /* End of taskT0C2 */





/**
 * The initalization of the RTOS tasks and general board initialization.
 */ 

void setup(void)
{
    /* Start serial port at 9600 bps. */
    Serial.begin(9600);
    Serial.println("\n" RTOS_RTUINOS_STARTUP_MSG);

    /* Initialize the digital pin as an output. The LED is used for most basic feedback about
       operability of code. */
    pinMode(LED, OUTPUT);
    
    /* Configure task 0 of priority class 0 */
    uint8_t idxTask = 0
          , idxClass = 0;
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0C0
                       , /* prioClass */        idxClass++
                       , /* pStackArea */       &_taskStackT0C0[0]
                       , /* stackSize */        sizeof(_taskStackT0C0)
                       , /* startEventMask */   RTOS_EVT_ABSOLUTE_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     5
                       );
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0C1
                       , /* prioClass */        idxClass++
                       , /* pStackArea */       &_taskStackT0C1[0]
                       , /* stackSize */        sizeof(_taskStackT0C1)
                       , /* startEventMask */   RTOS_EVT_ABSOLUTE_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     2
                       );
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0C2
                       , /* prioClass */        idxClass++
                       , /* pStackArea */       &_taskStackT0C2[0]
                       , /* stackSize */        sizeof(_taskStackT0C2)
                       , /* startEventMask */   RTOS_EVT_ABSOLUTE_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     25
                       );
                       
    /* Test if GCC actually recognizes the constant expression. */
    ASSERT(__builtin_constant_p(TIME_IN_MS(/* tiInMs */ 37)));

} /* End of setup */




/**
 * The application owned part of the idle task. This routine is repeatedly called whenever
 * there's some execution time left. It's interrupted by any other task when it becomes
 * due.
 *   @remark
 * Different to all other tasks, the idle task routine may and should return. (The task as
 * such doesn't terminate). This has been designed in accordance with the meaning of the
 * original Arduino loop function.
 */ 

void loop(void)
{
    /* The system load is computed in the idle task. */
    
    /* Compute the system load. Caution, this function may take a very long time to return
       in case of system loads close to 100%; normally, it takes about a second to return,
       this is the averaging time. */
    uint8_t systemLoad = gsl_getSystemLoad();
    Serial.print("System load: ");
    Serial.print((systemLoad+1)>>1);  /* Round to percent. */
    Serial.println("%");

    blink(1);
    
    ASSERT(rtos_getTaskOverrunCounter(0, false) == 0);
    ASSERT(rtos_getTaskOverrunCounter(1, false) == 0);
    ASSERT(rtos_getTaskOverrunCounter(2, false) == 0);

} /* End of loop */




