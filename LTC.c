#include "LTC.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++ EXTERNALLY DEFINED GLOBALS +++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Timecode Value Variables
extern volatile unsigned char frames;
extern volatile unsigned char seconds;
extern volatile unsigned char minutes;
extern volatile unsigned char hours;
extern volatile unsigned char userbits[8];
extern volatile unsigned char sections[10];
extern volatile unsigned char parity_bit;
extern volatile unsigned char i;

//Timecode Generator Variables
extern volatile unsigned char midbit_send;
extern volatile unsigned char sendbit;
extern volatile unsigned char sendsection;
extern volatile unsigned char sendsignal;
extern volatile unsigned int debugbit;

//Timecode Reader Variables
extern volatile unsigned char frame_subcount;
extern volatile unsigned char current_pin;
extern volatile unsigned char previous_pin;
extern volatile unsigned char jamDetect;
extern volatile unsigned char midbitBoundary;
extern volatile unsigned char jamSync;
extern volatile unsigned char phaseSync;
extern volatile unsigned char changeDetect;
extern volatile unsigned char codewordFound;
extern volatile unsigned char ltcBit;
extern volatile unsigned char ltcBitCount;
extern volatile unsigned char syncWordBufferA;
extern volatile unsigned char syncWordBufferB;
extern volatile unsigned char reverseSignal;
extern volatile unsigned char tempSections[10];

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++ SUBROUTINES +++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void smpte_increment()	//Increments LTC
{
    //FRAMES
    frames++;
    if ((frames & 0x0F) > 9)  //CHECKS FRAME UNITS
    {
        frames += 6;
    }
    if (frames < TC_FR)  //CHECKS FRAME TENS
    {
        return;
    }
    else           //IF FRAMES ARE GREATER THAN 30, INCREMENT SECONDS
    {
        frames = 0;
        
        //SECONDS
        seconds++;
        if ((seconds & 0x0F) > 9)  //CHECKS SECONDS UNITS
        {
            seconds += 6; //ADDING 0b0110 (6) to 0b1010 (10) clears SECONDS UNITS and adds 0b0001 to SECONDS TENS
        }
        if (seconds < 0x60)  //CHECKS SECONDS TENS
        {
            return;
        }
        else            //IF SECONDS ARE GREATER THAN 60, INCREMENT MINUTES
        {
            seconds = 0;
            
            //MINUTES
            minutes++;
            if ((minutes & 0x0F) > 9)  //CHECKS MINUTES UNITS
            {
                minutes += 6;  //ADDING 0b0110 (6) to 0b1010 (10) clears MINUTES UNITS and adds 0b0001 to MINUTES TENS
            }
            if (minutes < 0x60)  //CHECKS MINUTES TENS
            {
                return;
            }
            else        //IF MINUTES ARE GREATER THAN 60, INCREMENT HOURS
            {
                minutes = 0;
                
                //HOURS
                hours++;
                if ((hours & 0x0F) > 9)  //CHECKS HOURS UNITS
                {
                    hours += 6; //ADDING 0b0110 (6) to 0b1010 (10) clears HOURS UNITS and adds 0b0001 to HOURS TENS
                }
                if (hours < 0x24)  //CHECKS HOURS TENS
                {
                    return;
                }
                else
                {
                    hours = 0;  //IF HOURS GO OVER 24, RESET TO MIDNIGHT
                }
            }
        }
    }
}

void smpte_signal() //Fills out the sections of a frame signal
{
    sections[0] = ( userbits[0] << 4 ) | ( frames & 0x0F);      //FRAME UNITS & BINARY GROUP 1
    sections[1] = ( userbits[1] << 4 ) | ( frames >> 4 );       //FRAME TENS, CF/DF FLAGS, & BINARY GROUP 2
    sections[2] = ( userbits[2] << 4 ) | ( seconds & 0x0F );    //SECONDS UNITS & BINARY GROUP 3
    sections[3] = ( userbits[3] << 4 ) | ( seconds >> 4 );      //SECONDS TENS, BMC POLARITY CORRECTION & BINARY GROUP 4
    sections[4] = ( userbits[4] << 4 ) | ( minutes & 0x0F );    //MINUTES UNITS & BINARY GROUP 5
    sections[5] = ( userbits[5] << 4 ) | ( minutes >> 4 );      //MINUTES TENS, BGF0, & BINARY GROUP 6
    sections[6] = ( userbits[6] << 4 ) | ( hours & 0x0F );      //HOURS UNITS & BINARY GROUP 7
    sections[7] = ( userbits[7] << 4 ) | ( hours >> 4 );        //HOURS TENS, BGF1, BGF2 & BINARY GROUP 8
    sections[8] = 0xFC;     //0b1111 1100  SYNC WORD FIRST HALF
    sections[9] = 0xBF;     //0b1011 1111  SYNC WORD SECOND HALF
    
    
    //BMC POLARITY CORRECTION BIT
    //Parity is checking here for number of ones and if total ones are odd then the total number of zeroes must be odd too.  In that case, we need to make
    //...the number of zeroes even by making the parity bit zero.  Remember, we're starting with the parity bit already set to zero, so if the total
    //...number of zeroes (and ones of course) is odd, we need to switch it to a 1 to make them even!
    //We are checking for EVEN PARITY here, so we start with even parity bit
    parity_bit = 0;
    for (i = 0; i < 8; i++)
    {
        //XOR all of the sections gives a final 8 bit number showing overall odd or even 1s (and zeroes of course since two odd and two even numbers make an even number)
        parity_bit ^= sections[i];
    }
    //Essentially XOR an even number of 1s gives us a zero.  XOR and odd number of 1s gives us a 1
    //We're just trying to XOR ALL bits in the 80-bit string (including the already zeroed BMC Polarity Correction bit).
    //If the XORing leaves us with a 1, then we have an odd number of both 1's and more importantly 0s (by SMPTE standards).  In that case, we'll
    //...turn the polarity correction bit to a 1, which will give us a total even number of 1s and 0s.  This will make the sync word in correct polarity (which
    //...is the SMPTE standard we're aiming for)
    // parity_bit = a.b.c.d.e.f.g.h
    parity_bit ^= parity_bit >> 4;  // parity_bit = x.x.x.x.a^e.b^f.c^g.d^h  (x = don't care bit)
    parity_bit ^= parity_bit >> 2;  // parity_bit = x.x.x.x.x.x.a^c^e^g.b^d^f^h
    parity_bit ^= parity_bit >> 1;  // parity_bit = x.x.x.x.x.x.x.a^b^c^d^e^f^g^h  (this is how be XOR all bits in the string)
    if (parity_bit & 1) //parity_bit & 0b00000001  will give 1 if last bit in parity_bit is 1
    {
        sections[3] |= 8;  //Switch it on with an OR
    }
    
}

void smpte_signalGenerate() //Sends LTC out on PIN
{
    //Setup next sendsignal
    if(midbit_send == 1)
    {

        //Setup next sendsignal with value of Bit (transition if 1, no transition if 0)
        //Pulls out current bit to read and places it into the PORTB1 bit of PORTB register
        sendsignal = ( ( ( sections[sendsection] << (7 - sendbit) ) & 0x80 ) >> 6 );
        
        //Increment bits and sections for next interrupt
        sendbit++;
        if(sendbit > 7)
        {
            sendsection++;
            sendbit = 0;
            if(sendsection > 9)
            {
                sendsection = 0;
                //Setup next SMPTE LTC frame
                smpte_increment();
                smpte_signal();
            }
        }
    }
    if(midbit_send == 0)
    {
        //Setup next sendsignal to toggle the PORTB1
        sendsignal =  0b00000010; //PORTB1 for XORing into PORTB register
    }
    
    //Swich midbit_send and go to sleep
    midbit_send ^= 1;
    
}

void display_smpte()
{
    frame_subcount++;
    if (frame_subcount < FRAME_MIDBITCOUNT)
    {
        frame_subcount = 0; //Reset Frame Subcount Counter
        
        //Display Code Here
        
    }
}

void readJam_smpte()
{
    //IF JAMSYNC VARIABLE NOT SET, INITIATE READER:  (ELSE IF SET, LEAVE READER)
    if (jamSync == 0)
    {
        //READER
        //Fill Reader Buffer with bits from incoming LTC until codeword found
        if (phaseSync == 1)   //If phaseSync is set
        {
            //Strobe Red LED to show trying to find CodeWord
            
            //We are on beginning of LTC bit, right after bit boundary
            //Find Frame and Sync Generator
            syncJam_smpte();
        }
        
        //Detect when mid-bit timer interrupt is in phase (or close enough for proper read)
        if ((jamDetect == 1) && (phaseSync == 0))  //If jamDetect is set from last mid-bit and phaseSync isn't set
        {
            
            current_pin = ((0b00100000 & PINC) >> 5); //Record current pin value
            changeDetect = ( current_pin ^ previous_pin );  //Check previous pin and current: see if changed
            
            if (changeDetect == 0) //If same as before, then we found a Zero and are in phase!
            {
                phaseSync = 1;  //set phaseSync variable
                midbitBoundary = 0; //Next read will be at bit-boundary, not mid-bit
                return; //Leave READER
            }
            
            if (changeDetect == 1) //Else if different:
            {
                //We are crossing over a bit boundary, or a "1" mid-bit
                //...We'll check again on the next mid-bit, until we get a Zero
                previous_pin = current_pin; //Set previous value to current value
                return;  //Leave READER
            }
            
            return;  //Leave READER
        }
        
        //Check to see if signal on Reader Pin
        if ((jamDetect == 0) && (phaseSync == 0)) //If jamDetect variable not set
        {
            
            current_pin = ((0b00100000 & PINC) >> 5); //Record current pin value
            changeDetect = ( current_pin ^ previous_pin ); //Check previous pin and current: see if changed
            
            if (changeDetect == 1) //If change found:
            {
                jamDetect = 1;  //set jamDetect variable to on
                previous_pin = current_pin;  //Set previous value to current value
                return; //Leave READER
            }
                
            return; //else leave READER
        }

    }
    
    if (jamSync ==1)
    {
        //Hold Green LED
    }
    
    return;
    
}

void syncJam_smpte()
{
    if (codewordFound == 1)
    {
        //Strobe Green LED now to show syncing
        //Strobe Both Green & Red if syncing and a reverse signal
        
        if ((midbitBoundary == 0)) //At beginning of a bit
        {
            current_pin = ((0b00100000 & PINC) >> 5);  //Record Current Pin Value
            previous_pin = current_pin;  //Set previous value to current value
        }
        
        if ( (midbitBoundary == 1) && (reverseSignal == 0) ) //In last half of bit
        {
            current_pin = ((0b00100000 & PINC) >> 5);  //Record Current Pin Value
            changeDetect = ( current_pin ^ previous_pin ); //Check previous pin and current: see if changed
            
            //Read bit into tempSections Buffer
            //Sections Structure:
            //  Read out from LSB(bit0)-->MSB(bit7), from section 0 --> 9
            //Store "1" or "0" LTC bit  in Codeword Buffer
            ltcBit = changeDetect & 0b00000001; //set ltcBit.  Make sure only LSB is active
            //Queue Machine!  In 80-bits worth of time, we'll have our first frame bit in tempSection[0] bit0
            tempSections[0] = ( ((tempSections[0] >> 1) & 0b01111111) | ((tempSections[1] << 7) & 0b10000000) );  //FRAME UNITS & BINARY GROUP 1
            tempSections[1] = ( ((tempSections[1] >> 1) & 0b01111111) | ((tempSections[2] << 7) & 0b10000000) );  //FRAME TENS, CF/DF FLAGS, & BINARY GROUP 2
            tempSections[2] = ( ((tempSections[2] >> 1) & 0b01111111) | ((tempSections[3] << 7) & 0b10000000) );  //SECONDS UNITS & BINARY GROUP 3
            tempSections[3] = ( ((tempSections[3] >> 1) & 0b01111111) | ((tempSections[4] << 7) & 0b10000000) );  //SECONDS TENS, BPC & BINARY GROUP 4
            tempSections[4] = ( ((tempSections[4] >> 1) & 0b01111111) | ((tempSections[5] << 7) & 0b10000000) );  //MINUTES UNITS & BINARY GROUP 5
            tempSections[5] = ( ((tempSections[5] >> 1) & 0b01111111) | ((tempSections[6] << 7) & 0b10000000) );  //MINUTES TENS, BGF0, & BINARY GROUP 6
            tempSections[6] = ( ((tempSections[6] >> 1) & 0b01111111) | ((tempSections[7] << 7) & 0b10000000) );  //HOURS UNITS & BINARY GROUP 7
            tempSections[7] = ( ((tempSections[7] >> 1) & 0b01111111) | ((tempSections[8] << 7) & 0b10000000) );  //HOURS TENS, BGF1, BGF2 & BINARY GROUP 8
            tempSections[8] = ( ((tempSections[8] >> 1) & 0b01111111) | ((tempSections[9] << 7) & 0b10000000) );  //SYNC WORD FIRST HALF
            tempSections[9] = ( ((tempSections[9] >> 1) & 0b01111111) | ((ltcBit << 7) & 0b10000000) );  //SYNC WORD SECOND HALF
            
            ltcBitCount++; //Increment ltcBit [0-79 for each of the 80 LTC Bits]
        }
        
        if ( (midbitBoundary == 1) && (reverseSignal == 1) ) //In last half of bit
        {
            current_pin = ((0b00100000 & PINC) >> 5);  //Record Current Pin Value
            changeDetect = ( current_pin ^ previous_pin ); //Check previous pin and current: see if changed
            

        }
        
        
        if (ltcBitCount == 80)  //If full frame loaded:
        {
            //LOAD FULL FRAME INTO GENERATOR FRAME BUFFER
            sections[0] = tempSections[0];      //FRAME UNITS & BINARY GROUP 1
            sections[1] = tempSections[1];      //FRAME TENS, CF/DF FLAGS, & BINARY GROUP 2
            sections[2] = tempSections[2];      //SECONDS UNITS & BINARY GROUP 3
            sections[3] = tempSections[3];      //SECONDS TENS, BMC POLARITY CORRECTION & BINARY GROUP 4
            sections[4] = tempSections[4];      //MINUTES UNITS & BINARY GROUP 5
            sections[5] = tempSections[5];      //MINUTES TENS, BGF0, & BINARY GROUP 6
            sections[6] = tempSections[6];      //HOURS UNITS & BINARY GROUP 7
            sections[7] = tempSections[7];      //HOURS TENS, BGF1, BGF2 & BINARY GROUP 8
            sections[8] = tempSections[8];     //0b1111 1100  SYNC WORD FIRST HALF
            sections[9] = tempSections[7];     //0b1011 1111  SYNC WORD SECOND HALF
            
            jamSync = 1; //set jamSync variable
        }
        
        
    }
    
    if ((midbitBoundary == 0) && (codewordFound == 0)) //At beginning of a bit
    {
        current_pin = ((0b00100000 & PINC) >> 5);  //Record Current Pin Value
        previous_pin = current_pin;  //Set previous value to current value
    }
    
    if ((midbitBoundary == 1) && (codewordFound == 0)) //In last half of bit
    {
        current_pin = ((0b00100000 & PINC) >> 5);  //Record Current Pin Value
        changeDetect = ( current_pin ^ previous_pin ); //Check previous pin and current: see if changed
        
        //Store "1" or "0" LTC bit  in Codeword Buffer
        ltcBit = changeDetect & 0b00000001; //set ltcBit.  Make sure only LSB is active
        syncWordBufferB = ((syncWordBufferB << 1) & 0b11111110);  //Queue B forward one bit.  Make sure LSB is zero for next OR
        syncWordBufferB |= ((syncWordBufferA >> 7) & 0b00000001);  //Move MSB in A -> B's LSB.
        syncWordBufferA = ((syncWordBufferA << 1) & 0b11111110);//Queue A forward 1 bit.  Make sure LSB is zero for next OR
        syncWordBufferA |= ltcBit; //Store LTC bit in LSB
        
        //Codeword Buffer Format:
        //  Codeword coming in forward direction: 0011 1111 1111 1101  [64 --> 79]
        //  IF LTC in forward direction, we'll read in bit 64 first on through to 79
        //  Direction of Buffer Queue moves A(LSB)[last bit recorded) to B(MSB):
        //      syncWordBuffer B [0011 1111] <---- syncWordBuffer A [1111 1101]
        //          LSB in A is first;  B follows A, starting with LSB first too
        //          MSB in B is last bit in frame!
        
        //Check if codeword has been found,
        if ( (syncWordBufferB == 0b00111111) && (syncWordBufferA == 0b11111101) )
        {
            codewordFound = 1;  //set codewordFound variable
            //...we want frame load to start at next midbit (which is beginning bit boundary)
        }
        else if ( (syncWordBufferB == 0b10111111) && (syncWordBufferA == 0b11111100) )
        {
            //incoming signal stream is in reverse
            reverseSignal = 1;
            codewordFound = 1;
        }
        
        //Done here: go on and toggle midbitBoundary for next midbit read
    }
    
    midbitBoundary ^= 1; //Toggle Switch for next midbit Boundary
    
    
}


led_strobe()
{
    return;
}
