#ifndef _UART_2_H

#define _UART_2_H

void UART2SendData(uint8_t *data, uint16_t size);
uint16_t UART2ReceiveData(uint8_t *data);
void UART2TickHandler(void);
void U2Init(void);
void UART2SetClearBufferWhileSending(bool value);

#endif