#ifndef TC08_APPLEVENTS_INCLUDED
#define TC08_APPLEVENTS_INCLUDED
/* ------------------------------------------------------------------------------------- */
/**
 * @file        tc08_applEvents.h
 *
 *              Definition of application events. The application events are managed in a
 *              central file to avoid inconistencies and accidental double usage.
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

/** The resource is owned by the task, which received this event of kind mutex. */
#define EVT_MUTEX_OWNING_RESOURCE (RTOS_EVT_MUTEX_00)

/** This event is used as start condition for task T0_C0. */
#define EVT_START_TASK_T0_C0 (RTOS_EVT_EVENT_01)

/** This event is used as start condition for task T1_C0. */
#define EVT_START_TASK_T1_C0 (RTOS_EVT_EVENT_02)

/** This event is used as start condition for task T2_C0. */
#define EVT_START_TASK_T2_C0 (RTOS_EVT_EVENT_03)



/* TYPE DEFINITIONS -------------------------------------------------------------------- */
/* DATA DECLARATIONS ------------------------------------------------------------------- */
/* PROTOTYPES -------------------------------------------------------------------------- */



#endif  /* TC08_APPLEVENTS_INCLUDED */
