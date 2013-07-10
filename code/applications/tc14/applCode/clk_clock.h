#ifndef CLK_CLOCK_INCLUDED
#define CLK_CLOCK_INCLUDED
/**
 * @file clk_clock.h
 * Definition of global interface of module clk_clock.c
 *
 * Copyright (C) 2013 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
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

/** To be used to configure the regular task of the real time clock implementation: The
    task has to be called every #CLK_TASK_TIME_RTUINOS_STANDARD_TICS RTuinOS standard
    system clock tics.
      @remark The chosen value doesn't strongly matter. It must not be out of the range of
    the data type of the system time, i.e. not greater than 127. The greater it is the
    lower is the overhead of the RTOS, but if it gets to large the update of the display
    might become visibly irregular. We choose a crude value to demonstrate the don't matter
    character. */
#define CLK_TASK_TIME_RTUINOS_STANDARD_TICS 123


/*
 * Global type definitions
 */


/*
 * Global data declarations
 */
 
/** Counter of seconds. */ 
extern uint8_t clk_noSec;

/** Counter of minutes. */ 
extern uint8_t clk_noMin;

/** Counter of hours. */ 
extern uint8_t clk_noHour;


/*
 * Global prototypes
 */

/** Regular task function for real time clock support. */
void clk_taskRTC(void);


#endif  /* CLK_CLOCK_INCLUDED */
