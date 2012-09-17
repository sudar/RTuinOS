#ifndef RTOS_INCLUDED
#define RTOS_INCLUDED
/* ------------------------------------------------------------------------------------- */
/**
 * @file        rtos.h
 *
 *              Interface of module rtos.\n
 *                See module description of implementation file
 *              rtos.c for more details.
 */
/*              Copyright (c) 2012 FEV GmbH, Germany.
 *
 *              All rights reserved. Reproduction in whole or in part is
 *              prohibited without the written consent of the copyright
 *              owner.
 */
/* ------------------------------------------------------------------------------------- */

/* INCLUDES ---------------------------------------------------------------------------- */
/* DEFINES ----------------------------------------------------------------------------- */

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


/* TYPE DEFINITIONS -------------------------------------------------------------------- */

/** The type of the system time. The system time is a cyclic integer value. Most typical
    its unit is the period time of the fastest regular time if the use case of the RTOS is
    a traditional scheduling of regular tasks of different priorities. In general, the time
    doesn't need to be regular and its unit doesn't matter.\n
      You may define the time to be any unsigned integer considering following trade off:
    The shorter the type is the less the system overhead. Be aware that many operations in
    the core of the kernel are time based.\n
      The longer the type the larger the maximum ratio of period times of fastest and
    slowest task is. This maximum ratio is half the maxmum number. If you implement tasks
    of e.g. 10ms, 100ms and 1000ms, this could be handeld with a uint8. If you want to have
    an additional 1ms task, uint8 will no longer suffice, you need at least uint16. (uint32
    is probably never useful.)\n
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
typedef uintTime_t uint8;

/* DATA DECLARATIONS ------------------------------------------------------------------- */
/* PROTOTYPES -------------------------------------------------------------------------- */



#endif  /* RTOS_INCLUDED */
