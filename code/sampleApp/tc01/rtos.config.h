#ifndef RTOS_CONFIG_INCLUDED
#define RTOS_CONFIG_INCLUDED
/**
 * @file rtos.config.h
 * Switches to define the most relevant compile-time settings of RTuinoOS in an application
 * specific way.
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
#define RTOS_NO_TASKS    0


/** Number of distinct priorities of tasks. Since several tasks may share the same
    priority, this number is lower or equal to NO_TASKS. Permitted range is 0..NO_TASKS,
    but 1..NO_TASKS if at least one task is defined.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_NO_PRIO_CLASSES 1


/** Since many tasks will belong to distinct priority classes, the maximum number of tasks
    belonging to the same class will be significantly lower than the number of tasks. This
    setting is used to reduce the required memory size for the statically allocated data
    structures. Set the value as low as possible. Permitted range is min(1, NO_TASKS)..255,
    but a value greater than NO_TASKS is not reasonable.\n
      A runtime check is not done. The code will crash in case of a bad setting. */
#define RTOS_MAX_NO_TASKS_IN_PRIO_CLASS 1


/** Select the interrupt which clocks the system time. Side effects to consider: This
    interrupt (like all others which possibly result in a context switch) need to be
    inhibited by rtos_enterCriticalSection. */
/** @todo Declare external function void rtos_enableIRQTimerTic(void) which
    initializes/releases the IRQ to be used to clock the system. If the IRQ can be chosen
    by means of a macro it makes no sense not to be able to implement its details.
    Internally, the function could be implemented as weak, it as as kind of default
    implementation. */
#define RTOS_ISR_SYSTEM_TIMER_TIC TIMER2_OVF_vect


/** Enable the application defined interrupt 0. (Two such interrupts are pre-configured in
    the code and more can be implemented by taking these two as a code template.)\n
      To install an application interrupt, this define is set to #RTOS_FEATURE_ON.\n
      Secondary, you will define #RTOS_ISR_USER_00 in order to specify the interrupt
    source.\n
      Then, you will implement the callback \a rtos_enableIRQUser00(void) which enables the
    interrupt, typically by accessing the interrupt control register of some peripheral.\n
      Now the interrupt is enabled and if it occurs it'll post the event
    #RTOS_EVT_ISR_USER_00. You will probably have a task of high priority which is waiting
    for this event in order to handle the interrupt when it is resumed by the event. */
#define RTOS_USE_APPL_INTERRUPT_00 RTOS_FEATURE_OFF

/** The name of the interrupt vector which is assigned to application interrupt 0. The
    supported vector names can be derived from table 14-1 on page 105 in the CPU manual,
    doc2549.pdf (see http://www.atmel.com). */
#define RTOS_ISR_USER_00    xxx_vect


/** Enable the application defined interrupt 1. See #RTOS_USE_APPL_INTERRUPT_00 for
    details. */
#define RTOS_USE_APPL_INTERRUPT_01 RTOS_FEATURE_OFF

/** The name of the interrupt vector which is assigned to application interrupt 1. See
    #RTOS_ISR_USER_00 for details. */
#define RTOS_ISR_USER_01    xxx_vect


/** A macro which expands to the code which defines all types which are related to the
    system timer. We need the unsigned and the related signed type. The macro is applied
    just once, see below. */
#define RTOS_DEFINE_TYPE_OF_SYSTEM_TIME(noBits)     \
    typedef uint##noBits##_t uintTime_t;            \
    typedef int##noBits##_t intTime_t;                                  


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
      This typedef doesn't have the meaning of hiding the type. In contrary, the character
    of the time being a simple unsigned integer should be visible to the user. The meaning
    of the typedef only is to have an implementation with user-selectable number of bits
    for the integer. Therefore we choose its name \a uintTime_t similar to the common integer
    types.\n
      The argument of the macro, which actually makes the required typedefs is set to
    either 8, 16 or 32; the meaning is number of bits. */
RTOS_DEFINE_TYPE_OF_SYSTEM_TIME(8)



/*
 * Global data declarations
 */


/*
 * Global prototypes
 */



#endif  /* RTOS_CONFIG_INCLUDED */
