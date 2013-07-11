/**
 * @file adc_analogInput.cpp
 *   The ADC task code: Process the analog input.
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
 *   adc_onConversionComplete
 * Local functions
 *   selectAdcInput
 */

/*
 * Include files
 */

#include <Arduino.h>
#include "rtos.h"
#include "aev_applEvents.h"
#include "adc_analogInput.h"


/*
 * Defines
 */
 
 
/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
 
/*
 * Data definitions
 */
 
/** Global counter of all ADC conversion results starting with system reset. The frequency
    should be about 960 Hz. */
uint32_t adc_noAdcResults = 0;
 
 /** User selection of ADC's multiplexer input. The value is compatible with the ADC
     register MUX. The initial selection needs to be the button voltage, don't change it! */
uint8_t adc_userSelectedInput = ADC_INPUT_LCD_SHIELD_BUTTONS;

/** The voltage at the buton input. */
uint16_t adc_buttonVoltage = 0;


/** The voltage measured at the user selected analog input, see \a adc_userSelectedInput. */
uint16_t adc_inputVoltage = 0;


/*
 * Function implementation
 */



/**
 * Reprogram the ADC so that the next conversion will use another input.
 *   @param input
 * The input to select as ADC register value MUX5:0.
 */ 

static void selectAdcInput(uint8_t input)
{
    
    
} /* End of selectAdcInput */




/**
 * The main function of the ADC task: It gets every new input sample and processes it.
 * Processing means to do some averaging as a kind of simple down sampling and notify the
 * sub-sequent, slower running clients of the data.\n
 *   There are two kinds of data and two related clients: The analog input 0, which the LCD
 * shield's buttons are connected to, is read regularly and the input values are passed to
 * the button evaluation task, which implements the user interface state machine.\n
 *   A user selected ADC input is measured and converted to Volt. The client of this
 * information is a simple display task.
 *   @param 
 * The next conversion value as raw, right aligned binary 10 Bit word right from the ADC
 * register. Passed from the conversion-complete interrupt into this function.
 */

void adc_onConversionComplete(uint16_t adcResult)
{
    /* We need a status machine here. There are two alternating series of conversions: Read
       the LCD shield's button input every first time and the "true", user selected input
       every second time.
         Averaging: Each series accumulates 64 samples. */
    static bool readButton_ = true;
    static uint16_t accumuatedAdcResult_ = 0;
    static uint8_t noMean_ = ADC_NO_AVERAGED_SAMPLES;
    
    /* Accumulate all samples of the runnign series. */
    accumuatedAdcResult_ += adcResult;

    /* Accumulate up to 64 values to do averaging and anti-aliasing for slower
       reporting task. */
    if(--noMean_ == 0)
    {
        /* A new down-sampled result is available for one of our clients. Since the clients
           have a lower priority as this task we don't need a critical section to update
           the client's input. */
        if(readButton_)
        {
            /* New ADC input is the user selected input. We do this as early as possible in
               the processing here, to have it safely completed before the next conversion
               start interrupt fires. */
            selectAdcInput(adc_userSelectedInput);
            
            /* Notify the new result to the button evaluation task. */
            adc_buttonVoltage = accumuatedAdcResult_;
            rtos_setEvent(EVT_TRIGGER_TASK_BUTTON);
        }
        else
        {
            /* New ADC input is the user selected input. We do this as early as possible in
               the processing here, to have it safely completed before the next conversion
               start interrupt fires. */
            selectAdcInput(ADC_INPUT_LCD_SHIELD_BUTTONS);
           
            /* Notify the new result to the button evaluation task. */
            adc_inputVoltage = accumuatedAdcResult_;
            rtos_setEvent(EVT_TRIGGER_TASK_DISPLAY_VOLTAGE);
        }
        
        /* Start next series on averaged samples. */
        readButton_ = !readButton_;
        noMean_ = ADC_NO_AVERAGED_SAMPLES;
        accumuatedAdcResult_ = 0;
    }
        
    /* Count the read cycles. The frequency should be about 960 Hz. */
    ++ adc_noAdcResults;

} /* End of adc_onConversionComplete */




