#ifndef _RN4020_H_

#define _RN4020_H_

#define CONFIG_LEN  60
#define IN_BUFFER_LEN   256

void RN4020Init(char *ID);
void RN4020Task(void);
void RN4020_BT_OFF(void);
void RN4020SendData(uint8_t *data, uint16_t size);

uint8_t* RN4020GetData(void);
uint16_t RN4020GetDataSize(void);
void RN4020ClearBuffer(void);
bool RN4020isConnected(void);
extern uint8_t RN4020InBuffer[IN_BUFFER_LEN];
extern bool BTmoduleON;

#endif