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
#define RTOS_EVT_EVENT_00       (0x0001<<0)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_01       (0x0001<<1)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_02       (0x0001<<2)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_03       (0x0001<<3)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_04       (0x0001<<4)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_05       (0x0001<<5)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_06       (0x0001<<6)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_07       (0x0001<<7)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_08       (0x0001<<8)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_09       (0x0001<<9)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_10       (0x0001<<10)
/** General purpose event, posted explicitly by rtos_setEvent. */
#define RTOS_EVT_EVENT_11       (0x0001<<11)

/* The name of the next event depends on the configuration of RTuinOS. */
#if RTOS_USE_APPL_INTERRUPT_00 == RTOS_FEATURE_ON
/** This event is posted by the application defined ISR 00. */
# define RTOS_EVT_ISR_USER_00   (0x0001<<12)
#else
/** General purpose event, posted explicitly by rtos_setEvent. */
# define RTOS_EVT_EVENT_12      (0x0001<<12)
#endif

/* The name of the next event depends on the configuration of RTuinOS. */
#if RTOS_USE_APPL_INTERRUPT_01 == RTOS_FEATURE_ON
/** This event is posted by the application defined ISR 01. */
# define RTOS_EVT_ISR_USER_01   (0x0001<<13)
#else
/** General purpose event, posted explicitly by rtos_setEvent. */
# define RTOS_EVT_EVENT_13      (0x0001<<13)
#endif

/** Real time clock is elapsed for the task. */
#define RTOS_EVT_ABSOLUTE_TIMER (0x0001<<14)
/** The relative-to-start clock is elapsed for the task */
#define RTOS_EVT_DELAY_TIMER    (0x0001<<15)


/** Delay a task without looking at other events. rtos_delay(delayTime) is
    identical to rtos_waitForEvent(RTOS_EVT_DELAY_TIMER, false, delayTime), i.e.
    eventMask's only set bit is the delay timer event.\n
      delayTime: The duration of the delay in the unit of the system time. The permitted
    range is 0..max(uintTime_t).\n
      CAUTION: This method is one of the task suspend commands. It must not be used by the
    idle task, which can't be suspended. A crash would be the immediate consequence. */
#define rtos_delay(delayTime)                                               \
                rtos_waitForEvent(RTOS_EVT_DELAY_TIMER, false, delayTime)


/*
 * Global type definitions
 */

/** The type of any task.\n
      The function is of type void; it must never return.\n
      The function takes a single parameter. It is the event vector of the very event
    combination which made the task initially run. Typically this is just an absolute or
    delay timer event. */
typedef void (*rtos_taskFunction_t)(uint16_t postedEventVec);

/** The descriptor of any task. Contains static information like task priority class and
    dynamic information like received events, timer values etc.\n
      The application will fill an array of objects of this type. Some of the fields are
    initialized by the application and only read by the RTOS code. These fields are
    documented accordingly. All other fields can be prefilled by the application with dummy
    values (e.g. 0) -- the RTOS initialization will overwrite the dummy values with those
    values it needs for operation.\n
      After initailization the application must never touch any of the fields in the task
    objects -- the chance to cause a crash is very close to one. */
typedef struct
{
    /** The priority class this task belongs to. Priority class 255 has the highest
        possible priority and the lower the value the lower the priority.\n
          This settings has to be preset by the application at compile time or in function
        \a setup. After initialization it must never be touched any more, any change will
        result in a crash. */
    uint8_t prioClass;
    
    /** The task function as a function pointer. It is used once and only once: The task
        function is invoked the first time the task becomes active and must never end. A
        return statement would cause an immediate reset of the controller. */
    rtos_taskFunction_t taskFunction;
    
    /** The timer value triggering the task local absolute-timer event.\n
          This settings has to be preset by the application at compile time or in function
        \a setup. After initialization it must never be touched any more, any change will
        result in a crash.\n
          The initial value determines at which system timer tic the task becomes due the
        very first time. This may always by 0 (task becomes due immediately). In the use
        case of regular tasks it may however pay off to distribute the tasks on the time
        grid in order to avoid too many due tasks regularly at specific points in time. See
        documentation for more. */
    uintTime_t timeDueAt;
    
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
    /** The maximum time a task may be activated if it is operated in round-robin mode. The
        range is 1..max_value(uintTime_t).\n
          Specify a maximum time of 0 to switch round robin mode off for this task.\n
          Remark: Round robin like behavior is given only if there are several tasks in the
        same priority class and all tasks of this class have the round-robin mode
        activated. Otherwise it's just the limitation of execution time for an individual
        task.\n
          This settings has to be preset by the application at compile time or in function
        \a setup. After initialization it must never be touched any more, a change could
        result in a crash. */
    uintTime_t timeRoundRobin;
#endif

    /** The pointer to the preallocated stack area of the task. The area needs to be
        available all the RTOS runtime. Therfore dynamic allocation won't pay off. Consider
        to use the address of any statically defined array. There's no alignment
        constraint.\n
          This settings has to be preset by the application at compile time or in function
        \a setup. After initialization it must never be touched any more, a change could
        result in a crash. */
    uint8_t *pStackArea;
    
    /** The size in Byte of the memory area \a *pStackArea, which is reserved as stack for
        the task. Each task may have an individual stack size.\n
          This settings has to be preset by the application at compile time or in function
        \a setup. After initialization it must never be touched any more, a change could
        result in a crash. */
    uint16_t stackSize;
    
    /*
     * Internal fields start here. The application provided initialization values don't
     * matter.
     */
    /** @todo This part of the struct could be hidden in an anonymous type. Two arrays, one
        local to RTOS.c */

    /** The timer tic decremented counter triggering the task local delay-timer event. */
    uintTime_t cntDelay;
    
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
    /** The timer tic decremented counter triggering a task switch in round-robin mode. */
    uintTime_t cntRoundRobin;
#endif

    /** The events posted to this task. */
    uint16_t postedEventVec;
    
    /** The mask of events which will make this task due. */
    uint16_t eventMask;
    
    /** Do we need to wait for the first posted event or for all events? */
    bool waitForAnyEvent;

    /** The saved stack pointer of this task whenever it is not active. */
    uint16_t stackPointer;
    
    /** All recognized overruns of the timing of this task are recorded in this variable.
        The access to this variable is considered atomic by the implementation, therefore
        no other type than 8 Bit must be used.\n
          Task overruns are defined only in the (typical) use case of regular real time
        tasks. In all other applications of a task this value is useless and undefined.\n
          @remark Even for regular real time tasks, overruns can only be recognized with a
        certain probablity, which depends on the range of the cyclic system timer. Find a
        discussion in the documentation of type uintTime_t. */
    uint8_t cntOverrun;
    
    //uint8_t fillToPowerOfTwoSize[32-18];
    
} rtos_task_t;


/*
 * Global data declarations
 */

/** Array of all the task objects. It is declared extern here and owned by the application
    in order to enable compile-time definition of all contains task objects. The array has
    one additional element to store the information about the implictly defined idle
    task.\n
      The application must not specify any field of the idle element in the array, which is
    the last one. Just set all fields to null. */
extern rtos_task_t rtos_taskAry[RTOS_NO_TASKS+1];


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

/* Suspend a task untill a specified point in time. Used to implement regular real time
   tasks. */
volatile uint16_t rtos_suspendTaskTillTime(uintTime_t deltaTimeTillRelease);

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
