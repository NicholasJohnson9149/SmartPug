#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware.h"

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "UART2.h"

#include "Tick.h"

#define TX_BUFFER_SIZE          256
#define RX_BUFFER_SIZE          256

uint16_t u2PacketTick=0;
uint16_t u2RxTick=0;

uint16_t u2TxCount;
uint16_t u2IsDataReady=0;

uint8_t u2TxBuffer[TX_BUFFER_SIZE];
uint16_t u2TxLen;

uint8_t u2RxBuffer[RX_BUFFER_SIZE];
uint16_t u2RxCount = 0;

bool u2ClearBufferWhileSending=false;

void U2Init(void)
{

    RN4020_TX_U_RX_TRIS =1;
    RPINR19bits.U2RXR=RN4020_TX_U_RX_RINR;

    RN4020_RX_U_TX_IO=1;
    RN4020_RX_U_TX_TRIS=0;
    RN4020_RX_U_TX_RPOR=0x05;

    U2STAbits.URXISEL = 1;
    U2STAbits.UTXISEL0 = 0;
    U2STAbits.UTXISEL1 = 1;

    U2BRG = SET_BRG(115200);
    U2MODE = 0;
    U2MODEbits.BRGH = 1;
    IPC7bits.U2RXIP = 4;
    IPC7bits.U2TXIP = 4;

    IFS1bits.U2TXIF = 0;
    IFS1bits.U2RXIF = 0;
    IEC1bits.U2TXIE = 0;
    IEC1bits.U2RXIE = 1;
    U2MODEbits.UARTEN = 1;
    U2STAbits.UTXEN = 1;

    u2IsDataReady=0;
}

void UART2SendData(uint8_t *data, uint16_t size)
{
    if(IEC1bits.U2TXIE==0)
    {
        memcpy(u2TxBuffer,data,size);
        u2TxLen = size;
        u2TxCount = 0;

        IFS1bits.U2TXIF = 1;
        IEC1bits.U2TXIE = 1;
    }
}

uint16_t UART2ReceiveData(uint8_t *data)
{
    uint16_t rxLen=0;
    DI();
    if(u2RxTick>20)
    {
        rxLen=u2RxCount;
        if(rxLen>0)
        {
            if(rxLen>RX_BUFFER_SIZE-1)
            {
                rxLen=RX_BUFFER_SIZE-1;
            }
            memcpy(data,u2RxBuffer,rxLen);
            u2RxCount=0;
        }
    }
    EI();
    return(rxLen);
}

void __attribute__((interrupt, auto_psv)) _U2RXInterrupt(void)
{
    volatile uint8_t buff;
    if ((U2STA & 0x06) > 0)
    {
        U2STAbits.OERR = 0;
        buff = U2RXREG;
    } else
    {
        if (IFS1bits.U2RXIF)
        {
            u2RxTick=0;
            buff=U2RXREG;
            if( (u2ClearBufferWhileSending==false) ||
                    ( (u2ClearBufferWhileSending==true) && (IEC1bits.U2TXIE==false) ) )
            {
                if(u2PacketTick>100)
                {
                    //Reset count
                    u2RxCount=0;
                }
                u2PacketTick=1;
                if(u2IsDataReady==0)
                {
                    if (u2RxCount < RX_BUFFER_SIZE)
                    {
                        u2RxBuffer[u2RxCount++] = buff;
                    }
                    if(u2RxCount>=RX_BUFFER_SIZE)
                    {
                        u2IsDataReady=u2RxCount;
                    }
                }
            }else
            {
                u2PacketTick=1;
                u2RxCount=0;
                u2RxBuffer[0]=0;
            }
        }
    }//No Error
    IFS1bits.U2RXIF = false;
}

void __attribute__ ((interrupt, auto_psv)) _U2TXInterrupt(void)
{
    if (IFS1bits.U2TXIF)
    {
        IFS1bits.U2TXIF = false;
        if (IEC1bits.U2TXIE)
        {
            if (u2TxCount < u2TxLen)//Sending Strings Only
            {
                while (U2STAbits.UTXBF);
                U2TXREG = u2TxBuffer[u2TxCount++];
            } else
            {
                IEC1bits.U2TXIE = 0;
            }
        }
    }
}

void UART2TickHandler(void)
{
    if(u2RxTick<0xfffe)
    {
        u2RxTick++;
    }
    if(u2PacketTick<0xfffe)
    {
        u2PacketTick++;
    }
}

void UART2SetClearBufferWhileSending(bool value)
{
    u2ClearBufferWhileSending=value;
}