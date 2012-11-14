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

/** This event is used to trigger the reporting task to start a sequence of LED flashes. If
    the event is set while a prceeding sequence has not yet finished it is ignored. There
    are no overlapping or restarted sequences. */
#define EVT_START_FLASH_SEQUENCE (RTOS_EVT_EVENT_00)


/* TYPE DEFINITIONS -------------------------------------------------------------------- */
/* DATA DECLARATIONS ------------------------------------------------------------------- */
/* PROTOTYPES -------------------------------------------------------------------------- */



#endif  /* TC08_APPLEVENTS_INCLUDED */
