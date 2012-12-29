/*
 * ATtiny85_PWM_with_DeadTime.cpp
 *
 * Created: 27/12/2012 17:28:40
 *  Author: Adam
 */ 


#include <avr/io.h>

int main(void)
{
    while(1)
    {
        //TODO:: Please write your application code 
    }
}

I have a queston about the Dead Time Generator that is available for Timer1 in ATTiny25.
I have generated a PWM on pin5 [PB0, _OC1A] and pin6 [PB1, OC1A]. The duty cycle of these signal is 50%.
Now I want to generate 10% Dead Time on both the _OC1A as the OC1A.
My code is the following:

Code:
//   PWM UNIT TIMER/COUNTER1

// Make OC1A & _OC1A an output
DDRB   |=   (1<<DDB0);   //   Pin 5 [PB0] output maken (_OC1A)
DDRB   |=   (1<<DDB1);   //   Pin 6 [PB1] output maken (OC1A)

TCCR1   |=   (1<<PWM1A);   //   Enables PWM mode based on comparator OCR1A

//   Toggle the OC1A output line
TCCR1   &=   ~(1<<COM1A1);
TCCR1   |=   (1<<COM1A0);

//   Set Frequency
//   Clock Frequency = 8.000.000Hz
//   Precaler = 16
//   8.000.000Hz/16 = 500.000Hz.
//   Devide by OCR1C -> 500.000Hz/250 = 2.000Hz
OCR1C   =   0b11111010;      //   Make frequency 2000Hz by setting OCR1C to 250

//   Set Duty Cycle
OCR1A   =   OCR1C>>1;
//OCR1A   =   0b01111101;      //   Duty Cycle = 50%

//   Prescaler is set @ 16, PWM is turned ON
TCCR1   &=   ~(1<<CS13);
TCCR1   |=   (1<<CS12);
TCCR1   &=   ~(1<<CS11);
TCCR1   |=   (1<<CS10);



//   DEAD TIME GENERATOR

//   Set Prescaler Value @ 8
DTPS1   |=   (1<<DTPS11);
DTPS1   |=   (1<<DTPS10);

//   Dead time for OC1A output = max
DT1A   |=   (1<<DT1AH3);
DT1A   |=   (1<<DT1AH2);
DT1A   |=   (1<<DT1AH1);
DT1A   |=   (1<<DT1AH0);

//   Dead time for _OC1A output = max
DT1A   |=   (1<<DT1AL3);
DT1A   |=   (1<<DT1AL2);
DT1A   |=   (1<<DT1AL1);
DT1A   |=   (1<<DT1AL0);


I realize that I have not programmed a 10% dead time with the DT1AH & DT1AL, but here is my problem.

When I look at the scoop, I see a dead time period of 15uSec.
When I calculate this though, I should have a time of 240uSec.
If I change the Timer1 Frequency (by changing the prescaler value in TCCR1), the dead time remains 15uSec.

Does anybody have an Idea what seems to be the problem? Lajon - Feb 05, 2009 - 11:31 PM
Post subject: RE: Timer1 PWM Dead Time problem The dead time generator clock is not affected by the timer 1 prescaler. It is the timer 1 clock without prescaler (8Mhz) divided by 8 in your case (so 1Mhz).