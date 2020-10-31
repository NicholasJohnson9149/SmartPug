#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware.h"
#include "PowerMeter.h"
#include "UART1.h"
#include "UART2.h"

uint64_t TickMs=0;

void TickInit(void)
{
    TtickCON = 0;
    TtickCONbits.TCS = 0;
    TtickCONbits.TCKPS = 0;
    TtickPR = SET_PERIOD_US(1000);
    TtickIF = 0;
    TtickIP = 4;
    TtickIE = 1;
    TtickTMR = 0;
    TtickCONbits.TON = 1;
}

uint64_t TickGet(void)
{
    uint64_t val;
    TtickIE=0;
    val=TickMs;
    TtickIE=1;
    return(val);
}

void __attribute__((interrupt, auto_psv)) _T5Interrupt(void)
{
    UART1TickHandler();
    UART2TickHandler();
    TickMs++;
    TtickIF = 0;
}
