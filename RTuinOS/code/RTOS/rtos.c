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
 *   ISR
 *   rtos_initRTOS (internally called only)
 *   rtos_enableIRQTimerTic
 *   rtos_suspendTaskTillTime
 *   rtos_waitForEvent
 * Local functions
 *   prepareTaskStack
 *   onTimerTic
 *   suspendTaskTillTime
 *   waitForEvent
 */


/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"

/** @todo DEBUG code */
uint16_t _postEv = 0;
uint16_t _makeDue = 0;

/*
 * Defines
 */

/** The ID of the idle task. The ID of a task is identical with the index into the task
    array and the array of stack pointers. */
#define IDLE_TASK_ID    (RTOS_NO_TASKS)

/** A pattern byte, which is used as prefill byte of any task stack area. A simple and
    unexpensive stack usage check at runtime can be implemented by looking for up to where
    this pattern has been destroyed. Any value which is improbable to be a true stack
    contents byte can be used -- whatever this value might be. */
#define UNUSED_STACK_PATTERN 0x29

/** An important code pattern, which is used in every interrupt routine, which can result
    in a context switch. The CPU context except for the program counter is saved by pushing
    it onto the stack of the given context. The program counter is not explicitly saved:
    This code pattern needs to be used at the very beginning of a function so that the PC
    has been pushed onto the stack just before by the call of this function.\n
      @remark The function which uses this pattern must not be inlined, otherwise the PC
    would not be part of the saved context and the system would crash when trying to return
    to this context the next time!
      @remark This pattern needs to be changed only in strict accordance with the
    counterpart pattern, which pops the context from a stack back into the CPU. Both
    pattern needs to be the inverse of each other. */
#define PUSH_CONTEXT_ONTO_STACK                     \
    PUSH_CONTEXT_WITHOUT_R24R25_ONTO_STACK;         \
    asm volatile                                    \
    ( "push r24 \n\t"                               \
      "push r25 \n\t"                               \
    );
/* End of macro PUSH_CONTEXT_ONTO_STACK */


/** An important code pattern, which is used in every suspend command. The CPU context
    execpt for the register pair r24/r25 is saved by pushing it onto the stack of the given
    context. (Exception program counter: see macro #PUSH_CONTEXT_ONTO_STACK.)\n
      When returning to a context which has become un-due by invoking one of the suspend
    commands, the restore context should still be done with the other macro
    #POP_CONTEXT_FROM_STACK. However, before using this macro, the return code of the
    suspend command needs to be pushed onto the stack so that it is loaded into the CPU's
    register pair r24/r25 as part of macro #POP_CONTEXT_FROM_STACK.
      @remark The function which uses this pattern must not be inlined, otherwise the PC
    would not be part of the saved context and the system would crash when trying to return
    to this context the next time!
      @remark This pattern needs to be changed only in strict accordance with the
    counterpart pattern, which pops the context from a stack back into the CPU. */
#define PUSH_CONTEXT_WITHOUT_R24R25_ONTO_STACK         \
    asm volatile                                       \
    ( "push r0 \n\t"                                   \
      "in r0, __SREG__\n\t"                            \
      "push r0 \n\t"                                   \
      "push r1 \n\t"                                   \
      "push r2 \n\t"                                   \
      "push r3 \n\t"                                   \
      "push r4 \n\t"                                   \
      "push r5 \n\t"                                   \
      "push r6 \n\t"                                   \
      "push r7 \n\t"                                   \
      "push r8 \n\t"                                   \
      "push r9 \n\t"                                   \
      "push r10 \n\t"                                  \
      "push r11 \n\t"                                  \
      "push r12 \n\t"                                  \
      "push r13 \n\t"                                  \
      "push r14 \n\t"                                  \
      "push r15 \n\t"                                  \
      "push r16 \n\t"                                  \
      "push r17 \n\t"                                  \
      "push r18 \n\t"                                  \
      "push r19 \n\t"                                  \
      "push r20 \n\t"                                  \
      "push r21 \n\t"                                  \
      "push r22 \n\t"                                  \
      "push r23 \n\t"                                  \
      "push r26 \n\t"                                  \
      "push r27 \n\t"                                  \
      "push r28 \n\t"                                  \
      "push r29 \n\t"                                  \
      "push r30 \n\t"                                  \
      "push r31 \n\t"                                  \
    );
/* End of macro PUSH_CONTEXT_WITHOUT_R24R25_ONTO_STACK */


/** An important code pattern, which is used in every interrupt routine (including the
    suspend commands, which can be considered pseudo-software interrupts). The CPU context
    except for the program counter is restored by popping it from the stack of the given
    context. The program counter is not popped: This code pattern needs to be used at the
    very end of a function so that the PC will be restored by the return machine command
    (ret or reti).\n
      @remark The function which uses this pattern must not be inlined, otherwise the PC
    would not be part of the restored context and the system would crash.
      @remark This pattern needs to be changed only in strict accordance with the
    counterpart patterns, which push the context onto the stack. The pattern need to be
    the inverse of each other. */
#define POP_CONTEXT_FROM_STACK          \
    asm volatile                        \
    ( "pop r25 \n\t"                    \
      "pop r24 \n\t"                    \
      "pop r31 \n\t"                    \
      "pop r30 \n\t"                    \
      "pop r29 \n\t"                    \
      "pop r28 \n\t"                    \
      "pop r27 \n\t"                    \
      "pop r26 \n\t"                    \
      "pop r23 \n\t"                    \
      "pop r22 \n\t"                    \
      "pop r21 \n\t"                    \
      "pop r20 \n\t"                    \
      "pop r19 \n\t"                    \
      "pop r18 \n\t"                    \
      "pop r17 \n\t"                    \
      "pop r16 \n\t"                    \
      "pop r15 \n\t"                    \
      "pop r14 \n\t"                    \
      "pop r13 \n\t"                    \
      "pop r12 \n\t"                    \
      "pop r11 \n\t"                    \
      "pop r10 \n\t"                    \
      "pop r9 \n\t"                     \
      "pop r8 \n\t"                     \
      "pop r7 \n\t"                     \
      "pop r6 \n\t"                     \
      "pop r5 \n\t"                     \
      "pop r4 \n\t"                     \
      "pop r3 \n\t"                     \
      "pop r2 \n\t"                     \
      "pop r1 \n\t"                     \
      "pop r0 \n\t"                     \
      "out __SREG__, r0 \n\t"           \
      "pop r0 \n\t"                     \
    );
/* End of macro POP_CONTEXT_FROM_STACK */


/** An important code pattern, which is used in every interrupt routine (including the
    suspend commands, which can be considered pseudo-software interrupts). The code
    performs the actual task switch by saving the current stack pointer in a location owned
    by the left task and loading the stack pointer from a location owned by the new task
    (where its stack pointer value had been saved at initialization time or the last time
    it became inactive.).\n
      The code fragment then decides whether the new task had been inactivated by a timer
    interrupt or by a suspend command. In the latter case the return value of the suspend
    command is put onto the stack. From there it'll be loaded into the CPU when ending the
    interrupt routine.\n
      Side effects: The left task and the new task are read from the global variables
    _suspendedTaskId and _activeTaskId.\n
      Prerequisites: The use of the macro needs to be followed by a use of macro
    PUSH_RET_CODE_OF_SWITCH_CONTEXT. (Both macros have not been yoined to a single one only
    for sake of comprehensibility of the code using the code patterns.)\n
      The routine depends on a reset global interrupt flag.\n
      The implementation must be compatible with a naked function. In particular, it must
    not define any local data! */
// @todo For sake of readability, this could become two macros - although they will always be used as a pair, SWITCH_CONTEXT and PUSH_FUNCTION_RETURN_CODE
#define SWITCH_CONTEXT                                                                      \
{                                                                                           \
    /* Switch the stack pointer to the (saved) stack pointer of the new active task. */     \
    _tmpVarCToAsm_u16 = rtos_taskAry[_activeTaskId].stackPointer;                           \
    asm volatile                                                                            \
    ( "in r0, __SP_L__ /* Save current stack pointer at known, fixed location */ \n\t"      \
      "sts _tmpVarAsmToC_u16, r0 \n\t"                                                      \
      "in r0, __SP_H__ \n\t"                                                                \
      "sts _tmpVarAsmToC_u16+1, r0 \n\t"                                                    \
      "lds r0, _tmpVarCToAsm_u16 \n\t"                                                      \
      "out __SP_L__, r0 /* Write l-byte of new stack pointer content */ \n\t"               \
      "lds r0, _tmpVarCToAsm_u16+1 \n\t"                                                    \
      "out __SP_H__, r0 /* Write h-byte of new stack pointer content */ \n\t"               \
    );                                                                                      \
    rtos_taskAry[_suspendedTaskId].stackPointer = _tmpVarAsmToC_u16;                        \
                                                                                            \
} /* End of macro SWITCH_CONTEXT */



/** An important code pattern, which is used in every interrupt routine (including the
    suspend commands, which can be considered pseudo-software interrupts). Immediately
    after a context switch, the code fragment decides whether the task we had switch to had
    been inactivated by a timer interrupt or by a suspend command. (Only) in the latter
    case the return value of the suspend command is put onto the stack. From there it'll be
    loaded into the CPU when ending the interrupt routine.\n
      Side effects: The ID of the new task is read from the global variable _activeTaskId.\n
      Prerequisites: The use of the macro needs to be preceeded by a use of macro
    SWITCH_CONTEXT.\n
      The routine depends on a reset global interrupt flag.\n
      The implementation must be compatible with a naked function. In particular, it must
    not define any local data! */
#define PUSH_RET_CODE_OF_CONTEXT_SWITCH                                                     \
{                                                                                           \
    /* The first matter after a context switch is whether the new task became active the    \
       very first time after it had been suspended or if it became active again after being \
       temporarily only ready (but not suspended) because of being superseded by a higher   \
       prioritized task or because of a round-robin cycle.                                  \
         If this is the first activation after state suspended, we need to return the       \
       cause for release from suspended state as function return code to the task. When     \
       a task is suspended it always pauses inside the suspend command. */                  \
    _tmpVarCToAsm_u16 = rtos_taskAry[_activeTaskId].postedEventVec;                         \
    if(_tmpVarCToAsm_u16 > 0)                                                               \
    {                                                                                       \
        /* Neither at state changes active -> ready, and nor at changes ready ->            \
           active, the event vector is touched. It'll be set only at state changes          \
           suspended -> ready. If we reset it now, we will surely not run into this if      \
           clause again after later changes active -> ready -> active. */                   \
        rtos_taskAry[_activeTaskId].postedEventVec = 0;                                     \
                                                                                            \
        /* Yes, the new context was suspended before, i.e. it currently pauses inside a     \
           suspend command, waiting for its completion and expecting its return value.      \
           Place this value onto the new stack and let it be loaded by the restore          \
           context operation below. */                                                      \
        asm volatile                                                                        \
        ( "lds r0, _tmpVarCToAsm_u16 \n\t"      /* Read low byte of return code. */         \
          "push r0 \n\t"                        /* Push it at context position r24. */      \
          "lds r0, _tmpVarCToAsm_u16+1 \n\t"    /* Read high byte of return code. */        \
          "push r0 \n\t"                        /* Push it at context position r25. */      \
        );                                                                                  \
    } /* if(Do we need to place a suspend command's return code onto the new stack?) */     \
                                                                                            \
} /* End of macro PUSH_RET_CODE_OF_CONTEXT_SWITCH */



/*
 * Local type definitions
 */


/*
 * Local prototypes
 */
 
static bool onTimerTic(void);
volatile uint16_t rtos_suspendTaskTillTime(uintTime_t deltaTimeTillRelease) __attribute__((naked, noinline));
static void suspendTaskTillTime(uintTime_t) __attribute__((used, noinline));
static void waitForEvent(uint16_t eventMask, bool all, uintTime_t timeout) __attribute__((used, noinline));
volatile uint16_t rtos_waitForEvent(uint16_t eventMask, bool all, uintTime_t timeout) __attribute__((naked, noinline));


/*
 * Data definitions
 */

/** The system time. A cyclic counter of the timer tics. The counter is interrupt driven.
    The unit of the time is defined only by the it triggering source and doesn't matter at
    all for the kernel. The time even don't need to be regular.\n
      The initial value is such that the time is 0 during the execution of the very first
    system timer interrupt service. This is important for getting a task startup behavior,
    which is transparent and predictable for the application. */
/*static*/ uintTime_t _time = (uintTime_t)-1;

/** The one and only active task. This may be the only internally seen idle task which does
    nothing. */
/*static*/ uint8_t _activeTaskId = IDLE_TASK_ID;

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
volatile uint16_t _tmpVarAsmToC_u16, _tmpVarCToAsm_u16;


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

static uint8_t *prepareTaskStack( uint8_t * const pEmptyTaskStack
                                , uint16_t stackSize
                                , rtos_taskFunction_t taskEntryPoint
                                , uint16_t taskParam
                                )
{
    uint8_t r;

    /* -1: We handle the stack pointer variable in the same way like the CPU does, with
       post-decrement. */
    uint8_t *sp = pEmptyTaskStack + stackSize - 1
          , *retCode;

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
         Tip: Set the general purpose flag T controlled by a parameter of this function is a
       cheap way to pass a Boolean parameter to the task function. */
    * sp-- = 0x80;

    /* The next value is the initial value of r1. This register needs to be 0 -- the
       compiler's code inside a function depends on this and will crash if r1 has another
       value. This register is therefore also called __zero_reg__. */
    * sp-- = 0;

    /* All other registers nearly don't matter. We set them to 0. An exception is r24/r25,
       which are used by the compiler to pass a unit16_t parameter to a function. For all
       contexts of suspended tasks (including this one, which is a new one), the registers
       r25/r25 are not part of the context: The values of these registers will be loaded
       explicitly with the result of the suspend command immediately before the return to
       the task. */
    for(r=2; r<=23; ++r)
        * sp-- = 0;
    for(r=26; r<=31; ++r)
        * sp-- = 0;

    /* The stack is prepared. The value, the stack pointer has now needs to be returned to
       the caller. It has to be stored in the context save area of the new task as current
       stack pointer. */
    retCode = sp;

    /* The rest of the stack area doesn't matter. Nonetheless, we fill it with a specific
       pattern, which will permit to run a (a bit guessing) stack usage routine later on:
       We can look up to where the pattern has been destroyed. */
    while(sp >= pEmptyTaskStack)
        * sp-- = UNUSED_STACK_PATTERN;

    return retCode;

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

    /* Clock the system time. Cyclic overrun is intended. */
    ++ _time;

    for(idxSuspTask=0; idxSuspTask<_noSuspendedTasks; ++idxSuspTask)
    {
        rtos_task_t *pT = &rtos_taskAry[_suspendedTaskIdAry[idxSuspTask]];
        uint16_t eventVec;

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
++ _postEv;
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
        /* @todo The AND operation has been specified bad: AND must only refer to the
           postable events but not include the timer events. All postable events need to be
           set in either the mask and the vector of posted events OR any of the timer
           events in the mask are set in the vector of posted events. Consider if it still
           makes sense to have all events uniquely in a single vector: This decision was
           mainly taken because of the homegenous implementation -- which is no longer
           given with this specification change. */
        eventVec = pT->postedEventVec & pT->eventMask;
        if( (pT->waitForAnyEvent &&  eventVec != 0)
            ||  (!pT->waitForAnyEvent &&  eventVec == pT->eventMask)
          )
        {
            uint8_t u
                  , prio = pT->prioClass;

            /* This task becomes due. Move it from the list of suspended tasks to the list
               of due tasks of its priority class. */
            _dueTaskIdAryAry[prio][_noDueTasksAry[prio]++] = _suspendedTaskIdAry[idxSuspTask];
            -- _noSuspendedTasks;
            for(u=idxSuspTask; u<_noSuspendedTasks; ++u)
                _suspendedTaskIdAry[idxSuspTask] = _suspendedTaskIdAry[idxSuspTask+1];
++ _makeDue;
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

                /* If we only entered the outermost if clause we made at least one task
                   due; these statements are thus surely reached. As the due becoming task
                   might however be of lower priority it can easily be that we nonetheless
                   don't have a task switch. */
                isNewActiveTask = _activeTaskId != _suspendedTaskId;

                break;
            }
        }
    } /* if(Is there a non-zero probability for a task switch?) */

    /* If we have a new active task, the next question is whether it became active the very
       first time after it had been suspended or if it became active again after being
       temporarily un-due (but not suspended) because of being superseded by a higher
       prioritized task or because of a round-robin cycle.
         If this is the first activation after state suspended, we need to return the cause
       for release from suspended state as function return code to the task. When a task is
       suspended it always pauses inside the suspend command. */
    if(isNewActiveTask)
    {
        
    } /* if(The task is surely switched?) */
    
    /* The calling interrupt service routine will do a context switch only if we return
       true. Otherwise it'll simply do a "reti" to the interrupted context and continue
       it. */
    return isNewActiveTask;

} /* End of onTimerTic. */






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
    /* An ISR must not occur while we're updating the global data and checking for a
       possible task switch. To be more precise: The call of onTimerTic would just require
       to inhibit all those interrupts which might initiate a task switch. As long as no
       user defined interrupts are configured to set an RTOS event, this is only the single
       timer interrupt driving the system time. However, at latest when a task switch
       really is initiated we would need to lock all interrupts globally (as we modify the
       stack pointer in non-atomic operation). It doesn't matter to have locked all
       interrupts globally already here. */

    /* Save context onto the stack of the interrupted active task. */
    PUSH_CONTEXT_ONTO_STACK

    /* Check for all suspended tasks if this change in time is an event for them. */
    if(onTimerTic())
    {
        /* Yes, another task becomes active with this timer tic. Switch the stack pointer
           to the (saved) stack pointer of that task. */
        SWITCH_CONTEXT
        PUSH_RET_CODE_OF_CONTEXT_SWITCH
    }

    /* The highly critical operation of modifying the stack pointer is done. From now on,
       all interrupts could safely operate on the new stack, the stack of the new task. This
       includes such an interrupt which would cause another task switch. However, early
       releasing the global interrupts here could lead to higher use of stack area if many
       task switches appear one after another. Therefore we will reenable the interrupts
       only with the final reti command. The disadvantage is probably minor (some clock
       tics less of responsiveness of the system). */
    /* @todo It's worth a consideration if too many task switches at a time can really
       happen: While restoring the new context is running, the only source for those task
       switches would be a new timer tic and this comes determinitically far in the
       future.*/

    /* The stack pointer points to the now active task (which will often be still the same
       as at function entry). The CPU context to continue with is popped from this stack. If
       there's no change in active task the entire routine call is just like any ordinary
       interrupt. */
    POP_CONTEXT_FROM_STACK
    
    /* The global interrupt enable flag is not saved across task switches, but always set
       on entry into the new or same context by using a reti rather than a ret.
         If we return to the same context, this will not mean that we harmfully change the
       state of a running context without the context knowing or willing it: If the context
       had reset the bit we would never have got here, as this is an ISR controlled by the
       bit. */
    asm volatile
    ( "reti \n\t"
    );

} /* End of ISR to increment the system time by one tic. */




/**
 * Suspend operation of software interrupt rtos_suspendTaskTillTime.\n
 *   The action of this SW interupt is placed into an own function in order to let the
 * compiler generate the stack frame required for all local data. (The stack frame
 * generation of the SW interupt needs to be inhibited in order to permit the
 * implementation of saving/restoring the task context).
 *   @return
 * The function determines which task is to be activated and records which task is left in
 * the global variables _activeTaskId and _suspendedTaskId.
 *   @param deltaTimeTillRelease
 * See software interrupt \a rtos_suspendTaskTillTime.
 *   @see
 * uint16_t rtos_suspendTaskTillTime(uintTime_t)
 *   @remark
 * This function and particularly passing the return codes via a global variable will
 * operate only if all interrupts are disabled.
 */ 

static void suspendTaskTillTime(uintTime_t deltaTimeTillRelease)
{
    /* Avoid inlining under all circumstances. */
    asm("");
    
    int8_t idxPrio;
    uint8_t idxTask;
    
    /* Take the active task out of the list of due tasks. */
    rtos_task_t *pT = &rtos_taskAry[_activeTaskId];
    uint8_t prio = pT->prioClass;
    uint8_t noDueNow = -- _noDueTasksAry[prio];
    for(idxTask=0; idxTask<noDueNow; ++idxTask)
        _dueTaskIdAryAry[prio][idxTask] = _dueTaskIdAryAry[prio][idxTask+1];
    
    /* This suspend command want a reactivation at a certain time. */
    // @todo Here, we need some code for task overrun detection. The new time must not more than half a cycle in the future.
    pT->timeDueAt += deltaTimeTillRelease;
    pT->eventMask = RTOS_EVT_ABSOLUTE_TIMER;
    pT->waitForAnyEvent = true;
    
    /* Put the task in the list of suspended tasks. */
    _suspendedTaskIdAry[++_noSuspendedTasks] = _activeTaskId;

    /* Record which task suspends itself for the assembly code in the calling function
       which actually switches the context. */
    _suspendedTaskId = _activeTaskId;
    
    /* Look for the task we will return to. It's the first entry in the highest non-empty
       priority class. The loop requires a signed index.
         It's not guaranteed that there is any due task. Idle is the fallback. */
    _activeTaskId = IDLE_TASK_ID;
    for(idxPrio=RTOS_NO_PRIO_CLASSES-1; idxPrio>=0; --idxPrio)
    {
        if(_noDueTasksAry[idxPrio] > 0)
        {
            _activeTaskId = _dueTaskIdAryAry[idxPrio][0];
            break;
        }
    }
} /* End of suspendTaskTillTime */




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
 *   @remark
 * It is absolutely essential that this routine is implemented as naked and noinline. See
 * http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html for details
 *   @remark
 * GCC doesn't create a stack frame for naked functions. For normal functions, the calling
 * parameter of the function is stored in such a stack frame. In the combination naked and
 * having local function parameters, GCC has a problem when generating code without
 * optimization: It doesn't generate a stack frame but still does save the local parameter
 * into the (not existing) stack frame as very first assembly operation of the function
 * code. There's absolutely not work around; when the earliest code, we can write inside
 * the function is executed, the stack is already corrupted in a harzardous way. A crash is
 * unavoidable.\n
 *   A (less helpful) discussion of the issue can be found at
 * http://lists.gnu.org/archive/html/avr-gcc-list/2012-08/msg00014.html.\n
 *   An imaginable work around is to pass data to the function by global objects.
 * This data would be task related so that filling the data object and calling this
 * function needed to be an atomic operation. A macro resetting the global interrupt flag,
 * filling the data object and calling the function would be required to do this.\n
 *   So far, we do not use this very, very ugly work around. Instead, we forbid to compile
 * the code with optimization off. Nonetheless, who will ever know or understand under
 * which circumstances, e.g. which combination of optimization flags, GCC will again
 * generate this trash-code. This issue remains a severe risk! Consequently, at any change
 * of a compiler setting you will need to inspect the assembly listing file and
 * double-check that it is proper with respect of using (better not using) the stack frame
 * for this function (and all other suspend functions).\n
 *   Another idea would be the implementation of this function completely in assembly code.
 * Doing so, we have the new problem of calling assembly code as C functions. Find an
 * example of how to do in
 * file:///M:/SVNMainRepository/Arduino/RTuinOS/trunk/RTuinOS/code/RTOS/rtos.c, revision 215.
 */
#ifndef __OPTIMIZE__
# error This code must not be compiled with optimization off. See source code comments for more
#endif

volatile uint16_t rtos_suspendTaskTillTime(uintTime_t deltaTimeTillRelease)
{
    /* This function is a pseudo-software interrupt. A true interrupt had reset the global
       interrupt enable flag, we inhibit any interrupts now. */
    asm volatile
    ( "cli \n\t"
    );
    
    /* The program counter as first element of the context is already on the stack (by
       calling this function). Save rest of context onto the stack of the interrupted
       active task. */ 
    PUSH_CONTEXT_WITHOUT_R24R25_ONTO_STACK

    /* Here, we could double-check _activeTaskId for the idle task ID and return without
       context switch if it is active. (The idle task has illicitly called a suspend
       command.). However, all implementation rates performance higher than failure
       tolerance, and so do we here. */
       
    /* The actual implementation of the task switch logic is placed into a sub-routine in
       order to benefit from the compiler generated stack frame for local variables (in
       this naked function we must not have declared any). The call of the function is
       immediately followed by some assembly code which processes the return value of the
       function, found in register pair r25/25. */
    suspendTaskTillTime(deltaTimeTillRelease);
    
    /* Switch the stack pointer to the (saved) stack pointer of the new active task and
       push the function result onto the new stack - from where it is loaded into r24/r25
       by the subsequent pop-context command. */
    SWITCH_CONTEXT
    PUSH_RET_CODE_OF_CONTEXT_SWITCH

    /* The stack pointer points to the now active task (which will often be still the same
       as at function entry). The CPU context to continue with is popped from this stack. If
       there's no change in active task the entire routine call is just like any ordinary
       interrupt. */
    POP_CONTEXT_FROM_STACK
    
    /* The global interrupt enable flag is not saved across task switches, but always set
       on entry into the new or same context by using a reti rather than a ret. */
    asm volatile
    ( "reti \n\t"
    );
    
} /* End of rtos_suspendTaskTillTime. */




/**
 * Actual implentation of task suspension routine \a rtos_waitForEvent. The task is
 * suspended until a specified event occurs.\n
 *   The action of this SW interrupt is placed into an own function in order to let the
 * compiler generate the stack frame required for all local data. (The stack frame
 * generation of the SW interupt entry point needs to be inhibited in order to permit the
 * implementation of saving/restoring the task context).
 *   @return
 * The function determines which task is to be activated and records which task is left
 * (i.e. the task calling this routine) in the global variables _activeTaskId and
 * _suspendedTaskId.
 *   @param eventMask
 * See software interrupt \a rtos_waitForEvent.
 *   @param all
 * See software interrupt \a rtos_waitForEvent.
 *   @param timeout
 * See software interrupt \a rtos_waitForEvent.
 *   @see
 * uint16_t rtos_waitForEvent(uint16_t, bool, uintTime_t)
 *   @remark
 * This function and particularly passing the return codes via a global variable will
 * operate only if all interrupts are disabled.
 */ 

static void waitForEvent(uint16_t eventMask, bool all, uintTime_t timeout)
{
    /* Avoid inlining under all circumstances. */
    asm("");
    
    int8_t idxPrio;
    uint8_t idxTask;
    
    /* Take the active task out of the list of due tasks. */
    rtos_task_t *pT = &rtos_taskAry[_activeTaskId];
    uint8_t prio = pT->prioClass;
    uint8_t noDueNow = -- _noDueTasksAry[prio];
    for(idxTask=0; idxTask<noDueNow; ++idxTask)
        _dueTaskIdAryAry[prio][idxTask] = _dueTaskIdAryAry[prio][idxTask+1];
    
    /* This suspend command want a reactivation by a combination of events (which may
       include the timeout event).
         ++timeout: The call of the suspend function is in no way synchronized with the
       system clock. We define the delay to be a minimum and implement the resolution
       caused uncertainty as an additional delay. */
    if((uintTime_t)(timeout+1) != 0)
        ++ timeout;
    pT->cntDelay = timeout;
    pT->eventMask = eventMask;
    pT->waitForAnyEvent = !all;
    
    /* Put the task in the list of suspended tasks. */
    _suspendedTaskIdAry[++_noSuspendedTasks] = _activeTaskId;

    /* Record which task suspends itself for the assembly code in the calling function
       which actually switches the context. */
    _suspendedTaskId = _activeTaskId;
    
    /* Look for the task we will return to. It's the first entry in the highest non-empty
       priority class. The loop requires a signed index.
         It's not guaranteed that there is any due task. Idle is the fallback. */
    _activeTaskId = IDLE_TASK_ID;
    for(idxPrio=RTOS_NO_PRIO_CLASSES-1; idxPrio>=0; --idxPrio)
    {
        if(_noDueTasksAry[idxPrio] > 0)
        {
            _activeTaskId = _dueTaskIdAryAry[idxPrio][0];
            break;
        }
    }
    
    
} /* End of waitForEvent */




/**
 * Suspend the current task (i.e. the one which invokes this method) until a specified
 * combination of events occur.\n
 *   A task is suspended in the instance of calling this method. It specifies a list of
 * events. The task becomes due again, when either the first one or all of the specified
 * events have been posted by other tasks.
 *   @return
 * The event mask of resuming events is returned. See \a rtos.h for a list of known events.
 *   @param eventMask
 * The bit vector of events to wait for. Needs to include the delay timer event
 * RTOS_EVT_DELAY_TIMER, if a timeout is required.
 *   @param all
 * If true, the task is made due only if all events are posted.\n
 *   CAUTION: Due to a specification error, this flag can be reasonable set in only in
 * combination of \a not specifying a timeout. Otherwise the activation condition would be
 * to wait for all events and to wait for the timeout being elapsed -- which is surely not
 * what you mean with a timeout. Waiting for any event with timeout is obviously not a
 * problem.
 *   @param timeout
 * The number of system timer tics from now on until the timeout elapses. One should be
 * aware the resolution of any timing is the tic of the system timer. A timeout of n may
 * actually mean any delay in the range n..n+1 tics.\n
 *   Even specifying 0 will suspend the task a short time and give others the chance to
 * become active.\n
 *   If RTOS_EVT_DELAY_TIMER is not set in the event mask, this parameter doesn't matter.
 *   @see uint16_t rtos_suspendTaskTillTime(uintTime_t)
 *   @remark
 * It is absolutely essential that this routine is implemented as naked and noinline. See
 * http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html for details
 *   @remark
 * In optimization level 0 GCC has a problem with code generation for naked functions. See
 * function \a rtos_suspendTaskTillTime for details.
 */
#ifndef __OPTIMIZE__
# error This code must not be compiled with optimization off. See source code comments for more
#endif

volatile uint16_t rtos_waitForEvent(uint16_t eventMask, bool all, uintTime_t timeout)
{
    /* This function is a pseudo-software interrupt. A true interrupt had reset the global
       interrupt enable flag, we inhibit any interrupts now. */
    asm volatile
    ( "cli \n\t"
    );
    
    /* The program counter as first element of the context is already on the stack (by
       calling this function). Save rest of context onto the stack of the interrupted
       active task. */ 
    PUSH_CONTEXT_WITHOUT_R24R25_ONTO_STACK

    /* Here, we could double-check _activeTaskId for the idle task ID and return without
       context switch if it is active. (The idle task has illicitly called a suspend
       command.). However, all implementation rates performance higher than failure
       tolerance, and so do we here. */
       
    /* The actual implementation of the task switch logic is placed into a sub-routine in
       order to benefit from the compiler generated stack frame for local variables (in
       this naked function we must not have declared any). The call of the function is
       immediately followed by some assembly code which processes the return value of the
       function, found in register pair r25/25. */
    waitForEvent(eventMask, all, timeout);
    
    /* Switch the stack pointer to the (saved) stack pointer of the new active task and
       push the function result onto the new stack - from where it is loaded into r24/r25
       by the subsequent pop-context command. */
    SWITCH_CONTEXT
    PUSH_RET_CODE_OF_CONTEXT_SWITCH

    /* The stack pointer points to the now active task (which will often be still the same
       as at function entry). The CPU context to continue with is popped from this stack. If
       there's no change in active task the entire routine call is just like any ordinary
       interrupt. */
    POP_CONTEXT_FROM_STACK
    
    /* The global interrupt enable flag is not saved across task switches, but always set
       on entry into the new or same context by using a reti rather than a ret. */
    asm volatile
    ( "reti \n\t"
    );
    
} /* End of rtos_waitForEvent */




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


/* setEvent: Needs to push all, as it doesn't suspend a task. It can however yield a task
   switch as the set event may release a task of higher priority. */


/*
TODO: Implement basic suspend (absolute timer)
      Implement initialization of data structures
      ... and we can already make it run.
      Implement other suspends and postEvent
*/