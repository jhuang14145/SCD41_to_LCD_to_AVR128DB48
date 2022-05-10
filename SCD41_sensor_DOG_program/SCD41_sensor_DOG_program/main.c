/**
 * @file main.c
 *
 * Created: 4/21/2022 3:24:54 PM
 * Author : jhuan
 */ 

#include <avr/io.h>
#include "LCD_DOG_avr128_driver.h"
#include "SCD41_avr128_driver.h"
#include "USART3_avr128_driver.h"


int main(void)
{
    /* Replace with your application code */
	// initializing functions
	I2C0_SCD41_init();
	init_spi_lcd();
	init_lcd_dog();
	USART3_circular_buffer_init(); 
	USART0_Init(9600);
	bargraph_init();
	sei();
	
    while (1) 
    {
        SCD41_CO2_to_bargraph(merge_byte(MEASUREMENT_DATA[0],MEASUREMENT_DATA[1]));
		// first begin the initialization message, that sends out the address of the device, and performs startup
		start_periodic_measurement(START_PERIODIC_MEASUREMENT_ADDRESS, SLAVE_WRITE_ADDRESS);
		// this writes the read command, then returns an array whether or not the data is ready or not.
		get_data_ready_status(GET_DATA_READY_ADDRESS, SLAVE_WRITE_ADDRESS, SLAVE_READ_ADDRESS);
		// if the last three significant bits are 0, then the device is not ready to be polled
		if(!(DATA_READY[1] == 0x00 && (((int)(DATA_READY[0]) & (PIN2_bm | PIN1_bm | PIN0_bm)) == 0x00))){
			// stop polling for the data, and store the in an array
			stop_periodic_measurement(READ_MEASUREMENT_ADDRESS, SLAVE_WRITE_ADDRESS, SLAVE_READ_ADDRESS);
			// write the data to the screens while using dog lcd, and formatting the strings with padded white space
			// after every write, transmit the data to tera term through usart3
			sprintf(dsp_buff1, "CO2:  %-11d", merge_byte(MEASUREMENT_DATA[0], MEASUREMENT_DATA[1]));
			// calls the functions to read and send all the data from the buffer to tera term/termite after it has been updated
			USART3_buffer_to_tera_term(dsp_buff1);
			sprintf(dsp_buff2, "Temp: %-11d", get_temp(merge_byte(MEASUREMENT_DATA[3],MEASUREMENT_DATA[4])));
			USART3_buffer_to_tera_term(dsp_buff2);
			sprintf(dsp_buff3, "RH:   %-11d", get_hum(merge_byte(MEASUREMENT_DATA[6], MEASUREMENT_DATA[7])));
			USART3_buffer_to_tera_term(dsp_buff3);
			update_lcd_dog(); // update the dog lcd
		}
		// delay every 1 second, so that we update every second
		_delay_ms(500);

		// update bargraph with CO2 levels.
		SCD41_CO2_to_bargraph(merge_byte(MEASUREMENT_DATA[0],MEASUREMENT_DATA[1]));
		//
		
	}
}