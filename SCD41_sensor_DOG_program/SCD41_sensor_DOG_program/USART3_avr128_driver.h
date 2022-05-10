/*
 * usart3_avr128_cir_buff.c
 *
 * Created: 3/9/2022 7:59:38 PM
 * Author : jhuan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
// AVR306: Using the AVR UART in C
// Routines for interrupt controlled USART
// Last modified: 02-06-21
// Modified by: AR

/* Includes */
//#include <iom128.h>
//#include <ina90.h>

/* UART Buffer Defines */
#define USART_RX_BUFFER_SIZE 128     /* 2,4,8,16,32,64,128 or 256 bytes */
#define USART_TX_BUFFER_SIZE 128     /* 2,4,8,16,32,64,128 or 256 bytes */
#define USART_RX_BUFFER_MASK ( USART_RX_BUFFER_SIZE - 1 )
#define USART_TX_BUFFER_MASK ( USART_TX_BUFFER_SIZE - 1 )
#if ( USART_RX_BUFFER_SIZE & USART_RX_BUFFER_MASK )
	#error RX buffer size is not a power of 2
#endif
#if ( USART_TX_BUFFER_SIZE & USART_TX_BUFFER_MASK )
	#error TX buffer size is not a power of 2
#endif

/* Static Variables */
static unsigned char USART_RxBuf[USART_RX_BUFFER_SIZE];
static volatile unsigned char USART_RxHead;
static volatile unsigned char USART_RxTail;
static unsigned char USART_TxBuf[USART_TX_BUFFER_SIZE];
static volatile unsigned char USART_TxHead;
static volatile unsigned char USART_TxTail;

/* Prototypes */
void USART0_Init( unsigned int baudrate );
unsigned char USART0_Receive( void );
void USART0_Transmit( unsigned char data );

/************************************************************************/
// Function Name: USART3_circular_buffer_init
// Parameters: none
// Returns: none
// Description: initializes the circular buffer to all zero. then enables
// pb2 as an input
/************************************************************************/
/**
* initializes the circular buffer to all zero. then enables
* pb2 as an input
*/
void USART3_circular_buffer_init(){
	USART_RxTail = 0x00;
	USART_RxHead = 0x00;
	USART_TxTail = 0x00;
	USART_TxHead = 0x00;
	
	PORTB.DIR &= ~PIN2_bm; // 0b11111011
	PORTB.PIN2CTRL = 0x08;
	
	VPORTB.DIR |= 0x01; // enable pb0 as the output to transmit.
	
}



/* Initialize USART */
void USART0_Init( unsigned int baudrate )
{
    PORTB.DIR = 0x01;
	unsigned char x;

	/* Set the baud rate */
	//UBRR0H = (unsigned char) (baudrate>>8);                  
	USART3.BAUD = (64.0*4000000)/(16.0*baudrate);
	

	
    USART3.CTRLC = 0x03; // asynchronous, no parity, 1 stop bit, 8 bit character
    
	/* Set frame format: 8 data 2stop */
	//USART3.CTRLA = 0b10100000;             //For devices with Extended IO
	//UCSR0C = (1<<URSEL)|(1<<USBS0)|(1<<UCSZ01)|(1<<UCSZ00);   //For devices without Extended IO
	
    /* Enable UART receiver and transmitter */
	USART3.CTRLB = 0b11000000;
    
	/* Flush receive buffer */
	x = 0; 			    

	USART_RxTail = x;
	USART_RxHead = x;
	USART_TxTail = x;
	USART_TxHead = x;
}


/* Interrupt handlers */


//ISR(USART3_RXC_vect)
//{
    //cli();
	//unsigned char data;
	//unsigned char tmphead;
//
	///* Read the received data */
	//data = USART3.RXDATAL;                 
	///* Calculate buffer index */
	//tmphead = ( USART_RxHead + 1 ) & USART_RX_BUFFER_MASK;
	//USART_RxHead = tmphead;      /* Store new index */
//
	//if ( tmphead == USART_RxTail )
	//{
		///* ERROR! Receive buffer overflow */
	//}
	//
	//USART_RxBuf[tmphead] = data; /* Store received data in buffer */
    //sei();
//}
//
//ISR(USART3_DRE_vect)
//{
    //cli();
	//unsigned char tmptail;
//
	///* Check if all data is transmitted */
	//if ( USART_TxHead != USART_TxTail )
	//{
		///* Calculate buffer index */
		//tmptail = ( USART_TxTail + 1 ) & USART_TX_BUFFER_MASK;
		//USART_TxTail = tmptail;      /* Store new index */
	//
		//USART3.TXDATAL = USART_TxBuf[tmptail];  /* Start transmition */
	//}
	//else
	//{
		//USART3.CTRLA &= ~(USART_DREIE_bm);         /* Disable UDRE interrupt */
	//}
    //sei();
//}


/* Read and write functions */
unsigned char USART0_Receive( void )
{
	unsigned char tmptail;
	
	while ( USART_RxHead == USART_RxTail )  /* Wait for incomming data */
		;
	tmptail = ( USART_RxTail + 1 ) & USART_RX_BUFFER_MASK;/* Calculate buffer index */
	
	USART_RxTail = tmptail;                /* Store new index */
	
	return USART_RxBuf[tmptail];           /* Return data */
}

void USART0_Transmit( unsigned char data )
{
	unsigned char tmphead;
	/* Calculate buffer index */
	tmphead = ( USART_TxHead + 1 ) & USART_TX_BUFFER_MASK; /* Wait for free space in buffer */
	int time_out_counter = 0;
    while ( tmphead == USART_TxTail ){
        time_out_counter++;
        if(time_out_counter = 10000000){
            break;
        }
    }

	USART_TxBuf[tmphead] = data;           /* Store data in buffer */
	USART_TxHead = tmphead;                /* Store new index */

	USART3.CTRLA |= (USART_DREIE_bm);   
                 /* Enable UDRE interrupt */
}

int DataInReceiveBuffer( void )
{
	return ( USART_RxHead != USART_RxTail ); /* Return 0 (FALSE) if the receive buffer is empty */
}

/************************************************************************/
// Function Name: USART3_buffer_to_tera_term
// Parameters: char arr[]
// Returns: none
// Description: takes in a character array. this is the array that is
// generated from dsp_buffer. this holds up to 17 characters from the
// entire line to be printed on the lcd display. it then will read the 
// array one by one and output that to the usart3 pin to be read in
// either tera term of termite
/************************************************************************/
void USART3_buffer_to_tera_term(char arr[]){
    
	for(int i = 0; i < DOG_BUFFER_SIZE; i++){
        USART3.TXDATAL = arr[i];
        _delay_ms(10);
        //USART3.TXDATAL = '\n';
        //_delay_ms(10);
		//USART0_Transmit(arr[i]);
	}

	//USART0_Transmit('\n');
	//unsigned char tmptail;
	//
	//if ( USART_TxHead != USART_TxTail )
	//{
		///* Calculate buffer index */
		//tmptail = ( USART_TxTail + 1 ) & USART_TX_BUFFER_MASK;
		//USART_TxTail = tmptail;      /* Store new index */
	//
		//USART3.TXDATAL = USART_TxBuf[tmptail];  /* Start transmition */
	//}
	//else
	//{
		//USART3.CTRLA &= ~(USART_DREIE_bm);         /* Disable UDRE interrupt */
	//}
	//
	
}

void send_to_pc(){
	unsigned char tmptail;
	
	/* Check if all data is transmitted */
	if ( USART_TxHead != USART_TxTail )
	{
	/* Calculate buffer index */
		tmptail = ( USART_TxTail + 1 ) & USART_TX_BUFFER_MASK;
		USART_TxTail = tmptail;      /* Store new index */
	
		USART3.TXDATAL = USART_TxBuf[tmptail];  /* Start transmition */
	}
	else
	{
		USART3.CTRLA &= ~(USART_DREIE_bm);         /* Disable UDRE interrupt */
	}
}