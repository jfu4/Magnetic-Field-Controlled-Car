#define F_CPU 22118400UL
#define __DELAY_BACKWARD_COMPATIBLE__


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include "usart.h"

#define LEFT_MOTOR_1_PIN PD5
#define LEFT_MOTOR_2_PIN PD6
#define RIGHT_MOTOR_1_PIN PD7
#define RIGHT_MOTOR_2_PIN PB0

#define VOLTAGE_INPUT (PINB & 0b00000010) //will move this up as long as it doesn't mess with anything


#define STOP 1
#define FORWARD 4
#define BACKWARD 5
#define LEFT 6
#define RIGHT 7
#define TRACKING_MODE 2
#define COMMAND_MODE 3
#define NOSIGNAL 5000
#define DANCE 9

///////////////////INITIALIZATIONS//////////////////////////////

//void timer_init0 (void) //edited
//{
   
//  	TCCR0A = 0;
//  	TCCR0B = 1 << CS00; // Clock Select: clk_io / 1
//  	TIMSK0 = 1 << TOIE0; // Enable Timer 0 overflow interrupt

  	// Enable global interrupt
//  	sei();
//}

void timer_init1 (void)
{
	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet

	//overflow interrupt
	TIMSK1 |= (1 << TOIE1);
  	// Enable global interrupts
  	//sei();
}

void adc_init(void)
{
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

#define PIN_PERIOD (PINB & (1<<1)) // PB1


//////////////////////TOOLS///////////////////////////////

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriod (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TIFR1=1; // TOV1 can be cleared by writing a logic one to its bit location.  Check ATmega328P datasheet page 113.
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TIFR1=1;
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TIFR1=1;
	saved_TCNT1a=TCNT1;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TCNT1;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}


uint16_t adc_read(int channel)
{
    channel &= 0x7;
    ADMUX = (ADMUX & 0xf8)|channel;
     
    ADCSRA |= (1<<ADSC);
     
    while(ADCSRA & (1<<ADSC)); //as long as ADSC pin is 1 just wait.
     
    return (ADCW);
}

void PrintNumber(long int N, int Base, int digits)
{ 
	char HexDigit[]="0123456789ABCDEF";
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (N>0) | (digits>0) )
	{
		buff[j--]=HexDigit[N%Base];
		N/=Base;
		if(digits!=0) digits--;
	}
	usart_pstr(&buff[j+1]);
}

void ConfigurePins (void)
{
	DDRB  &= 0b11111101; // Configure PB1 as input
    PORTB |= 0b00000010; // Activate pull-up in PB1
	// Set the pins for the left motor to output
	DDRD |= (1 << PD5) | (1 << PD6);

	// Set the pins for the right motor to output (includes one PB pin, hence the two statements)
	DDRD |= (1 << PD7);
	DDRB |= (1 << PB0);
}


////////////////////DIRECTIONS///////////////////////////
void forward (void)
{
	// set left motor pins to move forward
	PORTD |= (1 << LEFT_MOTOR_1_PIN);
	PORTD &= ~(1 << LEFT_MOTOR_2_PIN);

	// set right motor pins to move forward
	PORTD |= (1 << RIGHT_MOTOR_1_PIN);
	PORTB &= ~(1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(5);

}

void backward (void)
{
	// set left motor pins to move backward
 	PORTD &= ~(1 << LEFT_MOTOR_1_PIN);
  	PORTD |= (1 << LEFT_MOTOR_2_PIN);
  
  	// set right motor pins to move backward
  	PORTD &= ~(1 << RIGHT_MOTOR_1_PIN);
  	PORTB |= (1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(5);
	
}

void right (void) 
{
	// set left motor pins to move backward
  	PORTD &= ~(1 << LEFT_MOTOR_1_PIN);
  	PORTD |= (1 << LEFT_MOTOR_2_PIN);
	
  	// set right motor pins to move forward
  	PORTD |= (1 << RIGHT_MOTOR_1_PIN);
  	PORTB &= ~(1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(5);
}

void left (void)
{
	// set left motor pins to move forward
	PORTD |= (1 << LEFT_MOTOR_1_PIN);
	PORTD &= ~(1 << LEFT_MOTOR_2_PIN);

	// set right motor pins to move backward
	PORTD &= ~(1 << RIGHT_MOTOR_1_PIN);
	PORTB |= (1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(5);
}

void stop (void)
{
	PORTD &= ~(1 << LEFT_MOTOR_1_PIN) & ~(1 << LEFT_MOTOR_2_PIN) & ~(1 << RIGHT_MOTOR_1_PIN);
 	PORTB &= ~(1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(5);
}

void forward_dance (void)
{
	// set left motor pins to move forward
	PORTD |= (1 << LEFT_MOTOR_1_PIN);
	PORTD &= ~(1 << LEFT_MOTOR_2_PIN);

	// set right motor pins to move forward
	PORTD |= (1 << RIGHT_MOTOR_1_PIN);
	PORTB &= ~(1 << RIGHT_MOTOR_2_PIN);

	_delay_ms(100);
}

void backward_dance (void)
{
	// set left motor pins to move backward
 	PORTD &= ~(1 << LEFT_MOTOR_1_PIN);
  	PORTD |= (1 << LEFT_MOTOR_2_PIN);
  
  	// set right motor pins to move backward
  	PORTD &= ~(1 << RIGHT_MOTOR_1_PIN);
  	PORTB |= (1 << RIGHT_MOTOR_2_PIN);

	_delay_ms(100);
}

void right_dance (void) 
{
	// set left motor pins to move backward
  	PORTD &= ~(1 << LEFT_MOTOR_1_PIN);
  	PORTD |= (1 << LEFT_MOTOR_2_PIN);
	
  	// set right motor pins to move forward
  	PORTD |= (1 << RIGHT_MOTOR_1_PIN);
  	PORTB &= ~(1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(100);
}

void left_dance (void)
{
	// set left motor pins to move forward
	PORTD |= (1 << LEFT_MOTOR_1_PIN);
	PORTD &= ~(1 << LEFT_MOTOR_2_PIN);

	// set right motor pins to move backward
	PORTD &= ~(1 << RIGHT_MOTOR_1_PIN);
	PORTB |= (1 << RIGHT_MOTOR_2_PIN);
	_delay_ms(100);
}

void clear (void) 
{
	PORTD &= 0b11000011; // clear bits 2-6 of PORTD - works
	PORTB &= 0x11111110; // PB0 = 0
}
// In order to keep this as nimble as possible, avoid
// using floating point or printf() on any of its forms!
int main (void)
{
	
	unsigned int adc;
    unsigned long int v1;
    unsigned long int v2;
    int th_v = 200;
	
	long int time_diff = 0;
	long int td = 0;

	int overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;

	//unsigned long int td_int;
	//float time_difference_ms;
	//int time_difference_ms_int;
	//uint16_t system_clock_frequency = F_CPU; 
//usart_pstr("\rbegin");
	usart_init(); // configure the usart and baudrate
	//usart_pstr("\rusart initializations\n");

	adc_init();
//usart_pstr("\radc initializations\r\n");

//usart_pstr("\rdelay\r\n\r\n");

	ConfigurePins();
//usart_pstr("\r configure pins\r\n");

	timer_init1();


	
	_delay_ms(500); // Wait for putty to start
	usart_pstr("\rdelay\r\n");

	usart_pstr("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	
	
	clear();

	loop:
	stop();
	while(1)
	{
		
		adc=adc_read(10);
		v1=(adc*5000L)/1023L;

		
		adc=adc_read(1);
		v2=(adc*5000L)/1023L;


		
		if (v1 < th_v && v2 < th_v) {
			overflow = 0;
			TIFR1 = 1;
			saved_TCNT1a = TCNT1;
			
			while ((v1 < th_v) && (v2 < th_v)) {
				adc=adc_read(0);
        		v1=(adc*5000L)/1023L;
		usart_pstr("ADC[0]=0x");
		PrintNumber(adc, 16, 3);
		usart_pstr(", ");
		PrintNumber(v1/1000, 10, 1);
		usart_pstr(".");
		PrintNumber(v1%1000, 10, 3);
		usart_pstr("V ");
        		adc=adc_read(1);
        		v2=(adc*5000L)/1023L;
						usart_pstr("ADC[1]=0x");
		PrintNumber(adc, 16, 3);
		usart_pstr(", ");
		PrintNumber(v2/1000, 10, 1);
		usart_pstr(".");
		PrintNumber(v2%1000, 10, 3);
		usart_pstr("V ");
		usart_pstr("\n\r");
				if(TIFR1&1)	{//adds overflow, if overflow exists
			 		TIFR1=1; overflow++; 
			 
			 		if(overflow>10000) {
						usart_pstr("Signal Lost\n");
						goto loop;
					}
					
				}
			}
			
			saved_TCNT1b=TCNT1;
			if  ((v1 > th_v) || (v2 > th_v)) {
				if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.
			}
			time_diff = overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
		}
			
		td = (time_diff/F_CPU) + 1;

		
		PrintNumber(td, 10, 3);
		usart_pstr("\n\r");
		
		
		switch(td)
		{
			case STOP:
				stop();
				goto loop;

			case FORWARD:
				forward();
				usart_pstr("forward\n");
				goto loop;

			case BACKWARD:
				backward();
				usart_pstr("backward\n");
				goto loop;

			case LEFT:
				left();
				usart_pstr("left\n");
				goto loop;
				
			case RIGHT:
				right();
				usart_pstr("right\n");
				goto loop;
			case TRACKING_MODE:
				goto tracking;

			case DANCE:
				goto dance;

			default:
				goto loop;
		}
		tracking:
			adc=adc_read(0);
        	v1=(adc*5000L)/1023L;

			adc=adc_read(1);
			v2=(adc*5000L)/1023L;

			
			if (v1 > th_v && v2 > th_v) {// if both voltages are high
				forward();
			}
			if (v1> th_v && v2 < th_v) {//if left rlc circuit high
				right();

			}
			if (v1< th_v && v2 > th_v) {//if right rlc circuit high
				left();
			}
			if (v1< th_v && v2 < th_v) {//if both voltages low
				backward();
				overflow = 0;
				TIFR1 = 1;
				saved_TCNT1a = TCNT1;
				
				while ((v1 < th_v) && (v2 < th_v)) {
					adc=adc_read(0);
	        		v1=(adc*5000L)/1023L;
	
	        		adc=adc_read(1);
	        		v2=(adc*5000L)/1023L;
	
					if(TIFR1&1)	{//adds overflow, if overflow exists
				 		TIFR1=1; overflow++; 
				 
				 		if(overflow>10000) {
							usart_pstr("Signal Lost\n");
							goto loop;
						}
						
					}
				}
				
				saved_TCNT1b=TCNT1;
				if  ((v1 > th_v) || (v2 > th_v)) {
					if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.
				}
				time_diff = overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
			
				
				td = (time_diff/F_CPU) + 1;
				PrintNumber(td, 10, 3);
				switch(td) {
					case COMMAND_MODE:   
						goto loop;
					default:
						goto tracking;
				}
				
			}
			if ((v1 > th_v - 200 || v1 < th_v +200) && (v2 > th_v - 200 || v2 < th_v +200)) { //halt 
				stop();
			}
			goto tracking;
			
		dance:
			adc=adc_read(0);
        	v1=(adc*5000L)/1023L;

			adc=adc_read(1);
			v2=(adc*5000L)/1023L;

			//right_dance();
			
			if (v1< th_v && v2 < th_v) {//if both voltages low

				overflow = 0;
				TIFR1 = 1;
				saved_TCNT1a = TCNT1;
				
				while ((v1 < th_v) && (v2 < th_v)) {
					adc=adc_read(0);
	        		v1=(adc*5000L)/1023L;
	
	        		adc=adc_read(1);
	        		v2=(adc*5000L)/1023L;
	
					if(TIFR1&1)	{//adds overflow, if overflow exists
				 		TIFR1=1; overflow++; 
				 
				 		if(overflow>10000) {
							usart_pstr("Signal Lost\n");
							goto loop;
						}
						
					}
				}
				
				saved_TCNT1b=TCNT1;
				if  ((v1 > th_v) || (v2 > th_v)) {
					if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.
				}
				time_diff = overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
			
				
				td = (time_diff/F_CPU) + 1;
				
				}
				switch(td) {
					case COMMAND_MODE:   
						goto loop;
					case STOP:
						stop();
						goto loop;
					default:
						forward_dance();
						backward_dance();
						left_dance();
						right_dance();
						stop();
						goto dance;
				}
		
	}
}


