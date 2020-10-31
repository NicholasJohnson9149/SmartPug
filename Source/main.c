#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hardware.h"
#include "Tick.h"
#include "PowerMeter.h"
#include "RN4020.h"
#include "UART2.h"

//#define VERSION         "1.1"
#define VERSION         "1.2"

#ifdef POWER_MONITOR
    #define METER_ID_STR    "1234561"    //
#endif

#ifdef POWER_MONITOR_II
    #define METER_ID_STR    "1234562"    //
#endif

void RN4020CommandHandler(void);

volatile uint8_t sendData=false;
static bool RelayStatus=false;
static bool LED_pulse=false;
bool BTmoduleON=true;
bool StartCommandDelay=false;
bool StartEnergyCounting=false;

#define Cal_interval MS(140)    //140ms delay after sending the calibration frame
#define Read_interval MS(150)   //150ms delay after sending the read frame
#define PulseDuration MS(100)   //100ms LED pulse
#define PulsePeriod MS(5000)    //5 seconds for BT module ON
#define ButtonDelay MS(200)     //200ms debounce time


int main(void)
{
    uint8_t outdata[50];
    TICK tStart, tLED;
    TICK tDelay;
    TICK tCalibration, tRelaytoDefault, tRelayOnStartUp, tReading, tPulseLED, tButtonDebounce;

    HardwareInit();
    TickInit();
    PowerMeterInit();
    RN4020Init("MCHP-PM-" METER_ID_STR);
    
    RelayOnStartUp = true;
    tRelayOnStartUp = TickGet();
    tStart=TickGet();
    tCalibration=TickGet();
    tReading=TickGet();
    tPulseLED=TickGet();
    
    while(1)
    {
        ClrWdt();
        if((RN4020isConnected()==false) && BTmoduleON)
        {
            if(TickGetDiff(TickGet(),tPulseLED)>PulsePeriod)
            {
                LED_B=1;
                LED_pulse = 1;
                tPulseLED = TickGet();
            }
        }
        else tPulseLED = TickGet();

        if (RelayOnStartUp)
        {
            if(TickGetDiff(TickGet(),tRelayOnStartUp)>MS(2000))
            {
                RELAY0_TRIS=0;
                RELAY1_TRIS=0;
                RelayOn();
                LED_R=LED_ON;
                RelaytoDefault = true;
                RelayStatus=true;       
                RelayOnStartUp = false;
                outdata[0]=0x11;        //Relay is ON

                StartCommandDelay = true;
                StartEnergyCounting = true;
                StopEnergyCount();
                        
                RN4020SendData(outdata,1);
            }
        }
        else
        {
            tRelayOnStartUp = TickGet();
            if (BUTTON_IN == false)
            {
                if(TickGetDiff(TickGet(),tButtonDebounce)>ButtonDelay)
                {
                    BTmoduleON ^= 1;        //ON/OFF BT module
                    if (!BTmoduleON)
                    {
                        LED_B=0;
                        LED_pulse = 0;                    
                    }
                    else
                    {
                        LED_B=1;
                        LED_pulse = 1;                    
                    }
                    RN4020_BT_OFF();
                }
            }
        }
        if (BUTTON_IN == false) tButtonDebounce=TickGet();
        
        if (RelaytoDefault)
        {
            if(TickGetDiff(TickGet(),tRelaytoDefault)>MS(80))
            {
                if (RelayStatus)
                {
                    (LATB |=((1<<11)));
                    outdata[0]=0x11;        //Relay is ON
                }
                else
                {
                    (LATB &= (~(1<<12)));
                    outdata[0]=0x12;        //Relay is OFF
                }
                RelaytoDefault = false;
                RN4020SendData(outdata,1);
            }
        }
        else tRelaytoDefault = TickGet();
        
        if (Calibration1)
        {
            if(TickGetDiff(TickGet(),tCalibration)>Cal_interval)
            {
                Calibration1Sequence();
                tCalibration=TickGet(); 
            }
        }
        else if (Calibration2)
        {
            if(TickGetDiff(TickGet(),tCalibration)>Cal_interval)
            {
                Calibration2Sequence();
                tCalibration=TickGet(); 
            }
        }
        else if (ReadCommand)
        {
            if(TickGetDiff(TickGet(),tReading)>Read_interval)
            {
                ReadVoltage();
                tReading=TickGet(); 
            }
        }
        else if (StartCommandDelay)
        {
            if(TickGetDiff(TickGet(),tDelay)>50)        //50 ms delay after sending the command
            {
                if (StartEnergyCounting)
                {
                    tDelay=TickGet();                   //another delay
                    StartEnergyCounting = false;
                    StartEnergyCount();                 //Energy counting is ON
                }
                else
                {
                    StartCommandDelay = false;
                }
            }
        }
        else 
        {
            tDelay=TickGet();
            tCalibration=TickGet();
            tReading=TickGet();

            if (!LED_pulse) tLED=TickGet();
            if(TickGetDiff(TickGet(),tLED)>PulseDuration)
            {
                tLED=TickGet();
                LED_pulse = 0;
                LED_B=0;
            }
        }


        RN4020CommandHandler();

        if (BTmoduleON) RN4020Task();

    }

}

int counter = 0;

void RN4020CommandHandler(void)
{
    uint8_t data[2];
    uint8_t dataLen;
    uint8_t outdata[50];

    dataLen=RN4020GetDataSize();

    if(dataLen<3)
    {
        if(dataLen==2)
        {
            data[0]=RN4020InBuffer[0];
            data[1]=RN4020InBuffer[1];
            switch (data[0])
            {
                case 0x10:                      //Relay commands
                    if (data[1]==0x11)          //Relay ON
                    {
                        RelayOn();
                        LED_R=LED_ON;
                        RelaytoDefault = true;
                        RelayStatus=true;
                        outdata[0]=0x11;        //Relay is ON
                        RN4020SendData(outdata,1);
                    }
                    if (data[1]==0x12)          //Relay OFF
                    {
                        RelayOff();
                        LED_R=LED_OFF;
                        RelaytoDefault = true;
                        RelayStatus=false;
                        outdata[0]=0x12;        //Relay is OFF
                        RN4020SendData(outdata,1);
                    }
                    if (data[1]==0x13)          //Relay ID
                    {
                        if (RelayStatus)
                        {
                            outdata[0]=0x11;        //Relay is ON
                        }
                        else
                        {
                            outdata[0]=0x12;        //Relay is OFF
                        }
                        RN4020SendData(outdata,1);
                    }
                    
                    break;
                case 0x20:      //Calibration commands
                    if (data[1]==0x21)          //Frequency and GAIN calibration at power factor = 1
                    {
                        if(!Calibration1)
                        {
                            Calibration1 = true;
                            CalibrationState = 0;
                        }
                    }
                    if (data[1]==0x22)          //Phase compensation and Reactive power GAIN calibration at power factor = 0.5
                    {
                        if(!Calibration2)
                        {
                            Calibration2 = true;
                            CalibrationState = 0;
                        }
                    }
                    break;
                case 0x30:      //Data requesting commands
                    if (data[1]==0x31)                      
                    {
                        if (!ReadCommand)
                        {
                            ReadCommand = true;
                            Voltage_Energy_switch = 0;              //Voltage, current power, frequency
                            ReadingState = 0;
                        }
                        
                    }
                    else if (data[1]==0x32)             //Energy
                    {
                        if (!ReadCommand)
                        {
                            ReadCommand = true;
                            Voltage_Energy_switch = 1;
                            ReadingState = 0;
                        }
                    }
                    else if (data[1]==0x34)          //Stop Energy Counting
                    {
                        StartCommandDelay = true;
                        StartEnergyCounting = true;
                        StopEnergyCount();
                        outdata[0]=0x34;            //Energy Counters are cleared
                        RN4020SendData(outdata,1);
                    }
                    break;
                default:
                    break;
            }
            RN4020ClearBuffer();
        }
        
    }
    else RN4020ClearBuffer();
}