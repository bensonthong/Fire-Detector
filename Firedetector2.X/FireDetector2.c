#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>

#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config BOREN = OFF
#pragma config CCP2MX = PORTBE

#define RED_LED         PORTDbits.RD7 
#define ENTERBUTTON     PORTBbits.RB1
#define LEAVEBUTTON     PORTBbits.RB2

#define FAN_EN          PORTAbits.RA5
#define FAN_PWM         PORTCbits.RC2

void Init_ADC();                                                                // Start of Function Prototype
void INT0_ISR();
void interrupt high_priority chkisr();
void Select_ADC_Channel(char);
void Activate_Buzzer();
void Deactivate_Buzzer();
void Wait_Half_Second();
void Wait_One_Second_With_Beep();
void Do_Flashing();                                                             // End of Function Prototype
void init_Interrupt();
void Display_Lower_Digit(char);
void Display_Upper_Digit(char);

char array[10]= {0x01, 0x4f, 0x12,0x06,0x4C,0x24,0x20,0x0f,0x00,0x04};  //7 segment display values from 0-9
char FLASHING; 
char Manual_Alarm; 

void Delay_One_Sec()
{
    for(int I=0; I <17000; I++);
} 

void init_UART()
{
 OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF &
USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX &
USART_BRGH_HIGH, 25);
 OSCCON = 0x60;
}

void putch (char c)
{
 while (!TRMT);
 TXREG = c;
} 

void INT0_ISR() 
{ 
 INTCONbits.INT0IF=0;                                                           // Clear the interrupt flag 
 Manual_Alarm = 1;                                                              // Turn on Manual_Alarm Button(RB0)
} 

void interrupt high_priority chkisr()                                           // a high priority interrupt ISR
{                                                                               // if statements are a priority network 
if (INTCONbits.INT0IF == 1) INT0_ISR();                                         // If INT0 Flag is set first proceed with INT0_ISR
} 
void init_Interrupt()
{
    INTCONbits.INT0IF = 0;                                                      // Clear INT0 flag
    INTCONbits.INT0IE = 1;                                                      // enable INT0 
    INTCON2bits.INTEDG0 = 0;                                                    // INT0 EDGE falling
    INTCONbits.GIE = 1;                                                         // Set Global Interrupt Enable  
}
void Init_ADC(void)
{

 ADCON1=0x1B ;                                                                  // select pins AN0 through AN3 as analog signal, VDD-VSS as
                                                                                // reference voltage
 ADCON2=0xA9;                                                                   // right justify the result. Set the bit conversion time (TAD) and
                                                                                // acquisition time
}
unsigned int get_full_ADC(void)
{
int result;
ADCON0bits.GO=1;                                                                // Start Conversion
while(ADCON0bits.DONE==1);                                                      // wait for conversion to be completed
result = (ADRESH * 0x100) + ADRESL;                                             // combine result of upper byte and
                                                                                // lower byte into result
 return result;                                                                 // return the result.
}

void Select_ADC_Channel(char channel)
{
    ADCON0 = channel * 4 + 1;                                                   // ADCON0 select channel depending on the char channel 
}

void init_IO(){
    TRISA = 0x01;                                                               // PORTA as input for first bit
    TRISB = 0x07;                                                               // PORTB as input for first 3 bits
    TRISC = 0x00;                                                               // PORTC as output
    TRISD = 0x00;                                                               // PORTD as output
    TRISE = 0x00;                                                               // PORTE as output 
}


void main()
{
 OSCCON=0x70;                                                                   // Set oscillator to 8 MHz  
 RBPU = 0; 
 init_IO();                                                                     // Initialize the IO function
 Init_ADC();                                                                    // Initialize ADC
 init_UART();                                                                   // Initialize UART 
 init_Interrupt();
 
unsigned int people = 0 ;

 while(1){

    Select_ADC_Channel(0);                                                      // Channel 0 for Flame sensor 
    int num_step1 = get_full_ADC();                                             // Getting number of steps from ADC
    float voltage_mv1 = (num_step1 * 4.0)/1000;                                 // The voltage in mv is num_steps * 4
    
//    Select_ADC_Channel(1);                                                    // Channel 1 for Motion Sensor 1 (Entering building)
//    int num_step2 = get_full_ADC();                       
//    float voltage_mv2 = (num_step2 * 4.0)/1000;                               
    
//    Select_ADC_Channel(2);                                                    // Channel 1 for Motion Sensor 2 (Leaving building)
//    int num_step3 = get_full_ADC();                      
//    float voltage_mv3 = (num_step3 * 4.0)/1000;                                

    FAN_EN = 1;
    FAN_PWM = 1;
        if(voltage_mv1 < 1.1){                                                  // Flame Sensor when detected
   
            RED_LED = 1;
            Wait_One_Second_With_Beep();            
            RED_LED = 0;
            Wait_One_Second_With_Beep(); 
            FAN_EN = 1;
        }
        else{
            FAN_EN = 0;
            RED_LED = 0;
        }

        if(ENTERBUTTON == 0){                                                   
            people+=1;  
        }
        
        if(LEAVEBUTTON == 0){                                                  
            people-=1; 
        }
       

        if(Manual_Alarm == 1){                                  
            
            Manual_Alarm = 0;
            Do_Flashing();
           
            FAN_EN = 1;
        }
        
        if(people <= 9)
        {
            char U = people/10;
            char L = people%10; 
            Display_Upper_Digit(U);
            Display_Lower_Digit(L);
            
        } 
        else if (people > 9)
        {
            
            Wait_One_Second_With_Beep();
            
            char U = 9;
            char L = 9;
            Display_Upper_Digit(U);
            Display_Lower_Digit(L);
            
        }

 printf ("Flame Sensor Voltage = %f F \r\n", voltage_mv1);                      // print out flame sensor voltage
//printf ("Motion1 Voltage = %d V \r\n\n", voltage_mv2);       
//printf ("Motion2 Voltage = %d V \r\n\n", voltage_mv3);      
 Delay_One_Sec();                                  //Delay One Sec to see TeraTerm results slower 
    } 
}


void Display_Lower_Digit(char digit)
{
    PORTD = array[digit];
}

void Display_Upper_Digit(char digit)
{
    PORTC = array[digit] & 0x3f;        // MASK PORTC with array[digit] lower 5 bits
    int bit6 = array[digit]&0x40;       // create a variable called bit 6 that mask the array[digit] 6 bit
    if(bit6 == 0x40){                   // checks if bit 6 is 1 
        PORTE = 0x01;                   // PORTE is 1 (OFF)
    }
    else if(bit6 == 0x00){              // checks if bit 6 is 0
        PORTE = 0x00;                   // PORT E is 0 (ON)
    }
}

void Activate_Buzzer()
{
    PR2 = 0b11111001;
    T2CON = 0b00000101;
    CCPR2L = 0b01001010;
    CCP2CON = 0b00111100;
}

void Deactivate_Buzzer()
{
    CCP2CON = 0x0;
    PORTBbits.RB3 = 0; 
}

void Wait_Half_Second()
{
    T0CON = 0x03;                                                               // Timer 0, 16-bit mode, prescaler 1:16
    TMR0L = 0xDB;                                                               // set the lower byte of TMR
    TMR0H = 0x0B;                                                               // set the upper byte of TMR
    INTCONbits.TMR0IF = 0;                                                      // clear the Timer 0 flag
    T0CONbits.TMR0ON = 1;                                                       // Turn on the Timer 0
    while (INTCONbits.TMR0IF == 0);                                             // wait for the Timer Flag to be 1 for done
    T0CONbits.TMR0ON = 0;                                                       // turn off the Timer 0
}
void Wait_One_Second_With_Beep()                                                // creates one second delay as well as sound buzzer
{
    Activate_Buzzer();                                                          // Turn on beep noise on speaker
    Wait_Half_Second();                                                         // Wait for half second (or 500 msec)
    Deactivate_Buzzer();                                                        // Turn off beep noise on speaker
    Wait_Half_Second();                                                         // Wait for half second (or 500 msec)
}

void Do_Flashing(){
    FLASHING = 1;                                                               // FlASHING is set to 1
    while(FLASHING == 1)                                                        // Checks if FLASHING is still 1 otherwise exits the while loop
    {
        if(Manual_Alarm == 1)                                               // If FLASHING_REQUEST is pressed after interrupt has occurred 
        {
            Manual_Alarm = 0;                                               // Clear the interrupt in INT0_ISR by setting FLASHING_REQUEST to 0 
            FLASHING = 0;                                                       // Exit while loop by setting FLASHING to 0 
        }
        else                                                                    // This part is to create the FLASHING effect if FLASHING_REQUEST = 0
        {
            RED_LED = 1;
            Wait_One_Second_With_Beep();            
            RED_LED = 0;
            Wait_One_Second_With_Beep();
        }
    }
}

