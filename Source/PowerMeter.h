#ifndef _POWER_METER_

#define _POWER_METER_


void PowerMeterInit(void);
void ReadVoltage(void);
void PowerMeterRead(void);
void PowerMeterReadEnergy(void);
extern void Calibration1Sequence(void);
extern void Calibration2Sequence(void);
void SetRange(void);
void FrequencyCalibrationCommand(void);
void GainCalibrationCommand(void);
void ReactiveCalibrationCommand(void);
void SaveToFlashCommand(void);

uint8_t* PowerMeterGetData(void);
uint8_t* PowerMeterGetDataEnergy(void);
uint8_t PowerMeterGetDataSize(void);
uint8_t PowerMeterGetDataSizeEnergy(void);
void PowerMeterClearBuffer(void);
//i.a.
void PowerMeterClearEnergyBuffer(void);
void StartEnergyCount(void);
void StopEnergyCount(void);

extern uint8_t CalibrationState;
extern uint8_t ReadingState;
extern bool RelaytoDefault;
extern bool RelayOnStartUp;
extern bool Calibration1;
extern bool Calibration2;
extern bool Voltage_Energy_switch;
extern bool ReadCommand;

#endif