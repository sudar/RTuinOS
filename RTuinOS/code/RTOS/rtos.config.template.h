#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos.config.h
 * Switches to define the most relevant compile-time settings of RTuinoOS in an application
 * specific way.
 * @todo Copy this file to your application code, rename it to rtos.config.h and adjust the
 * settings to the need of your RTuinoOS application. Then remove this hint.
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


/*
 * Defines
 */

/** Does the task scheduling concept support time slices of limited length for activated
    tasks? If on, the overhead of the scheduler slightly increases.\n
      Select either RTOS_FEATURE_OFF or RTOS_FEATURE_ON. */
#define RTOS_ROUND_ROBIN_MODE_SUPPORTED     RTOS_FEATURE_OFF


/** Number of tasks in the system. Tasks aren't created dynamically. This number of tasks
    will always be existent and alive. Permitted range is 0..255.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_NO_TASKS    5


/** Number of distinct priorities of tasks. Since several tasks may share the same
    priority, this number is lower or equal to NO_TASKS. Permitted range is 0..NO_TASKS,
    but 1..NO_TASKS if at least one task is defined.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_NO_PRIO_CLASSES 3


/** Since many tasks will belong to distinct priority classes, the maximum number of tasks
    belonging to the same class will be significantly lower than the number of tasks. This
    setting is used to reduce the required memory size for the statically allocated data
    structures. Set the value as low as possible. Permitted range is min(1, NO_TASKS)..255,
    but a value greater than NO_TASKS is not reasonable.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_MAX_NO_TASKS_IN_PRIO_CLASS 2


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



/*
 * Global data declarations
 */


/*
 * Global prototypes
 */



#endif  /* RTOS_CONFIG_INCLUDED */
