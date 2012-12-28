/*
 * ADC_Tiny85.cpp
 *
 * Various examples and experiments with the ADC.
 * ADC readings are sent as 3-wire serial (as master, between one and two bytes per cycle)
 * assumes that there is nothing else on the bus so does not do "proper" SPI
 *
 *FUSES:
 * set the clock to be output (to pin 3)
 * NB: some of these are meant to be run with other fuses at non-default settings. See below!
 *
 * Logic Sniffer Channel connections (typ)
 * 0 - /CS
 * 1 - SCK, the serial clock
 * 2 - DO (MOSI as far as logic analyser is concerned but this is labelled MISO on the ATtiny)
 * 3 - GND (MISO as far as logic analyser is concerned)
 * 4 - CLKO, the system clock
 *
 * Created: 17/12/2012 22:27:27
 *  Author: Adam
 */

/*
* ***Made available using the The MIT License (MIT)***
* Copyright (c) 2012, Adam Cooper
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
* associated documentation files (the "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or 
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* ************ end licence ***************
*/

#include <avr/io.h>
#include <avr/interrupt.h> //needed for TimerTriggered

//stereotypes
void HelloWorld();
void SimpleRead();
void Degrade_1();
void Degrade_2();
void Init_TimerTriggered();
void sendBytes(unsigned char bytes[],unsigned char size);

/*
* Constants
*/
//values to set USICR to strobe out
const unsigned char usi_low = (1<<USIWM0) | (1<<USITC);
const unsigned char usi_high = (1<<USIWM0) | (1<<USITC) | (1<<USICLK);

int main(void)
{
	//setup ADC -	ADC3/PB3 pin2
	ADMUX = (1<<MUX1) | (1<<MUX0); // MUX set to ADC3. VCC as ref. Right justified
	ADCSRA = (1<<ADEN); // turn ADC on and initialise. no auto-trigger, no interrupt. prescaler to div2
	DIDR0 = (1<<ADC3D);//turn digital input circuitry off
	
	//setup pins for serial
	//  PB2 (SCK/USCK/SCL/ADC1/T0/INT0/PCINT2)
	// 	PB1 (MISO/DO/AIN1/OC0B/OC1A/PCINT1)
	// 	PB0 (MOSI/DI/SDA/AIN0/OC0A/OC1A/AREF/PCINT0) - will not be used as DI
	//	PB0 used as /CS
	DDRB = (1<<DDB0) | (1<<DDB2) | (1<<DDB1);// /CS, USCK and DO as outputs
	PORTB |= (1<<PB0);//slave not selected
	
	//setup serial comms - 3 wire
	USICR = (1<<USIWM0);	
	
	//place example-specific initialisers here
	Init_TimerTriggered();	
    while(1)
    {
		//SimpleRead();
        //Degrade_2();		
		//HelloWorld();
    }
}

//sends the specified byte as serial (Three wire style).
void sendBytes(unsigned char bytes[], unsigned char size){
	//slave select
	PORTB &= ~(1<<PB0);
		
	//loop over bytes
	for(unsigned char b = 0; b<size;b++){
		//load the byte to the output register
		USIDR = bytes[b];
		//strobe USICLK 8 cycles (also toggles clock output thanks to USITC)
		//an unrolled loop gives 50% duty and saves 3 clock cycles per bit sent
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;
		USICR = usi_low;
		USICR = usi_high;		
	}//bytes loop	
	
	//slave de-select
	PORTB |= (1<<PB0);		
}

//Test the communications works OK.
void HelloWorld(){
		unsigned char send[] = "Hello World!";
		sendBytes(send, sizeof(send)-1);	//-1 drops the null
}	

//perform a single read and send H and L bytes to serial
//assumes 8MHz clock, hence prescaling 
void SimpleRead(){
	//set prescaler to div64 --> 125kHz ADC clock
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1);
	//start a conversion
	ADCSRA |= (1<<ADSC);
	//wait for end of conversion
	while (ADCSRA & (1<<ADSC))
	{
		//do nothing
	}
	unsigned char result[2];
	result[1] = ADCL;// datasheet says read low first
	result[0] = ADCH;
	sendBytes(result, 2);
}


/*
* TimerTriggered -	timer0 triggered ADC.
*		FUSES: use 128kHz clock to get an interval >1s.
*		sends 2 bytes via an interrupt on ADC completion. ADC start is triggered by timer0 compareA
*/
//NB: there is no "main while loop" code; this is fully harware triggered
//setup for TimerTriggered
void Init_TimerTriggered(){
	//1. enable ADC completion interrupt
	sei();//global interrupts
	ADCSRA |= (1<<ADIE);//ADC interrupt
	
	//2. set the ADC to be timer triggered
	ADCSRB |= (1<<ADTS1) | (1<<ADTS0); //this defines the trigger source
	ADCSRA |= (1<<ADATE);//this is needed to enable auto-triggering
	
	//3. setup timer
	TCNT0 = 0x00;//counter to 0
	TCCR0A =  (1<<WGM01);//use "clear timer on compare match" mode
	OCR0A = 0xFF;//output compare to 128 gives about 1s with 128kHz sys clock and prescaler (below)
	TCCR0B = (1<<CS02) | (1<<CS00);//prescaler to 1024, which enables the counter
}

//ADC completion interrupt service.
//Sends the data from the ADC register
ISR(ADC_vect)
{
	//read and send ADC
	unsigned char result[2];
	result[1] = ADCL;// datasheet says read low first
	result[0] = ADCH;
	sendBytes(result, 2);
	
	//clear timer compare flag otherwise the ADC will not be re-triggered!
	TIFR |= (1<<OCF0A);
}

/* ----------------------------
*  These are the Degrade experiments.
*  See how the precision degrades with ADC clock freq outside the range specified in the datasheet
*  The experiment makes 10 readings of the ADC with minimal (/2) prescaling of the system clock and
*  Then successively increases the pre-scaling before taking 10 more readings.
*  For a 8MHz system clock this means the ADC clock will be 4MHz, 2MHz, 1MHz, 500kHz, 100kHz, 50kHz, 25kHz
*  FUSES:	CKDIV8 = [ ]
*			SUT_CKSEL = INTRCOSC_8MHZ_6CK_14CK_0MS
*  For a 128kHz system clock this means the ADC will be clocked at 64kHz, 32kHz, 16kHz, 8kHz, 4kHz, 2kHz, 1kHz
*  FUSES:	CKDIV8 = [ ]
*			SUT_CKSEL = WDOSC_128KHZ_6CK_14CK_0MS
*  Maximum resolution is specified for between 50kHz and 200kHz ADC clocks.
*  ---------------------------*/

/*
* 1 -	8 bit precision (left-align and read out high byte only)
*		Sends a single byte to serial
*/
void Degrade_1(){
	//set left align
	ADMUX |= (1<<ADLAR);
	for(unsigned char prescale=1; prescale<=7; prescale++){
		//clear then re-assert the prescaler
		ADCSRA &= 0xF8;
		ADCSRA |= prescale;
		//start a conversion
		ADCSRA |= (1<<ADSC);
		//wait for end of conversion
		while (ADCSRA & (1<<ADSC));
	unsigned char result[]={ADCH};
	//result[0] = ADCH;
	sendBytes(result,1);
}

// 	//send a comma to separate readings
unsigned char comma[]=",";
sendBytes(comma,1);
}

/*
* 2 -	10 bit precision (right-align and read out both bytes)
*		Sends 2 bytes to serial
*/
void Degrade_2(){
	unsigned char result[2];
	for(unsigned char prescale=1; prescale<=7; prescale++){
		//clear then re-assert the prescaler
		ADCSRA &= 0xF8;
		ADCSRA |= prescale;
		//start a conversion
		ADCSRA |= (1<<ADSC);
		//wait for end of conversion
		while (ADCSRA & (1<<ADSC));
		result[1] = ADCL;// datasheet says read low first
		result[0] = ADCH;
		sendBytes(result, 2);
	}

	//send a comma to separate readings
	unsigned char comma[]=",";
	sendBytes(comma,1);
}