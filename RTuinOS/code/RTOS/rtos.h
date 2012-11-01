#ifndef RTOS_INCLUDED
#define RTOS_INCLUDED
/**
 * @file rtos.h
 * Definition of global interface of module rtos.c
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
 */

/*
 * Include files
 */
#include "rtos.config.h"

/*
 * Defines
 */

/** Switch to make feature selecting defines readable. Here: Feature is enabled. */
#define RTOS_FEATURE_ON     1
/** Switch to make feature selecting defines readable. Here: Feature is disabled. */
#define RTOS_FEATURE_OFF    0

/* Some global, general purpose events and the two timer events. Used to specify the
   resume condition when suspending a task.
     Conditional definition: If the application defines an interrupt which triggers an
   event, the same event gets a deviating name. */
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_00       (0x0001u<<0)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_01       (0x0001u<<1)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_02       (0x0001u<<2)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_03       (0x0001u<<3)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_04       (0x0001u<<4)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_05       (0x0001u<<5)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_06       (0x0001u<<6)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_07       (0x0001u<<7)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_08       (0x0001u<<8)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_09       (0x0001u<<9)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_10       (0x0001u<<10)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_11       (0x0001u<<11)

/* The name of the next event depends on the configuration of RTuinOS. */
#if RTOS_USE_APPL_INTERRUPT_00 == RTOS_FEATURE_ON
/** This event is posted by the application defined ISR 00. */
# define RTOS_EVT_ISR_USER_00   (0x0001u<<12)
#else
/** General purpose event, posted explicitly by rtos_setEvent. */
# define RTOS_EVT_EVENT_12      (0x0001u<<12)
#endif

/* The name of the next event depends on the configuration of RTuinOS. */
#if RTOS_USE_APPL_INTERRUPT_01 == RTOS_FEATURE_ON
/** This event is posted by the application defined ISR 01. */
# define RTOS_EVT_ISR_USER_01   (0x0001u<<13)
#else
/** General purpose event, posted explicitly by rtos_setEvent. */
# define RTOS_EVT_EVENT_13      (0x0001u<<13)
#endif

/** Real time clock is elapsed for the task. */
#define RTOS_EVT_ABSOLUTE_TIMER (0x0001u<<14)
/** The relative-to-start clock is elapsed for the task */
#define RTOS_EVT_DELAY_TIMER    (0x0001u<<15)


/** The system timer frequency as floating point constant. The unit is Hz.\n
      The value is derived from  #RTOS_TIC, which is about 2 ms in the RTuinOS standard
    configuration. The macro is defined in the configuration file rtos.config.h as it might
    be subject to changes by the application. */
#define RTOS_TIC_FREQUENCY (1.0/(RTOS_TIC))

/** The scale factor between RTuinOS' system timer tic and Arduinos \a millis() as a
    floating point constant. Same as tic period in unit ms.
      The value is derived from  #RTOS_TIC, which is about 2 ms in the RTuinOS standard
    configuration. The macro is defined in the configuration file rtos.config.h as it might
    be subject to changes by the application. */
#define RTOS_TIC_MS ((RTOS_TIC)*1000.0)


/** Function prototype decoration which declares a function of RTuinOS just a default
    implementation of the required functionality. The application code can redefine the
    function and override the default implementation.\n
      We use this type decoration for the initialization of the system timer interrupt --
    an RTuinOS application may use any other interrupts source than the default
    TIMER2_OVF. */ 
#define RTOS_DEFAULT_FCT __attribute__((weak))
/** Function prototype decoration which ensures that a function is implemented without stack
    frame generating machine code. The machine code starts with the implementation of the
    visible code lines of the function body. If used, one has to ensure, that the function
    body does not require a stack frame or has to set it up himself by some introductory
    inline assembly code.\n
      We use this type decoration for all software interrupts (all API functions which can
    cause a task switch). */
#define RTOS_NAKED_FCT __attribute__((naked, noinline))
/** Function prototype decoration which ensures that a function is generated by the compiler
    as such, but neither inline nor removed from the code.\n
      We use this type decoration for all functions called from a software interrupt. If such
    a function was inlined it could cause the (hazardous) need for a stack frame in the
    calling function which implements the software interrupt. */
#define RTOS_TRUE_FCT __attribute__((used, noinline)) 


/** Delay a task without looking at other events. rtos_delay(delayTime) is
    identical to rtos_waitForEvent(RTOS_EVT_DELAY_TIMER, false, delayTime), i.e.
    eventMask's only set bit is the delay timer event.\n
      delayTime: The duration of the delay in the unit of the system time. The permitted
    range is 0..max(uintTime_t).\n
      CAUTION: This method is one of the task suspend commands. It must not be used by the
    idle task, which can't be suspended. A crash would be the immediate consequence. */
#define rtos_delay(delayTime)                                               \
                rtos_waitForEvent(RTOS_EVT_DELAY_TIMER, false, delayTime)


/**
 * Suspend the current task (i.e. the one which invokes this method) until a specified
 * point in time.\n
 *   Although specified as a increment in time, the time is meant absolute. The meant time
 * is the time specified at the last recent call of this function by this task plus the now
 * specified increment. This way of specifying the desired time of resume supports the
 * intended use case, which is the implementation of regular real time tasks: A task will
 * suspend itself with a constant time value at the end of the infinite loop which contains
 * its functional code. This (fixed) time value becomes the sample time of the task. This
 * behavior is opposed to a delay or sleep function: The execution time of the task is no
 * time which additionally elapses between two task resumes.\n
 *   The idle task can't be suspended. If it calls this function a crash would be the
 * immediate result.
 *   @return
 * The event mask of resuming events is returned. Since no combination with other events
 * than the elapsed system time is possible, this will always be RTOS_EVT_ABSOLUTE_TIMER.
 *   @param deltaTimeTillRelease
 * \a deltaTimeTillRelease specifies a time in the future at which the task will become due
 * again. To support the most relevant use case of this function, the implementation of
 * regular real time tasks, the time designation is relative. It refers to the last recent
 * absolute time at which this task had been resumed. This time is defined by the last
 * recent call of either this function or rtos_waitForEvent. In the very first call of
 * the function it refers to the point in time the task was started.
 *   @see rtos_waitForEvent
 *   @remark
 * This function actually is a macro calling rtos_waitForEvent using fixed parameters.
 */
#define rtos_suspendTaskTillTime(/* uintTime_t */ deltaTimeTillRelease)     \
    rtos_waitForEvent( /* eventMask */ RTOS_EVT_ABSOLUTE_TIMER              \
                     , /* all */       false                                \
                     , /* timeout */   deltaTimeTillRelease                 \
                     )
                


/*
 * Global type definitions
 */

/** The type of any task.\n
      The function is of type void; it must never return.\n
      The function takes a single parameter. It is the event vector of the very event
    combination which made the task initially run. Typically this is just the delay timer
    event. */
typedef void (*rtos_taskFunction_t)(uint16_t postedEventVec);


/*
 * Global data declarations
 */


/*
 * Global prototypes
 */

/* Initialze all application parameters of one task. To be called for each of the tasks in
   setup(). */
void rtos_initializeTask( uint8_t idxTask
                        , rtos_taskFunction_t taskFunction
                        , uint8_t prioClass
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
                        , uintTime_t timeRoundRobin
#endif
                        , uint8_t * const pStackArea
                        , uint16_t stackSize
                        , uint16_t startEventMask
                        , bool startByAllEvents
                        , uintTime_t startTimeout
                        );

/** Configure and enable the interrupt which clocks the system time of RTuinOS. This
    function has a default implementation, the application may but need not to implement
    it.\n
      If the application decides to set up its specific system timer interrupt, it'll
    probably have to alter also the interrupt vector name, see
    #RTOS_ISR_SYSTEM_TIMER_TIC.\n
      If the application decides to set up its specific system timer interrupt, it'll
    probably have to state the new system clock frequency, see #RTOS_TIC. */
void rtos_enableIRQTimerTic(void);

#if RTOS_USE_APPL_INTERRUPT_00 == RTOS_FEATURE_ON
/** An application supplied callback, which contains the code to set up the hardware to
    generate application interrupt 0. */
extern void rtos_enableIRQUser00(void);
#endif

#if RTOS_USE_APPL_INTERRUPT_01 == RTOS_FEATURE_ON
/** An application supplied callback, which contains the code to set up the hardware to
    generate application interrupt 1. */
extern void rtos_enableIRQUser01(void);
#endif

/* Initialization of the internal data structures of RTuinOS and start of the timer
   interrupt (see void rtos_enableIRQTimerTic(void)). This function does not return but
   forks into the configured tasks.
     This function is not called by the application (but only from main()). */
void rtos_initRTOS(void);

/* Post a set of events to all suspended tasks. Suspend the current task if the events
   release another task of higher priority. */
volatile void rtos_setEvent(uint16_t eventVec);

/* Suspend task until a combination of events appears or a timeout elapses. */
volatile uint16_t rtos_waitForEvent(uint16_t eventMask, bool all, uintTime_t timeout);

/* How often could a real time task not be reactivated timely? */
uint8_t rtos_getTaskOverrunCounter(uint8_t idxTask, bool doReset);

/* How many bytes of the stack of a task are still unsed? */
uint16_t rtos_getStackReserve(uint8_t idxTask);

#endif  /* RTOS_INCLUDED */
