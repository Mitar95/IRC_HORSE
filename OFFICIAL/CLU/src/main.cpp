#include "Wire.h"
#include "Helper.h" 
#include "SD_helper.h"
#include "TwclHandler.h"
#include "Relay.h"
#include "DS18B20.h" 
#include "PT100.h"
#include "SoftwareTimer.h"

#define _CRT_SECURE_NO_WARNINGS
#define F_CPU   72000000UL

//#pragma pack(1)

TwclHandler twclHandler(CLU); // Create instance of twcl handler object
TwclPacket twclPacket;	// Create instance of twcl packet object

volatile char treatmentMessageVOL[101];
volatile bool waitingForTreatment = true;
volatile unsigned char GLOBAL_ERROR = 0;
                //     ---
#define LISTEN_RATE 10000    // in microseconds

void ListenHandler(void) {
    if (twclHandler.listen()) {           
            // Packet is in buffer
        twclPacket = twclHandler.getPacket();

        // Check if packet was created successfully
        if (twclPacket.isCreated()) {

            switch (twclPacket.getD())
            {
            case GENERIC:               
                switch (twclPacket.getD1())
                {
                    case NEW_TREATMENT:
                        if (twclPacket.getC() == STRING) {
                            if(waitingForTreatment){
                                ConvertVolatileStringToChar(twclPacket.getE(), treatmentMessageVOL);
                            }
                            waitingForTreatment = false;
                        }
                        break;
                    
                    default:
                        break;
                }
                break;
            case SYSTEM_STOP:
                switch (twclPacket.getD1())
                {
                    case STOP_USER:            
                        GLOBAL_ERROR = STOP_USER;
                        break;
                    
                    default:
                        break;
                }
                break;
            default:
                break;
            }
        }
    }
}

void setup() {}

void loop() {
    pinMode(PC13, OUTPUT); // LED

    #pragma region Switches
        uint8_t DS2_SWITCH_PIN = PB9;
        pinMode(DS2_SWITCH_PIN, INPUT);

        uint8_t HS3_SWITCH_PIN = PC7;
        pinMode(HS3_SWITCH_PIN, INPUT);
    #pragma endregion

    #pragma region Comunication
        twclHandler.init(&Serial1, 115200, PA11, PA12);
        HardwareSerial* console = InitSerial(&Serial3, 115200);
        Wire.begin();
    #pragma endregion

    #pragma region Relay    
        Relay dht1, mht1, aht1, hht1;
        dht1.init(PB13);
        mht1.init(PC1);
        aht1.init(PC4);
        hht1.init(PC12);
    #pragma endregion

    #pragma region Temeprature sensors
        DS18B20 ds4;    // Temperature of salt container wall
        ds4.init(PB14); // 45 CONST

        DS18B20 ms2;    // Temperature of outside wall of the mill
        ms2.init(PB15); // 45 CONST

        DS18B20 as2;    // Temperature of air inside main air tube
        as2.init(PC2); // ? CONST

        DS18B20 as3;    // Temperature of internal body
        as3.init(PC3); // ? CONST
    #pragma endregion

    #pragma region Timer ListenHandler
        // We'll use timer 2
        HardwareTimer listen(2);
        // Pause the timer while we're configuring it
        listen.pause();
        // Set up period
        listen.setPeriod(LISTEN_RATE); // in microseconds
        // Set up an interrupt on channel 1
        listen.setChannel1Mode(TIMER_OUTPUT_COMPARE);
        listen.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
        listen.attachCompare1Interrupt(ListenHandler);
        // Refresh the timer's count, prescale, and overflow
        listen.refresh();
        // Start the timer counting
        listen.resume();
    #pragma endregion
   
    #pragma region Heater temperature sensors
        #pragma region DS3 Dispense system
            // Create Object.
            PT100 ds3;

            // Initialize the sensor.
            double points_DS3[8] = {
                0.037, 0.220, 0.0614, 0.349, 
                0.0, 100.0, 440.0, 260.72
            };	// Data points from table.
            ds3.init(PA0, points_DS3, 9960.0, 3.402);

            if(ds3.calibrate(10000))	{
                // Sensor was calibrated.
            }else	{
                // Was not able to calibrate sensor in 10s.
                // Try again or continue without calibration.
            //    ds3.calibrate(10000);
            }
        #pragma endregion
        #pragma region MS1 Mlin system
            // Create Object.
            PT100 ms1;

            // Initialize the sensor.
            double points_MS1[8] = {
                0.037, 0.217, 0.0628, 0.349, 
                0.0, 100.0, 440.0, 260.72
            };	// Data points from table.
            ms1.init(PA1, points_MS1, 9960.0, 3.402);

            if(ms1.calibrate(10000))	{
                // Sensor was calibrated.
            }else	{
                // Was not able to calibrate sensor in 10s.
                // Try again or continue without calibration.
                ms1.calibrate(10000);
            }
        #pragma endregion
        #pragma region AS1 AirTube system
            // Create Object.
            PT100 as1;

            // Initialize the sensor.
            double points_AS1[8] = {
                0.037, 0.219, 0.0604, 0.344, 
                0.0, 100.0, 440.0, 260.72
            };	// Data points from table.
            as1.init(PA2, points_AS1, 9960.0, 3.402);

            if(as1.calibrate(10000))	{
                // Sensor was calibrated.
            }else	{
                // Was not able to calibrate sensor in 10s.
                // Try again or continue without calibration.
                as1.calibrate(10000);
            }
        #pragma endregion  
        #pragma region HS11 AirTube system
            // Create Object.
            PT100 hs11;

            // Initialize the sensor.
            double points_HS11[8] = {
                0.037, 0.217, 0.0604, 0.344, 
                0.0, 100.0, 440.0, 260.72
            };	// Data points from table.
            hs11.init(PA3, points_HS11, 9960.0, 3.402);

            if(hs11.calibrate(10000))	{
                // Sensor was calibrated.
            }else	{
                // Was not able to calibrate sensor in 10s.
                // Try again or continue without calibration.
                hs11.calibrate(10000);
            }
        #pragma endregion  
    #pragma endregion

    #pragma region DateTime And SD

        DS3231_init(DS3231_INTCN);
            //   TssmmhhWDDMMYYYY aka set time
    //   SetTime("T001116513072018", 16);

        if (! InitSD(console)) {
            console->println("Card failed, or not present");
        //    EndProgram();
        }
    #pragma endregion

    #pragma region Temperature sensors variable
        float ambientTemperatureDS4 = 0.0;
        float ambientTemperatureMS2 = 0.0;
        float ambientTemperatureAS2 = 0.0;
        float ambientTemperatureAS3 = 0.0;

        float heaterTemperatureDS3 = 0.0;
        float heaterTemperatureMS1 = 0.0;
        float heaterTemperatureAS1 = 0.0;
        float heaterTemperatureHS11 = 0.0;

        bool ambientTemperatureDS4isSent = false;
        bool ambientTemperatureMS2isSent = false;
        bool ambientTemperatureAS2isSent = false;
        bool ambientTemperatureAS3isSent = false;

        bool heaterTemperatureDS3isSent = false;
        bool heaterTemperatureMS1isSent = false;
        bool heaterTemperatureAS1isSent = false;
        bool heaterTemperatureHS11isSent = false;
    #pragma endregion

    #pragma region Fans
        unsigned char DFN1_PWM_PIN = PB0; // Fan for blowing hot air at the outside walls of salt container.
        unsigned char DFN1_HAL_PIN = PB12;
        uint16_t DFN1_PWM = 0;
        uint16_t DFN1_HAL = 0;
        pinMode(DFN1_PWM_PIN, OUTPUT);
        pinMode(DFN1_HAL_PIN, INPUT);

        unsigned char MFN1_PWM_PIN = PC8; // Fan for blowing hot air at mill walls.
        unsigned char MFN1_HAL_PIN = PC10;
        uint16_t MFN1_PWM = 0;
        uint16_t MFN1_HAL = 0;
        pinMode(MFN1_PWM_PIN, OUTPUT);
        pinMode(MFN1_HAL_PIN, INPUT);

        unsigned char AFN1_PWM_PIN = PC9; // Fan for blowing hot air through the main air tube.
        unsigned char AFN1_HAL_PIN = PC11;
        uint16_t AFN1_PWM = 0;
        uint16_t AFN1_HAL = 0;
        pinMode(AFN1_PWM_PIN, OUTPUT);
        pinMode(AFN1_HAL_PIN, INPUT);

        unsigned char HFN3_PWM_PIN = PB1; // For blowing hot air at the internal body.
        unsigned char HFN3_HAL_PIN = PA15;
        uint16_t HFN3_PWM = 0;
        uint16_t HFN3_HAL = 0;
        pinMode(HFN3_PWM_PIN, OUTPUT);
        pinMode(HFN3_HAL_PIN, INPUT);
    #pragma endregion

    #pragma region Capacity tables
        CapacityTable ct10;
        ct10.SENSOR_DS3 = 50.0;
        ct10.SENSOR_MS1 = 50.0;
        ct10.SENSOR_AS1 = 50.0;
        ct10.SENSOR_HS11 = 50.0;
        ct10.FAN_DFN1 = 70;
        ct10.FAN_MFN1 = 70;
        ct10.FAN_AFN1 = 70;
        ct10.FAN_HFN3 = 70;
        ct10.MLIN_MM1 = 170;

        CapacityTable ct20;
        ct20.SENSOR_DS3 = 150.0;
        ct20.SENSOR_MS1 = 150.0;
        ct20.SENSOR_AS1 = 150.0;
        ct20.SENSOR_HS11 = 150.0;
        ct20.FAN_DFN1 = 80;
        ct20.FAN_MFN1 = 80;
        ct20.FAN_AFN1 = 80;
        ct20.FAN_HFN3 = 80;
        ct20.MLIN_MM1 = 170;

        CapacityTable ct30;
        ct30.SENSOR_DS3 = 160.0;
        ct30.SENSOR_MS1 = 160.0;
        ct30.SENSOR_AS1 = 160.0;
        ct30.SENSOR_HS11 = 160.0;
        ct30.FAN_DFN1 = 90;
        ct30.FAN_MFN1 = 90;
        ct30.FAN_AFN1 = 90;
        ct30.FAN_HFN3 = 90;
        ct30.MLIN_MM1 = 180;

        CapacityTable ct40;
        ct40.SENSOR_DS3 = 170.0;
        ct40.SENSOR_MS1 = 170.0;
        ct40.SENSOR_AS1 = 170.0;
        ct40.SENSOR_HS11 = 170.0;
        ct40.FAN_DFN1 = 100;
        ct40.FAN_MFN1 = 100;
        ct40.FAN_AFN1 = 100;
        ct40.FAN_HFN3 = 100;
        ct40.MLIN_MM1 = 190;

        CapacityTable ct50;
        ct50.SENSOR_DS3 = 180.0;
        ct50.SENSOR_MS1 = 180.0;
        ct50.SENSOR_AS1 = 180.0;
        ct50.SENSOR_HS11 = 180.0;
        ct50.FAN_DFN1 = 110;
        ct50.FAN_MFN1 = 110;
        ct50.FAN_AFN1 = 110;
        ct50.FAN_HFN3 = 110;
        ct50.MLIN_MM1 = 200;

        CapacityTable ct60;
        ct60.SENSOR_DS3 =  190.0;
        ct60.SENSOR_MS1 =  190.0;
        ct60.SENSOR_AS1 =  190.0;
        ct60.SENSOR_HS11 = 190.0;
        ct60.FAN_DFN1 = 120;
        ct60.FAN_MFN1 = 120;
        ct60.FAN_AFN1 = 120;
        ct60.FAN_HFN3 = 120;
        ct60.MLIN_MM1 = 210;

        CapacityTable ct70;
        ct70.SENSOR_DS3 =  200.0;
        ct70.SENSOR_MS1 =  200.0;
        ct70.SENSOR_AS1 =  200.0;
        ct70.SENSOR_HS11 = 200.0;
        ct70.FAN_DFN1 = 130;
        ct70.FAN_MFN1 = 130;
        ct70.FAN_AFN1 = 130;
        ct70.FAN_HFN3 = 130;
        ct70.MLIN_MM1 = 220;

        CapacityTable ct80;
        ct80.SENSOR_DS3 =  210.0;
        ct80.SENSOR_MS1 =  210.0;
        ct80.SENSOR_AS1 =  210.0;
        ct80.SENSOR_HS11 = 210.0;
        ct80.FAN_DFN1 = 140;
        ct80.FAN_MFN1 = 140;
        ct80.FAN_AFN1 = 140;
        ct80.FAN_HFN3 = 140;
        ct80.MLIN_MM1 = 230;

        CapacityTable ct90;
        ct90.SENSOR_DS3 =  220.0;
        ct90.SENSOR_MS1 =  220.0;
        ct90.SENSOR_AS1 =  220.0;
        ct90.SENSOR_HS11 = 220.0;
        ct90.FAN_DFN1 = 150;
        ct90.FAN_MFN1 = 150;
        ct90.FAN_AFN1 = 150;
        ct90.FAN_HFN3 = 150;
        ct90.MLIN_MM1 = 240;

        CapacityTable ct100;
        ct100.SENSOR_DS3 =  230.0;
        ct100.SENSOR_MS1 =  230.0;
        ct100.SENSOR_AS1 =  230.0;
        ct100.SENSOR_HS11 = 230.0;
        ct100.FAN_DFN1 = 160;
        ct100.FAN_MFN1 = 160;
        ct100.FAN_AFN1 = 160;
        ct100.FAN_HFN3 = 160;
        ct100.MLIN_MM1 = 255;
    #pragma endregion

    while(1){
        console->println("Waiting for new treatment...");

        while(waitingForTreatment){
            
            #pragma region Check Heater Temperature
                if(ms1.isRequested()){ 
                    ms1.getSample();
                }else{
                    ms1.requestTemperature();
                }

                if(as1.isRequested()){ 
                    as1.getSample();
                }else{
                    as1.requestTemperature();
                }

                if(hs11.isRequested()){ 
                    hs11.getSample();
                }else{
                    hs11.requestTemperature();
                }

                if(ms1.isReady()){
                    heaterTemperatureMS1 = ms1.getTemperature();
                }
                
                if(as1.isReady()){
                    heaterTemperatureAS1 = as1.getTemperature();
                }

                if(hs11.isReady()){
                    heaterTemperatureHS11 = hs11.getTemperature();
                }
            #pragma endregion
        }

        // Check SWITCHES

        TimeFormat startTime = GetTime();

        LogFormat tretman;
        tretman.ToModelFromMessage(ConvertVolatileCharToString(treatmentMessageVOL));
        tretman.SetDateTime(ConvertIntToString(startTime.day) + "." + ConvertIntToString(startTime.mounth) + "." + ConvertIntToString(startTime.year) + ". " + ConvertIntToString(startTime.hour) + ":" + ConvertIntToString(startTime.minute) + ":" + ConvertIntToString(startTime.second));

        SetEndTime(&startTime, tretman.GetDuration());

        tretman.Print(console);

        console->println("End time: " + String(startTime.end_hour) + ":" + String(startTime.end_minute));

        CapacityTable onlineCapacity;

        switch (tretman.GetCapacity()){
            case 10:
                onlineCapacity = ct10;
                break;
            case 20:
                onlineCapacity = ct20;
                break;
            case 30:
                onlineCapacity = ct30;
                break;
            case 40:
                onlineCapacity = ct40;
                break;
            case 50:
                onlineCapacity = ct50;
                break;
            case 60:
                onlineCapacity = ct60;
                break;
            case 70:
                onlineCapacity = ct70;
                break;
            case 80:
                onlineCapacity = ct80;
                break;
            case 90:
                onlineCapacity = ct90;
                break;
            case 100:
                onlineCapacity = ct100;
                break;
            default:
                onlineCapacity = ct50;
                break;
        }

        while(!twclHandler.send(BLDCCU, MOTOR, MM1, onlineCapacity.MLIN_MM1)){}

        while(!twclHandler.send(STEPCU, MOTOR, DM1, (float)tretman.GetCapacity())){}

/*
        for(int i = 0; i <= onlineCapacity.FAN_DFN1 && i <= 255; i++){
            analogWrite(DFN1_PWM_PIN, i);
            delay(15);
        }
*/
        for(int i = 0; i <= onlineCapacity.FAN_MFN1 && i <= 255; i++){
            analogWrite(MFN1_PWM_PIN, i);
            delay(10);
        }

        for(int i = 0; i <= onlineCapacity.FAN_AFN1 && i <= 255; i++){
            analogWrite(AFN1_PWM_PIN, i);
            delay(10);
        }

        for(int i = 0; i < onlineCapacity.FAN_HFN3 && i <= 255; i++){
            analogWrite(HFN3_PWM_PIN, i);
            delay(10);
        }

    //    analogWrite(DFN1_PWM_PIN, onlineCapacity.FAN_DFN1);
        analogWrite(MFN1_PWM_PIN, onlineCapacity.FAN_MFN1);
        analogWrite(AFN1_PWM_PIN, onlineCapacity.FAN_AFN1);
        analogWrite(HFN3_PWM_PIN, onlineCapacity.FAN_HFN3);

        float heaterTemperatureOffset = 10.0;

        SoftwareTimer timerCheckIsFinish;
        SoftwareTimer timerSendDataToLCD;
        SoftwareTimer timerSendDataToLLHCU;
        
        int minuteLeft = 0;

        GLOBAL_ERROR = 0;

        while(!GLOBAL_ERROR){
            #pragma region Check Heater Temperature
                if(ms1.isRequested()){ 
                    ms1.getSample();
                }else{
                    ms1.requestTemperature();
                }

                if(as1.isRequested()){ 
                    as1.getSample();
                }else{
                    as1.requestTemperature();
                }

                if(hs11.isRequested()){ 
                    hs11.getSample();
                }else{
                    hs11.requestTemperature();
                }

                if(ms1.isReady()){
                    heaterTemperatureMS1 = ms1.getTemperature();
                }
                
                if(as1.isReady()){
                    heaterTemperatureAS1 = as1.getTemperature();
                }

                if(hs11.isReady()){
                    heaterTemperatureHS11 = hs11.getTemperature();
                }
            #pragma endregion

            aht1.tryChangeState(heaterTemperatureAS1, onlineCapacity.SENSOR_AS1, heaterTemperatureOffset);
            mht1.tryChangeState(heaterTemperatureMS1, onlineCapacity.SENSOR_MS1, heaterTemperatureOffset);
            hht1.tryChangeState(heaterTemperatureHS11, onlineCapacity.SENSOR_HS11, heaterTemperatureOffset);

            #pragma region Send Data To LLHCU
                if(!timerSendDataToLLHCU.isStarted()){
                    ambientTemperatureDS4 = ds4.getTemperature();
                    ambientTemperatureDS4isSent = twclHandler.send(LLHCU, TEMPERATURE, DS4, ambientTemperatureDS4);
                    
                    ambientTemperatureMS2 = ms2.getTemperature();
                    ambientTemperatureMS2isSent = twclHandler.send(LLHCU, TEMPERATURE, MS2, ambientTemperatureMS2);

                    ambientTemperatureAS2 = as2.getTemperature();
                    ambientTemperatureAS2isSent = twclHandler.send(LLHCU, TEMPERATURE, AS2, ambientTemperatureAS2);

                    ambientTemperatureAS3 = as3.getTemperature();
                    ambientTemperatureAS3isSent = twclHandler.send(LLHCU, TEMPERATURE, AS3, ambientTemperatureAS3);

                    if(!ambientTemperatureDS4isSent){
                        ambientTemperatureDS4isSent = twclHandler.send(LLHCU, TEMPERATURE, DS4, ambientTemperatureDS4);
                    }
                    
                    if(!ambientTemperatureMS2isSent){
                        ambientTemperatureMS2isSent = twclHandler.send(LLHCU, TEMPERATURE, MS2, ambientTemperatureMS2);
                    }

                    if(!ambientTemperatureAS2isSent){
                        ambientTemperatureAS2isSent = twclHandler.send(LLHCU, TEMPERATURE, AS2, ambientTemperatureAS2);
                    }

                    if(!ambientTemperatureAS3isSent){
                        ambientTemperatureAS3isSent = twclHandler.send(LLHCU, TEMPERATURE, AS3, ambientTemperatureAS3);
                    }

                    timerSendDataToLLHCU.start(4000); // 4 sec
                }else{
                    if(timerSendDataToLLHCU.check())   {
                        timerSendDataToLLHCU.stop();
                    }
                }
            #pragma endregion

            #pragma region Send Data To LCD
                if(!timerSendDataToLCD.isStarted()){
                    
                    heaterTemperatureMS1isSent = twclHandler.send(UICU, TEMPERATURE, MS1, heaterTemperatureMS1);
                    heaterTemperatureAS1isSent = twclHandler.send(UICU, TEMPERATURE, AS1, heaterTemperatureAS1);
                    heaterTemperatureHS11isSent = twclHandler.send(UICU, TEMPERATURE, HS11, heaterTemperatureHS11);
            
                    if(!heaterTemperatureMS1isSent){
                        heaterTemperatureMS1isSent = twclHandler.send(UICU, TEMPERATURE, MS1, heaterTemperatureMS1);
                    }

                    if(!heaterTemperatureAS1isSent){
                        heaterTemperatureAS1isSent = twclHandler.send(UICU, TEMPERATURE, AS1, heaterTemperatureAS1);
                    }

                    if(!heaterTemperatureHS11isSent){
                        heaterTemperatureHS11isSent = twclHandler.send(UICU, TEMPERATURE, HS11, heaterTemperatureHS11);
                    }

                    timerSendDataToLCD.start(2000); // 2 sec
                }else{
                    if(timerSendDataToLCD.check())   {
                        timerSendDataToLCD.stop();
                    }
                }
            #pragma endregion


 /*           
            console->println("-----------------");

            PrintSensor(console, "DS4", ambientTemperatureDS4, ambientTemperatureDS4isSent);
            PrintSensor(console, "MS2", ambientTemperatureMS2, ambientTemperatureMS2isSent);
            PrintSensor(console, "AS2", ambientTemperatureAS2, ambientTemperatureAS2isSent);
            PrintSensor(console, "AS3", ambientTemperatureAS3, ambientTemperatureAS3isSent);

    //        PrintSensor(console, "DS3", heaterTemperatureDS3, heaterTemperatureDS3isSent);
            PrintSensor(console, "MS1", heaterTemperatureMS1, heaterTemperatureMS1isSent);
            PrintSensor(console, "AS1", heaterTemperatureAS1, heaterTemperatureAS1isSent);
            PrintSensor(console, "HS11", heaterTemperatureHS11, heaterTemperatureHS11isSent);

            console->println("-----------------");

            ambientTemperatureDS4isSent = false;
            ambientTemperatureMS2isSent = false;
            ambientTemperatureAS2isSent = false;
            ambientTemperatureAS3isSent = false;

            heaterTemperatureMS1isSent = false;
            heaterTemperatureAS1isSent = false;
            heaterTemperatureHS11isSent = false;
*/
            if(!timerCheckIsFinish.isStarted()){
                if (IsFinish(startTime, &minuteLeft)){
                    break;
                }else{
                    while(!twclHandler.send(UICU, GENERIC, MINUTE_LEFT, (float)minuteLeft)){}
                }

                console->println("Time left: " + String(minuteLeft));

                timerCheckIsFinish.start(20000); // 20 sec
            }else{
                if(timerCheckIsFinish.check())   {
                    timerCheckIsFinish.stop();
                }
            }
        }

        waitingForTreatment = true;

        
        aht1.off();
        mht1.off();
        hht1.off();

        onlineCapacity.MLIN_MM1 = 0;

        while(!twclHandler.send(STEPCU, MOTOR, DM1, onlineCapacity.MLIN_MM1)){}

        if(GLOBAL_ERROR){
            tretman.SetError(String(GLOBAL_ERROR));
            while(!twclHandler.send(UICU, SYSTEM_STOP, GLOBAL_ERROR, "ERROR " + String(GLOBAL_ERROR))){}
        }else{
            while(!twclHandler.send(UICU, SYSTEM_STOP, STOP_OK, "STOP_OK")){}
        }
        
        while(!twclHandler.send(BLDCCU, MOTOR, MM1, onlineCapacity.MLIN_MM1)){}

        analogWrite(MFN1_PWM_PIN, 0);
        analogWrite(AFN1_PWM_PIN, 0);
        analogWrite(HFN3_PWM_PIN, 0);

        

        #pragma region Write treatment in log file
            String path = ConvertIntToString(startTime.year) + "/" + ConvertIntToString(startTime.mounth) + "/" + ConvertIntToString(startTime.day) + "/" + ConvertIntToString(startTime.hour);
            String fileName = "log.txt";

            if(! MakeDirection(path, console)){
        //        EndProgram();
            }
            
            if(! CreateFile(path, fileName, console)){
        //        EndProgram();
            }
            
            if(! WriteToFile(path, fileName, tretman, console)){
        //        EndProgram();
            }
        
            console->println("Successfully written data in log!");
        #pragma endregion
    }
}