#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <p32xxxx.h>
#include <sys/attribs.h>

//MACROS
#define LED_W       LATBbits.LATB2
#define LED_G       LATAbits.LATA0
#define LED_R       LATAbits.LATA1
#define SCSC_FREQ   0x8000
#define FULL_BRIGHTNESS 0x18
//WATCHDOG CONFIG
#pragma config FWDTEN = OFF     //DISABLE WATCHDOG TIMER

//OSCILLATOR CONFIG
#pragma config POSCMOD  = 0x3   // DISABLE PRIMARY PSCILLATOR
#pragma config SOSCEN   = ON    // ENABLE SECONDARY OSCILLATOR
#pragma config FNOSC    = 0x0   // INTERNAL RC OSC AS SYSCLK

int state = 0, flag = 1;
void __ISR(_TIMER_1_VECTOR, IPL7SRS) TIMER1ISR(void)
{
   if(flag)
    {
       state += 1;
       flag = !(state == FULL_BRIGHTNESS);
    }
   else
   {
       state -= 1;
       flag = (state == 0x0);
   }
   CCP1RB = state; // Set the falling edge compare value
   CCP2RB = state << 2;
   IFS0bits.T1IF = 0;

}
    

int main() 
{
    // I/O PORT SETUP
    ANSELA = 0x0;   //SET ALL PINS TO DIGITAL
    ANSELB = 0x0;  
    TRISA  = 0x0;   //SET ALL PORTS TO OUTPUT DIRECTION
    TRISB  = 0x0;
     
    RPOR3bits.RP16R = 0x6;         // SET RP16 TO SCCP2 OUTPUT COMPARE
           
    __builtin_disable_interrupts(); //DISABLE ALL ISRs
  
    /*ISR CONFIGURATION SET UP*/
    PRISSbits.PRI7SS = 0x0;       //PRIORITY LEVEL 7 USES SHADOW REGISTER SET 0
    IEC0bits.T1IE    = 0x1;       //ENABLE TIMER 1 INTERRUPT
    IPC2bits.T1IP    = 0x7;       //SET TIMER 1 TO THE HIGHEST POSSIBLE PRIORITY
    IPC2bits.T1IS    = 0x3;       //SET TIMER 1 TO THE HIGHEST POSSIBLE SUBPRIORITY
    
    /*TIMER 1 SETUP*/
    PR1 = (SCSC_FREQ >> 5) + (SCSC_FREQ >> 6);            //16 BIT TIMER1 ACC REG
    T1CONbits.TECS   = 0;       //TIMER 1 EXT CLK - SOSC
    T1CONbits.TCS    = 1;       //TIMER 1 USES EXT CLK
    T1CONbits.TCKPS0 = 0;       //1?1 PRESCALE
    T1CONbits.TCKPS1 = 0;       //1?1 PRESCALE
    T1CONbits.TGATE  = 1;       //ENABLE ACCUMULATION
    T1CONbits.ON     = 1;       //ENABLE TIMER 1
      
    // Set MCCP operating mode
    CCP1CON1bits.CCSEL = 0; // Set MCCP operating mode (OC mode)
    CCP1CON1bits.MOD = 0b0101; // Set mode (Buffered Dual-Compare/PWM mode)
    //Configure MCCP Timebase
    CCP1CON1bits.T32 = 0; // Set timebase width (16-bit)
    CCP1CON1bits.TMRSYNC = 0; // Set timebase synchronization (Synchronized)
    CCP1CON1bits.CLKSEL = 0b010; // Set the clock source (OSOC)
    CCP1CON1bits.TMRPS = 0b00; // Set the clock prescaler (1:1)
    CCP1CON1bits.TRIGEN = 0; // Set Sync/Triggered mode (Synchronous)
    CCP1CON1bits.SYNC = 0b00000; // Select Sync/Trigger source (Self-sync)
    //Configure MCCP output for PWM signal
    CCP1CON2bits.OCEEN = 1; // Enable desired output signals (OC1E)
    CCP1CON2bits.OCFEN = 1; // Enable desired output signals (OC1E)
    CCP1CON3bits.OUTM = 0b000; // Set advanced output modes (Standard output)
    CCP1CON3bits.POLACE = 0; // Configure output polarity (Active High)
    CCP1TMRbits.TMRL = 0x0000; // Initialize timer prior to enable module.
    CCP1PRbits.PRL = 0x0050; // Configure timebase period
    CCP1RA = 0x0000; // Set the rising edge compare value
    CCP1RB = 0x0000; // Set the falling edge compare value
    CCP1CON1bits.ON = 1;
     
    
    // Set SCCP operating mode
    CCP2CON1bits.CCSEL = 0; 
    CCP2CON1bits.MOD = 0b0101; 
    //Configure SCCP Timebase
    CCP2CON1bits.T32 = 0; 
    CCP2CON1bits.TMRSYNC = 0;
    CCP2CON1bits.CLKSEL = 0b010; 
    CCP2CON1bits.TMRPS = 0b00;
    CCP2CON1bits.TRIGEN = 0;
    CCP2CON1bits.SYNC = 0b00000;
    //Configure SCCP output for PWM signal
    CCP2CON2bits.OCAEN = 1; 
    CCP2CON3bits.POLACE = 0; 
    CCP2TMRbits.TMRL = 0x0000; 
    CCP2PRbits.PRL = 0x0050; 
    CCP2RA = 0x0000; 
    CCP2RB = 0x0000;
    CCP2CON1bits.ON = 1;
    
    __builtin_enable_interrupts();//ENABLE ALL ISRs
   
    while (1);
    
    return 0;
}
