/**
 * @file clk_clock.c
 *   Implementation of a real time clock for a sample task.
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
/* Module interface
 * Local functions
 */

/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"
#include "rtos_assert.h"
#include "aev_applEvents.h"
#include "tc14_adcInput.h"
#include "dpy_display.h"
#include "clk_clock.h"


/*
 * Defines
 */
 
/** The standard RTuinOS clock tic on the Arduino Mega 2560 board is 1/(16MHz/64/510) =
    51/25000s. We add 51 in each tic and have the next second when we reach 25000. This
    permits a precise implementation of the real time clock even with 16 Bit integer
    arithmetics. See #CLOCK_TIC_DENOMINATOR also. */
#define CLOCK_TIC_NUMERATOR     51

/** Denominator of ratio, which implements the clock's task rate. See #CLOCK_TIC_NUMERATOR
    for details. */
#define CLOCK_TIC_DENOMINATOR   (25000-(CLOCK_TIC_DENOMINATOR_TRIM_TERM))
 
/** Trim term for clock: By long term observation a correction has been figured out, which
    makes the clock significantly more accurate. The term is defined as addend to the
    denominator #CLOCK_TIC_DENOMINATOR. It is device dependent. Starting point on a new
    hardware device should be 0. Positive values advance the clock. */
#define CLOCK_TIC_DENOMINATOR_TRIM_TERM 59

/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
 
/*
 * Data definitions
 */

/* Accumlator for task tics which generates a precise one second clock. */
static uint16_t _noTaskTics = 0;

/** Counter of seconds. */ 
uint8_t clk_noSec = 0;

/** Counter of minutes. */ 
uint8_t clk_noMin = 30;

/** Counter of hours. */ 
uint8_t clk_noHour = 22;


/*
 * Function implementation
 */

/**
 * The regular task function of the real time clock. Has to be called every 100th tic of
 * the RTuinOS system time, which is running in its standard copnfiguration of about 2 ms a
 * tic.
 */
void clk_taskRTC()
{
    _noTaskTics += CLK_TASK_TIME_RTUINOS_STANDARD_TICS*CLOCK_TIC_NUMERATOR;
    if(_noTaskTics >= CLOCK_TIC_DENOMINATOR)
    {
        _noTaskTics -= CLOCK_TIC_DENOMINATOR;
        
        if(++clk_noSec > 59)
        {
            clk_noSec = 0;
            if(++clk_noMin > 59)
            {
                clk_noMin = 0;
                if(++clk_noHour > 23)
                    clk_noHour = 0;
            }
        }       
        
        /* We display hh:mm:ss, so a change of the seconds leads to a write into the
           display. */
        dpy_display.printTime(clk_noHour, clk_noMin, clk_noSec);
    }
} /* End of clk_taskRTC */