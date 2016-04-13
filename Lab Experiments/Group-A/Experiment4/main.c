#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include"inc/hw_ints.h"
#include"driverlib/interrupt.h"
#include"driverlib/debug.h"
#include"driverlib/adc.h"

#define DEGREE 0
#define MAX_STRING_LENGHT 100

volatile uint32_t ui32TempAvg;
volatile uint32_t ui32TempValueC;
volatile uint32_t ui32TempValueF;

volatile uint32_t ui32TempSet;


void clrscr(){
	uint32_t i;
	for(i=0;i<MAX_STRING_LENGHT;i++)
			UARTCharPut(UART0_BASE,'\b');
	for(i=0;i<MAX_STRING_LENGHT;i++)
		UARTCharPut(UART0_BASE,' ');
	for(i=0;i<MAX_STRING_LENGHT;i++)
		UARTCharPut(UART0_BASE,'\b');
}

uint32_t inpTemp(){
	clrscr();
	char str[25] = "Enter the temperature : ";
	uint32_t temp=0,i;

	for(i=0;i<25;i++)
		UARTCharPut(UART0_BASE,str[i]);

	int flag=1;
	while (flag)
	{
		if (UARTCharsAvail(UART0_BASE)){
			char c = UARTCharGet(UART0_BASE);
			UARTCharPut(UART0_BASE,c);
			if(c>=48 && c<58){
				if(temp==0&&c=='0'){
					UARTCharPut(UART0_BASE,'\b');
					UARTCharPut(UART0_BASE,' ');
					UARTCharPut(UART0_BASE,'\b');
				}
				else
				temp = temp*10 + (c-48);
			}else if(c=='\b'){
				if(temp>0){
					temp = temp/10;
					UARTCharPut(UART0_BASE,' ');
					UARTCharPut(UART0_BASE,c);
				}else
					UARTCharPut(UART0_BASE,' ');
			}
			else
				flag=0;
		}
	}

	clrscr();
	char prompt[MAX_STRING_LENGHT]="Set Temperature updated to XX oC";
	prompt[28]=temp%10+48;
	prompt[27]=(temp/10)%10+48;
	if(DEGREE)
	prompt[30]=176;
	for(i=0;i<32;i++)
		UARTCharPut(UART0_BASE,prompt[i]);

	SysCtlDelay(SysCtlClockGet()); // Wait around 3 sec
	clrscr();
	return temp;
}

void UARTIntHandler(void)
{

	uint32_t ui32Status;
	char c;
	ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status

	UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
	{
		c = UARTCharGet(UART0_BASE);
		if(c=='s'||c=='S')
			ui32TempSet = inpTemp();
	}
}

void print_temp(uint32_t temp){
	char str[MAX_STRING_LENGHT] = "Current Temp = 25 oC , Set Temp = 30 oC";
	if(DEGREE)
	str[18] = 176;
	str[16] = temp%10+48;
	str[15] = (temp/10)%10+48;

	if(DEGREE)
	str[37] = 176;
	str[35] = ui32TempSet%10+48;
	str[34] = (ui32TempSet/10)%10+48;

	int l = 40,i;
	clrscr();
	for(i=0;i<l;i++)
		UARTCharPut(UART0_BASE,str[i]);

}



int main(void) {

	uint32_t ui32ADC0Value[4];

	ui32TempSet = 25;

	// Set System CLock
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	// Enaable UART
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	// Enable GPIO
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	// Enable ADC Peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	//Allow the ADC12 to run at its default rate of 1Msps.
	ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 0);

	//our code will average all four samples of temperature sensor data	ta on sequencer 1 to calculate the temperature, so all four sequencer steps will measure the temperature sensor
	ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 1, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE, 1, 2, ADC_CTL_TS);
	ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_TS|ADC_CTL_IE|ADC_CTL_END);

	//enable ADC sequencer 1.
	ADCSequenceEnable(ADC0_BASE, 1);

	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	IntMasterEnable();
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_3);

	// Set bit rate fr serial communication
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));



	while (1)
	{
		//indication that the ADC conversion process is complete
		ADCIntClear(ADC0_BASE, 1);
		//trigger the ADC conversion with software
		ADCProcessorTrigger(ADC0_BASE, 1);
		//wait for the conversion to complete
		while(!ADCIntStatus(ADC0_BASE, 1, false))
		{
		}
		//read the ADC value from the ADC Sample Sequencer 1 FIFO
		ADCSequenceDataGet(ADC0_BASE, 1, ui32ADC0Value);
		ui32TempAvg = (ui32ADC0Value[0] + ui32ADC0Value[1] + ui32ADC0Value[2] + ui32ADC0Value[3] + 2)/4;


		ui32TempValueC = (1475 - ((2475 * ui32TempAvg)) / 4096)/10;
		ui32TempValueF = ((ui32TempValueC * 9) + 160) / 5;

		print_temp(ui32TempValueC);
		SysCtlDelay(SysCtlClockGet() / 3); //delay ~1 sec
		if(ui32TempValueC < ui32TempSet)
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 8);
		else
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 2);
	}
}
