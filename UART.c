#include <mega16.h>
#include <delay.h>

#define FOSC 8000000// frecventa de oscilatie
#define BAUD 9600 // dorim un baud-rate de 9600
#define UBRR  FOSC/16/BAUD - 1

void USART_init(unsigned int ubrr);
void USART_transmit(unsigned char data);
void serial_print(flash unsigned char str[]);
void print_str(char *strptr);
char* receive_str(void);
unsigned char USART_receive(void);


void main(void)
{
    USART_init(UBRR);
    while(1)
    {
        USART_transmit(32);
        print_str(receive_str());
        delay_ms(100);
    }
}

void USART_init(unsigned int ubrr)
{
    UBRRL = (unsigned char)ubrr;
    UBRRH = (unsigned char)(ubrr>>8);
    
    // now we enable the transmission and reception of data, by enabling the RX, TX and UDRE flags from UCSRA register
    UCSRA = 0x00; 
    UCSRB = (1<<RXEN) | (1<<TXEN);
    UCSRC = 0x86;
}

void USART_transmit(unsigned char data)
{
    // Wait for the end of transmission. That is when the flag UDRE is set to 1;
    while(!(UCSRA & (1<<UDRE))) // Wait for UDRE flag to be set to 1
         ;
    UDR = data; // now the UDRE flag is cleared
}

unsigned char USART_receive(void)
{
    //Wait for the reception to end  
    while(!(UCSRA & (1<<RXC)))// RXC flag to be set to 1
        ;
    return UDR;
}

void serial_print(flash unsigned char str[])
{
    short i;
    for(i = 0; str[i]!='\0'; i++)
    {
        USART_transmit(str[i]);
    }
}

void print_str(char *strptr)
{
    while(*strptr!=0x00)
    {
        USART_transmit(*strptr);
        strptr++;
    }
}

char *receive_str(void)
{
      char str[20], i = 0;
      str[0] = USART_receive();
      while(str[i] != 46)
      {
        str[++i] = USART_receive();
      }
      str[i] = 0x00;
      return str;
}