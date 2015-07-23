/*
 TCSLT
 Copyright (C) 2015 Joshua Jackson
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public
 License along with this library.
 If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef LTC
#define LTC

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

//MACROS
#define CPUFREQ 20000000 // 20MHZ with exteral crystal (set Fuses!) (Clock Period is .05us)
#define F_CPU (20000000UL)
#define MIDBIT_CLOCKPERIOD 4167 //4171-4 (4 cycles removed for ISR entry timing). Each BPM(biphase) 1/2 period clock is 208.5419us (or approx. 4170.8375 clock cycles)
//Our midbit_clock period approximation is .008125us ahead, so over the full 80-bits we are 1.3us ahead of accurate time per generated frame.  Since each frame is 33.3667ms long (or 33366.7us), it would take 25666.6923 of our generated frames to drift ahead by a single frame of accurate timecode time.  That would mean we drift ahead a single frame in 856.4128 seconds, or 14.27 minutes!  Jam-Sync often!
//Perhaps we can use an 8-bit timer counter (set to 97) that increments an integer (up to 43).  97 cycles incremented 43 times gives us 4171 total cycles!  Note that we need the ISR that uses this counter to be under 97 cycles worth of instructions!
#define TC_FR 0x30 // 30 FRAMES TC
#define FRAME_MIDBITCOUNT 160  //Each frame @ 29.97 is 33.3667ms.  That's 667,334.0007 cycles. or 160 midbits (rounded up)
#define MAX_CLEAR 0x0F

//Function Prototypes
void smpte_increment(void);
void smpte_signal(void);
void smpte_signalGenerate(void);
void display_smpte(void);
void readJam_smpte(void);
void syncJam_smpte(void);
void led_strobe(void);
void initializeMAX(void);
void clear_MAX7219(void);

#endif
