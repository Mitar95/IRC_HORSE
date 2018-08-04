#include <Arduino.h>
#include "TwclHandler.h"
#include "global.h"
#include "speed_cntr.h"
#include "SoftwareTimer.h"

struct GLOBAL_FLAGS status = {FALSE, FALSE, 0};

void setup() {}

void loop() { 
    TwclHandler twclHandler(STEPCU);
    TwclPacket twclPacket;
    
    twclHandler.init(&Serial, 19200);

    uint8_t POSSITION_DS5_PIN = PD7;
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(DIRECTION_PIN, OUTPUT);
    pinMode(PULSE_PIN, OUTPUT);
    pinMode(POSSITION_DS5_PIN, INPUT);

    digitalWrite(ENABLE_PIN, LOW);
    digitalWrite(DIRECTION_PIN, HIGH);

    Calibrate(POSSITION_DS5_PIN);

    speed_cntr_Init_Timer1();
	
	
    float capacity = 0.0;

	signed int steps = AngleToStep(90.0);
	unsigned int acceleration = 23; // 23
	unsigned int deceleration = acceleration;
	unsigned int speed = 100; // 100

    unsigned int msDelay = 1;
	
    SoftwareTimer timer;

    while(1)
    {		
        if (twclHandler.listen()) {           
            twclPacket = twclHandler.getPacket();

            if (twclPacket.isCreated()) {
                switch (twclPacket.getD())
                {
                case MOTOR:               
                    switch (twclPacket.getD1())
                    {
                    case DM1:
                        if (twclPacket.getC() == FLOAT) {
                            char buf[twclPacket.getE().length()];
                            twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
                            capacity = atof(buf);
                            
                            if(capacity > 0){
                                msDelay = 123456 / capacity;
                            }

                            while(!twclHandler.send(CLU, MOTOR, DM1, "RECEIVED: " + String(capacity, 1))) {}
                        }
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

        if(capacity < 1){
            continue;
        }

        if(!timer.isStarted()){
            if(status.running == FALSE){
                speed_cntr_Move(steps, acceleration, deceleration, speed);
            }

            timer.start(msDelay);
        }else{
            if(timer.check()){
                timer.stop();
            }
        }
    }
}