#define F_CPU 4000000
#define SLAVE_WRITE_ADDRESS 0xc4
#define SLAVE_READ_ADDRESS 0xc5
#define START_PERIODIC_MEASUREMENT_ADDRESS 0x21b1
#define READ_MEASUREMENT_ADDRESS 0xec05
#define STOP_PERIODIC_MEASUREMENT_ADDRESS 0x3f86
#define GET_DATA_READY_ADDRESS 0xe4b8

#include <avr/io.h>
#include <util/delay.h>

uint8_t MEASUREMENT_DATA[9];
uint8_t DATA_READY[3];
/************************************************************************/
// Function Name: I2C0_SCD41_init
// Parameters: none
// Returns: none
// Description: This enables the I2C setup connection from the avr128, to
// the scd sensor. it calculates and set's the baud rate, pa2 and pa3 as
// the scl and sda of the device, it then finally enables all the 
// interrupt flags in order to poll them when data has been written/read
/************************************************************************/
void I2C0_SCD41_init(){
	VPORTA.DIR &= 0x00;//~(PIN2_bm | PIN3_bm); // set pa3 and pa3 and inputs
	//TWI0.MBAUD = (F_CPU/(2*(1/(100*pow(10,-6))))-(5+((F_CPU*(1000*pow(10,-9)))/2))); // calculate the baud rate
	TWI0.MBAUD = 14;
	TWI0.MCTRLA = /*TWI_RIEN_bm | TWI_WIEN_bm |*/ TWI_ENABLE_bm; // enable the TWI controller, and the write int flag
	TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc; // force idle to prevent floating state

}

/************************************************************************/
// Function Name: start_periodic_measurement
// Parameters: uint16_t opcode, uint8_t slave_address
// Returns: none
// Description: This writes to the scd41 devices and tells it to begin
// polling for data. It writes the opcode for begin polling, it then
// writes the address of the slave(scd41) and where to establish a 
// connection to the device. Since the opcode is 16-bit integers, it 
// must be send twice one by one in order to process it
/************************************************************************/
void start_periodic_measurement(uint16_t opcode, uint8_t slave_address){
	TWI0.MADDR = slave_address;	 // write the address of the slave
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t) (opcode >> 8); //send first half
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t) (opcode); //send second half
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc; // send stop command and ACK
}

/************************************************************************/
// Function Name: get_data_ready_status
// Parameters: uint16_t opcode, uint8_t write_header, uint8_t read_header
// Returns: none
// Description: writes the opcode direction in order to first begin 
// writing the data of the slave and directions. then one by one, the
// data is read three time because it sends 3 bytes of data 3 times, we 
// then store this data into an array which is read later in order to
// determine if all the data has been transmitted and is ok or ready
// to be read into another array to then read/print out
/************************************************************************/
void get_data_ready_status(uint16_t opcode, uint8_t write_header, uint8_t read_header){
	TWI0.MADDR = write_header; // enable writing
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t)(opcode >> 8);
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t)(opcode);
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	_delay_ms(1); // delay for command execution time
	
	TWI0.MADDR = read_header; // enable reading from the device
	while(!(TWI0.MSTATUS & TWI_RIF_bm)){
		// wait for the read to complete
	}
	
	DATA_READY[0] = TWI0.MDATA; // store first data
	TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // send an ACK
	while(!(TWI0.MSTATUS & TWI_RIF_bm)){
		// wait for the read to complete
	}
	
	DATA_READY[1] = TWI0.MDATA; // store second data
	TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // send an ACK
	while(!(TWI0.MSTATUS & TWI_RIF_bm)){
		// wait for the read to complete
	}
	
	DATA_READY[2] = TWI0.MDATA; // store third data
	TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc; // send an ACK
	while(!(TWI0.MSTATUS & TWI_RIF_bm)){
		// wait for the read to complete
	}
	
	TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc; // issue a NACK with a stop command
}

/************************************************************************/
// Function Name: stop_periodic_measurement
// Parameters: uint16_t opcode, uint8_t write_header, uint8_t read_header
// Returns: none
// Description: writes the opcode direction in order to first begin
// writing the data of the slave and directions. This function should
// only run when the data is all ready to be read. once called, it will
// enable the writing of the slave address and the opcode associated
// with the device. afterwards, it will begin to read in the data one by 
// one from the device, while providing proper ACKs and NACKs per section
// of data read by the device. It will then store the data into an array
/************************************************************************/
void stop_periodic_measurement(uint16_t opcode, uint8_t write_header, uint8_t read_header){
	TWI0.MADDR = write_header; // enable writing
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t)(opcode >> 8);
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	TWI0.MDATA = (uint8_t)(opcode);
	while(!(TWI0.MSTATUS & TWI_WIF_bm)){
		// wait for the write to complete
	}
	
	_delay_ms(1); // wait for command execution time
	
	TWI0.MADDR = read_header; // enable reading
	// this is responsible for all the indexing for the array
	for(int i = 0; i < 9; i++){
		while(!(TWI0.MSTATUS & TWI_RIF_bm)){
			// wait for the read to complete
		}
		MEASUREMENT_DATA[i] = TWI0.MDATA;  // obtain data
		// since every three data sections, is a different type of data, we must code to
		// send out a NACK response after every three reads instead of just sending an
		// ACK to the device for one part of the read
		TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;  // send a NACK
	}
	TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc; // issue a NACK and a stop command 
}

/************************************************************************/
// Function Name: merge_byte
// Parameters: uint8_t x1, uint8_t x2
// Returns: uint16_t
// Description: take sin two uint8_t integers, then merges it into one
// uint16_t integer with x1 as the high byte and x2 as the low byte
/************************************************************************/
uint16_t merge_byte(uint8_t x1, uint8_t x2){
	// merge x1, x2 as high-byte/low-byte
	uint16_t temp = x1;
	temp = temp << 8;
	temp = temp | x2;
	return temp;
}

/************************************************************************/
// Function Name: get_temp
// Parameters: uint16_t x
// Returns: uint16_t
// Description: using scd41 documentation format, it converts a uint16_t
// integer into a readable temperature in Celsius using SCD41 document
// formula for conversions : (-45+((175*(uint16_t))/(pow(2,16))))
/************************************************************************/
uint16_t get_temp(uint16_t x){
	// (-45+((175*(x))/(pow(2,16))))
	x = x / 16;
	x = x * 5;
	x = x / 16;
	x = x * 5;
	x = x / 16;
	x = x * 7;
	x = x / 16;
	x = x - 45;
	return x;
}

/************************************************************************/
// Function Name: get_hum
// Parameters: uint16_t x
// Returns: uint16_t
// Description: using scd41 documentation format, it converts a uint16_t
// integer into a readable relative humidity using SCD41 document
// formula for conversions : ((100*uint16_t)/(pow(2,16)))
/************************************************************************/
uint16_t get_hum(uint16_t x){
	// ((100*MEASUREMENT_DATA)/(pow(2,16)))
	x = x * 5;
	x = x / 16;
	x = x * 2;
	x = x / 16;
	x = x * 2;
	x = x / 16;
	x = x * 5;
	x = x / 16;
	return x;
}

/************************************************************************/
// Function Name: bargraph_init
// Parameters: none
// Returns: none
// Description: initializes the bargraph by setting PORTD to outputs
/************************************************************************/
void bargraph_init(){
	VPORTD.DIR = 0xFF; // turn on all PORTD as outputs for bargraph
	VPORTD.OUT = 0x00;
}

/************************************************************************/
// Function Name: SCD41_CO2_to_bargraph
// Parameters: uint16_t value
// Returns: none
// Description: reads the uint16_t and determines the relative output 
// for the bargraph. bargarph_init(), must be called before this. lights
// begins at 400ppm, to 499ppm, then lights up two bars at 500ppm to
// 599ppm and so on. up till over 1100ppm where all lights are on
/************************************************************************/
void SCD41_CO2_to_bargraph(uint16_t value){
	double num_bars = value;
	// check if the value is under 400, if so, don't turn on any lights
	if(num_bars < 400){
		return;
	}
	else{
		// if 400 or over, turn on LEDs
		if(num_bars >= 400 && num_bars <= 499){
			VPORTD_OUT = ~0x01;
		}
		else if(num_bars >= 500 && num_bars <= 599){
			VPORTD_OUT = ~0b00000011;
		}
		else if(num_bars >= 600 && num_bars <= 699){
			VPORTD_OUT = ~0b00000111;
		}
		else if(num_bars >= 700 && num_bars <= 799){
			VPORTD_OUT = ~0b00001111;
		}
		else if(num_bars >= 800 && num_bars <= 899){
			VPORTD_OUT = ~0b00011111;
		}
		else if(num_bars >= 900 && num_bars <= 999){
			VPORTD_OUT = ~0b00111111;
		}
		else if(num_bars >= 1000 && num_bars <= 1099){
			VPORTD_OUT = ~0b01111111;
		}
		else if(num_bars >= 1100){
			VPORTD_OUT = ~0b11111111;
		}
		else{
			VPORTD_OUT = ~0xCA;
		}
	}
}

