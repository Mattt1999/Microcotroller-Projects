#include <mega16.h>
#include <delay.h>

//variabile globale
short pr1, pr2, pr3;//pr stands for prize 
short seed=0; // for random number generator.

void initial_light();

void main(void)
{ 
    DDRA = 0b11111111; // we declae port A as output  . DDR(Data direcion register) -> decides if the port will be used as input or output
    DDRD = 0b00000000; // config port D for digital input
    PORTD = 0x00;
    PORTA = 0x00;  // turn off all output bits of port A
    
    while (1)
      {     
            if(PIND & (1<<PIND0))
            {
                while(PIND & (1<<PIND0))//cat timp butonul este apasat;
                {   initial_light();
                    seed++;         
                    pr1 ^= (seed>>1);
                    pr2 ^= (seed>>2);
                    pr3 ^= (seed>>3);
                }
                pr1 &= 3;
                pr2 &= 3;
                pr3 &= 3;
                
                if(pr1 == 3 && pr2 == 3 && pr3 == 3)
                    PORTA |= (1<<PINA0);
                else if (pr1 == pr2 || pr2 == pr3 && pr1 == pr3)
                    PORTA |= (1<<PINA1);
                else if(pr1 == 3 || pr2 == 3 || pr3 == 3)
                    PORTA |= (1<<PINA2);
                else
                    PORTA |= (1<<PINA3); 
                
            }
            
      }
}

void initial_light()
{
    PORTA |= (1<<PINA0);
    delay_ms(250);
    PORTA &= ~(1<<PINA0);
    
    PORTA |= (1<<PINA1);
    delay_ms(200);
    PORTA &= ~(1<<PINA1);
    
    PORTA |= (1<<PINA2);
    delay_ms(150);
    PORTA &= ~(1<<PINA2);
    
    PORTA |= (1<<PINA3);
    delay_ms(100);
    PORTA &= ~(1<<PINA3);
}
