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

/** This event is used as start condition for task T0_C0. */
#define EVT_START_TASK_T0_C0 (RTOS_EVT_EVENT_00)

/** This event is used as start condition for task T1_C0. */
#define EVT_START_TASK_T1_C0 (RTOS_EVT_EVENT_01)

/** This event is used as start condition for task T2_C0. */
#define EVT_START_TASK_T2_C0 (RTOS_EVT_EVENT_02)

/** This event signals that the resource has been released. */
#define EVT_RESOURCE_IS_AVAILABLE (RTOS_EVT_EVENT_03)


/* TYPE DEFINITIONS -------------------------------------------------------------------- */
/* DATA DECLARATIONS ------------------------------------------------------------------- */
/* PROTOTYPES -------------------------------------------------------------------------- */



#endif  /* TC08_APPLEVENTS_INCLUDED */
