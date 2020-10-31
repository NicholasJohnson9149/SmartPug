#ifndef _HARDWARE_H_

#define _HARDWARE_H_

    //#define POWER_MONITOR_II    //Large
    #define POWER_MONITOR       //Small


    #define FOSC                        32000000UL
    #define FCY                         (FOSC/2)

    #define SET_BRG(freq)               (FCY/(4UL*freq)-1)
    #define SET_PERIOD_US(us)           (uint16_t)((FCY/1000000UL)*us)

    //RN4020 Interface
    #define RN4020_TX_U_RX_TRIS         TRISBbits.TRISB4
    #define RN4020_TX_U_RX_IO           LATBbits.LATB4
    #define RN4020_TX_U_RX_RINR         4

    #define RN4020_RX_U_TX_TRIS         TRISBbits.TRISB5
    #define RN4020_RX_U_TX_IO           LATBbits.LATB5
    #define RN4020_RX_U_TX_RPOR         RPOR2bits.RP5R

#ifdef POWER_MONITOR_II
    #define RN4020_MLDP_TRIS            TRISAbits.TRISA2 //B3???
    #define RN4020_MLDP_IO              LATAbits.LATA2
    #define RelayOn()                   (LATB |=((1<<11)|(1<<12)))
    #define RelayOff()                  (LATB &= (~((1<<11)|(1<<12))))

#endif

#ifdef POWER_MONITOR
    #define RN4020_MLDP_TRIS            TRISBbits.TRISB3
    #define RN4020_MLDP_IO              LATBbits.LATB3
    #define RelayOff()                  (LATB |=((1<<11)|(1<<12)))
    #define RelayOn()                   (LATB &= (~((1<<11)|(1<<12))))
#endif

    #define RN4020_MLDP_EV_TRIS         TRISAbits.TRISA4
    #define RN4020_MLDP_EV_IO           PORTAbits.RA4

    #define RN4020_WAKE_TRIS            TRISAbits.TRISA1
    #define RN4020_WAKE_IO              LATAbits.LATA1

    #define RN4020_SW_WAKE_TRIS         TRISAbits.TRISA2
    #define RN4020_SW_WAKE_IO           LATAbits.LATA2

    #define BUTTON_TRIS                 TRISBbits.TRISB10
    #define BUTTON_IN                   PORTBbits.RB10

    #define RELAY0_TRIS                 TRISBbits.TRISB11
    #define RELAY1_TRIS                 TRISBbits.TRISB12

//RN4020 - Pin 6 (RX)
//Micro Pin 11 (RB5) + Micro Pin 3 (RB2) througb 100ohm (Meter's TX)


    //Power Meter Interface
    #define PM_TX_U_RX_TRIS             TRISBbits.TRISB2
    #define PM_TX_U_RX_IO               LATBbits.LATB2
    #define PM_TX_U_RX_RINR             2

    #define PM_RX_U_TX_TRIS             TRISBbits.TRISB7
    #define PM_RX_U_TX_IO               LATBbits.LATB7
    #define PM_RX_U_TX_RPOR             RPOR3bits.RP7R

    //Tick Timer
    #define TtickCON                    T5CON
    #define TtickCONbits                T5CONbits
    #define TtickPR                     PR5
    #define TtickIF                     IFS1bits.T5IF
    #define TtickIE                     IEC1bits.T5IE
    #define TtickIP                     IPC7bits.T5IP
    #define TtickTMR                    TMR5

    //LEDs
    #define LED_R_TRIS                  TRISBbits.TRISB14
    #define LED_R                       LATBbits.LATB14

    #define LED_B_TRIS                  TRISBbits.TRISB15
    #define LED_B                       LATBbits.LATB15

    #define LED_ON                      1
    #define LED_OFF                     0


    #define DI()    {IEC1bits.T5IE = 0; IEC0bits.U1RXIE = 0;}
    #define EI()    {IEC1bits.T5IE = 1; IEC0bits.U1RXIE = 1;}




void HardwareInit(void);

#endif