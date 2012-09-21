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

/** Switches to make feature selecting #defines readable. */
#define RTOS_FEATURE_ON     1
#define RTOS_FEATURE_OFF    0

/** Some global, general purpose events and the two timer events. Used to specify the
    resume condition when suspending a task. */
#define RTOS_EVT_EVENT_00       (0x0001<<00)
#define RTOS_EVT_EVENT_01       (0x0001<<01)
#define RTOS_EVT_EVENT_02       (0x0001<<02)
#define RTOS_EVT_EVENT_03       (0x0001<<03)
#define RTOS_EVT_EVENT_04       (0x0001<<04)
#define RTOS_EVT_EVENT_05       (0x0001<<05)
#define RTOS_EVT_EVENT_06       (0x0001<<06)
#define RTOS_EVT_EVENT_07       (0x0001<<07)
#define RTOS_EVT_EVENT_08       (0x0001<<08)
#define RTOS_EVT_EVENT_09       (0x0001<<09)
#define RTOS_EVT_EVENT_10       (0x0001<<10)
#define RTOS_EVT_EVENT_11       (0x0001<<11)
#define RTOS_EVT_EVENT_12       (0x0001<<12)
#define RTOS_EVT_EVENT_13       (0x0001<<13)
#define RTOS_EVT_ABSOLUTE_TIMER (0x0001<<14)
#define RTOS_EVT_DELAY_TIMER    (0x0001<<15)

/** Select the interrupt which clocks the system time. Side effects to consider: This
    interrupt (like all others which possibly result in a context switch) need to be
    inhibited by rtos_enterCriticalSection. */
/** @todo Declare external function void rtos_enableIRQTimerTic(void) which
    initializes/releases the IRQ to be used to clock the system. If the IRQ can be chosen
    by means of a macro it makes no sense not to be able to implement its details.
    Internally, the function could be implemented as weak, it as as kind of default
    implementation. */
#define RTOS_ISR_SYSTEM_TIMER_TIC TIMER2_OVF_vect



/*
 * Global type definitions
 */

/** The type of the system time. The system time is a cyclic integer value. Most typical
    its unit is the period time of the fastest regular time if the use case of the RTOS is
    a traditional scheduling of regular tasks of different priorities. In general, the time
    doesn't need to be regular and its unit doesn't matter.\n
      You may define the time to be any unsigned integer considering following trade off:
    The shorter the type is the less the system overhead. Be aware that many operations in
    the core of the kernel are time based.\n
      The longer the type the larger the maximum ratio of period times of slowest and
    fastest task is. This maximum ratio is half the maxmum number. If you implement tasks
    of e.g. 10ms, 100ms and 1000ms, this could be handeld with a uint8. If you want to have
    an additional 1ms task, uint8 will no longer suffice, you need at least uint16. (uint32
    is probably never useful.)\n
      The longer the type the higher the resolution of timeout timers can be chosen when
    waiting for events. The resolution is the tic frequency of the system time. With an 8
    Bit system time one would probably choose a tic frequency identical to (or at least only
    higher by a small factor) the repetition time of the fastest task. Then, this task can
    only specify a timeout which ends at the next regular due time of the task. The
    statement made before needs refinement: The cylce time of the system time limits the
    ratio of the period time of the slowest task and the resolution of timeout
    specifications.\n
      The shorter the type the higher the probability of not recognizing task overruns when
    implementing the use case mentioned before: Due to the cyclic character of the time
    definition a time in the past is seen as a time in the future, if it is past more than
    half the maximum integer number. Example: Data type is uint8. A task is implemented as
    regular task of 100 units. Thus, at the end of the functional code it suspends itself with 
    time increment 100 units. Let's say it had been resumed at time 123. In normal
    operation, no task overrun it will end e.g. 87 tics later, i.e. at 210. The demanded
    resume time is 123+100 = 223, which is seen as +13 in the future. If the task execution
    was too long and ended e.g. after 110 tics, the system time was 123+110 = 233. The
    demanded resume time 223 is seen in the past and a task overrun is recognized. A
    problem appears at excessive task overruns. If the execution had e.g. taken 230 tics
    the current time is 123 + 230 = 353 = 97, due to its cyclic character. The demanded
    resume time 223 is 126 tics ahead, which is considered a future time -- no task overrun
    is recognized. The problem appears if the overrun lasts more than half the system time
    cycle. With uint16 this problem becomes negligible.\n
      This typedef doen't have the meaning of hiding the type. In contrary, the character
    of the time being a simple unsigned integer should be visible to the user. The meaning
    of the typedef only is to have an implementation with user-selectable number of bits
    for the integer. Therefore we choose a name similar to the common integer types. */
typedef uint8_t uintTime_t;


/** The type of any task.\n
      The function is of type void; it must never return.\n
      The function takes a single parameter. It can be a simple integer or a typecasted
    pointer into the memory address space and therefore convey any kind of information to
    the task function. */
typedef void (*rtos_taskFunction_t)(uint16_t taskParam);

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
    
    /** The parameter of the task function. */
    uint16_t taskFunctionParam;
    
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

void rtos_initRTOS(void);


#endif  /* RTOS_INCLUDED */
