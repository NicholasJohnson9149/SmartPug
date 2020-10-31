#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hardware.h"
#include "PowerMeter.h"
#include "Tick.h"
#include "UART2.h"
#include "RN4020.h"

typedef enum bOwner_t_def
{
    bOwnerUser,
    bOwnerSystem
}bOwner_t;

typedef struct rnConfig_t_def
{
    char config[CONFIG_LEN];
    char reply[CONFIG_LEN];
    uint16_t timeout;
    uint16_t delayaftercommand;
}rnConfig_t;


const rnConfig_t rnConfig[]=
{
    /*{"\r\n",MS(200)},
    {"SR,10000000\r\n",MS(200)},
    {"I\r\n",MS(200)},
    {"",MS(200)},*/
    {"\r\n",                                       "",  MS(500),    MS(100)},
    {"y\r\n",                                       "",  MS(500),    MS(100)},
    {"v\r\n",                                       "",  MS(500),    MS(100)},
    {"SF,2\r\n",                                "AOK",  MS(500),    MS(100)},
    {"SR,32004000\r\n",                         "AOK",  MS(500),    MS(100)},
    {"ST,0064,0002,0064\r\n",                   "AOK",  MS(500),    MS(100)},
    {"SS,00000000\r\n",                         "AOK",  MS(500),    MS(100)},
    {"SN,SPlug\r\n",                               "AOK",  MS(500),    MS(100)},
    //{"PZ\r\n",                                  "AOK",  MS(500),    MS(100)},
    //{"PS,11223344556677889900AABBCCDDEEFF\r\n", "AOK",  MS(500),    MS(100)},
    {"R,1\r\n",                                 "Reboot",MS(500),   MS(3000)},
    {"","",0,0},//Terminator
};

const char BT_off[]= "O\r\n";
    

typedef enum rnState_t_def
{
    rnPOR,
    rnSendConfig,
    rnDelay,
    rnWaitForReply,
    rnError,
    rnOperating,
    rnSystemMessage,
}rnState_t;

rnState_t rnState=rnPOR;
bOwner_t bOwner=bOwnerSystem;
uint8_t RN4020InBuffer[IN_BUFFER_LEN];
uint16_t RN4020InBufferLen=0;

char RN4020ID[20];
void toUpper(char *buffer);
bool RN4020Connected; 

bool IsSystemMessage(uint8_t *data, uint16_t incount);


void RN4020Init(char *ID)
{
    uint8_t idlen=strlen(ID);
    if(idlen>sizeof(RN4020ID)-1)
    {
        idlen=sizeof(RN4020ID)-1;
    }
    memcpy(RN4020ID,ID,idlen);
    RN4020ID[idlen]=0;

    U2Init();

    RN4020_MLDP_IO=0;//Command Mode
    RN4020_MLDP_TRIS=0;

    RN4020_WAKE_IO=1;
    RN4020_WAKE_TRIS=0;

    RN4020_SW_WAKE_IO=1;
    RN4020_SW_WAKE_TRIS=0;

    RN4020_MLDP_EV_TRIS=1;
    
    rnState=rnPOR;
}


void RN4020Task(void)
{
    static uint16_t iCount;
    static TICK tStart;
    static bool sysMessageReceived;
    char outbuffer[CONFIG_LEN];
    char inbuffer[CONFIG_LEN];
    char buffer[CONFIG_LEN];
    uint16_t incount;
    char *pid;

    if( (RN4020_MLDP_EV_IO==1) && (rnState==rnOperating) )
    {
        //Handle Event
        RN4020_MLDP_IO=0;
        rnState=rnSystemMessage;
        sysMessageReceived=false;
    }
    switch(rnState)
    {
        case rnPOR:
            RN4020_MLDP_IO=0;//Command Mode
            UART2SetClearBufferWhileSending(true);
            
            rnState=rnSendConfig;
            iCount=0;
            bOwner=bOwnerSystem;
            break;

        case rnSendConfig:
            if(rnConfig[iCount].config[0]!=0)
            {
                strcpy(outbuffer,rnConfig[iCount].config);
                pid=strstr(outbuffer,"%s");
                if(pid!=NULL)
                {
                    //Fill in the device ID
                    strcpy(pid,RN4020ID);
                    strcpy(&outbuffer[strlen(outbuffer)],(char*)&rnConfig[iCount].config[pid-outbuffer+2]);
                }
                UART2SendData((uint8_t*)outbuffer,strlen(outbuffer));
                tStart=TickGet();
                rnState=rnWaitForReply;
            }else
            {
                bOwner=bOwnerSystem;
                rnState=rnSystemMessage;
            }
            break;
            
        case rnWaitForReply:
            incount=UART2ReceiveData((uint8_t*)inbuffer);
            if(rnConfig[iCount].reply[0]==0)
            {
                tStart=TickGet();
                rnState=rnDelay;
            }else if(incount>0)
            {
                strcpy(buffer,rnConfig[iCount].reply);
                toUpper(buffer);
                toUpper(inbuffer);
                inbuffer[strlen(buffer)]=0;
                if(strcmp(buffer,inbuffer)==0)
                {
                    tStart=TickGet();
                    rnState=rnDelay;
                }
            }else if(TickGetDiff(TickGet(),tStart)>rnConfig[iCount].timeout)
            {
                //Error!
                tStart=TickGet();
                rnState=rnError;
            }
            break;

        case rnDelay:
            if(TickGetDiff(TickGet(),tStart)>rnConfig[iCount].delayaftercommand)
            {
                iCount++;
                rnState=rnSendConfig;
            }
            break;

        case rnError:
            if(TickGetDiff(TickGet(),tStart)>MS(500))
            {
                rnState=rnPOR;
            }
            break;
            
        case rnOperating:
            incount=UART2ReceiveData((uint8_t*)inbuffer);
            if(incount>0)
            {
                if(IsSystemMessage((uint8_t*)inbuffer,incount)==false)
                {
                    memcpy(RN4020InBuffer,inbuffer,incount);
                    RN4020InBufferLen=incount;
                }
            }
            break;

        case rnSystemMessage:
            if( (sysMessageReceived==true) && (RN4020_MLDP_EV_IO==0) && (RN4020Connected==true) )
            {
                RN4020_MLDP_IO=1;
                bOwner=bOwnerUser;
                rnState=rnOperating;
            }else
            {
                incount=UART2ReceiveData((uint8_t*)inbuffer);
                if(incount>0)
                {
                    if(IsSystemMessage((uint8_t*)inbuffer,incount)==true)
                    {
                        sysMessageReceived=true;
                    }
                }
            }
            break;

    }

}

void RN4020_BT_OFF(void)
{
    char outbuffer[10];
    
    if (!BTmoduleON)
    {
        RN4020_WAKE_IO=0;   //BT module OFF
        RN4020_SW_WAKE_IO=0;   //BT module OFF
                       
        strcpy(outbuffer,BT_off);
        UART2SendData((uint8_t*)outbuffer,strlen(outbuffer));
        rnState=rnPOR;
    }
    else
    {
        RN4020_WAKE_IO=1;   //BT module ON
        RN4020_SW_WAKE_IO=1;   //BT module ON
        rnState=rnPOR;
    }
}

bool IsSystemMessage(uint8_t *data, uint16_t incount)
{
    bool sysMessage=false;
    uint8_t inbuffer[256];

    memcpy(inbuffer,data,incount);
    inbuffer[incount]=0;

    toUpper((char*)inbuffer);
    if(strstr((char*)inbuffer,"CONNECTED\r\n")==(char*)inbuffer)
    {
        sysMessage=true;
        RN4020Connected=true;
        LED_B=1;
    }
    if(strstr((char*)inbuffer,"CONNECTION END\r\n")==(char*)inbuffer)
    {
        sysMessage=true;
        RN4020Connected=false;
        rnState=rnPOR;//Reset RN4020!!!
        LED_B=0;
    }
    if(strstr((char*)inbuffer,"MLDP\r\n")==(char*)inbuffer)
    {
        //Entered MLDP mode
        sysMessage=true;
    }
    return(sysMessage);
}

void RN4020SendData(uint8_t *data, uint16_t size)
{
    if( (rnState==rnOperating) && (bOwner==bOwnerUser) && (RN4020Connected==true) )
    {
        UART2SendData(data,size);
    }
}

uint8_t* RN4020GetData(void)
{
    uint8_t* retval=NULL;
    if(RN4020InBufferLen>0)
    {
        retval=RN4020InBuffer;
    }
    return(retval);
}

uint16_t RN4020GetDataSize(void)
{
    return(RN4020InBufferLen);
}

void RN4020ClearBuffer(void)
{
    RN4020InBufferLen=0;
}

void toUpper(char *buffer)
{
    uint16_t i;
    uint16_t len=strlen(buffer);
    for(i=0;i<len;i++)
    {
        if( (buffer[i]>='a') && (buffer[i]<='z') )
        {
            buffer[i]-=('a'-'A');
        }
    }
}

bool RN4020isConnected(void)
{
    return(RN4020Connected);
}