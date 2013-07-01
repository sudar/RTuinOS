/**
 * @file tc14_adcInput.c
 *
 * Test case 14 of RTuinOS. A user interrupt is applied to pick the results of an analog
 * input channel, which is running in regular, hardware triggered Auto Trigger Mode.
 *   It could seem to be straight forward, to use the timing capabilities of an RTOS to
 * trigger the conversions of an ADC, a regular task would be used to do so. However,
 * signal processing of fluctuating input signals by regular sampling the input suffers
 * from incorrect timing. Although in mean the timing of a regular task is very precise, the
 * actual points in time, when a task is invoked are not precisely equidistant. The
 * invokations may be delayed by an arbitrary, fluctuating tiny time span. This holds true
 * even for a task of high priority -- even if here the so called jitter will be less. If
 * the signal processing assumes regular sampling of the input but actually does do this
 * with small time shifts, it will see an error, which is approximately equal to the first
 * derivative of the input signel times the time shift. The latter is a random quantity so
 * the error also is a random quantity proportional to the derivative of the input signal.
 * In the frequency domain this mean that the expected error increases linearly with the
 * input frequency. Consequently, task triggered ADC conversions must be used only for
 * slowly changing input signals, it might e.g. be adequate for reading a temperature
 * input. All other applications need to trigger the conversions by a sofwtare independent,
 * accurate hardware signal. The software becomes a slave of this hardware trigger.
 *   This RTuinOS sample application uses timer/counter 0 in the unchanged Arduino standard
 * configuration to trigger the conversions of the ADC. The overflow interrupt is used for
 * this purpose yielding a conversion rate of about 1221 Hz. A task of high priority is
 * awaked on each conversion-complete event and reads the conversion result. The read
 * values are down-sampled and passed to a much slower secondary task, which prints them to
 * the Arduino console window.
 *   Proper down-sampling is a CPU time consuming operation, which is hard to implement on
 * a tiny eight Bit controller. Here we use the easiest possible to implement filter with
 * rectangular impulse response. It adds the last recent N input values and divides the
 * reuslt by N. We exploit the fact, that we have 10 Bit ADC values but use a 16 Bit
 * arithemtics anyway: We can safely sum up up to 64 values without any danger of overflow.
 * The division by N=64 is not necessary at all; this constant value just changes the
 * scaling of the result (i.e. the scaling binary value to Volt), which has to be
 * considered for any output operation anyway. It doesn't matter to this "consider" which
 * scaling we actually have, it's just another constant to use.
 *   Regardless of the poor damping of this filter we use it to reduce the task rate by 32.
 * (TBC)
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
 *
 * Module interface
 *   setup
 *   loop
 * Local functions
 *   blink
 */

/*
 * Include files
 */

#include <arduino.h>
#include "rtos.h"
#include "stdout.h"
#include "gsl_systemLoad.h"


/*
 * Defines
 */
 
/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13
 
/** The input of the analog to digital converter. Either an Arduino analog input or the
    internal reference voltage of 1.1 V for testing. (If you measure this using an internal
    reference you just believe to have a good accuracy.) Select a none existing analog
    input (>=16) to select the internal 1.1 V source as input.
      @remark This macro is used within macro expression #VAL_MUX with double evaluation.
    Just define simple literals without side effects here. */
#define ADC_INPUT   9


/** ADMUX/REFS1:0: Reference voltage or full scale respectively. 1 means Ucc=5V, 2 means
    1.1 V and 3 means 2.56 V. The internal references (1.1 V and 2.56 V are related to each
    other and undergo the same errors. The accuracy of these reference voltages is poor
    (about 5% deviation), the stabilized operational voltage seems to be more accurate. */
#define VAL_REFS    1    

/** The reference voltage as floating point value for scaling purpose. */
#if VAL_REFS == 1
# define U_REF 5.0
#elif VAL_REFS == 2
# define U_REF 1.1
#elif VAL_REFS == 3
# define U_REF 2.56
#else
# error Illegal value for ADMUX/REFS (External reference is not supported)
#endif
 

/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
 
/*
 * Data definitions
 */
 
 
/*
 * Function implementation
 */


/**
 * Trivial routine that flashes the LED a number of times to give simple feedback. The
 * routine is blocking.
 *   @param noFlashes
 * The number of times the LED is lit.
 */
static void blink(uint8_t noFlashes)
{
#define TI_FLASH 150
    while(noFlashes-- > 0)
    {
        digitalWrite(LED, HIGH);  /* Turn the LED on. (HIGH is the voltage level.) */
        delay(TI_FLASH);          /* The flash time. */
        digitalWrite(LED, LOW);   /* Turn the LED off by making the voltage LOW. */
        delay(TI_FLASH);          /* Time between flashes. */
    }                              
    delay(1000-TI_FLASH);         /* Wait for a second after the last flash - this command
                                     could easily be invoked immediately again and the
                                     bursts need to be separated. */
#undef TI_FLASH
}



/**
 * The initalization of the RTOS tasks and general board initialization.
 */ 

void setup(void)

{
    /* Start serial port at 9600 bps. */
    Serial.begin(9600);
    //Serial.println("\n" RTOS_RTUINOS_STARTUP_MSG);
    init_stdout();
    puts_progmem(rtos_rtuinosStartupMsg);

    /* Initialize the digital pin as an output. The LED is used for most basic feedback about
       operability of code. */
    pinMode(LED, OUTPUT);
    
//    /* Setup the ADC configuration. */
//    printf( "At startup:\n"    
//            "  ADMUX  = 0x%02x\n"
//            "  ADCSRB = 0x%02x\n"
//          , ADMUX 
//          , ADCSRB
//          );

    /* ADMUX */
#define VAL_ADLAR   0    /* ADLAR: Result must not be left aligned. */

/** The setting for register MUX is derived from the channel number or it selects the
    internal reference voltage of 1.1 V. */
#if ADC_INPUT >= 0  &&  ADC_INPUT < 16
# define VAL_MUX ((((ADC_INPUT) & 0x8) << 2) + ((ADC_INPUT) & 0x7))
#else
# define VAL_MUX 0x1e
#endif

    ADMUX = (VAL_REFS << 6)    
            + (VAL_ADLAR << 5)
            + ((VAL_MUX & 0x1f) << 0);

#undef VAL_REFS
#undef VAL_ADLAR

    /* ADCSRB */
#define VAL_ACME    0   /* Don't allow analog comparator to use ADC multiplex inputs. */
#define VAL_ADTS    4   /* Auto trigger source is Timer/Counter 0, Overflow, 1221 Hz. */

    ADCSRB = (((VAL_MUX & 0x20) != 0) << 3)
             + (VAL_ADTS << 0);
             
#undef VAL_MUX
#undef VAL_ACME
#undef VAL_ADTS
    
    /* ADCSRA */
#define VAL_ADEN 1 /* Turn ADC on. */
#define VAL_ADSC 1 /* Start series of conversion; may be done in same register write access. */
#define VAL_ADATE 1 /* Turn auto triggering on to minimize jitter in conversion timing. */ 
#define VAL_ADIF 1 /* Reset the "conversion-ready" flag by writing a one. */
#define VAL_ADIE 0 /* Do not allow interrupts on conversion-ready. */
#define VAL_ADPS 7 /* ADPS2:0: Prescaler needs to generate lowest possible frequency. */ 
    
    /* The regular conversions are running after this register write operation. */
    ADCSRA = (VAL_ADEN << 7)
             + (VAL_ADSC << 6)
             + (VAL_ADATE << 5)
             + (VAL_ADIF << 4)
             + (VAL_ADIE << 3)
             + (VAL_ADPS << 0);
            
#undef VAL_ADEN
#undef VAL_ADSC
#undef VAL_ADATE 
#undef VAL_ADIF
#undef VAL_ADIE
#undef VAL_ADPS
} /* End of setup */




/**
 * The application owned part of the idle task. This routine is repeatedly called whenever
 * there's some execution time left. It's interrupted by any other task when it becomes
 * due.
 *   @remark
 * Different to all other tasks, the idle task routine may and should return. (The task as
 * such doesn't terminate). This has been designed in accordance with the meaning of the
 * original Arduino loop function.
 */ 

void loop(void)

{
    blink(1);
    Serial.println("RTuinOS is idle");
    
    /* Just read ADCL then ADCH and print result. */
    // @todo Consider conversion-ready flag. */
    uint16_t adcResult = ADCL + (ADCH<<8);
    printf("ADC result: %.5f V\n", U_REF/1024.0 * adcResult); 
    
    printf("CPU load: %.1f %%\n", gsl_getSystemLoad()/2.0);
    
} /* End of loop */




