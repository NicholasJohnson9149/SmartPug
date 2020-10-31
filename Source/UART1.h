#ifndef _UART_1_H

#define _UART_1_H

void UART1SendData(uint8_t *data, uint16_t size);
uint16_t UART1ReceiveData(uint8_t *data);
void UART1TickHandler(void);
void U1Init(void);

#endif