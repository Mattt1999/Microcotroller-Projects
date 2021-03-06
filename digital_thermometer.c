/*******************************************************
Chip type               : ATmega16
Program type            : Application
AVR Core Clock frequency: 8,000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*******************************************************/

#include <mega16.h>
#include <delay.h>
#include <stdint.h>

#define RS  6
#define E 7

volatile uint16_t flag=2; // flag for button
volatile bit flag_start_conversion = 0; // we wait 1s for a new conversion
volatile uint16_t timer_counter = 0;
uint8_t nr_cf;
int8_t vec[8], j;

void send_command(unsigned char cmd);// char data type is 8 bits wide
void send_character(unsigned char character);
void send_string(char *string);
void send_int(int16_t data);
void interrupt_init(void);
void lcd_reset(void);
void timer_init(void);
void adc_init(void);
int16_t adc_get_result(uint8_t channel);
 

interrupt [EXT_INT0] void ext_int0_int(void)
{
    flag = 0;
}

interrupt [EXT_INT1] void ext_int1_int(void)
{
    flag = 1;
}

interrupt [TIM0_OVF] void timer0_ovf_int(void)
{
    /*
        8Mhz / 1024 =  7812.5; 
        1/7812.5 = 0.000128;
        ==> each 128 us a pulse will be applied to timer0
        ==> 128*256 = 32768 us = 32.76 ms. This time the timer counts before an overflow.
        We want to measure the temp each 1 second, so we will make another variable to count the no. of times the interrupt occured. it should be eq with 30     
    */  
         
    if(timer_counter == 29*5)//wait 5s for a new conversion
    { 
        flag_start_conversion = 1;
        timer_counter = 0;// reinitialise the counter
    }
    else
    {
        timer_counter++;
    }
} 


void main(void)
{    
    char i;
    bit minus = 0;  
    int16_t data;
    interrupt_init();
    timer_init();   

    DDRC = 0xff;// all data pins are at PORTC are output
    DDRD = 0xff;// the 6 and 7 bits of PORTD are set for output 
    DDRA = 0x00;// all pins from A are inputs
    
    lcd_reset();    
    
    send_string("Temp is: ");
    send_command(0xC0);
    
    adc_init();
    
    while(1)
    {    
        
        if(flag == 0)//
        {
            data =   ((adc_get_result(2)-adc_get_result(1))/2.0)*100;
            if(data < 0)
            {
                send_character('-');
            }
            send_int(data); // send the result to the LCD
            for(i = 0; i<nr_cf+1; i++)
                send_command(0x10);//move cursor right by one character
            flag = 2; // clear the flag
        }
        if(flag==1)
        {
            /*
                We have Internal Ref at 5V. => Vlsb = 5/1024 aprox 5mV.
                LM35 offers a 10mV change with each degree celsius ==> 1 step of ADC represents 0.5 degrees 
                So we can divide the raw ADC val to 2 to get the temp
            */
            
            data =   ((adc_get_result(2)-adc_get_result(1))/2.0)*100;
            if(data < 0 && minus != 1) // for printing negative values
            {
                send_character('-');
                minus = 1;
            }
            
            if(flag_start_conversion == 1)
            {
                send_int(data);
                for(i = 0; i<nr_cf+1; i++)
                    send_command(0x10);//move cursor right by one character
            }
            flag_start_conversion = 0;
        }
        
    }
}

void send_command(unsigned char cmd)
{
    //we must configure the lcd to know that we are sending a command, so we set RS low and E high, then
    PORTC = cmd; // we put the command on port C 
    PORTD &= ~(1<<RS);
    PORTD |= (1<<E);
    delay_ms(50);
    PORTD &= ~(1<<E);
    PORTC = 0x00; // we clear the PORTC
}

void send_character(unsigned char character)
{
    PORTC = character;
    PORTD |= (1<<RS); // we set RS high, i.e. we send data
    PORTD |= (1<<E);
    delay_ms(100);
    PORTD &= ~(1<<E);
    PORTC = 0;
}

void send_string(char *string)
{
    unsigned char i = 0;
    while((*string)!='\0')
    {          
        i++;
        send_character(*string);
        string++;
        if(i==16) 
            send_command(0xC0);//moves the cursor on the second line
    }      
}

void interrupt_init(void)
{
    GICR |= ((1<<INT0) | (1<<INT1)); // enable INT0 and INT1 interrupts
    MCUCR |= (1<<ISC01) | (1<<ISC00); // we have set the ISC01 and ISC00 bits in order to have the interrupt occuring on the rising edge
    MCUCR |= (1<<ISC11) | (1<<ISC10); // analog, the interrupt will be generated by the rising edge, this time on INT1
    MCUSR |= (1<<ISC2);
    GIFR |= (1<<INTF1) | (1<<INTF2);// clears the intf_k flags before the interrupts to be executed --> the leds will be not lighted by default
    #asm("sei"); // enable the global interrupt bit
}

void lcd_reset(void)
{
    send_command(0x38); // init lcd in 16/2 8 bit mode
    delay_ms(20);
    send_command(0x0E); //clear LCD
    delay_ms(20);
    send_command(0x01);// cursor mode ON 
    delay_ms(20);
    send_command(0x80);// go to first line and position 0. If we add an offset, the character will be moved to the positio denoted by the offset on the first line;
    delay_ms(20);
}

void timer_init(void)
{
    TCCR0 |= ((1<<CS02) | (1<<CS00));// set the prescaler value to clk/1024
    TCNT0 = 0x00; // initialize the value from which the counter starts to count with 0.
    OCR0 = 0x00; // setting it to 0, it means that the counter will count from 0 to 255 and the interrupt will be generated at the 257th step, so it will count 256 ticks
    TIMSK |= (1<<TOIE0); // enable overflow interrupt. 
}

void send_int(int16_t data)
{         
    nr_cf = 0;
    if(data < 0)
    {    
        data = data * (-1);
    }
    while(data!=0)
    {
        vec[nr_cf] = data%10;
        data = data/10; 
        nr_cf++;
    } 
    for(j = nr_cf-1; j>=0; j--)
    {
        if(j == nr_cf-3 && nr_cf == 4)
            send_character('.');
        if(j == nr_cf-2 && nr_cf == 3)
            send_character('.');
       send_character(vec[j] + '0'); 
    }                 
}

void adc_init(void)
{
    ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1); // we enable the ADC and set its freq at clk/64. (8Mhz/200kHz = 40 -> we choose clk/64). We won't use interrupts here
    ADMUX |= (0<<REFS0) | (1<<REFS0); // external ref of 5V
    ADCSRA |= 0x10;
}

int16_t adc_get_result(uint8_t channel)
{
    //int16_t adcr;
    int8_t Ain_low;
    ADMUX &= ~(1<<MUX1); 
    ADMUX &= ~(1<<MUX0);
    if(channel == 2)
    {
        //send_string("c ");
        ADMUX |= (1<<MUX1);//input on pin 2
        ADCSRA |= (1<<ADSC);// we start a conversion
        while(ADCSRA & (1<<ADIF));//wait for it to finish, i.e. wait for bit ADIF to become 0   
        
        ADCSRA |= 0x10;
        
        delay_ms(1);
        Ain_low = (int)ADCL;
        return(Ain_low + (int)ADCH*256);
    }
    if(channel == 1)
    { 
        ADMUX |= (1<<MUX0);
        ADCSRA |= (1<<ADSC);// we start a conversion
        while(ADCSRA & (1<<ADIF));//wait for it to finish, i.e. wait for bit ADIF to become 0
        
        ADCSRA |= 0x10;
          
        delay_ms(1);
        Ain_low = (int)ADCL;
        return(Ain_low + (int)ADCH*256);
    }
}