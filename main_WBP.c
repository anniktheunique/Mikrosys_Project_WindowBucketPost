/*
 * WindowBucketPost.c
 * 
 * Created: Spring 2021
 * Author : Annik
 */ 

/* ----- Defines----- */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
//Calculate the value needed for the CTC match value in OCR1A
#define CTC_MATCH_OVERFLOW ((F_CPU/1000)/8)

/* ----- Variable ----- */
int bucket_covering_ldr = 100;

/* ----- Included Libraries ----- */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/* ----- Analog to Digital Converter (ADC) ----- */
void adc_init();
uint16_t adc_read(uint8_t ch);
volatile uint16_t adc_value_LDR1, adc_value_LDR2, adc_value_LDR3;

/* ----- Liquid Crystal Display (LCD) ----- */
void command_enable();
void data_enable();
void lcd_init();
void from_hedda_LDR1();
void from_tobias_LDR2();
void from_annik_LDR3();

/* ----- Timer ----- */
volatile unsigned long timer1_millis;
void timer_init();
unsigned long millis();

/* ----- Interrupt ----- */
ISR (TIMER1_COMPA_vect) // Interrupt Service Routine 
{
	timer1_millis++;
}


/* ===== Main Loop ===== */
int main()
{	
	adc_init(); // initialize adc
	_delay_ms(100);
	lcd_init(); // initialize lcd
	timer_init(); // initialize timer
	
	/* ===== Event Loop ===== */
	while(1)
	{
		adc_value_LDR1 = adc_read(0x00);	// read adc value at A0=0
		_delay_ms(100);
		adc_value_LDR2 = adc_read(0x01);	// read adc value at A1=1
		_delay_ms(100);
		adc_value_LDR3 = adc_read(0x02);	// read adc value at A2=2
		_delay_ms(100);
		
		/* ----- Checking where the post came from ----- */
		
		/* ----- Checking if it's from Hedda ----- */
		if (adc_value_LDR1 < bucket_covering_ldr) //Sensor detecting bucket
		{
			unsigned long time_dark_LDR1 = millis(); //Starts counting time
			if (time_dark_LDR1 > 5000) //If 5s passed, Hedda has put post in bucket
			{
				from_hedda_LDR1();
				if (time_dark_LDR1 > 10000) //Clear LCD after 10s
				{
					PORTD = 0x01;
					command_enable();
				}
			}
		}
		/* ----- Checking if it's from Tobias ----- */
		else if (adc_value_LDR2 < bucket_covering_ldr) //Sensor detecting bucket
		{
			unsigned long time_dark_LDR2 = millis(); //Starts counting time
			if (time_dark_LDR2 > 5000) //If 5s passed, Tobias has put post in bucket
			{
				from_tobias_LDR2();
				if (time_dark_LDR2 > 10000) //Clear LCD after 10s
				{
					PORTD = 0x01;
					command_enable();
				}
			}
		}
		/* ----- Checking if it's from Annik ----- */
		else if (adc_value_LDR3 < bucket_covering_ldr) //Sensor detecting bucket
		{
			unsigned long time_dark_LDR3 = millis(); //Starts counting time
			if (time_dark_LDR3 > 5000) //If 5s passed, Annik has put post in bucket
			{
				from_annik_LDR3();
				if (time_dark_LDR3 > 10000) //Clear LCD after 10s
				{
					PORTD = 0x01;
					command_enable();
				}
			}
		}
		/* ----- No Post ----- */
		else
		{
			PORTD = 0x01;
			command_enable();
		}
		_delay_ms(200);
	}
	return 0; //This line is never reached 
}


/* ----- Initialize Analog to Digital Converter (ADC) ----- */
void adc_init()
{
	ADMUX = (1<<REFS0); //Select AREF
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Enable ADC, prescaler F_ADC = 16M/128 = 125kHz
}

/* ----- Read ADC Value ----- */
uint16_t adc_read(uint8_t channel)
{
	//Selecting ADC input channels
	channel &= 0b00000111; //Keeps value between 0-7 (the 8 different ADC channels)
	ADMUX = (ADMUX & 0xF8)|channel; //Writing MUX bits
	
	ADCSRA |= (1<<ADSC); //Start single conversion, writing '1' to ADSC
	while(ADCSRA & (1<<ADSC)); //Wait for conversion to be done, then ADSC becomes '0' again
	return ADC; //Read ADC value
}

/* ----- Enable LCD Function ----- */
void command_enable() //Enable communication 
{
	PORTB &=! (1<<PB1); //Register Select Pin = 0 (command)
	PORTB |= (1<<PB0); //Enable high to low
	_delay_ms(10);
	PORTB &=! (1<<PB0); //Enable low to high
}

/* ----- Enable Data Transmission LCD Function ----- */
void data_enable() //enable to send data
{
	PORTB |= (1<<PB1); //Regiser Select Pin = 1 (data)
	PORTB |= (1<<PB0); //Enable high to low
	_delay_ms(10);
	PORTB &=! (1<<PB0); //Enable low to high
}

/* ----- Initialize Liquid Crystal Display ----- */
void lcd_init()
{
	DDRD = 0xFF; //PORTD as output (pins D4-D7)

	DDRB |= (1<<PB0); //PORTB pin 0 enable as output
	DDRB |= (1<<PB1); //PORTB pin 1 register select as output
	
	PORTD = 0x38; //Initialize as 7*5 matrix, 8-bit
	command_enable(); //Sends that command to the LCD
	
	PORTD = 0x0E; //Turn on LCD, cursor displayed
	command_enable();
	
	PORTD = 0x01; //Clear data
	command_enable();
	
	PORTD = 0x80; //First row and first column is chosen
	command_enable();
}

/* ----- Number1 Light Dependant Resistor Function (first post station) ----- */
void from_hedda_LDR1()
{	
	//Print massage on LCD one character at a time
	unsigned char name[17]={'P','O','S','T',' ','F','R','A',' ','H','E','D','D','A','!',' ',' '};
	int i;
	for (i = 0; i < 17; i ++)
	{
		PORTD = name[i]; 
		data_enable(); //Send data
		_delay_ms(10);
	}
}

/* ----- Number2 Light Dependant Resistor Function (second post station) ----- */
void from_tobias_LDR2()
{
	//Print massage on LCD one character at a time	
	unsigned char name[18]={'P','O','S','T',' ','F','R','A',' ','T','O','B','I','A','S','!',' ',' '};
	int i;
	for (i = 0; i < 18; i ++)
	{
		PORTD = name[i];
		data_enable(); //Send data
		_delay_ms(10);
	}
}

/* ----- Number3 Light Dependant Resistor Function (third post station) ----- */
void from_annik_LDR3()
{	
	//Print massage on LCD one character at a time
	unsigned char name[17]={'P','O','S','T',' ','F','R','A',' ','A','N','N','I','K','!',' ',' '};
	int i;
	for (i = 0; i < 17; i ++)
	{
		PORTD = name[i];
		data_enable(); //Send data
		_delay_ms(10);
	}
}

/* ----- Initialize Timer ----- */
void timer_init()
{
	TCCR1B |= (1 << WGM12) | (1 << CS11); //CTC mode, Clock/8
	
	//Load the high byte, then the low byte into Output Compare A on Timer 1
	OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	OCR1AL = CTC_MATCH_OVERFLOW;

	TIMSK1 |= (1 << OCIE1A); //Enable Compare Match Interrupt
	sei(); //Enable global interrupts
}

/* ----- Counting Milliseconds ---- */
unsigned long millis() //variable that holds milliseconds counted 
{
	unsigned long millis_return;
	ATOMIC_BLOCK(ATOMIC_FORCEON) //Ensures this cannot be disrupted
	{
		millis_return = timer1_millis;
	}
	return millis_return;
}