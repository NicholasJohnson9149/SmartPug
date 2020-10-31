#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware.h"

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "UART1.h"

#include "Tick.h"

#define TX_BUFFER_SIZE          50
#define RX_BUFFER_SIZE          50

uint16_t u1PacketTick=0;
uint16_t u1RxTick=0;

uint16_t u1TxCount;
uint16_t u1IsDataReady=0;

uint8_t u1TxBuffer[TX_BUFFER_SIZE];
uint16_t u1TxLen;

uint8_t u1RxBuffer[RX_BUFFER_SIZE];
uint16_t u1RxCount = 0;

void U1Init(void)
{

    PM_TX_U_RX_TRIS =1;
    RPINR18bits.U1RXR=PM_TX_U_RX_RINR;

    PM_RX_U_TX_IO=1;
    PM_RX_U_TX_TRIS=0;
    PM_RX_U_TX_RPOR=0x03;

    U1STAbits.URXISEL = 1;
    U1STAbits.UTXISEL0 = 0;
    U1STAbits.UTXISEL1 = 1;

    U1BRG = SET_BRG(115200);
    U1MODE = 0;
    U1MODEbits.BRGH = 1;
    IPC2bits.U1RXIP = 4;
    IPC3bits.U1TXIP = 4;

    IFS0bits.U1TXIF = 0;
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1TXIE = 0;
    IEC0bits.U1RXIE = 1;
    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;

    u1IsDataReady=0;
}

void UART1SendData(uint8_t *data, uint16_t size)
{
    if(IEC0bits.U1TXIE==0)
    {
        memcpy(u1TxBuffer,data,size);
        u1TxLen = size;
        u1TxCount = 0;

        IFS0bits.U1TXIF = 1;
        IEC0bits.U1TXIE = 1;
    }
}

uint16_t UART1ReceiveData(uint8_t *data)
{
    uint16_t rxLen=0;
    DI();
    if(u1RxTick>20)
    {
        rxLen=u1RxCount;
        if(rxLen>0)
        {
            if(rxLen>RX_BUFFER_SIZE-1)
            {
                rxLen=RX_BUFFER_SIZE-1;
            }
            memcpy(data,u1RxBuffer,rxLen);
            u1RxCount=0;
        }
    }
    EI();
    return(rxLen);
}

bool Checksum(uint8_t *Data, uint16_t Size)
{
    uint16_t i;
    uint16_t csVal = 0;
    uint16_t csPkt = 0;
    return (true);
    for (i = 2; i < (Size - 2); i++)
    {
        csVal += Data[i];
    }
    csPkt = Data[i++];
    csPkt += Data[i++];

    return (csPkt == csVal);

}

void __attribute__((interrupt, auto_psv)) _U1RXInterrupt(void)
{
    volatile uint8_t buff;
    if ((U1STA & 0x06) > 0)
    {
        U1STAbits.OERR = 0;
        buff = U1RXREG;
    } else
    {
        if (IFS0bits.U1RXIF)
        {
            u1RxTick=0;
            buff=U1RXREG;
            if(u1PacketTick>100)
            {
                //Reset count
                u1RxCount=0;
            }
            u1PacketTick=1;
            if(u1IsDataReady==0)
            {
                if (u1RxCount < RX_BUFFER_SIZE)
                {
                    u1RxBuffer[u1RxCount++] = buff;
                }
                if(u1RxCount>=RX_BUFFER_SIZE)
                {
                    u1IsDataReady=u1RxCount;
                }
            }
        }
    }//No Error
    IFS0bits.U1RXIF = false;
}

void __attribute__ ((interrupt, auto_psv)) _U1TXInterrupt(void)
{
    if (IFS0bits.U1TXIF)
    {
        IFS0bits.U1TXIF = false;
        if (IEC0bits.U1TXIE)
        {
            if (u1TxCount < u1TxLen)//Sending Strings Only
            {
                while (U1STAbits.UTXBF);
                U1TXREG = u1TxBuffer[u1TxCount++];
            } else
            {
                IEC0bits.U1TXIE = 0;
            }
        }
    }
}

void UART1TickHandler(void)
{
    if(u1RxTick<0xfffe)
    {
        u1RxTick++;
    }
    if(u1PacketTick<0xfffe)
    {
        u1PacketTick++;
    }
}
