#ifndef ADC_ANALOGINPUT_INCLUDED
#define ADC_ANALOGINPUT_INCLUDED
/**
 * @file adc_analogInput.h
 * Definition of global interface of module adc_analogInput.c
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

#include <Arduino.h>


/*
 * Defines
 */

/** Do not change: The ADC input which the buttons of the LCD shield are connected to. */
#define ADC_INPUT_LCD_SHIELD_BUTTONS    0


/** The number of sub-sequent ADC conversion results, which are avaraged before the mean
    value is passed to the waiting client tasks. The values 1..64 are possible. The smaller
    the value the higher the overhead of the task processing. */
#define ADC_NO_AVERAGED_SAMPLES     64


/*
 * Global type definitions
 */


/*
 * Global data declarations
 */

/** Global counter of all ADC conversion results starting with system reset. The frequency
    should be about 960 Hz.
      @remark The values are written by the ADC task without access synchronization. They
    can be safely read only by tasks of same or lower priority and the latter need a
    critical section to do so. */
extern uint32_t adc_noAdcResults;


/** The user selected ADC input. Caution, the value is expected as can be written directly
    into the ADC register MUX5:0. The numbers of the normal inputs in the range 0..15 are
    split by two inserted null bits at position b3 and b4 and the internal band gap
    reference is selected by the constant 0x1e.
      @remark The values are read by the ADC task without access synchronization. They
    can be safely written only by tasks of same or lower priority and the latter need a
    critical section to do so. */
extern uint8_t adc_userSelectedInput;


/** The voltage measured at analog input #ADC_INPUT_LCD_SHIELD_BUTTONS which the buttons of
    the LCD shield are connected to. Scaling: worldValue = 5/1024/#ADC_NO_AVERAGED_SAMPLES
    * \a adc_buttonVoltage [V].
      @remark The values are written by the ADC task without access synchronization. They
    can be safely read only by tasks of same or lower priority and the latter need a
    critical section to do so. */
extern uint16_t adc_buttonVoltage;


/** The voltage measured at the user selected analog input, see \a adc_userSelectedInput.
    Scaling: worldValue = 5/1024/#ADC_NO_AVERAGED_SAMPLES * \a adc_inputVoltage [V].
      @remark The values are written by the ADC task without access synchronization. They
    can be safely read only by tasks of same or lower priority and the latter need a
    critical section to do so. */
extern uint16_t adc_inputVoltage;


/*
 * Global prototypes
 */

/* Main function of ADC task. Process next conversion result. */
void adc_onConversionComplete(uint16_t adcResult);


#endif  /* ADC_ANALOGINPUT_INCLUDED */
