/*====================================================================
======================================================================
	SENDS SMPTE LTC SIGNAL VIA DIFFERENTIAL MANCHESTER ENCODING 
	[BI-PHASE MARK CODE]
	29.97fps (30 * 1000/1001) Version
    Version 1.0:  Reads & Stores 29.97 NDF Timecode
======================================================================
======================================================================
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <string.h>
#include "LTC.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++ GLOBAL SYMBOLS ++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Timecode Values Variables
volatile unsigned char frames = 0, seconds = 0, minutes = 0, hours = 0;
volatile unsigned char userbits[8] = {2,4,6,8,1,3,5,7};
static unsigned char sections[10]; //10 8-bit sections to make up 80-bit LTC block
volatile unsigned char parity_bit = 0; //Even Parity

//Timecode Generator Variables
volatile unsigned char midbit_send = 1;
volatile unsigned char sendbit = 0;
volatile unsigned char sendsection = 0;
volatile unsigned char sendsignal = 0b00000010; //For PORTB1
volatile unsigned char i = 0;
volatile unsigned int debugbit = 0;

//Timecode Reader Variables
volatile unsigned in frame_subcount = 0;  //Counts to "FRAME_MIDBITCOUNT" to display Frame

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++ MAIN ENTRY ++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int main(void)
{
	//Sleep Mode and Power Management Setup
	SMCR |= 0b00000001; //Enable SE(sleep enable) bit and Idle Sleep Mode
	//PRR |=  ; //Power Reduction Register
    MCUCR |= 0b01100000; //The following two commands work to disable Brown-Out detection (BOD)
	MCUCR ^= 0b00100000;
	DIDR1 |= 0b00000011; //Disable Analog Comparator Pins AIN1/AIN0 Digital Input
	DIDR0 |= 0b00000000; //Disable ADC Pins ADC0D/ADC1D/ADC2D/ADC3D/ADC4D/ADC5D Digital Input
	ACSR |= 0b10000000; //Disable Analog Comparator

	//Timer Setup
	GTCCR = (1 << TSM) | (1 << PSRASY) | (1 << PSRSYNC); //Reset Prescaler and halts Timer0/1/2 so they can be synced when TSM bit cleared
    //Timer1 Output Compare Match Setup
        TCCR1A = 0x00; //0b00000000;  OC1A disconnected from Timer
        TCCR1B |= (1 << WGM12); //CTC (Compare Clear) Set to recount once Compare Match made
        TCCR1B |= (1 << CS10); 	//Start Timer by setting clock source
        TIMSK1 |= (1 << OCIE1A); //Timer/Counter1 Output Compare A Interrupt Enabled
        OCR1A = MIDBIT_CLOCKPERIOD; //Set Output Compare Value
        TCNT1 = 0x0000; //Set 16-bit Counter to 0
    //Timer1 Input Capture Setup

	//PIN Setup
    //Generator Pin
        DDRB |= (1 << DDB1); //Set PB1 Pin as output
        PORTB = 0b00000000;	//Generates LTC
    
    //Enable Global Interrupt Flag
	sei(); //Enable Global Interrupts
    
    //Start all Timers
    GTCCR = (0 << TSM); //Start Timers
    
    //Generator Preset (starting at zero time [HIGH], so that next period is the mid-bit)
    PORTB ^= sendsignal; //Start Generator LTC High at top of signal
    smpte_signalGenerate(); //Presets for first mid-bit value
    
	while(1)
	{
		sleep_mode();	//Go into Idle Sleep Mode until Interrupt occurs
	}
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++ INTERRUPTS ++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//GENERATOR & DISPLAY: Mid-bit (of the 80 LTC bits) Timer Compare Interrupt
ISR(TIMER1_COMPA_vect)
{
    //4 cycles from Main to Here
    
    //GENERATOR
    PORTB ^= sendsignal;     //XOR with sendsignal.  Will switch when one in its place in sendsignal
    smpte_signalGenerate();     //Setup Next Generator Signal midbit value

    //DISPLAY
    frame_subcount++;
    if (frame_subcount < FRAME_MIDBITCOUNT)
    {
        frame_subcount = 0; //Reset Frame Subcount Counter
        
        //Display Code Here
        display_smpte();
    }
}

//READER: Captures input and overrides current timer
}

ISR(TIMER1_CAPT_vect)
{
    //4 cycles from Main to Here
    
    //Flash Red Led to signal incoming Signal

    //Read Jammed Timecode and store
    //Upon store in Timecode Sections, reset smpte_signalGenerate values to be ready to send from start
    //...of frame
    readJam_smpte();
    
    //Flash Green Led to signal Jam Sync Lock
    
}

