//  freq_gen.c: Uses timer 2 interrupt to generate a square wave at pins
//  P2.0 and P2.1.  The program allows the user to enter a frequency.
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <EFM8LB1.h>
#include <stdlib.h>
#include <stdio.h>
#include "globals.h"
#include "lcd.h"
// Uses Timer4 to delay <ms> mili-seconds. 
void Timer4ms(unsigned char ms)
{
	unsigned char i;// usec counter
	unsigned char k;
	
	k=SFRPAGE;
	SFRPAGE=0x10;
	// The input for Timer 4 is selected as SYSCLK by setting bit 0 of CKCON1:
	CKCON1|=0b_0000_0001;
	
	TMR4RL = 65536-(SYSCLK/1000L); // Set Timer4 to overflow in 1 ms.
	TMR4 = TMR4RL;                 // Initialize Timer4 for first overflow
	
	TF4H=0; // Clear overflow flag
	TR4=1;  // Start Timer4
	for (i = 0; i < ms; i++)       // Count <ms> overflows
	{
		while (!TF4H);  // Wait for overflow
		TF4H=0;         // Clear overflow indicator
	}
	TR4=0; // Stop Timer4
	SFRPAGE=k;	
}

void I2C_write (unsigned char output_data)
{
	SMB0DAT = output_data; // Put data into buffer
	SI = 0;
	while (!SI); // Wait until done with send
}

unsigned char I2C_read (void)
{
	unsigned char input_data;

	SI = 0;
	while (!SI); // Wait until we have data to read
	input_data = SMB0DAT; // Read the data

	return input_data;
}

void I2C_start (void)
{
	ACK = 1;
	STA = 1;     // Send I2C start
	STO = 0;
	SI = 0;
	while (!SI); // Wait until start sent
	STA = 0;     // Reset I2C start
}

void I2C_stop(void)
{
	STO = 1;  	// Perform I2C stop
	SI = 0;	// Clear SI
	//while (!SI);	   // Wait until stop complete (Doesn't work???)
}

void nunchuck_init(bit print_extension_type)
{
	unsigned char i, buf[6];
	
	// Newer initialization format that works for all nunchucks
	I2C_start();
	I2C_write(0xA4);
	I2C_write(0xF0);
	I2C_write(0x55);
	I2C_stop();
	Timer4ms(1);
	 
	I2C_start();
	I2C_write(0xA4);
	I2C_write(0xFB);
	I2C_write(0x00);
	I2C_stop();
	Timer4ms(1);

	// Read the extension type from the register block.  For the original Nunchuk it should be
	// 00 00 a4 20 00 00.
	I2C_start();
	I2C_write(0xA4);
	I2C_write(0xFA); // extension type register
	I2C_stop();
	Timer4ms(3); // 3 ms required to complete acquisition

	I2C_start();
	I2C_write(0xA5);
	
	// Receive values
	for(i=0; i<6; i++)
	{
		buf[i]=I2C_read();
	}
	ACK=0;
	I2C_stop();
	Timer4ms(3);
	
	if(print_extension_type)
	{
		printf("Extension type: %02x  %02x  %02x  %02x  %02x  %02x\n", 
			buf[0],  buf[1], buf[2], buf[3], buf[4], buf[5]);
	}

	// Send the crypto key (zeros), in 3 blocks of 6, 6 & 4.

	I2C_start();
	I2C_write(0xA4);
	I2C_write(0xF0);
	I2C_write(0xAA);
	I2C_stop();
	Timer4ms(1);

	I2C_start();
	I2C_write(0xA4);
	I2C_write(0x40);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_stop();
	Timer4ms(1);

	I2C_start();
	I2C_write(0xA4);
	I2C_write(0x40);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_stop();
	Timer4ms(1);

	I2C_start();
	I2C_write(0xA4);
	I2C_write(0x40);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_write(0x00);
	I2C_stop();
	Timer4ms(1);
}

void nunchuck_getdata(unsigned char * s)
{
	unsigned char i;

	// Start measurement
	I2C_start();
	I2C_write(0xA4);
	I2C_write(0x00);
	I2C_stop();
	Timer4ms(3); 	// 3 ms required to complete acquisition

	// Request values
	I2C_start();
	I2C_write(0xA5);
	
	// Receive values
	for(i=0; i<6; i++)
	{
		s[i]=(I2C_read()^0x17)+0x17; // Read and decrypt
	}
	ACK=0;
	I2C_stop();
}

//void main (void)
//{
	//unsigned char rbuf[6];
 	//int joy_x, joy_y, off_x, off_y, acc_x, acc_y, acc_z;
 	//bit but1, but2;
 	
	//printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	//printf("\n\nEFM8LB1 WII Nunchuck I2C Reader\n");

	//Timer4ms(200);
	//nunchuck_init(1);
	//Timer4ms(100);

	//nunchuck_getdata(rbuf);

	//off_x=(int)rbuf[0]-128;
	//off_y=(int)rbuf[1]-128;
	//printf("Offset_X:%4d Offset_Y:%4d\n\n", off_x, off_y);

	//while(1)
	//{
		//nunchuck_getdata(rbuf);

		//joy_x=(int)rbuf[0]-128-off_x;
		//joy_y=(int)rbuf[1]-128-off_y;
		//acc_x=rbuf[2]*4; 
		//acc_y=rbuf[3]*4;
		//acc_z=rbuf[4]*4;

		//but1=(rbuf[5] & 0x01)?1:0;
		//but2=(rbuf[5] & 0x02)?1:0;
		//if (rbuf[5] & 0x04) acc_x+=2;
		//if (rbuf[5] & 0x08) acc_x+=1;
		//if (rbuf[5] & 0x10) acc_y+=2;
		//if (rbuf[5] & 0x20) acc_y+=1;
		//if (rbuf[5] & 0x40) acc_z+=2;
		//if (rbuf[5] & 0x80) acc_z+=1;
		
		//printf("Buttons(Z:%c, C:%c) Joystick(%4d, %4d) Accelerometer(%3d, %3d, %3d)\x1b[0J\r",
			  // but1?'1':'0', but2?'1':'0', joy_x, joy_y, acc_x, acc_y, acc_z);
		//Timer4ms(100);

   //}
//}

void Timer2_ISR (void) interrupt INTERRUPT_TIMER2
{
	TF2H = 0; // Clear Timer2 interrupt flag
	OUT0=!OUT0;
	OUT1=!OUT0;
}

void main (void)
{
	unsigned char rbuf[6];
 	int joy_x, joy_y, off_x, off_y, acc_x, acc_y, acc_z;
 	bit but1, but2;
	unsigned long int x, f;
	int flag = 0;
	int mode = 0;
	// Configure the LCD
	LCD_4BIT();
	
	f = 15625;
	x=(SYSCLK/(2L*f));
	TR2=0; // Stop timer 2
	TMR2RL=0x10000L-x; // Change reload value for new frequency
	TR2=1; // Start timer 2
	while(1){
		MODESWITCH:
		if(pb6==0){
			if(mode ==0){
				mode = 1;
			}
			else{
				mode = 0;
			}
		}
		if(mode ==0){
			LCDprint("Command",1,1);
			TR2=0;
			OUT0=0;
			OUT1=0;
			waitms(3000);
			TR2=1;
			while(1){
				CHECK:
				if(pb5==0){
					if(flag == 0){
						flag = 1;
					}
					else{
						flag = 0;
					}
				}
				if(flag == 0){
				
					printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
					printf("Variable frequency generator for the EFM8LB1.\r\n"
					       "Check pins P2.0 and P2.1 with the oscilloscope.\r\n");
					printf("\nActual frequency: %lu\n", f);
					LCDprint("Button",2,1);
					
					while(1)
					{
						if(pb5==0){
							waitms(100);
							goto CHECK;
						}
						if(pb6==0){
							waitms(100);
							goto MODESWITCH;
						}
						if(pb1==0){
							LOOP:
							if(pb1==0){
								printf("Forward\n");
								LCDprint("Forward",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(4000);
								TR2=1;
								goto LOOP;
							}
						}
						if(pb2==0){
							LOOP2:
							if(pb2==0){
								printf("Backward\n");
								LCDprint("Backward",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(5000);
								TR2=1;
								goto LOOP2;
							}
						}			
						if(pb3==0){
							LOOP3:
							if(pb3==0){
								printf("Left\n");
								LCDprint("Left",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(6000);
								TR2=1;
								goto LOOP2;
							}
						}
						if(pb4==0){
							LOOP4:
							if(pb4==0){
								printf("Right\n");
								LCDprint("Right",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(7000);
								TR2=1;
								goto LOOP4;
							}
						}		
						//printf("New frequency=");
						//scanf("%lu", &f);
						//x=(SYSCLK/(2L*f));
						//if(x>0xffff)
						//{
							//printf("Sorry %lu Hz is out of range.\n", f);
						//}
						//else
						//{
							//TR2=0; // Stop timer 2
							//TMR2RL=0x10000L-x; // Change reload value for new frequency
							//TR2=1; // Start timer 2
							//f=SYSCLK/(2L*(0x10000L-TMR2RL));
							//printf("\nActual frequency: %lu\n", f);
						//}
					}
				}
				if(flag == 1){
					printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
					printf("\n\nEFM8LB1 WII Nunchuck I2C Reader\n");
					
					Timer4ms(200);
					nunchuck_init(1);
					Timer4ms(100);
					LCDprint("Nunchuck",2,1);
					
					nunchuck_getdata(rbuf);
					
					off_x=(int)rbuf[0]-128;
					off_y=(int)rbuf[1]-128;
					printf("Offset_X:%4d Offset_Y:%4d\n\n", off_x, off_y);
					
					while(1)
					{
						if(pb5==0){
							waitms(100);
							goto CHECK;
						}
						if(pb6==0){
							waitms(100);
							goto MODESWITCH;
						}						
						nunchuck_getdata(rbuf);
				
						joy_x=(int)rbuf[0]-128-off_x;
						joy_y=(int)rbuf[1]-128-off_y;
						acc_x=rbuf[2]*4; 
						acc_y=rbuf[3]*4;
						acc_z=rbuf[4]*4;
				
						but1=(rbuf[5] & 0x01)?1:0;
						but2=(rbuf[5] & 0x02)?1:0;
						if (rbuf[5] & 0x04) acc_x+=2;
						if (rbuf[5] & 0x08) acc_x+=1;
						if (rbuf[5] & 0x10) acc_y+=2;
						if (rbuf[5] & 0x20) acc_y+=1;
						if (rbuf[5] & 0x40) acc_z+=2;
						if (rbuf[5] & 0x80) acc_z+=1;
						
						if(joy_x > 80){
								LCDprint("Right",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(7000);
								TR2=1;
						}
						if(joy_x < -80){
								LCDprint("Left",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(6000);
								TR2=1;
						}
						if(joy_y > 80){
								printf("Forward\n");
								LCDprint("Forward",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(4000);
								TR2=1;
						}
						if(joy_y < -80){
								printf("Backward\n");
								LCDprint("Backward",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(5000);
								TR2=1;
						}
						if(but1 == 0){
								printf("Stop\n");
								LCDprint("Stop",1,1);
								LCDprint(" ",2,1);
								TR2=0;
								OUT0=0;
								OUT1=0;
								waitms(1000);
								TR2=1;
						}		
						printf("Buttons(Z:%c, C:%c) Joystick(%4d, %4d) Accelerometer(%3d, %3d, %3d)\x1b[0J\r",
							  but1?'1':'0', but2?'1':'0', joy_x, joy_y, acc_x, acc_y, acc_z);
						Timer4ms(100);
					}
				}
			}
		}
		if(mode == 1){
			LCDprint("Track",1,1);
			TR2=0;
			OUT0=0;
			OUT1=0;
			waitms(2000);
			TR2=1;
			while(1){
				if(pb6==0){
				waitms(100);
				goto MODESWITCH;
				}
				LCDprint("Tracking",1,1);
			}
		}
	}	
}
