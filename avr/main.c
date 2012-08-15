/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * AVR Software Framework (ASF).
 */
#include eeprom.h

//#include <asf.h>
//#include <avr/eeprom.h>

static volatile bool main_b_cdc_enable = false;

//#define PHASED_ARRAY_DEBUG
#define ENABLE_USB

#ifdef PHASED_ARRAY_DEBUG
	#define PA_ADC_POS ADCCH_POS_DAC
#else
	#define PA_ADC_POS ADCCH_POS_PIN4
#endif

static uint32_t frame_ctr = 0;
#define NUM_WF (320)
#define NUM_CHANNELS (14)
#define NUM_DUMMY_CH (1)
#define NUM_SAMPLES (NUM_WF*(NUM_CHANNELS+NUM_DUMMY_CH) + 1)
#define NUM_SAMPLES_XFER (NUM_WF*NUM_CHANNELS)
#define ADC_DMA_CH (2)
#define DAC_DMA_CH (1)
#define DIO_DMA_CH (0)

static struct {
	uint16_t dummy_word;
	uint16_t dummy_ch[NUM_DUMMY_CH][NUM_WF];
	uint16_t adc_data[NUM_CHANNELS][NUM_WF];
} buff_struct;

static const uint16_t const wf_data[NUM_WF] = {
	11,24,37,50,62,75,88,101,113,126,139,152,164,177,190,203,215,228,241,254,266,279,292,305,318,330,343,356,369,381,394,407,420,432,445,458,471,483,496,509,522,534,547,560,573,585,598,611,624,637,649,662,675,688,700,713,726,739,751,764,777,790,802,815,828,841,853,866,879,892,904,917,930,943,956,968,981,994,1007,1019,1032,1045,1058,1070,1083,1096,1109,1121,1134,1147,1160,1172,1185,1198,1211,1223,1236,1249,1262,1275,1287,1300,1313,1326,1338,1351,1364,1377,1389,1402,1415,1428,1440,1453,1466,1479,1491,1504,1517,1530,1542,1555,1568,1581,1594,1606,1619,1632,1645,1657,1670,1683,1696,1708,1721,1734,1747,1759,1772,1785,1798,1810,1823,1836,1849,1861,1874,1887,1900,1913,1925,1938,1951,1964,1976,1989,2002,2015,2027,2040,2053,2066,2078,2091,2104,2117,2129,2142,2155,2168,2180,2193,2206,2219,2232,2244,2257,2270,2283,2295,2308,2321,2334,2346,2359,2372,2385,2397,2410,2423,2436,2448,2461,2474,2487,2499,2512,2525,2538,2551,2563,2576,2589,2602,2614,2627,2640,2653,2665,2678,2691,2704,2716,2729,2742,2755,2767,2780,2793,2806,2818,2831,2844,2857,2870,2882,2895,2908,2921,2933,2946,2959,2972,2984,2997,3010,3023,3035,3048,3061,3074,3086,3099,3112,3125,3137,3150,3163,3176,3189,3201,3214,3227,3240,3252,3265,3278,3291,3303,3316,3329,3342,3354,3367,3380,3393,3405,3418,3431,3444,3456,3469,3482,3495,3508,3520,3533,3546,3559,3571,3584,3597,3610,3622,3635,3648,3661,3673,3686,3699,3712,3724,3737,3750,3763,3775,3788,3801,3814,3827,3839,3852,3865,3878,3890,3903,3916,3929,3941,3954,3967,3980,3992,4005,4018,4031,4043,4056,4069,4082
};

static const uint8_t const control_portE[NUM_CHANNELS+NUM_DUMMY_CH] = {
	//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14
	0,9,8,13,12,1,0,11,4,15,14,3,2,7,6
};

static void toggle_led_callback(void) {
	gpio_toggle_pin(LED1_GPIO);
}

static void frame_callback() {
	struct dma_channel_config config_params;
	struct adc_config adc_conf;
	struct adc_channel_config adc_ch_conf;
	struct dac_config dac_conf;
	
	/*
	if (frame_ctr) {
		// display / process data
		if (gpio_pin_is_low(GPIO_PUSH_BUTTON_0)) {
			do {
				asm("nop");
			} while (gpio_pin_is_low(GPIO_PUSH_BUTTON_0));
	
			// Enable display backlight
			gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT);
			
			uint8_t eIdx = 0;
			do {
				uint16_t xIdx, dIdx;
				uint8_t myBar;
			
				dIdx = 0;
				for (xIdx = 0; xIdx < 128; xIdx++) {
					myBar = (uint8_t)(buff_struct.adc_data[eIdx][dIdx] >> 7);
					gfx_mono_draw_vertical_line(xIdx,0,32 - myBar,GFX_PIXEL_CLR);
					gfx_mono_draw_vertical_line(xIdx,32 - myBar,myBar,GFX_PIXEL_SET);
					dIdx += (uint16_t)(NUM_WF/128); // modified for shorter waveform
				}
			
				if (gpio_pin_is_low(GPIO_PUSH_BUTTON_0)) {
					do {
						asm("nop");
					} while (gpio_pin_is_low(GPIO_PUSH_BUTTON_0));
					break;
				}
				if (gpio_pin_is_low(GPIO_PUSH_BUTTON_1)) {
					do {
						asm("nop");
					} while (gpio_pin_is_low(GPIO_PUSH_BUTTON_1));
					eIdx++;
					if (eIdx > NUM_CHANNELS-1) eIdx = 0;
				}
				if (gpio_pin_is_low(GPIO_PUSH_BUTTON_2)) {
					do {
						asm("nop");
					} while (gpio_pin_is_low(GPIO_PUSH_BUTTON_2));
					eIdx--;
					if (eIdx > NUM_CHANNELS-1) eIdx = NUM_CHANNELS-1;
				}
			} while (true);
			
			// clear the screen
			gfx_mono_draw_filled_rect(0,0,128,32,GFX_PIXEL_CLR);
			
			// Disable display backlight
			gpio_set_pin_low(NHD_C12832A1Z_BACKLIGHT);
		}			
	}
	*/
	// toggle led as frames are collected
	gpio_toggle_pin(LED0_GPIO);
	
	irqflags_t flags;
	flags = cpu_irq_save();
	
	// disable the root clock
	tc_write_clock_source(&TCC0,TC_CLKSEL_OFF_gc);
	
	// This broke my glitch detector but didn't fix the glitch
	// try pausing here to make sure clock is actually off
	/*for(int delayInd = 0; delayInd < 1000; delayInd++)
	{
		asm("nop");
	}*/
	
	dma_channel_disable(0);
	dma_channel_disable(1);
	dma_channel_disable(2);
	tc_disable(&TCC0);
	tc_disable(&TCD0);
	//adc_disable(&ADCA);
	//dac_disable(&DACB);
	
	frame_ctr++;
		
	// setup a 1 MHz event #0 using a timer
	// ASF example project on DMA "unit_tests" from XMega,
	//    function "run_dma_triggered_with_callback"
	tc_enable(&TCC0);
	tc_write_clock_source(&TCC0,TC_CLKSEL_OFF_gc);
	tc_write_count(&TCC0,0);
	tc_set_direction(&TCC0,TC_UP);
	tc_write_period(&TCC0,23);
	
	// setup a 1/NUM_WF MHz event #1 using a timer fed off the 1 MHz event
	tc_enable(&TCD0);
	tc_write_clock_source(&TCD0,TC_CLKSEL_OFF_gc);
	tc_set_direction(&TCD0,TC_UP);
	tc_write_period(&TCD0,NUM_WF-1);
	tc_write_clock_source(&TCD0,TC_CLKSEL_EVCH1_gc);
	tc_write_count(&TCD0,0);
	
	// adc setup learned mostly from:
	// ASF example project "adc_example_1_gfx" from XMega Xplained
	// should use pin 5 on the J2 header (ADC4), which is port A pin 4 (PA4 or ADCA4)
	adc_read_configuration(&ADCA,&adc_conf);
	adcch_read_configuration(&ADCA,ADC_CH0,&adc_ch_conf);
	adc_set_conversion_parameters(&adc_conf,ADC_SIGN_OFF,ADC_RES_12,ADC_REF_VCC);
	adc_set_clock_rate(&adc_conf,2000000UL);
	adc_set_conversion_trigger(&adc_conf,ADC_TRIG_EVENT_SINGLE,1,1);
	adc_write_configuration(&ADCA,&adc_conf);
	adcch_set_input(&adc_ch_conf,PA_ADC_POS,ADCCH_NEG_NONE,ADC_CH_GAIN_1X_gc); // the dac
	adcch_write_configuration(&ADCA,ADC_CH0,&adc_ch_conf);
	ADCA.CAL = adc_get_calibration_data(ADC_CAL_ADCA);
	adc_enable(&ADCA);
	
	// setup dac too
	// the dac is on port b here
	// should use pin 3 on the J2 header (ADC2), which is port B pin 2 (PB2 or DACB0)
	dac_read_configuration(&DACB,&dac_conf);
	dac_set_conversion_parameters(&dac_conf,DAC_REF_BANDGAP,DAC_ADJ_RIGHT);
	#ifdef PHASED_ARRAY_DEBUG
		dac_set_active_channel(&dac_conf,DAC_CH0,DAC_CH0);
	#else
		dac_set_active_channel(&dac_conf,DAC_CH0,0);
	#endif
	dac_set_conversion_trigger(&dac_conf,DAC_CH0,1);
	dac_write_configuration(&DACB,&dac_conf);
	dac_enable(&DACB);
	dac_wait_for_channel_ready(&DACB,DAC_CH0);
	DACB.CH0GAINCAL = dac_get_calibration_data(DAC_CAL_DACB0_GAIN);
	DACB.CH0OFFSETCAL = dac_get_calibration_data(DAC_CAL_DACB0_OFFSET);

	
	// dma setup learned mostly from:
	// 1) ASF example project on DMA "unit_tests" from XMega
	// 2) XMega example document AVR1304, section 3.3 on peripherals
	dma_enable();
	// dma_set_priority_mode(DMA_PRIMODE_RR0123_gc);					// Round Robin Priority Mode
	dma_set_priority_mode(DMA_PRIMODE_CH0123_gc);					// Priority based on value
	
	// setup dma channel to read from the A/D
	memset(&config_params,0,sizeof(config_params));
	dma_channel_set_single_shot(&config_params);											// complete a single data transfer as opposed to a block transfer at each trigger
	dma_channel_set_burst_length(&config_params,DMA_CH_BURSTLEN_2BYTE_gc);					// set the burst length to 2 bytes
	dma_channel_set_src_reload_mode(&config_params,DMA_CH_SRCRELOAD_BURST_gc);				// reload the source address (ADC register) after every (2 byte) burst
	dma_channel_set_src_dir_mode(&config_params,DMA_CH_SRCDIR_INC_gc);						// increment the source address (ADC register) after each byte access
	dma_channel_set_dest_dir_mode(&config_params,DMA_CH_DESTDIR_INC_gc);					// increment the destination address (SRAM) after each byte access
	dma_channel_set_trigger_source(&config_params,DMA_CH_TRIGSRC_ADCA_CH0_gc);				// trigger the DMA from the ADC
	dma_channel_set_transfer_count(&config_params,(NUM_SAMPLES*sizeof(uint16_t)));			// set the number of bytes in the block transfer to total number of samples (single shot so this will take multiple events, but other settings are unaffected)
	dma_channel_set_source_address(&config_params,(uint16_t)(uintptr_t)(&(ADCA.CH0RES)));	// set the source address ti the ADC register
	dma_channel_set_destination_address(&config_params,(uint16_t)(uintptr_t)&buff_struct);  // set the destination address to buff_struct
	dma_channel_set_dest_reload_mode(&config_params,DMA_CH_DESTRELOAD_TRANSACTION_gc);		// reload the destination address after the 1 block transaction is transferred
	dma_channel_set_repeats(&config_params,1);												// set the number of blocks in the transaction to 1
	dma_channel_set_interrupt_level(&config_params,DMA_INT_LVL_LO);							// set the interrupt priority (WHY IS THERE NO INTERRUPT LEVEL ON THE OTHER CHANNELS?)
	dma_channel_write_config(ADC_DMA_CH, &config_params);									// write the configurations 
	dma_channel_enable(ADC_DMA_CH);															// enable the DMA channel (won't be triggered until clock is turned on at end of function)
	
	// setup dma channel to write to the D/A
	memset(&config_params,0,sizeof(config_params));
	dma_channel_set_single_shot(&config_params);
	dma_channel_set_burst_length(&config_params,DMA_CH_BURSTLEN_2BYTE_gc);
	dma_channel_set_src_reload_mode(&config_params,DMA_CH_SRCRELOAD_BLOCK_gc);
	dma_channel_set_src_dir_mode(&config_params,DMA_CH_SRCDIR_INC_gc);
	dma_channel_set_dest_dir_mode(&config_params,DMA_CH_DESTDIR_INC_gc);
	dma_channel_set_trigger_source(&config_params,DMA_CH_TRIGSRC_DACB_CH0_gc);
	dma_channel_set_transfer_count(&config_params,(NUM_WF*sizeof(uint16_t)));
	dma_channel_set_destination_address(&config_params,(uint16_t)(uintptr_t)(&DACB.CH0DATA));
	dma_channel_set_source_address(&config_params,(uint16_t)(uintptr_t)wf_data);
	dma_channel_set_dest_reload_mode(&config_params,DMA_CH_DESTRELOAD_BURST_gc);
	dma_channel_set_repeats(&config_params,NUM_CHANNELS+NUM_DUMMY_CH);
	// dma_channel_set_interrupt_level(&config_params,DMA_INT_LVL_OFF);
	dma_channel_write_config(DAC_DMA_CH, &config_params);
	dma_channel_enable(DAC_DMA_CH);	
	
	// for control (8-bit), use pins 1-8 on the J3 header, which is port D and E
	// setup dma channel to write control words to portE
	// note: the first write occurs AFTER the first pulse, so..
	//    - the first word is setup at portE in advance
	//    - there are only 13 values to write, starting with the second one
	memset(&config_params,0,sizeof(config_params));
	dma_channel_set_single_shot(&config_params);
	dma_channel_set_burst_length(&config_params,DMA_CH_BURSTLEN_1BYTE_gc);
	dma_channel_set_src_reload_mode(&config_params,DMA_CH_SRCRELOAD_BLOCK_gc);
	dma_channel_set_src_dir_mode(&config_params,DMA_CH_SRCDIR_INC_gc);
	dma_channel_set_dest_dir_mode(&config_params,DMA_CH_DESTDIR_FIXED_gc);
	dma_channel_set_trigger_source(&config_params,DMA_CH_TRIGSRC_EVSYS_CH2_gc);
	dma_channel_set_transfer_count(&config_params,NUM_CHANNELS+NUM_DUMMY_CH-1);
	dma_channel_set_destination_address(&config_params,(uint16_t)(uintptr_t)(&PORTE.OUT));
	dma_channel_set_source_address(&config_params,(uint16_t)(uintptr_t)&control_portE[1]);
	dma_channel_set_dest_reload_mode(&config_params,DMA_CH_DESTRELOAD_BURST_gc);
	dma_channel_set_repeats(&config_params,1);
	// dma_channel_set_interrupt_level(&config_params,DMA_INT_LVL_MED);
	dma_channel_write_config(DIO_DMA_CH, &config_params);
	dma_channel_enable(DIO_DMA_CH);
	PORTE.OUT = control_portE[0]; // assign the first word here
	
	// start things going by starting the first clock

    tc_write_clock_source(&TCC0,TC_CLKSEL_DIV2_gc); // 12 MHz clock
	// tc_write_clock_source(&TCC0,TC_CLKSEL_DIV4_gc); // 6 MHz clock
	cpu_irq_restore(flags);
}

int main(void) {
	EVSYS.CH1MUX = EVSYS_CHMUX_TCC0_OVF_gc; // the 1 MHz clock
	EVSYS.CH2MUX = EVSYS_CHMUX_TCD0_OVF_gc; // the 1 MHz / NUM_WF clock
		
	sysclk_init();
	board_init();
	pmic_init();
	gfx_mono_init();
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// note: need to refine when we know which pins we will use
	// must come after board_init, because it pre-configures ports
	PORTE.DIR = 0x0F;
	PORTB.DIR = 0x02;
	
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	udc_start();
	if (!udc_include_vbus_monitoring()) {
		udc_attach();
	}
	
	// for debugging
	tc_enable(&TCE0);
	tc_set_direction(&TCE0,TC_UP);
	tc_write_period(&TCE0,46874UL);
	tc_write_clock_source(&TCE0,TC_CLKSEL_DIV256_gc);
	tc_write_count(&TCE0,0);
	tc_set_overflow_interrupt_level(&TCE0,TC_OVFINTLVL_LO_gc);
	tc_set_overflow_interrupt_callback(&TCE0,toggle_led_callback);

	frame_callback();

	// turn off the "programming" led
	ioport_set_pin_high(LED3_GPIO);
	
	uint32_t tmp_ctr;
	do {
		sleep_mode();
				
		tmp_ctr = 0;
		do {
			tmp_ctr++;			
			asm("nop");
		} while( dma_get_channel_status(ADC_DMA_CH) != DMA_CH_FREE || dma_get_channel_status(DAC_DMA_CH) == DMA_CH_BUSY );

		// doesn't mitigate the glitch
		// for(uint32_t ii = 0; ii < 100000; ii++)
		// {
		// 	asm("nop");
		// 	asm("nop");
		// }

		enum dma_channel_status adc_status = dma_get_channel_status(ADC_DMA_CH);
		enum dma_channel_status dac_status = dma_get_channel_status(DAC_DMA_CH);
		if (adc_status == DMA_CH_FREE) {
			//bool is_err = (dac_status == DMA_CH_BUSY || tmp_ctr < 11170 || tmp_ctr > 11220); // didn't mitigate glitch
			bool is_err = (dac_status == DMA_CH_BUSY);
			dma_disable();
			dac_set_channel_value(&DACB,DAC_CH0,wf_data[160]);
			dac_wait_for_channel_ready(&DACB,DAC_CH0);
			if (!is_err) {
				for (uint16_t cIdx = 0; cIdx < NUM_CHANNELS; cIdx++) {
					buff_struct.adc_data[cIdx][0] |= ((cIdx+1) << 12);
				}
				
				// store tmp_ctr in buff_struct.adc_data[0:7][1]
				buff_struct.adc_data[0][1] |= (uint16_t)((tmp_ctr & 0xF0000000) >> 16);
				buff_struct.adc_data[1][1] |= (uint16_t)((tmp_ctr & 0x0F000000) >> 12);
				buff_struct.adc_data[2][1] |= (uint16_t)((tmp_ctr & 0x00F00000) >> 8);
				buff_struct.adc_data[3][1] |= (uint16_t)((tmp_ctr & 0x000F0000) >> 4);
				buff_struct.adc_data[4][1] |= (uint16_t)(tmp_ctr & 0x0000F000);
				buff_struct.adc_data[5][1] |= (uint16_t)((tmp_ctr & 0x00000F00) << 4);
				buff_struct.adc_data[6][1] |= (uint16_t)((tmp_ctr & 0x000000F0) << 8);
				buff_struct.adc_data[7][1] |= (uint16_t)((tmp_ctr & 0x0000000F) << 12);
				
				#ifdef ENABLE_USB					
					udi_cdc_write_buf((const int *)(&buff_struct.adc_data), NUM_SAMPLES_XFER*2);
				#endif
			}						
			frame_callback();	
		}
	} while (true);
}	

void main_vbus_action(bool b_high)
{
	if (b_high) {
		// Attach USB Device
		udc_attach();
	} else {
		// VBUS not present
		udc_detach();
	}
}
