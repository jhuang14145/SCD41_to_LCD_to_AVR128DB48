#include <stdio.h>
#include <avr/io.h>

#define DOG_BUFFER_SIZE 17

char dsp_buff1[DOG_BUFFER_SIZE];
char dsp_buff2[DOG_BUFFER_SIZE];
char dsp_buff3[DOG_BUFFER_SIZE];

void lcd_spi_transmit_CMD (unsigned char cmd);
void lcd_spi_transmit_DATA (unsigned char cmd);
void init_spi_lcd (void);
void init_lcd_dog (void);
void delay_40mS(void);
void delay_30uS(void);
void update_lcd_dog(void);


/************************************************************************/
/*
** Title: lcd_spi_transmit_CMD
** Parameters: utint8_t data
** Returns: none
** Description: selects the chip, then outputs the data from parameter.
** It then waits until the data is done sending, then deselects the chip
**
*/
/************************************************************************/
void lcd_spi_transmit_CMD(uint8_t data) {
	// 0 to rs
	VPORTC_OUT = 0x00;
	// slave select
	VPORTA_OUT &= ~PIN7_bm;
	// output data
	SPI0.DATA = data;
	// loop until interrupt flag is set, which means data is done transmitting
	while ((SPI0.INTFLAGS & SPI_IF_bm) != SPI_IF_bm) {}
	// deselect
	VPORTA_OUT |= PIN7_bm;
}

/************************************************************************/
/*
** Title: lcd_spi_transmit_DATA
** Parameters: utint8_t data
** Returns: none
** Description: selects the chip, then outputs the data from parameter.
** It then waits until the data is done sending, then deselects the chip.
** Only difference between this and above isoutputting 1 to RS of lcd
*/
/************************************************************************/
//poll the IF flag and write the parameter 'data' to the SPI0 module's DATA register, then deselect the slave select pin,
void lcd_spi_transmit_DATA(uint8_t data) {
	//output 1 to rs
	VPORTC_OUT = PIN0_bm;
	//slave select
	VPORTA_OUT &= ~PIN7_bm;
	//output data
	SPI0.DATA = data;
	//wait for flag to be set
	while ((SPI0.INTFLAGS & SPI_IF_bm) != SPI_IF_bm) {}
	//now that the transfer is complete, deselect SS
	VPORTA_OUT |= PIN7_bm;

}

/************************************************************************/
/*
** Title: init_spi_lcd
** Parameters: none
** Returns: none
** Description: This the initialization of lcd. it assigns the output and
** pin locations for all mosi,sck,_ss,miso, and rs. for the lcd
**
*/
/************************************************************************/
void init_spi_lcd (void) {
	//vport a pin setup for mosi...etc
	VPORTA_DIR |= PIN4_bm | PIN6_bm | PIN7_bm;
	//pc0 for rs
	VPORTC_DIR |= PIN0_bm;
	//select chip
	VPORTA_OUT &= ~PIN7_bm;
	//enabvle precaler and enable master
	SPI0.CTRLA = SPI_MASTER_bm | SPI_ENABLE_bm;
	//disable slave disable
	//enable sampling for rsiing edge
	SPI0.CTRLB = SPI_SSD_bm | SPI_MODE_3_gc ;
	// deseclect chip
	VPORTA_OUT |= PIN7_bm;
}

/************************************************************************/
/*
** Title: init_lcd_dog
** Parameters: none
** Returns: none
** Description: Initialize the dog
**
**
*/
/************************************************************************/
void init_lcd_dog (void) {

	init_spi_lcd(); //Initialize mcu for LCD SPI

	//start_dly_40ms:
	delay_40mS();    //startup delay.


	//func_set1:
	lcd_spi_transmit_CMD(0x39);   // send function set #1
	delay_30uS(); //delay for command to be processed


	//func_set2:
	lcd_spi_transmit_CMD(0x39); //send function set #2
	delay_30uS(); //delay for command to be processed


	//bias_set:
	lcd_spi_transmit_CMD(0x1E); //set bias value.
	delay_30uS(); //delay for command to be processed


	//power_ctrl:
	lcd_spi_transmit_CMD(0x55); //~ 0x50 nominal for 5V
	//~ 0x55 for 3.3V (delicate adjustment).
	delay_30uS(); //delay for command to be processed


	//follower_ctrl:
	lcd_spi_transmit_CMD(0x6C); //follower mode on...
	delay_40mS(); //delay for command to be processed


	//contrast_set:
	lcd_spi_transmit_CMD(0x7F); //~ 77 for 5V, ~ 7F for 3.3V
	delay_30uS(); //delay for command to be processed


	//display_on:
	lcd_spi_transmit_CMD(0x0c); //display on, cursor off, blink off
	delay_30uS(); //delay for command to be processed


	//clr_display:
	lcd_spi_transmit_CMD(0x01); //clear display, cursor home
	delay_30uS(); //delay for command to be processed


	//entry_mode:
	lcd_spi_transmit_CMD(0x06); //clear display, cursor home
	delay_30uS(); //delay for command to be processed
}
/************************************************************************/
/*
** Title: delay_40mS
** Parameters: none
** Returns: none
** Description: loop nop until roughly 40ms
**
**
*/
/************************************************************************/
void delay_40mS(void) {
	int i;
	for (int n = 40; n > 0; n--)
	for (i = 0; i < 800; i++)
	__asm("nop");
}
/************************************************************************/
/*
** Title: delay_30uS
** Parameters: none
** Returns: none
** Description: loop for roughly 30ms
**
**
*/
/************************************************************************/
void delay_30uS(void) {
	int i;
	for (int n = 1; n > 0; n--)
	for (i = 0; i < 2; i++)
	__asm("nop");
}
/************************************************************************/
/*
** Title: update_lcd_dog
** Parameters: none
** Returns: none
** Description: initialize the lcd, transmit the data from one buffer to
** another line by line
**
*/
/************************************************************************/
void update_lcd_dog(void) {

	//call the init function
	init_spi_lcd();

	//transmite line1
	lcd_spi_transmit_CMD(0x80); //init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 16; i++) {
		lcd_spi_transmit_DATA(dsp_buff1[i]);
		delay_30uS();
	}

	// loop through each character in the buffer and output the data
	lcd_spi_transmit_CMD(0x90); //init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 16; i++) {
		lcd_spi_transmit_DATA(dsp_buff2[i]);
		delay_30uS();
	}

	// same as above, just throgh each buffer
	lcd_spi_transmit_CMD(0xA0); //init DDRAM addr-ctr
	delay_30uS();
	for (int i = 0; i < 16; i++) {
		lcd_spi_transmit_DATA(dsp_buff3[i]);
		delay_30uS();
	}
}


