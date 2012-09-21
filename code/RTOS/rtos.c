/**
 * @file rtos.c
 *   Implementation of a Real Time Operation System for the Arduino Mega board in the
 * Arduino environment 1.1.\n
 *   The implementation is dependent on the board (the controller) and the GNU C++ compiler
 * (thus the release of the Arduino environment) but should be easily portable to other
 * boards and Arduino releases. See documentation for details.
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
 *
 * Module interface
 *   rtos_initRTOS (internally called only)
 *   rtos_enableIRQTimerTic
 *   rtos_suspendTaskTillTime
 * Local functions
 *   prepareTaskStack
 *   onTimerTic
 */
 
 
/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"


/*
 * Defines
 */
#define IDLE_TASK_ID    (RTOS_NO_TASKS)


/*
 * Local type definitions
 */


/*
 * Local prototypes
 */
static bool onTimerTic(void);
 

/*
 * Data definitions
 */

/** The system time. A cyclic counter of the timer tics. The counter is interrupt driven.
    The unit of the time is defined only by the it triggering source and doesn't matter at
    all for the kernel. The time even don't need to be regular.\n
      The initial value is such that the time is 0 during the execution of the very first
    system timer interrupt service. This is important for getting a task startup behavior,
    which is transparent and predictable for the application. */
static uintTime_t _time = (uintTime_t)-1;

/** The one and only active task. This may be the only internally seen idle task which does
    nothing. */
static uint8_t _activeTaskId;

/** The task which is to be suspended because of a newly activated one. Only temporarily
    used in the instance of a task switch. */
static uint8_t _suspendedTaskId;

/** Array holding all due (but not active) tasks. Ordered according to their priority
    class. */
static uint8_t _dueTaskIdAryAry[RTOS_NO_PRIO_CLASSES][RTOS_MAX_NO_TASKS_IN_PRIO_CLASS];

/** Number of due tasks in the different priority classes. */
static uint8_t _noDueTasksAry[RTOS_NO_PRIO_CLASSES];

/** Array holding all currently suspended tasks. */
static uint8_t _suspendedTaskIdAry[RTOS_NO_TASKS];

/** Number of currently suspended tasks. */
static uint8_t _noSuspendedTasks;

/** Temporary data, internally used to pass information between assembly and C code. */
volatile uint16_t _leftStackPointer_u16, _newStackPointer_u16;


/*
 * Function implementation
 */


/**
 * Prepare the still unused stack area of a new task in a way that the normal context
 * switching code will load the desired initial context into the CPU. Context switches are
 * implemented symmetrically: The left context (including program counter) is pushed onto
 * the stack of the left context and the new context is entered by expecting the same
 * things on its stack. This works well at runtime but requires a manual pre-filling of the
 * stack of the new context before it has ever been executed.\n
 *   While the CPU registers don't matter here too much is the program counter of
 * particular interest. By presetting the PC in the context data the start address of the
 * task is defined.
 *   @return
 * The value of the stack pointer of the new task after pushing the initial CPU context
 * onto the stack is returned. It needs to be stored in the context safe area belonging to
 * this task for usage by the first context switch to this task.
 *   @param pEmptyTaskStack
 * A pointer to a RAM area which is reserved for the stack data of the new task. The
 * easiest way to get such an area is to define a uint8_t array on module scope.
 *   @param stackSize
 * The number of bytes of the stack area.
 *   @param taskEntryPoint
 * The start address of the task code as a function pointer of the only type a task
 * function may have. See typedef for details.
 *   @param taskParam
 * The task function is defined to have a single parameter. The value which is passed to
 * the task needs to be passed now.
 */

static uint8_t *prepareTaskStack( uint8_t *pEmptyTaskStack
                                , uint16_t stackSize
                                , rtos_taskFunction_t taskEntryPoint
                                , uint16_t taskParam
                                )
{
    uint8_t r;

    /* -1: We handle the stack pointer variable in the same way like the CPU does, with
       post-decrement. */
    uint8_t *sp = pEmptyTaskStack + stackSize - 1;
    
    /* Push 3 Bytes of guard program counter, which is the reset address, 0x00000. If
       someone returns from a task, this will cause a reset of the controller (instead of
       an undertemined kind of crash).
         CAUTION: The distinction between 2 and 3 byte PC is the most relevant modification
       of the code whenporting to another AVR CPU. Many types use a 16 Bit PC. */
    * sp-- = 0x00;
    * sp-- = 0x00;
#ifdef __AVR_ATmega2560__
    * sp-- = 0x00;
#else
# error Modifcation of code for other AVR CPU required
#endif
    
    /* Push 3 Byte program counter of task start address onto the still empty stack of the
       new task. The order is LSB, MidSB, MSB from bottom to top of stack (where the
       stack's bottom is the highest memory address). */
    * sp-- = (uint32_t)taskEntryPoint & 0x000000ff;
    * sp-- = ((uint32_t)taskEntryPoint & 0x0000ff00) >> 8;
#ifdef __AVR_ATmega2560__
    * sp-- = ((uint32_t)taskEntryPoint & 0x00ff0000) >> 16;
#else    
# error Modifcation of code for other AVR CPU required
#endif
    /* Now we have to push the initial value of r0, which is the __tmp_reg__ of the
       compiler. The value actually doesn't matter, we set it to 0. */
    * sp-- = 0;
    
    /* Now we have to push the initial value of the status register. The value basically
       doesn't matter, but why should we set any of the arithmetic flags? Also the global
       interrupt flag actually doesn't matter as the context switch will always enable
       global interrupts.
         Tip: Set the genral purpose flag T controlled by a parameter of this function is a
       cheap way to pass a Boolean parameter to the task function. */
    * sp-- = 0x80;
       
    /* The next value is the initial value of r1. This register needs to be 0 -- the
       compiler's code inside a function depends on this and will crash if r1 has another
       value. This register is therefore also called __zero_reg__. */
    * sp-- = 0;
    
    /* All other registers nearly don't matter. We set them to 0. An exception is r24/r25,
       which are used by the compiler to pass a unit16_t parameter to a function. */
    for(r=2; r<24; ++r)
        * sp-- = 0;
    
    /* Push the function parameter. */
    * sp-- = (uint16_t)taskParam & 0x00ff;
    * sp-- = ((uint16_t)taskParam & 0xff00) >> 8;
    
    /* And the the remaining registers to default value 0. */
    for(r=26; r<=31; ++r)
        * sp-- = 0;
    
    /* The stack is prepared. The value, the stack pointer has now needs to be returned to
       the caller. It has to be stored in the context save area of the new task as current
       stack pointer. */
    return sp;

} /* End of prepareTaskStack. */





/**
 * Start the interrupt which clocks the system time. Timer 2 is used as interrupt source
 * with a period time of about 2 ms or a frequency of 490.1961 Hz respectively.\n
 *   This is the default implementation of the routine, which can be overloaded by the
 * application code if another interrupt or other interrupt settings should be used.
 */

void rtos_enableIRQTimerTic(void)

{
    /* Initialize the timer. Arduino (wiring.c, init()) has initialized it to count up and
       down (phase correct PWM mode) with prescaler 64 and no TOP value (i.e. it counts
       from 0 till MAX=255). This leads to a call frequency of 16e6Hz/64/510 = 490.1961 Hz,
       thus about 2 ms period time.
         Here, we found on this setting (in order to not disturb any PWM related libraries)
       and just enable the overflow interrupt. */
    TIMSK2 |= _BV(TOIE2);
    
} /* End of rtos_enableIRQTimerTic */




/**
 * Each call of this function cyclically increments the system time of the kernel by one.\n
 *   Incrementing the system timer is an important system event. The routine will always
 * include an inspection of all suspended tasks, whether they could become due again.
 *   The cycle time of the system time is low (typically implemented as 0..255) and
 * determines the maximum delay time or timeout for a task which suspends itself and the
 * ration of task periods of the fastest and the slowest regular task. Furthermore it
 * determines the reliability of task overrun recognition. Task overrun events in the
 * magnitude of half the cycle time won't be recognized as such.\n
 *   The unit of the time is defined only by the it triggering source and doesn't matter at
 * all for the kernel. The time even don't need to be regular.\n
 *   @remark
 * The function needs to be called by an interrupt and can easily end with a context change,
 * i.e. the interrupt will return to another task as that it had interrupted.
 *   @remark
 * The connected interrupt is defined by macro #RTOS_ISR_SYSTEM_TIMER_TIC. This interrupt
 * needs to be disabled/enabled by the implementation of \a enterCriticalSection and \a
 * leaveCriticalSection.
 *   @remark
 * The cycle time of the system time can be influenced by the typedef of uintTime_t. Find a
 * discussion of pros and cons at the location of this typedef.
 *   @see bool onTimerTic(void)
 *   @see void enterCriticalSection(void)
 */

ISR(RTOS_ISR_SYSTEM_TIMER_TIC, ISR_NAKED)

{
    /* Save context onto the stack of the interrupted active task. */
    asm volatile
    ( "push r0 \n\t"
      "in r0, __SREG__\n\t"
      "push r0 \n\t"
      "push r1 \n\t"
      "push r2 \n\t"
      "push r3 \n\t"
      "push r4 \n\t"
      "push r5 \n\t"
      "push r6 \n\t"
      "push r7 \n\t"
      "push r8 \n\t"
      "push r9 \n\t"
      "push r10 \n\t"
      "push r11 \n\t"
      "push r12 \n\t"
      "push r13 \n\t"
      "push r14 \n\t"
      "push r15 \n\t"
      "push r16 \n\t"
      "push r17 \n\t"
      "push r18 \n\t"
      "push r19 \n\t"
      "push r20 \n\t"
      "push r21 \n\t"
      "push r22 \n\t"
      "push r23 \n\t"
      "push r24 \n\t"
      "push r25 \n\t"
      "push r26 \n\t"
      "push r27 \n\t"
      "push r28 \n\t"
      "push r29 \n\t"
      "push r30 \n\t"
      "push r31 \n\t"
    );
    
    /* An ISR must not occur while we're updating the global data and checking for a
       possible task switch. To be more precise: The call of onTimerTic would just require
       to inhibit all those interrupts which might initiate a task switch. As long as no
       user defined interrupts are configured to set an RTOS event, this is only the single
       timer interrupt driving the system time. However, at latest when a task switch
       really is initiated we would need to lock all interrupts globally (as we modify the
       stack pointer in non-atomic operation). It doesn't matter to globally lock
       interrupts already here. */
    // @todo Global interrupt flag is set on entry into this function
    asm volatile
    ( "cli \n\t"
    );

    /* Check for all suspended tasks if this change in time is an event for them. */
    if(onTimerTic())
    {
        /* Yes, another task becomes active with this timer tic. Switch the stack pointer
           to the (saved) stack pointer of that task. */
        
        // @todo Find more elegant way of passing the stack pointer values to/from the assembly code
        
        _newStackPointer_u16 = rtos_taskAry[_activeTaskId].stackPointer;
        asm volatile
        ( "in r0, __SP_L__ /* Save current stack pointer at known, fixed location */ \n\t"
          "sts _leftStackPointer_u16, r0 \n\t"
          "in r0, __SP_H__ \n\t"
          "sts _leftStackPointer_u16+1, r0 \n\t"
          "lds r0, _newStackPointer_u16 \n\t"
          "out __SP_L__, r0 /* Write l-byte of new stack pointer content */ \n\t"
          "lds r0, _newStackPointer_u16 + 1 \n\t"
          "out __SP_H__, r0 /* Write h-byte of new stack pointer content */ \n\t"
        );
        rtos_taskAry[_suspendedTaskId].stackPointer = _leftStackPointer_u16;
    }    
    
    /* The highly critical operation of modifying the stack pointer is done. From now on,
       all interrupts can safely operate on the new stack, the stack of the new task. This
       includes such an interrupt which would cause another task switch. */
    /** @todo Early releasing the global interrupts is basically possible but could lead to
        higher use of stack area if many task switches appear one after another. Find out
        if such a situation can be constructed and maybe decide to do cli/sei as outermost
        operations. The disadvantage is probably minor (some clock tics less of
        responsiveness). */
    asm volatile
    ( "sei /* Continue normal operation. */ \n\t"
    );
    
    /* The stack pointer points to the now active task (which will often be still the same
       as at function entry). The CPU to continue with is popped from this stack. If
       there's no change in active task the entire routine call is just like any ordinary
       interrupt. */
    asm volatile
    ( "pop r31 /* Do inverse operations to return to other context */ \n\t"
      "pop r30 \n\t"
      "pop r29 \n\t"
      "pop r28 \n\t"
      "pop r27 \n\t"
      "pop r26 \n\t"
      "pop r25 \n\t"
      "pop r24 \n\t"
      "pop r23 \n\t"
      "pop r22 \n\t"
      "pop r21 \n\t"
      "pop r20 \n\t"
      "pop r19 \n\t"
      "pop r18 \n\t"
      "pop r17 \n\t"
      "pop r16 \n\t"
      "pop r15 \n\t"
      "pop r14 \n\t"
      "pop r13 \n\t"
      "pop r12 \n\t"
      "pop r11 \n\t"
      "pop r10 \n\t"
      "pop r9 \n\t"
      "pop r8 \n\t"
      "pop r7 \n\t"
      "pop r6 \n\t"
      "pop r5 \n\t"
      "pop r4 \n\t"
      "pop r3 \n\t"
      "pop r2 \n\t"
      "pop r1 \n\t"
      "pop r0 \n\t"
      "out __SREG__, r0 \n\t"
      "pop r0 \n\t"
      "reti /* The global interrupt enable flag is not saved across task switches, but always set */ \n\t"
    );
    
} /* End of ISR to increment the system time by one tic. */




/**
 * This function is called from the system interrupt triggered by the main clock. The
 * timers of all due tasks are served and - in case they elapse - timer events are
 * generated. These events may then release some of the tasks. If so, they are placed in
 * the appropriate list of due tasks. Finally, the longest due task in the highest none
 * empty priority class is activated.
 *   @return
 * The Boolean information is returned whether we have or not have a task switch. In most
 * invokations we won't have and therefore it's worth to optimize the code for this case:
 * Don't do the expensive switch of the stack pointers.\n
 *   The most important result of the function, the ID of the active task after leaving the
 * function, is returned by side effect: The global variable _activeTaskId is updated.
 */

static bool onTimerTic(void)
{
    uint8_t idxSuspTask;
    bool isNewActiveTask = false;
          
    for(idxSuspTask=0; idxSuspTask<_noSuspendedTasks; ++idxSuspTask)
    {
        rtos_task_t *pT = &rtos_taskAry[_suspendedTaskIdAry[idxSuspTask]];
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
                 For these reasons, the code doesn't double check for repeatedly setting the
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
        if( (pT->waitForAnyEvent &&  eventVec != 0)
            ||  (!pT->waitForAnyEvent &&  eventVec == pT->eventMask)
          )
        {           
            uint8_t u
                  , prio = pT->prioClass;
            
            /* This task became due. Move it from the list of suspended tasks to the list
               of due tasks of its priority class. */
            _dueTaskIdAryAry[prio][_noDueTasksAry[prio]++] = _suspendedTaskIdAry[idxSuspTask];
            -- _noSuspendedTasks;
            for(u=idxSuspTask; u<_noSuspendedTasks; ++u)
                _suspendedTaskIdAry[idxSuspTask] = _suspendedTaskIdAry[idxSuspTask+1];
            
            /* Since a task became due there might be a change of the active task. */
            isNewActiveTask = true;
        }
        
    } /* End for(All suspended tasks) */
    
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
    /* Round-robin: Applies only to the active task. It can become inactive, however not
       undue. If its time slice is elapsed it is put at the end of the due list in its
       priority class. */
    // @todo Implement round-robin. Shortcut isNewActiveTask if the active task is made undue: Now we just have to take the first one from the known list of due tasks in the given prio class - after having rotated the list contents.
#endif    

    /* Here, isNewActiveTask actually means "could be new active task". Find out now if
       there's really a new active task. */
    if(isNewActiveTask)
    {
        int8_t idxPrio;
        
        /* Look for the task we will return to. It's the first entry in the highest
           non-empty priority class. */
        for(idxPrio=RTOS_NO_PRIO_CLASSES-1; idxPrio>=0; --idxPrio)
        {
            if(_noDueTasksAry[idxPrio] > 0)
            {
                _suspendedTaskId = _activeTaskId;
                _activeTaskId    = _dueTaskIdAryAry[idxPrio][0];
                
                /* As we made at least one task due, these statements are surely reached if
                   we only entered the outermost if clause. As the due becoming task might
                   be of lower priority it can easily be that we nonetheless don't have a
                   task switch. */
                isNewActiveTask = _activeTaskId == _suspendedTaskId;

                break;
            }
        }
    } /* if(Is there a non-zero probability for a task switch?) */
    
    /* The calling interrupt service routine will do a context switch only if we return
       true. Otherwise it'll simply do a "reti" to the interrupted context and continue
       it. */ 
    return isNewActiveTask;
    
} /* End of onTimerTic. */






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

uint16_t rtos_suspendTaskTillTime(uintTime_t deltaTimeTillRelease)

{
    return 0;
    
} /* End of rtos_suspendTaskTillTime. */



/**
 * Application called initialization of RTOS.\n
 *   Most important is the application handled task information. A task is characterized by
 * several static settings which need to be preset by the application. To save ressources,
 * a standard situation will be the specification of all relevant settings at compile time
 * in the initializer expression of the array definition. The array is declared extern to
 * enable this mode.\n
 *   The application however also has the chance to provide this information at runtime.
 * Early in the execution of this function a callback \a setup into the application is
 * invoked. If the application has setup everything as a compile time expression, the
 * callback may simply contain a return.\n
 *   The callback \a setup is invoked before any RTOS related interrupts have been
 * initialized, the routine is executed in the same context as and as a substitute of the
 * normal Arduino setup routine. The implementation can be made without bothering with
 * interrupt inhibition or data synchronization considerations. Furthermore it can be used
 * for all the sort of things you've ever done in Arduino's setup routine.\n
 *   After returning from \a setup all tasks defined in the task array are made due. The
 * main interrupt, which clocks the RTOS system time is started and will immediately make
 * the very task the active task which belongs to the highest priority class and which was
 * found first (i.e. in the order of rising indexes) in the task array. The system is
 * running.\n
 *   No idle task is specified in the task array. The idle task is implicitly defined and
 * implemented as the external function \a loop. To stick to Arduino's convention (and to
 * give the RTOS the chance to benefit from idle as well) \a loop is still implemented as
 * such: You are encouraged to return from \a loop after doing things. RTOS will call \a
 * loop again as soon as it has some time left.\n
 *   As for the original Arduino code, \a setup and \a loop are mandatory, global
 * functions.\n
 *   This routine is not called by the application but in C's main function. Your code
 * seems to start with setup and seems then to branch into either \a loop (the idle task)
 * or any other of the tasks defined by your application.\n
 *   This function never returns. No task must ever return, a reset will be the immediate
 * consequence. Your part of the idle task, function \a loop, may and should return, but
 * the actual idle task as a whole won't change neither. Instead it'll repeat to call \a
 * loop.
 */

void rtos_initRTOS(void)

{
    uint8_t idxTask, idxClass;
    rtos_task_t *pT;
    
    /* Give the application the chance to do all its initialization -- regardless of RTOS
       related or whatever else. After return, the task array needs to be properly
       filled. */
    setup();
       
    /* Handle all tasks. */
    for(idxTask=0; idxTask<RTOS_NO_TASKS; ++idxTask)
    {
        pT = &rtos_taskAry[idxTask];
        
        /* Prepare the stack of the task and store the initial stack pointer value. */
        pT->stackPointer = (uint16_t)prepareTaskStack( pT->pStackArea
                                                     , pT->stackSize
                                                     , pT->taskFunction
                                                     , pT->taskFunctionParam
                                                     );
#if 1
        {
            uint16_t i;
            
            Serial.print("Task ");
            Serial.print(idxTask);
            Serial.print(":\nStack pointer: 0x");
            Serial.println(pT->stackPointer, HEX);
            for(i=0; i<pT->stackSize; ++i)
            {
                if(i%8 == 0)
                {
                    Serial.println("");
                    Serial.print(i, HEX);
                    Serial.print(", 0x");
                    Serial.print((uint16_t)(pT->pStackArea+i), HEX);
                    Serial.print(":\t");
                }
                Serial.print(pT->pStackArea[i], HEX);
                Serial.print("\t");
            }
            Serial.println("");
        }
#endif
        /* The delay counter will only be set by some of the suspend commands issued by the
           task itself. */
        pT->cntDelay = 0;
    
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
        /* The round robin counter is loaded to its maximum when the tasks becomes due.
           Now, the value doesn't matter. */
        pT->cntRoundRobin = 0;
#endif
        /* No events have been posted to this task yet. */
        pT->postedEventVec = 0;
    
        /* Initially, all tasks are suspended and will be awaked by an absolute time event.
           This strategy avoids the need for an additional, explicitly invoked context
           switch. Just start the timer interrupt and let it do a few tics and the task
           will run. The application can determine how many tics (including the very first)
           in order to spraed the tasks over the time grid. */
        /** @todo Consider to have a delay event as initial event. The user would
            initialize the delay time. This is basically equivalent but could be easier to
            understand. */
        pT->eventMask = RTOS_EVT_ABSOLUTE_TIMER;
    
        /* Mode of waiting doen't matter as we just set one. */
        pT->waitForAnyEvent = true;

        /* Any task is suspended at the beginning. No task is active, see before. */
        _suspendedTaskIdAry[idxTask] = idxTask;
       
    } /* for(All tasks to initialize) */
    
    /* Number of currently suspended tasks: All. */
    _noSuspendedTasks = RTOS_NO_TASKS;

    /* The idle task is stored in the last array entry. It differs, there's e.g. no task
       function defined. We mainly need to storage location for the stack pointer. */
    pT = &rtos_taskAry[IDLE_TASK_ID];
    pT->stackPointer = 0;           /* Used only at de-activation. */
    pT->cntDelay = 0;               /* Not used at all. */
#if RTOS_ROUND_ROBIN_MODE_SUPPORTED == RTOS_FEATURE_ON
    pT->cntRoundRobin = 0;          /* Not used at all. */
#endif
    pT->postedEventVec = 0;         /* Not used at all. */
    pT->eventMask = 0;              /* Not used at all. */
    pT->waitForAnyEvent = false;    /* Not used at all. */
    
    /* Any task is suspended at the beginning. No task is active, see before. */
    for(idxClass=0; idxClass<RTOS_NO_PRIO_CLASSES; ++idxClass)
        _noDueTasksAry[idxClass] = 0;
    _activeTaskId = IDLE_TASK_ID;
    _suspendedTaskId = IDLE_TASK_ID;

    /* All data is prepared. Let's start the IRQ which clocks the system time. */
    rtos_enableIRQTimerTic();
    
    /* From here, all further code implicitly becomes the idle task. */
    while(true)
        loop();

} /* End of rtos_initRTOS */





/*
TODO: Implement basic suspend (absolute timer)
      Implement initialization of data structures
      ... and we can already make it run.
      Implement other suspends and postEvent
*/