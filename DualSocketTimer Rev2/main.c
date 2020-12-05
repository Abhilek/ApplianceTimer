/*
 * DualSocketTimer Rev2.c
 *
 * Created: 11/2/2020 8:19:11 PM
 * Author : Abhilekh
 */ 



//A single 16 bit timer used to keep two timings for switch 2 relays.

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <SSD1306.h>
#include <SSD1306.c>
#include <i2c.c>
#include <i2c.h>
#define RPORT PORTB
#define RDDR DDRB
#define INDDR DDRC
#define INPIN PINC
#define INPORT PORTC
#define BuzzPORT PORTD
#define BuzzDDR DDRD
#define switch1pin 0   // input pin for mins
#define switch2pin  1  // input pin for hours
#define portselectionpin 2// pressing this pin will toggle  select_port_switch_flag
#define hours_pos10 60  //column number where ten's digit value of hour will be displayed in OLED display
#define hours_pos1 66  ////column number where one's digit value of hour will be displayed in OLED display
#define colon1_pos 72  //+6 for gap between characters
#define minutes_pos10 78
#define minutes_pos1 84
#define colon2_pos 90
#define seconds_pos10 96
#define seconds_pos1 112
#define relay1_pin 0
#define relay2_pin 1
#define buzzer_pin 6
#define off 0
#define on 1

volatile uint8_t relay1_state=off; 
volatile uint8_t relay2_state=off; 
volatile uint8_t seconds1=0;  // set by internal timer
volatile uint8_t minutes1=0;
volatile uint8_t hours1=0;
volatile uint8_t seconds2=0;  // set by internal timer
volatile uint8_t minutes2=0;
volatile uint8_t hours2=0;
volatile uint8_t set_minutes1=0;  // user input for timer
volatile uint8_t set_hours1=0;
volatile uint8_t set_minutes2=0;  // user input for timer
volatile uint8_t set_hours2=0;
volatile uint8_t select_port_switch_flag=0;//if this value is 0, the time setting buttons will set minutes1,hours1 variables
volatile int time_remaininginminutes_timer1=0; //used to convert time to minutes for easy substraction in order to display remaining time
volatile int time_remaininginminutes_timer2=0;

void printzerotime1();
void printzerotime2();
void printPortselected();
void relay1off();
void relay2off();
void relay1on();
void relay2on();
void relay1_off_sequence();
void relay2_off_sequence();
void disp_time_remaining_timer1();
void disp_time_remaining_timer2();
void keypress_sound();
void timeup_sound();

int main(void)
{
	RDDR=0xFF; //relay O/P
	BuzzDDR=0xFF;
	INDDR=0x00;
	INPORT=0xFF;//pullup
	OLED_Init();  //initialize the OLED
	OLED_Clear();
	TCNT1H=(-7812)>>8;
	TCNT1L=(-7812)&0xFF;// over flow after 977 clocks__works for1MHZ...[1/1000000x1024x977=1 second]
	TCCR1A=0x00;//normal mode
	TCCR1B=0x05;//internal clock ,prescaler 1:1024
	TIMSK=(1<<TOIE1);// enable timer1 interrupt
	_delay_ms(100);  /// delay added to stabilize
	printzerotime1();
	printzerotime2();
	printPortselected(); //prints on OLED which timer will be set when buttons pressed
	while (1)
	{
		printPortselected(); // prints active port
		if(!(INPIN&(1<<portselectionpin)))
		{
			_delay_ms(100);//debounce
			if(!(INPIN&(1<<portselectionpin)))
			{
				keypress_sound();
				select_port_switch_flag=!select_port_switch_flag;	
			}
		}
		if((!(INPIN&(1<<switch1pin)))&&select_port_switch_flag==0)
		{
			_delay_ms(100);
			if(!(INPIN&(1<<switch1pin)))
			{
				keypress_sound();
				set_minutes1=set_minutes1+15;
				if(set_minutes1>45)
				{
					set_minutes1=0;
				}
				if(set_minutes1>0)
				{
					disp_time_remaining_timer1();
					relay1on();
					sei();
				}
			}
		}
		if((!(INPIN&(1<<switch2pin)))&&select_port_switch_flag==0)
		{
			_delay_ms(100);
			if(!(INPIN&(1<<switch2pin)))
			{
				keypress_sound();
				set_hours1++;
				if(set_hours1>11)
				{
					set_hours1=0;
				}
				if(set_hours1>0)
				{
					disp_time_remaining_timer1();
					relay1on();
					sei();
				}		
			}
		}
		if((!(INPIN&(1<<switch1pin)))&&select_port_switch_flag==1)
		{
			_delay_ms(100);
			if(!(INPIN&(1<<switch1pin)))
			{
				keypress_sound();
				set_minutes2=set_minutes2+15;
				if(set_minutes2>45)
				{
					set_minutes2=0;
				}
				if(set_minutes2>0)
				{
					disp_time_remaining_timer2();
					relay2on();
					sei();
				}
			}
		}
		if((!(INPIN&(1<<switch2pin)))&&select_port_switch_flag==1)
		{
			_delay_ms(100);
			if(!(INPIN&(1<<switch2pin)))
			{
				keypress_sound();
				set_hours2++;
				if(set_hours2>11)
				{
					set_hours2=0;
				}
				if(set_hours2>0)
				{
					disp_time_remaining_timer2();
					relay2on();
					sei();
				}
				
			}
		}
		
		if(!(INPIN&(1<<switch2pin)))// if both switches pressed simultaneously//
		{
			_delay_ms(100);
			if(!(INPIN&(1<<switch1pin)))
			{
				keypress_sound();
				if(select_port_switch_flag==0){
				relay1_off_sequence();}
				if(select_port_switch_flag==1){
				relay2_off_sequence();}
			}
		}
		if(relay1_state==on)
		{
			disp_time_remaining_timer1();//displays time remaining for relay1 to switch off
			if((minutes1==set_minutes1)&&(hours1==set_hours1)&&(seconds1==59))
			{
				relay1_off_sequence();
			}
		}
		if(relay2_state==on)
		{
			disp_time_remaining_timer2();//displays time remaining for relay1 to switch off
			if((minutes2==set_minutes2)&&(hours2==set_hours2)&&(seconds2==59))
			{
				relay2_off_sequence();
			}
		}
	}
	return 0;
}

ISR(TIMER1_OVF_vect)
{
	if(relay1_state==on){
	seconds1++;}
	if(relay2_state==on){
	seconds2++;}
	
	if(seconds1>59)
	{
		minutes1++;
		seconds1=0;
	}
	if(seconds2>59)
	{
		minutes2++;
		seconds2=0;
	}
	if(minutes1>59)
	{
		hours1++;
		minutes1=0;
	}
	if(minutes2>59)
	{
		hours2++;
		minutes2=0;
	}
	TCNT1H=(-7812)>>8;
	TCNT1L=(-7812)&0xFF;
	
}
void disp_time_remaining_timer1()
{
	time_remaininginminutes_timer1=((set_hours1*60)+set_minutes1)-((hours1*60)+minutes1);
	OLED_SetCursor(3,hours_pos10);
	OLED_DisplayNumber(10,(time_remaininginminutes_timer1/60),2); // time remaining is calculated in mins so dividing by 60 gives hour value
	OLED_SetCursor(3,minutes_pos10);
	OLED_DisplayNumber(10,(time_remaininginminutes_timer1%60),2);
	OLED_SetCursor(3,seconds_pos10);
	OLED_DisplayNumber(10,(60-seconds1),2);
}
void disp_time_remaining_timer2()
{
		time_remaininginminutes_timer2=((set_hours2*60)+set_minutes2)-((hours2*60)+minutes2);
		OLED_SetCursor(5,hours_pos10);
		OLED_DisplayNumber(10,(time_remaininginminutes_timer2/60),2); // time remaining is calculated in mins so dividing by 60 gives hour value
		OLED_SetCursor(5,minutes_pos10);
		OLED_DisplayNumber(10,(time_remaininginminutes_timer2%60),2);
		OLED_SetCursor(5,seconds_pos10);
		OLED_DisplayNumber(10,(60-seconds2),2);
}
void relay1off()
{
	RPORT&=~(1<<relay1_pin);
	relay1_state=off;
}
void relay2off()
{
	RPORT&=~(1<<relay2_pin);
	relay2_state=off;
}
void relay1on()
{
 		RPORT|=(1<<relay1_pin);
		relay1_state=on;
	
}
void relay2on()
{
	RPORT|=(1<<relay2_pin);
	relay2_state=on;
	
}
void printPortselected() //prints on OLED which timer will be set when buttons pressed
{
	OLED_SetCursor(1,1);
	OLED_Printf("PORT selected =");
	OLED_SetCursor(1,96);
	OLED_DisplayNumber(10,select_port_switch_flag+1,1);
}

void printzerotime1()
{
	OLED_SetCursor(3,1);           // go to row 0 column 1
	OLED_Printf("<Timer 1>"); 
	OLED_SetCursor(3,hours_pos10);           // display set time as 0:00
	OLED_DisplayNumber(10,0,2);   //(uint8_t v_numericSystem_u8, uint32_t v_number_u32, uint8_t v_numOfDigitsToDisplay_u8) , 0 hours
	OLED_SetCursor(3,colon1_pos);
	OLED_Printf(":");
	OLED_SetCursor(3,minutes_pos10);
	OLED_DisplayNumber(10,0,2);		// 0 minutes
	OLED_SetCursor(3,colon2_pos);
	OLED_Printf(":");
	OLED_SetCursor(3,seconds_pos10);
	OLED_DisplayNumber(10,0,2);	// 0 seconds
}
void printzerotime2()
{
	OLED_SetCursor(5,1);           
	OLED_Printf("<Timer 2>");
	OLED_SetCursor(5,hours_pos10);           // display set time as 0:00
	OLED_DisplayNumber(10,0,2);   //(uint8_t v_numericSystem_u8, uint32_t v_number_u32, uint8_t v_numOfDigitsToDisplay_u8) , 0 hours
	OLED_SetCursor(5,colon1_pos);
	OLED_Printf(":");
	OLED_SetCursor(5,minutes_pos10);
	OLED_DisplayNumber(10,0,2);		// 0 minutes
	OLED_SetCursor(5,colon2_pos);
	OLED_Printf(":");
	OLED_SetCursor(5,seconds_pos10);
	OLED_DisplayNumber(10,0,2);

}

void relay1_off_sequence()
{
	printzerotime1();
	if(relay2_state==off){
	cli();}
	relay1off();
	_delay_ms(50);
	set_minutes1=0;
	set_hours1=0;
	minutes1=0;
	hours1=0;
	seconds1=0;
	timeup_sound();
}
void relay2_off_sequence()
{
	printzerotime2();
	if(relay1_state==off){
	cli();}
	relay2off();
	_delay_ms(50);
	set_minutes2=0;
	set_hours2=0;
	minutes2=0;
	hours2=0;
	seconds2=0;
	timeup_sound();
}
void keypress_sound()
{
	BuzzPORT|=(1<<buzzer_pin);
	_delay_ms(80);
	BuzzPORT&=~(1<<buzzer_pin);
}
void timeup_sound()
{
	BuzzPORT|=(1<<buzzer_pin);
	_delay_ms(300);
	BuzzPORT&=~(1<<buzzer_pin);	
}

