/**
 * @file tc08_applInterrupt.c
 * Test case 08 of RTuinOS. A timer different to the RTuinOS system timer is installed as
 * second task switch causing interrupt. This interrupt set an event which triggers a task
 * of high priority. The interrupt events are counted to demonstrate the operation.
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
 *   rtos_enableIRQUser00
 * Local functions
 *   blinkNoBlock
 *   taskT0_C0
 *   taskT0_C1
 *   taskT0_C2
 */

/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"
#include "rtos_assert.h"
#include "tc08_applEvents.h"

/*
 * Defines
 */

/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13

/** Stack size of all the tasks. */
#define STACK_SIZE 200

/** The number of system timer tics required to implement the time span given in Milli
    seconds.
      @remark
    The double operations are limited to the compile time. No such operation is found in
    the machine code. */
#define TIC(tiInMs) ((uint8_t)((double)(tiInMs)/RTOS_TIC_MS+0.5))


/*
 * Local type definitions
 */

/** The task indexes. */
enum { idxTaskT0_C0 = 0
     , idxTaskT0_C1
     , idxTaskT0_C2
     , noTasks
     };
     

/*
 * Local prototypes
 */

static void taskT0_C0(uint16_t postedEventVec);
static void taskT0_C1(uint16_t postedEventVec);
static void taskT0_C2(uint16_t postedEventVec);


/*
 * Data definitions
 */

static uint8_t _stackT0_C0[STACK_SIZE]
             , _stackT0_C1[STACK_SIZE]
             , _stackT0_C2[STACK_SIZE];

/** Task owned variables which record what happens. */
static volatile uint32_t _cntLoopsT0_C0 = 0
                       , _cntLoopsT0_C1 = 0
                       , _cntLoopsT0_C2 = 0;

/** The application interrupt handler counts missing interrupt events (timeouts) as errors. */
static volatile uint16_t _errT0_C2 = 0;

/** Input for the blink-task: If it is triggered, it'll read this varaible and produce a
    sequence of flashes of according length. */
static volatile uint8_t _blink_noFlashes = 0;
   

/*
 * Function implementation
 */


/**
 * Trivial routine that flashes the LED a number of times to give simple feedback. The
 * routine is none blocking. It must not be called by the idle task as it uses a suspend
 * command.\n
 *   The number of times the LED is lit is read by side effect from the global variable \a
 * _blink_noFlashes. Writing this variable doesn't require access synchronization as this
 * function is called solely from the task of lowest priority.\n
 *   The flash sequence is started by setting 
 */

static void blinkNoBlock(uint8_t noFlashes)
{
#define TI_FLASH 150 /* ms */

    while(noFlashes-- > 0)
    {
        digitalWrite(LED, HIGH);    /* Turn the LED on. (HIGH is the voltage level.) */
        rtos_delay(TIC(TI_FLASH));  /* The flash time. */
        digitalWrite(LED, LOW);     /* Turn the LED off by making the voltage LOW. */
        rtos_delay(TIC(TI_FLASH));  /* Time between flashes. */
    }    
    
    /* Wait for a second after the last flash - this command could easily be invoked
       immediately again and the bursts need to be separated. */
    delay(TIC(1000-TI_FLASH));
    
#undef TI_FLASH
}




/**
 * The task of lowest priority (besides idle) is used for reporting. When released by an
 * event it produces a sequence of flash events of the LED. The number of flashes is
 * determined by the value of the global variable \a _blink_noFlashes. The sequence is
 * released by RTuinOS event \a EVT_START_FLASH_SEQUENCE.
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.\n
 *   The number of times the LED is lit is read by side effect from the global variable \a
 * _blink_noFlashes.
 */

static void taskT0_C0(uint16_t initCondition)
{
    ASSERT(initCondition == EVT_START_FLASH_SEQUENCE);
    do
    {
        /* The access to the shared variable is not protected: The variable is an uint8_t
           and a read operation is atomic anyway. */
        blinkNoBlock(_blink_noFlashes);
    }
    while(rtos_waitForEvent(EVT_START_FLASH_SEQUENCE, /* all */ false, /* timeout */ 0));
    
} /* End of taskT0_C0 */




/**
 * A task of medium priority. It looks at the counter incremented by the interrupt handler
 * and reports when it reaches a certain limit. Reporting is done by releasing the blinking
 * task. 
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */
 
static void taskT0_C1(uint16_t initCondition)
{
#define TASK_TIME_T0_C1_MS  50 
#define TRIGGER_DISTANCE    500

    /* Since we are the only client of the blink task we can abuse the interface variable as
       static counter at the same time. The first sequence shall have a single flash. */
    _blink_noFlashes = 0;
    
    /* The task inspects the results of the interrupt on a regular base. */
    do
    {
        static uint32_t lastTrigger = 500;
        
        cli();//rtos_enterCriticalSection();
        bool trigger = _cntLoopsT0_C2 >= lastTrigger;
        sei();//rtos_leaveCriticalSection();

        if(trigger)
        {
            /* Next reported event is reached. Start the flashing task. The number of times
               the LED is lit is exchanged by side effect in the global variable
               _blink_noFlashes. Writing this variable doesn't basically require access
               synchronization as this task has a higher priority than the blink task and
               because it's a simple uint8_t. However, we are anyway inside a critical
               section. */
               
            /* Limit the length of the sequence to a still recognizable value.
                 A read-modify-write on the shared variable outside a critical section can
               solely be done since we are the only writing task. */
            if(_blink_noFlashes < 10)
                ++ _blink_noFlashes;
                
            /* TRigger the other task. As it has the lower priority, it's actually not
               activated before we suspend a little bit later. */
            rtos_setEvent(EVT_START_FLASH_SEQUENCE);
            
            /* Set next trigger point. If we are too slow, it may run away. */
            lastTrigger += TRIGGER_DISTANCE;
        }
    }
    while(rtos_suspendTaskTillTime(TIC(TASK_TIME_T0_C1_MS)));

#undef TASK_TIME_T0_C1_MS
} /* End of taskT0_C1 */




/**
 * A task of high priority is associated with the application interrupts. It counts its
 * occurances and when it is missing (timeout).
 *   @param initCondition
 * Which events made the task run the very first time?
 *   @remark
 * A task function must never return; this would cause a reset.
 */
 
static void taskT0_C2(uint16_t initCondition)
{
#define TIMEOUT_MS  100

    /* This task just reports the application interrupt by incrementing a global counter. */
    while(true)
    {
        while(rtos_waitForEvent( RTOS_EVT_ISR_USER_00 | RTOS_EVT_DELAY_TIMER
                               , /* all */ false
                               , /* timeout */ TIC(TIMEOUT_MS)
                               )
              == RTOS_EVT_ISR_USER_00
             )
        {
            /* Normal situation: Application interrupt came before timeout. No access
               synchronization is required as this task has the highest priority of all
               data accessors. */
            ++ _cntLoopsT0_C2;
        }
    
        /* Inner loop left because of timeout. This may happen only at system
           initialization, because the application interrupts are always enabled a bit
           later than the RTuinOS system timer intterrupt.
              No access synchronization is required as this task has the highest priority
           of all data accessors. */
        if(_errT0_C2 < (uint16_t)-1)
            ++ _errT0_C2;

        /* Outer while condition: No true error recovery, just wait for next application
           interrupt. */
    }
#undef TIMEOUT_MS
} /* End of taskT0_C2 */




/**
 * Callback from RTuinOS: The application interrupt is configured and released.
 */

void rtos_enableIRQUser00()
{
    /* Timer 5 is reconfigured. Arduino has put it into 8 Bit fast PWM mode. We need the
       phase and frequency correct mode, in which the frequency can be controlled by
       register OCA. This register is buffered, the CPU writes into the buffer only. After
       each period, the buffered value is read and determines the period duration of the
       next period. The period time (and thus the frequency) can be varied in a well
       defined and glitch free manner. The settings are:
         WGM5 = %1001, the mentioned operation mode is selected. The 4 Bit word is partly
       found in TCCR5A and partly in TCCR5B.
         COM5A/B/C: don't change. The three 2 Bit words determine how to derive up to
       three PWM output signals from the counter. We don't change the Arduino setting; no
       PWM wave form is generated. The three words are found as the most significant 6 Bit
       of register TCCR5A.
         OCR5A = 8192 Hz/f_irq, the frequency determining 16 Bit register. OCR5A must not
       be less than 3.
         CS5 = %101, the counter selects the CPU clock divided by 1024 as clock. This
       yields the lowest possible frequencies -- good make the operation visible using the
       LED. */
    TCCR5A &= ~0x03; /* Lower half word of WGM */
    TCCR5A |=  0x01;
    
    TCCR5B &= ~0x1f; /* Upper half word of WGM and CS */
    TCCR5B |=  0x15;
    
    /* We choose 82 as initial value, or f_irq = 100. */
    OCR5A = 82u;

    TIMSK5 |= 1;    /* Enable overflow interrupt. */

} /* End of rtos_enableIRQUser00 */





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

    uint8_t idxTask = 0;
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0_C0
                       , /* prioClass */        0
                       , /* pStackArea */       &_stackT0_C0[0]
                       , /* stackSize */        sizeof(_stackT0_C0)
                       , /* startEventMask */   EVT_START_FLASH_SEQUENCE
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0_C1
                       , /* prioClass */        1
                       , /* pStackArea */       &_stackT0_C1[0]
                       , /* stackSize */        sizeof(_stackT0_C1)
                       , /* startEventMask */   RTOS_EVT_DELAY_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );
    rtos_initializeTask( /* idxTask */          idxTask++
                       , /* taskFunction */     taskT0_C2
                       , /* prioClass */        2
                       , /* pStackArea */       &_stackT0_C2[0]
                       , /* stackSize */        sizeof(_stackT0_C2)
                       , /* startEventMask */   RTOS_EVT_DELAY_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );
    ASSERT(idxTask == RTOS_NO_TASKS  &&  noTasks == RTOS_NO_TASKS);

} /* End of setup */




/**
 * The application owned part of the idle task. This routine is repeatedly called whenever
 * there's some execution time left. It's interrupted by any other task when it becomes
 * due.
 *   @remark
 * Different to all other tasks, the idle task routine may and should terminate. This has
 * been designed in accordance with the meaning of the original Arduino loop function.
 */

void loop(void)
{
    /* Get a safe copy of the volatile global data. */
    cli();//rtos_enterCriticalSection();
    uint32_t noInt = _cntLoopsT0_C2;
    uint16_t noTimeout = _errT0_C2;
    sei();//rtos_leaveCriticalSection();
            
    Serial.print("No application interrupts: ");
    Serial.print(noInt);
    Serial.print(", timeouts: ");
    Serial.println(noTimeout);

    Serial.print("Stack reserve: ");
    Serial.print(rtos_getStackReserve(/* idxTask */ 0));
    Serial.print(", ");
    Serial.print(rtos_getStackReserve(/* idxTask */ 1));
    Serial.print(", ");
    Serial.println(rtos_getStackReserve(/* idxTask */ 2));

    Serial.print("Overrun T0_C1: "); 
    Serial.println(rtos_getTaskOverrunCounter(/* idxTask */ idxTaskT0_C1
                                             , /* doReset */ false
                                             )
                  );
} /* End of loop */




