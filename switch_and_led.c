#include<htc.h>
 
#define sw RB3                  //Switch is connected at PORTD.0
 
void main()
{
    TRISB=0xff;               //Port B act as intput
    TRISD=0;            //Port D act as output
    while(1) {
        if(!sw) {
            PORTD=0xff;       //LED ON
        } else 
            PORTD=0;          //LED OFF
    }
}