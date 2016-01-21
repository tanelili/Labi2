/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#include <cstdio>
#include <iostream>


#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here
static volatile bool adcdone = false;
static volatile bool adcstart = false;
#define TICKRATE_HZ (100)	/* 100 ticks per second */

extern "C" {

void SysTick_Handler(void)
{
	static uint32_t count;

	/* Every 1/2 second */
	count++;
	if (count >= (TICKRATE_HZ * 2)) {
		count = 0;
		adcstart = true;
		Board_LED_Toggle(1);
	}
}

void ADC0A_IRQHandler(void)
{
	uint32_t pending;

	/* Get pending interrupts */
	pending = Chip_ADC_GetFlags(LPC_ADC0);

	/* Sequence A completion interrupt */
	if (pending & ADC_FLAGS_SEQA_INT_MASK) {
		adcdone = true;
	}

	/* Clear any pending interrupts */
	Chip_ADC_ClearFlags(LPC_ADC0, pending);
}

} // extern "C"


int main(void) {

#if defined (__USE_LPCOPEN)
	// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	// Set up and initialize all required blocks and
	// functions related to the board hardware
	Board_Init();
	// Set the LED to the state of "On"
	Board_LED_Set(1, true);
#endif
#endif

	// TODO: insert code here
	/* Setup ADC for 12-bit mode and normal power */
	Chip_ADC_Init(LPC_ADC0, 0);

	/* Setup for maximum ADC clock rate */
	Chip_ADC_SetClockRate(LPC_ADC0, ADC_MAX_SAMPLE_RATE);

	/* For ADC0, sequencer A will be used without threshold events.
	   It will be triggered manually  */
	Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX, (ADC_SEQ_CTRL_CHANSEL(0) | ADC_SEQ_CTRL_CHANSEL(3) | ADC_SEQ_CTRL_MODE_EOS));

	/* For ADC0, select analog input pint for channel 0 on ADC0 */
	Chip_ADC_SetADC0Input(LPC_ADC0, 0);

	/* Use higher voltage trim for both ADC */
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);

	/* Assign ADC0_0 to PIO1_8 via SWM (fixed pin) and ADC0_3 to PIO0_5 */
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_0);
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_3);

	/* Need to do a calibration after initialization and trim */
	Chip_ADC_StartCalibration(LPC_ADC0);
	while (!(Chip_ADC_IsCalibrationDone(LPC_ADC0))) {}

	/* Clear all pending interrupts and status flags */
	Chip_ADC_ClearFlags(LPC_ADC0, Chip_ADC_GetFlags(LPC_ADC0));

	/* Enable sequence A completion interrupts for ADC0 */
	Chip_ADC_EnableInt(LPC_ADC0, ADC_INTEN_SEQA_ENABLE);

	/* Enable related ADC NVIC interrupts */
	NVIC_EnableIRQ(ADC0_SEQA_IRQn);

	/* Enable sequencer */
	Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);

	/* Configure systick timer */
	SysTick_Config(Chip_Clock_GetSysTickClockRate() / TICKRATE_HZ);

	uint32_t a0;
	uint32_t d0;
	uint32_t a3;
	uint32_t d3;

	while(1) {
		while(!adcstart) __WFI();
		adcstart = false;

		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
		while(!adcdone) __WFI();
		adcdone = false;

		a0 = Chip_ADC_GetDataReg(LPC_ADC0, 0);
		d0 = ADC_DR_RESULT(a0);
		a3 = Chip_ADC_GetDataReg(LPC_ADC0, 3);
		d3 = ADC_DR_RESULT(a3);
		printf("a0 = %08X, a1 = %08X, d0 = %d, d1 = %d\n", a0, a3, d0, d3);
	}

	// Force the counter to be placed into memory
	volatile static int i = 0 ;
	// Enter an infinite loop, just incrementing a counter
	while(1) {
		i++ ;
	}

	return 0 ;
}
