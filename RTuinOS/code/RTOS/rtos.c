/* ------------------------------------------------------------------------------------- */
/**
 * @file        rtos.c
 *
 *              A small but effective real time kernel for Arduino Mega 2560
 *
 *              For more information also see the module description of interface file
 *              rtos.h.
 */
/*              Copyright (c) 2012 Peter Vranken, Germany.
 *
 *              All rights reserved. Reproduction in whole or in part is
 *              prohibited without the written consent of the copyright
 *              owner.
 *
 * Functions
 *              rtos_incSystemTime
 *              onTimerTic
 */
/* ------------------------------------------------------------------------------------- */


/* INCLUDES----------------------------------------------------------------------------- */

#include "rtos.h"


/* DEFINES ----------------------------------------------------------------------------- */

/** Number of tasks in the system. Tasks aren't created dynamically. This number of tasks
    will always be existent and alive. Permitted range is 1..255. */
#define NO_TASKS    5

/** Number of distinct priorities of tasks. Since several tasks may share the same
    priority, this number is lower or equal to NO_TASKS. Permitted range is 1..255. */
#define NO_PRIO_CLASSES 3

/** Since many tasks will belong to distinct priority classes, the maximum number of tasks
    belonging to the same class will be significantly lower than the number of tasks. This
    setting is used to reduce the required memory size for the statically allocated data
    structures. Set the value as low as possible. Permitted range is 1..255, but a value
    greater than NO_TASKS is not reasonable.
      A runtime check is however not done. The code will crash in case of a bad setting. */
#define MAX_NO_TASKS_IN_PRIO_CLASS 2

/** The lists of tasks IDs are closed by an entry which does not refer to any task. */
#define INVALID_TASK_ID 0xff

/** The task ID of the invisible idle task. */
#define IDLE_TASK_ID    (NO_TASKS)


/* TYPE DEFINITIONS -------------------------------------------------------------------- */

/** The descriptor of any task. Contains static information like task priority class and
    dynamic information like received events, timer values etc. */
typedef struct
{
    /** The priority class this task belongs to. Priority class 0 has the highest priority
        and the higher the value the lower the priority. */
    uint8_t prioClass;
    
    /** The timer value triggering the task local absolute-timer event. */
    uint8_t timeDueAt;
    
    /** The timer tic decremented counter triggering the task local delay-timer event. */
    uint8_t cntDelay;
    
    /** The events posted to this task. */
    uint16_t postedEventVec;
    
    /** The mask of events which will make this task due. */
    uint16_t eventMask;
    
    /** Do we need to wait for the first posted event or for all events? */
    bool_t waitForAnyEvent;
    
} task_t;


/* PROTOTYPES -------------------------------------------------------------------------- */
/* DATA DEFINITIONS -------------------------------------------------------------------- */

/** The system time. A cyclic counter of the timer tics. The counter is interrupt driven.
    The unit of the time is defined only by the it triggering source and doesn't matter at
    all for the kernel. The time even don't need to be regular. */
static uint8_t _time;

/** The one and only active task. This may be the only internally seen idle task which does
    nothing. */
static uint8_t _activeTaskId;

/** Array holding all due (but not active) tasks. Ordered according to their priority
    class. */
static uint8_t _dueTaskIdAryAry[NO_PRIO_CLASSES][MAX_NO_TASKS_IN_PRIO_CLASS];

/** Number of due tasks in the different priority classes. */
static uint8_t _noDueTasksAry[NO_PRIO_CLASSES];

/** Array holding all currently suspended tasks. */
static uint8_t _suspendedTaskIdAry[NO_TASKS];

/** Number of currently suspended tasks. */
static uint8_t _noSuspendedTasks;

/** Array of all the task objects. */
static task_t _taskAry[NO_TASKS];


/* CODE -------------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------------------- */
/**
 * Each call of this function cyclically increments the system time of the kernel by one.
 * The cycle time of the system time is low (currently implemented as 0..255) and
 * determines the maximum delay time or timeout for a task which suspends itself and the
 * ration of task periods of the fastest and the slowest regular task. Furthermore it
 * determines the reliability of task overrun recognition. Task overrun events in the
 * magnitude of half the cycle time won't be recognized as such.
 *   The unit of the time is defined only by the it triggering source and doesn't matter at
 * all for the kernel. The time even don't need to be regular.
 *   @see void_t onTimerTic(void_t)
 * Incrementing the system timer is an important system event at the same time. The routine
 * will always include an inspection of all suspended tasks, whether they could become due
 * again.
 *   @remark
 * The function needs to be called by an interrupt and can easily end with a context change,
 * i.e. the interrupt will return to another task as that it had interrupted.
 */
/* ------------------------------------------------------------------------------------- */

void_t rtos_incSystemTime(void_t)

{
    /* Save context to storage of active task. */    
    
    /* Check for all suspended tasks if this change in time is an event for them. */
    onTimerTic();
    
    /* Look for the task we will return to. It's the first entry in the highest priority
       class.
         Remark: Some optimizing code could be implemented in onTimerTic to shortcut this
       search: Only tasks becoming due there are candidates for the new active task,
       otherwise just return to the current task. However, in normal applications, any
       timer tic will typically activate the task of highest priority, so that the code
       here is already optimal: The very first test will match. */
    _activeTaskId != IDLE_TASK_ID;
    for(idxPrio=0; idxPrio<NO_PRIO_CLASSES; ++idxPrio)
    {
        if(_noDueTasksAry[idxPrio] > 0)
        {
            _activeTaskId = _dueTaskIdAryAry[idxPrio][0];
            break;
        }
    }
    
    /* Get context of newly activated task and return from interrupt (into that task). */
    @todo What about the return address of onTimerTic: Will it be on all the stacks? If so will we have to put it artificially onto the initial stacks when initializing all tasks?
    
} /* End of rtos_incSystemTime. */




/* ------------------------------------------------------------------------------------- */
/**
 * This function is called from the system interrupt triggered by the main clock. The
 * timers of all due tasks are served and - in case they elapse - timer events are
 * generated. These events may then release some of the tasks. If so, they are placed in
 * the appropriate list of due tasks. Finally, the longest due task in the highest none
 * empty priority class is activated.
 */
/* ------------------------------------------------------------------------------------- */

static void_t onTimerTic(void_t)
{
    uint8_t idxSuspTask
          , tId = _suspendedTaskAry[0];
          
    for(idxSuspTask=0; idxSuspTask<_noSuspendedTasks; ++idxSuspTask)
    {
        task_t *pT = &_taskAry[_suspendedTaskAry[idxSuspTask]];
        uint8_t eventVec;

        /* Check for absolute timer event. */
        if(_time == pT->timeDueAt)
        {
            /* Setting the absolute timer event when it already is set looks like a task
               overrun indication. It isn't for two reasons. First, by means of available
               API calls the absolute timer event can't be AND combined with other events,
               so the event will immediately change the status to due (see below), so that
               setting it a second time will never occur. Secondary, an AND combination is
               basically possible by the kernel and would work fine, and if it would be
               used setting the timer event here multiple times could be an obvious
               possible consequence, but not an indication of a task overrun - as it were
               the other event which blocks the task.
                 For these reasons, the code doesn't double check for multiply setting the
               same event. */
            pT->postedEventVec |= RTOS_EVT_ABSOLUTE_TIMER;
        }
            
        /* Check for delay timer event. The code here should optimally support the standard
           situation that the counter is constantly 0. */
        if(pT->cntDelay > 0)
        {
            if(-- pT->cntDelay == 0)
                pT->postedEventVec |= RTOS_EVT_DELAY_TIMER;
        }
            
        /* Check if the task becomes due because of the possibly occured timer events. The
           optimally supported case is the more probable OR combination of events. */
        eventVec = pT->postedEventVec & pT->eventMask;
        if( pT->waitForAnyEvent &&  eventVec != 0
            ||  !pT->waitForAnyEvent &&  eventVec == pT->eventMask
          )
        {           
            uint8_t u
                  , prio = pT->prioClass;
            
            /* This task became due. Move it from the list of suspended tasks to the list
               of due tasks of its priority class. */
            _dueTaskIdAryAry[prio][_noDueTasksAry[prio]++] = _suspendedTaskAry[idxSuspTask];
            -- _noSuspendedTasks;
            for(u=idxSuspTask; u<_noSuspendedTasks; ++u)
                _suspendedTaskAry[idxSuspTask] = _suspendedTaskAry[idxSuspTask+1];
        }
        
    } /* End for(All suspended tasks) */
    
} /* End of onTimerTic. */






/* ------------------------------------------------------------------------------------- */
/**
 * Suspend the current task (i.e. the one which invokes this method) until a specified
 * point in time.\n
 *   Although specified as a increment in time, the time is meant absolute. The meant time
 * is the time of the last recent call of this function by this task plus the now specified
 * increment. This way of specifying the desired time of resume supports the intended use
 * case, which is the implementation of regular real time tasks: A task will suspend itself
 * at the end of the infinite loop which contains its functional code with a constant time
 * value. This (fixed) time value becomes the sample time of the task. This behavior is
 * opposed to a delay or sleep function: The execution time of the task is no time which
 * additionally elapses between two task resumes.
 *   @return
 * The event mask of resuming events is returned. Since no combination with other events
 * than the elapsed system time is possible, this will always be RTOS_EVT_ABSOLUTE_TIMER.
 *   @param deltaTimeTillRelease
 * \a deltaTimeTillRelease refers to the last recent absolute time at which this task
 * had been resumed. This time is defined by the last recent call of either this function
 * or waitForEventTillTime. In the very first call of the function it refers to the point
 * in time the task was started.
 *   @see waitForEventTillTime
 */
/* ------------------------------------------------------------------------------------- */

uint16 rtos_suspendTaskTillTime()

{
    
    
} /* End of rtos_suspendTaskTillTime. */

 *              rtos_suspendTaskTillTime




TODO: Implement basic suspend (absolute timer)
      Implement initialization of data structures
      How to start everything? How to set up the initial stack areas?
      How to return the event vector from suspend? Argument by reference or can we set a
      return value by asm()?
      Implement context save and restore.
      ... and we can already make it run.
      Implement other suspends and postEvent


