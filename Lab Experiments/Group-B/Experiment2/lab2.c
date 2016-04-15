/*

* Author: Texas Instruments 

* Editted by: Saurav Shandilya, Vishwanathan Iyer 
	      ERTS Lab, CSE Department, IIT Bombay

* Description: This code will familiarize you with using GPIO on TMS4C123GXL microcontroller. 

* Filename: lab-1.c 

* Functions: setup(), ledPinConfig(), switchPinConfig(), main()  

* Global Variables: none

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"

// LOCK_F and CR_F - used for unlocking PORTF pin 0
#define LOCK_F (*((volatile unsigned long *)0x40025520))
#define CR_F   (*((volatile unsigned long *)0x40025524))

/*
 ------ Global Variable Declaration
*/
	

/*

* Function Name: setup()

* Input: none

* Output: none

* Description: Set crystal frequency and enable GPIO Peripherals  

* Example Call: setup();

*/
void setup(void)
{	
/* Configure the clock to run at 40MHz  */
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
}

/*

* Function Name: ledPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 1, Pin 2, Pin 3 as output.

* Example Call: ledPinConfig();

*/

void ledPinConfig(void)
{
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);   
}

/*

* Function Name: switchPinConfig()

* Input: none

* Output: none

* Description: Set PORTF Pin 0 and Pin 4 as input. Note that Pin 0 is locked.

* Example Call: switchPinConfig();

*/
void switchPinConfig(void)
{
	// Following two line removes the lock from SW2 interfaced on PORTF Pin0 -- leave this unchanged
	LOCK_F=0x4C4F434BU;
	CR_F=GPIO_PIN_0|GPIO_PIN_4;
	
	// GPIO PORTF Pin 0 and Pin4
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_DIR_MODE_IN); // Set Pin-4 of PORT F as Input. Modifiy this to use another switch
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4|GPIO_PIN_0);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4|GPIO_PIN_0,GPIO_STRENGTH_12MA,GPIO_PIN_TYPE_STD_WPU);
}

int main(void)
{
	setup();
	ledPinConfig();

	switchPinConfig();

	uint32_t ui32Period;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ui32Period = (SysCtlClockGet() / 100) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();
	TimerEnable(TIMER0_BASE, TIMER_A);
	while(1)
	{
	}

}
/*
*State variables to maintain state
*state 0 : idle
*state 1: pressed
*state 2: released
*/
uint8_t sw1State = 0,sw2State = 0;
/*
* Function Name: detectKeyPress(bool)

* Input: selectSwitch , select switch1 or switch2

* Output: 1 if switch press is detected, 0 otherwise

* Description: changes states of switches

* Example Call: switchPinConfig();

*/
unsigned char detectKeyPress(bool selectSwitch){
	unsigned char retVal = 0;
	uint8_t curr_state,final_state,pressed;
	if (selectSwitch){
		pressed = (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_4)==0x00);
		curr_state = sw1State;
	}
	else {
		pressed = (GPIOPinRead(GPIO_PORTF_BASE,GPIO_PIN_0)==0x00);
		curr_state = sw2State;
	}

	switch(curr_state){
		case 0: if (pressed) final_state = 1; break;
		case 1: if (pressed) {final_state = 2;
		 		retVal = 1;}
		 		else {
		 			final_state = 0;
		 		}
		 		break;
		case 2: if (pressed) {final_state = 2;
				retVal = 1;}
				else final_state = 0;
				break;
	}
	if (selectSwitch){
		sw1State = final_state;
	}
	else {
		sw2State = final_state;
	}
	return retVal;
}	

//Global variable incremented on every switch2 press
int sw2Status = 0;
//keeping track of last state of switches
bool sw1Pressed = 0,sw2Pressed = 0;
/*
* Function Name: Timer0IntHandler()

* Input: none

* Output: none

* Description: timer interrupt handler

* Example Call: Timer0IntHandler();

*/
void Timer0IntHandler(void){
	uint8_t ui8LED = 0;

	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	//detect if switch 1 is pressed
	unsigned char press = detectKeyPress(true);
	if (press == 1){
		if (!sw1Pressed){
			switch(ui8LED){
				case 0:{ui8LED = 2;break;}
				case 2: {ui8LED = 8; break;}
				case 4: {ui8LED = 2;break;}
				case 8: {ui8LED = 4;break;}
			}
			sw1Pressed = 1;
		}
		//turn on the LED
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, ui8LED);
	}
	else {
		sw1Pressed = 0;
		//turn off the led
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0);

	}
	press = detectKeyPress(false);
	if (press){
		if (sw2Pressed == 0){
			sw2Status = sw2Status + 1;
			sw2Pressed = 1;
		}
	}
	else {
		sw2Pressed = 0;
	}
}