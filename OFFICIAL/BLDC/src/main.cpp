#include <Arduino.h>
#include "TwclHandler.h"
#include "BLDC.h"
#include "SoftwareTimer.h"


#define _CRT_SECURE_NO_WARNINGS

//#pragma pack(1)

/*
void InitializeTimer1(unsigned long rate){
        // TIMER 1 for interrupt frequency 100 Hz:
	cli(); // stop interrupts
	TCCR1A = 0; // set entire TCCR1A register to 0
	TCCR1B = 0; // same for TCCR1B
	TCNT1 = 0; // initialize counter value to 0
			   // set compare match register for 100 Hz increments
	OCR1A = rate; // = 16000000 / (8 * 100) - 1 (must be <65536)
				   // turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS12, CS11 and CS10 bits for 8 prescaler
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	sei(); // allow interrupts
}

ISR(TIMER1_COMPA_vect){
    if (twclHandler.listen()) {           
            // Packet is in buffer
        twclPacket = twclHandler.getPacket();

        // Check if packet was created successfully
        if (twclPacket.isCreated()) {

            switch (twclPacket.getD())
            {
                case MOTOR:               
                    switch (twclPacket.getD1())
                    {
                        case MM1: // set mlin speed
                            if (twclPacket.getC() == UINT8) {
                                char buf[twclPacket.getE().length()];
                                twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
                                MM1_PWM_VOL = atoi(buf);
                                isRunning = true;
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
}

void UpdateBLDC(BLDC* motor, uint8_t pwm, SoftwareTimer* timer){
    
    for(uint8_t i = 0; i <= pwm; i++){
        if(!timer->isStarted())  {
            motor->setSpeed(i);
            motor->update();

            timer->start(100);
        }else   {
            if(timer->check())   {
                timer->stop();
            }
        }
    }
}
*/

void setup() {}

void loop() {

 //   InitializeTimer1(2692);

    TwclHandler twclHandler(BLDCCU);
    TwclPacket twclPacket;

    twclHandler.init(&Serial, 19200);
    SoftwareTimer timer;
    TCCR2B = TCCR2B & 0b11111000 | 0x01;
    
    float PWM = 0;
    float PWM_OLD = 0;
    bool messageReceived = false;

    BLDC bldc;
    bldc.init(8, 9, 3);
    bldc.setDirection(true);
    bldc.setSpeed(0);
    bldc.start();
/*
    int bldcFade = 0;
    bool bldcFadeDir = true;

    while(1){
        if(!timer.isStarted())  {
            if(bldcFadeDir) {
                bldcFade++;
                bldc.setSpeed(bldcFade);
                bldc.update();

                if(bldcFade > 254) {
                    bldcFadeDir = false;
                }
            }else   {
                bldcFade--;
                bldc.setSpeed(bldcFade);
                bldc.update();

                if(bldcFade < 1) {
                    bldcFadeDir = true;
                }
            }

            timer.start(69);
        }else   {
            if(timer.check())   {
                timer.stop();
            }
        }
    }
*/
    while(1)    {

        if (twclHandler.listen()) {           
            twclPacket = twclHandler.getPacket();

            if (twclPacket.isCreated()) {

                switch (twclPacket.getD())
                {
                    case MOTOR:               
                        switch (twclPacket.getD1())
                        {
                            case MM1: // set mlin speed
                                if (twclPacket.getC() == FLOAT) {
                                    char buf[twclPacket.getE().length()];
                                    twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
                                    PWM_OLD = PWM;
                                    PWM = atof(buf);
                                    messageReceived = true;
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

        if(messageReceived){
            while(!twclHandler.send(LLHCU, GENERIC, MM1, "RECEIVED: " + String(PWM, 1))){}

            float fadePWM = 0;
            
            if(PWM > PWM_OLD){  // acceleration
                fadePWM = PWM_OLD;

                while(1){
                    if(!timer.isStarted())  {
                        if(fadePWM < PWM && fadePWM < 255){
                            fadePWM++;
                            bldc.setSpeed(fadePWM);
                            bldc.update();
                            timer.start(15);
                        }else{
                            messageReceived = false;
                    //        while(!twclHandler.send(CLU, MOTOR, MM1, "RUNNING @" + String(PWM, 1))){}
                            
                            uint16_t RPM = map(PWM, 0, 255, 0, 3500);
                            while(!twclHandler.send(CLU, MOTOR, MM1, RPM)){}

                            break;
                        }
                    }else   {
                        if(timer.check())   {
                            timer.stop();
                        }
                    }
                }
            }else{ // deceleration
                fadePWM = PWM_OLD;
                
                while(1){
                    if(!timer.isStarted())  {
                        if(fadePWM > PWM && fadePWM > 0){
                            fadePWM--;
                            bldc.setSpeed(fadePWM);
                            bldc.update();
                            timer.start(15);
                        }else{
                            messageReceived = false;
                    //        while(!twclHandler.send(CLU, MOTOR, MM1, "RUNNING @" + String(PWM, 1))){}
                            uint16_t RPM = map(PWM, 0, 255, 0, 3500);
                            while(!twclHandler.send(CLU, MOTOR, MM1, RPM)){}

                            break;
                        }
                    }else   {
                        if(timer.check())   {
                            timer.stop();
                        }
                    }
                }
            }
        }
    }
}