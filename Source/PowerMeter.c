#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hardware.h"
#include "PowerMeter.h"
#include "UART1.h"
#include "Tick.h"
#include "RN4020.h"

uint8_t pmData[50];
uint8_t pmDataEnergy[50];
uint8_t pmDataLen=0;
uint8_t pmDataLenEnergy=0;
uint8_t CalibrationState=0;
uint8_t ReadingState=0;
bool RelaytoDefault = false;
bool RelayOnStartUp = false;
bool Calibration1 = false;
bool Calibration2 = false;
bool Voltage_Energy_switch=false;
bool ReadCommand = false;

#define ACK 0x06
#define NAK 0x15

void PowerMeterInit(void)
{
    U1Init();
    pmDataLen=0;
}

uint8_t* PowerMeterGetData(void)
{
    uint8_t* retval=NULL;
    if(pmDataLen>0)
    {
        retval=pmData;
    }
    
    return(retval);
}

uint8_t* PowerMeterGetDataEnergy(void)
{
    uint8_t* retval=NULL;
    if(pmDataLenEnergy>0)
    {
        retval=pmDataEnergy;
    }
    return(retval);
}

uint8_t PowerMeterGetDataSize(void)
{
    return(pmDataLen);
}

uint8_t PowerMeterGetDataSizeEnergy(void)
{
    return(pmDataLenEnergy);
}

void PowerMeterClearBuffer(void)
{
    pmDataLen=0;
}

//i.a.
void PowerMeterClearEnergyBuffer(void)
{
    pmDataLenEnergy = 0;
}

void PowerMeterRead(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 8;
    TxBuffer[i++] = 0x41;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0x06;
    TxBuffer[i++] = 0x4E;
    TxBuffer[i++] = 0x18;
    TxBuffer[i++] = 0x5A;

    UART1SendData(TxBuffer,i);
}

void PowerMeterReadEnergy(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 8;
    TxBuffer[i++] = 0x41;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0x1E;
    TxBuffer[i++] = 0x4E;
    TxBuffer[i++] = 0x20;
    TxBuffer[i++] = 0x7A;
    
    UART1SendData(TxBuffer,i);
}

void StartEnergyCount(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 0x0A;
    TxBuffer[i++] = 0x41;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0xDC;
    TxBuffer[i++] = 0x4D;
    TxBuffer[i++] = 0x02;
    TxBuffer[i++] = 0x01;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0x1C;
    
    UART1SendData(TxBuffer,i);
}

void StopEnergyCount(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 0x0A;
    TxBuffer[i++] = 0x41;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0xDC;
    TxBuffer[i++] = 0x4D;
    TxBuffer[i++] = 0x02;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0x00;
    TxBuffer[i++] = 0x1B;
    
    UART1SendData(TxBuffer,i);
}

void Calibration1Sequence(void)
{
    uint8_t byteCount=0;
    uint8_t RxData[50];
    uint8_t Response[3];
    
    switch(CalibrationState)
    {
        case 0:
            SetRange();
            CalibrationState++;
            break;
        case 1:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 9;       //calibration failed
                //here the error code must be send back to Android
                //...
                Response[0]=0x28;           //Set range failed
            }
            break;
        case 2:
            FrequencyCalibrationCommand();
            CalibrationState++;
            break;
        case 3:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 9;       //calibration failed
                //here the error code must be sent back to Android
                //...
                Response[0]=0x29;           //Frequency calibration failed
            }
            break;
        case 4:
            GainCalibrationCommand();
            CalibrationState++;
            break;
        case 5:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 9;       //calibration failed
                //here the error code must be sent back to Android
                //...
                Response[0]=0x2A;           //Gain voltage calibration failed
            }
            break;
        case 6:
            SaveToFlashCommand();
            CalibrationState++;
            break;
        case 7:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 9;       //calibration failed
                //here the error code must be sent back to Android device
                //...
                Response[0]=0x2D;           //Save to flash failed
            }
            break;
        case 8:     
            Response[0]=0x20;               //Calibration successful
        case 9:     
            RN4020SendData(Response,1);     //Calibration failed
        default:
            CalibrationState = 0;
            Calibration1 = false;
            break;
    }
}

void Calibration2Sequence(void)
{
    uint8_t byteCount=0;
    uint8_t RxData[50];
    uint8_t Response[3];
    
    switch(CalibrationState)
    {
        case 0:
            ReactiveCalibrationCommand();
            CalibrationState++;
            break;
        case 1:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 5;       //calibration failed
                //here the error code must be sent back to Android
                //...
                Response[0]=0x2F;           //Gain reactive power calibration failed
            }
            break;
        case 2:
            SaveToFlashCommand();
            CalibrationState++;
            break;
        case 3:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>0)  //response received after the calibration command
            {
                memcpy(pmData,RxData,byteCount);
                pmDataLen=byteCount;
            }
            if ((pmData[0] == ACK) && (pmDataLen == 1))     //just a single ACK
            {
                CalibrationState++;
            }
            else
            {
                CalibrationState = 5;       //calibration failed
                //here the error code must be sent back to Android device
                //...
                Response[0]=0x2D;           //Save to flash failed
            }
            break;
        case 4:     
            Response[0]=0x20;               //Calibration successful
        case 5:     
            RN4020SendData(Response,1);     //Calibration failed
        default:
            CalibrationState = 0;
            Calibration2 = false;
            break;
    }
}

void ReadVoltage(void)
{
    uint8_t byteCount=0;
    uint8_t RxData[50];
    uint8_t Response[50];
    
    switch(ReadingState)
    {
        case 0:
            if (Voltage_Energy_switch)
            {
                PowerMeterReadEnergy();
            }
            else
            {
                PowerMeterRead();
            }
            ReadingState++;
            break;
        case 1:
            byteCount=UART1ReceiveData(RxData);
            if(byteCount>1)
            {
                if (Voltage_Energy_switch)
                {
                    memcpy(pmDataEnergy,RxData,byteCount);
                    pmDataLenEnergy=byteCount;
                }
                else
                {
                    memcpy(pmData,RxData,byteCount);
                    pmDataLen=byteCount;    
                }
            }

            if (Voltage_Energy_switch)
            {
                if (pmDataEnergy[0] == ACK)     //ACK received
                {
                    Response[0]=0x32;
                    Response[1]=0xfe;
                    Response[2]=PowerMeterGetDataSizeEnergy();
                    memcpy(&Response[3],PowerMeterGetDataEnergy(),PowerMeterGetDataSizeEnergy());
                    RN4020SendData(Response, PowerMeterGetDataSizeEnergy() + 3);
                    PowerMeterClearEnergyBuffer();
                }
                else
                {
                    Response[0]=NAK;               //error
                    RN4020SendData(Response,1);
                }
            }
            else
            {
                if (pmData[0] == ACK)     //ACK received
                {
                    Response[0]=0x31;
                    Response[1]=0xfe;
                    Response[2]=PowerMeterGetDataSize();
                    memcpy(&Response[3],PowerMeterGetData(),PowerMeterGetDataSize());
                    RN4020SendData(Response, PowerMeterGetDataSize() + 3);
                    PowerMeterClearBuffer();
                }
                else
                {
                    Response[0]=NAK;               //error
                    RN4020SendData(Response,1);
                }
            }    
        default:
            ReadCommand = false;               //Reading command executed
            break;
    }
}

void SetRange(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 0x1C;
    TxBuffer[i++] = 0x41;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x82;
    TxBuffer[i++] = 0x4D;
    TxBuffer[i++] = 0x14;
    TxBuffer[i++] = 0x12;
    TxBuffer[i++] = 0x0C;
    TxBuffer[i++] = 0x14;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x50;
    TxBuffer[i++] = 0xC3;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x98;
    TxBuffer[i++] = 0x08;
    TxBuffer[i++] = 0xB0;
    TxBuffer[i++] = 0xAD;
    TxBuffer[i++] = 0x01;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x24;
    TxBuffer[i++] = 0x74;
    TxBuffer[i++] = 0x01;
    TxBuffer[i++] = 0x0;
    TxBuffer[i++] = 0x50;
    TxBuffer[i++] = 0xC3;
    TxBuffer[i++] = 0xD4;
    
    UART1SendData(TxBuffer,i);
}

void FrequencyCalibrationCommand(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 4;
    TxBuffer[i++] = 0x76;
    TxBuffer[i++] = 0x1F;
    
    UART1SendData(TxBuffer,i);
}

void GainCalibrationCommand(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 4;
    TxBuffer[i++] = 0x5A;
    TxBuffer[i++] = 0x03;
    
    UART1SendData(TxBuffer,i);
}

void ReactiveCalibrationCommand(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 4;
    TxBuffer[i++] = 0x7A;
    TxBuffer[i++] = 0x23;
    
    UART1SendData(TxBuffer,i);
}

void SaveToFlashCommand(void)
{
    uint16_t i = 0;
    uint8_t TxBuffer[50];
    
    TxBuffer[i++] = 0xA5;
    TxBuffer[i++] = 4;
    TxBuffer[i++] = 0x53;
    TxBuffer[i++] = 0xFC;
    
    UART1SendData(TxBuffer,i);
}
