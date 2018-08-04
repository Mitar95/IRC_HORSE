#define F_CPU	16000000UL // For delay 

#include <UTFT.h>
#include <ClickEncoder.h>

#include <TwclHandler.h>

#include<avr/io.h>
#include<avr/interrupt.h>
#include <util\delay.h>

// Declare which fonts we will be using
extern uint8_t BigFont[];

// Set the pins to the correct ones for your development shield
// --------------------------------- ---------------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41

// Remember to change the model parameter to suit your display module!
UTFT myGLCD(SSD1963_800480, 38, 39, 40, 41);  //(byte model, int RS, int WR, int CS, int RST, int SER)

#define SW_1 4
#define SW_2 5
#define SW_3 6
#define SW_4 7
#define SW_5 54
#define SW_6 55

#define ENC1_A 56 // A2
#define ENC1_B 57
#define ENC1_SW PE6 // nema E6

#define ENC2_A 58
#define ENC2_B 59
#define ENC2_SW 9

#define LED_PIN PB7

typedef struct Encoder {
	int pinA;
	int pinB;
	int button;

	int value;
	int a_last;

	int maxValue;
	int minValue;
}Encoder;

typedef struct Treatment {
	String horse;
	String user;
	int duration;
	int capacity;
}Treatment;

typedef struct LCDText {
	String opt_1;
	String opt_2;
	String opt_3;
	String opt_4;
	String opt_5;
	String opt_6;

	String statusBar;
	String header;
}LCDText;

LCDText lcdText;

Encoder enc1, enc2;

volatile bool isRunning = false;
volatile bool waitingForHorseNames = true; // treba true
// volatile
// volatile String vol_horseNames = "Mitar@Drazic@Test 3@Test 4@Test 5";
String onlineUser = "Admin"; // volatile

TwclHandler twclHandler(UICU); // Create instance of twcl handler object
TwclPacket twclPacket;	// Create instance of twcl packet object

/******* VOLATILE *******/
volatile float ambientTemperatureDS4 = 0.0;
volatile float ambientTemperatureMS2 = 0.0;
volatile float ambientTemperatureAS2 = 0.0;
volatile float ambientTemperatureAS3 = 0.0;

volatile float heaterTemperatureDS3 = 0.0;
volatile float heaterTemperatureMS1 = 0.0;
volatile float heaterTemperatureAS1 = 0.0;
volatile float heaterTemperatureHS11 = 0.0;

volatile uint16_t fanSpeendDFN1 = 0;
volatile float timeLeft = 0.0;
volatile unsigned char GLOBAL_ERROR = 0;


void setup() {
	Serial.begin(115200);

	randomSeed(analogRead(0));

	myGLCD.InitLCD();
	pinMode(8, OUTPUT);  //backlight 
	digitalWrite(8, HIGH);

	twclHandler.init(&Serial1, 115200, 3, 2);

	DDRB |= (1 << LED_PIN);

	pinMode(SW_1, INPUT);
	pinMode(SW_2, INPUT);
	pinMode(SW_3, INPUT);
	pinMode(SW_4, INPUT);
	pinMode(SW_5, INPUT);
	pinMode(SW_6, INPUT);

	pinMode(ENC1_A, INPUT);
	pinMode(ENC1_B, INPUT);
	DDRE &= ~(1 << PORTE6);

	pinMode(ENC2_A, INPUT);
	pinMode(ENC2_B, INPUT);
	pinMode(ENC2_SW, INPUT);

	enc1.pinA = ENC1_A;
	enc1.pinB = ENC1_B;
	enc1.button = ENC1_SW;
	enc1.value = 0;
	enc1.a_last = digitalRead(enc1.pinA);

	enc2.pinA = ENC2_A;
	enc2.pinB = ENC2_B;
	enc2.button = ENC2_SW;
	enc2.value = 0;
	enc2.a_last = digitalRead(enc2.pinA);

	SetLCDText(1);

	myGLCD.setFont(BigFont);

	// TIMER 1 for interrupt frequency 100 Hz:
	cli(); // stop interrupts
	TCCR1A = 0; // set entire TCCR1A register to 0
	TCCR1B = 0; // same for TCCR1B
	TCNT1 = 0; // initialize counter value to 0
			   // set compare match register for 100 Hz increments
	OCR1A = 10000; // = 16000000 / (8 * 100) - 1 (must be <65536)
				   // turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS12, CS11 and CS10 bits for 8 prescaler
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	sei(); // allow interrupts

/*	// TIMER 2 for interrupt frequency 16000000 Hz:
	cli(); // stop interrupts
	TCCR2A = 0; // set entire TCCR2A register to 0
	TCCR2B = 0; // same for TCCR2B
	TCNT2 = 0; // initialize counter value to 0
			   // set compare match register for 16000000 Hz increments
	OCR2A = 0; // = 16000000 / (1 * 16000000) - 1 (must be <256)
			   // turn on CTC mode
	TCCR2B |= (1 << WGM21);
	// Set CS22, CS21 and CS20 bits for 1 prescaler
	TCCR2B |= (0 << CS22) | (0 << CS21) | (1 << CS20);
	// enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2A);
	sei(); // allow interrupts
*/
}

void loop() {
	myGLCD.clrScr();
	myGLCD.setColor(64, 64, 64);
	myGLCD.fillRect(0, 466, 799, 479);

	DrawStatusBar();

	delay(200);

	//  DrawIntro();

	//  delay(1500);

	DrawMainMenu();

	Treatment onlineTreatment;

	Treatment lastTreatment;
	lastTreatment.duration = -1;


	while (1) {
		int sw = IsPressed();

		if (sw) {

			myGLCD.setColor(255, 255, 255);
			myGLCD.setBackColor(0, 0, 0);

			switch (sw) {
				case 1: // START
					OpenNewTretmanWindow(&onlineTreatment);
					
					if (onlineTreatment.duration > 0) {
						lastTreatment = onlineTreatment;
						Start(&onlineTreatment);
					}

					break;
				case 2: // REPEAT LAST
					if (lastTreatment.duration > 0) {
						Start(&lastTreatment);
					} else {
						OpenNewTretmanWindow(&onlineTreatment);

						if (onlineTreatment.duration > 0) {
							lastTreatment = onlineTreatment;
							Start(&onlineTreatment);
						}
					}
					break;
				case 3:
					myGLCD.print(lcdText.opt_3, CENTER, 467 / 2);
					delay(555);
					break;
				case 4:
					myGLCD.print(lcdText.opt_4, CENTER, 467 / 2);
					delay(555);
					break;
				case 5:
					while (!twclHandler.send(CLU, SYSTEM_STOP, STOP_USER, String("USER_END"))) {}
					myGLCD.print(lcdText.opt_5, CENTER, 467 / 2);
					delay(555);
					break;
				case 6:	// SETTINGS
					OpenSettingsWindow();
					delay(555);
					break;
				case 11:
					myGLCD.print("OPTION 11", CENTER, 467 / 2);
					delay(555);
					break;
				case 12:
					myGLCD.print("OPTION 12", CENTER, 467 / 2);
					delay(555);
					break;
				default:
					myGLCD.print("error", CENTER, 467 / 2);
					delay(555);
					break;
			}

			DrawMainMenu();
		} else {
			DrawStatusBar();
		}
	}
}

ISR(TIMER1_COMPA_vect) {

//	PORTB ^= (1 << LED_PIN);

	// Listen for incoming packets
	if (twclHandler.listen()) {

		// Packet is in buffer
		twclPacket = twclHandler.getPacket();
		twclPacket.log(&Serial);

		// Check if packet was created successfully
		if (twclPacket.isCreated()) {
			switch (twclPacket.getD()) {
				case GENERIC:
					switch (twclPacket.getD1()) {
						
						case MINUTE_LEFT:
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								timeLeft = atof(buf);
							}
							break;
						
						default:
							break;
					}
					break;
				case SYSTEM_STOP:
					switch (twclPacket.getD1()) {

						case STOP_OK:
							GLOBAL_ERROR = STOP_OK;
							break;

						case STOP_USER:
							GLOBAL_ERROR = STOP_USER;
							break;

						default:
							break;
					}
					break;
				case TEMPERATURE:
					switch (twclPacket.getD1()) {
						
						case DS4: // DS4
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								ambientTemperatureDS4 = atof(buf);
							}
							break;
						case MS2: // MS2
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								ambientTemperatureMS2 = atof(buf);
							}
							break;
						case AS2: // AS2
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								ambientTemperatureAS2 = atof(buf);
							}
							break;
						case AS3: // AS3
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								ambientTemperatureAS3 = atof(buf);
							}
							break;

						

						case DS3:
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								heaterTemperatureDS3 = atof(buf);
							}
							break;
						case MS1:
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								heaterTemperatureMS1 = atof(buf);
							}
							break;
						case AS1:
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								heaterTemperatureAS1 = atof(buf);
							}
							break;
						case HS11:
							if (twclPacket.getC() == FLOAT) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								heaterTemperatureHS11 = atof(buf);
							}
							break;

						default:
							break;
					}
					break;
				case FAN:
					switch (twclPacket.getD1()) {
						case DFN1:
							if (twclPacket.getC() == UINT16) {
								char buf[twclPacket.getE().length()];
								twclPacket.getE().toCharArray(buf, twclPacket.getE().length());
								fanSpeendDFN1 = atoi(buf);
							}
						default:
							break;
					}
				default:
					break;
			}
		}
	}
}
/*
ISR(TIMER2_COMPA_vect) {
	//interrupt commands for TIMER 2 here
	DrawStatusBar();
}
*/
void DrawStatusBar() {
	myGLCD.setBackColor(64, 64, 64);
	myGLCD.setColor(255, 255, 0);
	//	myGLCD.print("DS3: " +  String(tempDS3) + " DS4: " + String(tempDS4), CENTER, 464);

	SetDefaultFont(true);
}

void SetLCDText(int x) {
	switch (x) {
		case 1: // MENU
			lcdText.opt_1 = "START NEW";
			lcdText.opt_2 = "REPEAT LAST";
			lcdText.opt_3 = " OPTION 3";
			lcdText.opt_4 = " OPTION 4";
			lcdText.opt_5 = "FORCE STOP";
			lcdText.opt_6 = "SETTINGS";
			break;

		case 2: // NEW TREATMENT
			lcdText.opt_1 = " UP";
			lcdText.opt_2 = " DOWN";
			lcdText.opt_3 = " ";
			lcdText.opt_4 = " ";
			lcdText.opt_5 = " START";
			lcdText.opt_6 = " BACK";
			break;

		case 3: // START
			lcdText.opt_1 = "";
			lcdText.opt_2 = "";
			lcdText.opt_3 = "";
			lcdText.opt_4 = "";
			lcdText.opt_5 = "";
			lcdText.opt_6 = " STOP";
			break;

		default:
			lcdText.opt_1 = "-";
			lcdText.opt_2 = "-";
			lcdText.opt_3 = "-";
			lcdText.opt_4 = "-";
			lcdText.opt_5 = "-";
			lcdText.opt_6 = "-";
			break;
	}

	DrawOptions();
}

int Start(Treatment* online) {
	timeLeft = online->duration;

	while (!twclHandler.send(CLU, GENERIC, NEW_TREATMENT, online->user + "@" + online->horse + "@" + String(online->duration) + "@" + String(online->capacity))) {}

	ClearScreen();

	myGLCD.print("User: " + online->user, CENTER, 150);
	myGLCD.print("Horse: " + online->horse, CENTER, 165);
	myGLCD.print("Duration: " + String(online->duration), CENTER, 180);
	myGLCD.print("Capacity: " + String(online->capacity), CENTER, 195);

	delay(1000);

	ClearScreen();

	SetLCDText(3);

	isRunning = true;

	int x = 5;
	int y = 10;
	int step = 22;

	GLOBAL_ERROR = 0;

	while (GLOBAL_ERROR == 0) {
		
		if (IsPressed() == 6) {
			while (!twclHandler.send(CLU, SYSTEM_STOP, STOP_USER, String("USER_END"))) { }
			break;
		}
		/* 
		myGLCD.print("DS4: " + String(ambientTemperatureDS4, 1) + "   ", x, y);
		myGLCD.print("MS2: " + String(ambientTemperatureMS2, 1) + "   ", x, y += step);
		myGLCD.print("AS2: " + String(ambientTemperatureAS2, 1) + "   ", x, y += step);
		myGLCD.print("AS3: " + String(ambientTemperatureAS3, 1) + "   ", x, y += step);

		myGLCD.print("==========", x, y += step);
		*/

//		myGLCD.print("DS3: " + String(heaterTemperatureDS3, 1) + "   ", x, y += step);
		myGLCD.print("MS1: " + String(heaterTemperatureMS1, 1) + "   ", x, y += step);
		myGLCD.print("AS1: " + String(heaterTemperatureAS1, 1) + "   ", x, y += step);
		myGLCD.print("HS11:" + String(heaterTemperatureHS11, 1) + "   ", x, y += step);

		myGLCD.print("==========", x, y += step);

//		myGLCD.print("DFN1:" + String(fanSpeendDFN1) + "   ", x, y += step);

		myGLCD.print("Left: " + String(timeLeft, 0) + " min  ", x, y += step);

		x = 5;
		y = 10;
	}

	isRunning = false;

	return 0;
}

void OpenNewTretmanWindow(Treatment* online) {
	ClearScreen();

	SetLCDText(2);

	int posX = 15;
	int posY = 20;
	int sizeY = 155;

	int selHorsePosX = posX + 250;
	int selHorsePosY = posY;

	int selDurationPosX = posX + 250;
	int selDurationPosY = posY + sizeY;

	int selCapacityPosX = posX + 250;
	int selCapacityPosY = posY + 2 * sizeY;

	myGLCD.print("    HORSE:", posX, posY);
	myGLCD.print("    DURATION:", posX, posY + sizeY);
	myGLCD.print("    CAPACITY:", posX, posY + 2 * sizeY);

	int horseCnt = 30; // 0 if SD
	/*
	while(waitingForHorseNames){ }
	waitingForHorseNames = true;

	for (int i = 0; i < vol_horseNames.length(); i++) {
		if (vol_horseNames.charAt(i) == '@') {
			horseCnt++;
		}
	}

	String horseNames[++horseCnt]; // horseCNT
	horseNames[0] = "";

	for (int i = 0, j = 0; i < vol_horseNames.length(); i++) {
		if (vol_horseNames.charAt(i) == '@') {
			j++;
			horseNames[j] = "";
		}
		else {
			horseNames[j] += vol_horseNames.charAt(i);
		}
	}
	*/

	String horseNames[horseCnt];

	for (int i = 0; i < horseCnt; i++) {
		horseNames[i] = "Horse " + String(i + 1);
	}


	int horseIndex = 0;
	int capacity = 5;
	int duration = 15;

	int navigation = 0;

	DrawSelectHorse(horseNames, horseCnt, 0, selHorsePosX, selHorsePosY);
	DrowNavigation(posX, posY + navigation * sizeY, posX, posY + navigation * sizeY);
	DrawSelectCapacity(capacity * 10, selCapacityPosX, selCapacityPosY);
	DrawSelectDuration(duration * 2, selDurationPosX, selDurationPosY);


	while (1) {
		switch (navigation) {
			case 0: // HORSE
				enc1.minValue = 0;
				enc1.maxValue = horseCnt - 1;
				enc1.value = horseIndex;
				break;
			case 1:	// DURATION
				enc1.minValue = 1;
				enc1.maxValue = 30;
				enc1.value = duration;
				break;
			case 2:	// CAPACITY
				enc1.minValue = 1;
				enc1.maxValue = 10;
				enc1.value = capacity;
				break;
			default:
				break;
		}

		switch (IsPressed()) {

			case 1: // UP
				if (navigation >= 1) {
					navigation--;
					DrowNavigation(posX, posY + navigation * sizeY, posX, posY + (navigation + 1) * sizeY);
				}
				break;
			case 2: // DOWN
				if (navigation <= 1) {
					navigation++;
					DrowNavigation(posX, posY + navigation * sizeY, posX, posY + (navigation - 1) * sizeY);
				}
				break;

			case 5: // START
				online->user = onlineUser;
				online->horse = horseNames[horseIndex];
				online->duration = duration * 2;
				online->capacity = capacity * 10;
				return;

			case 6: // BACK
				online->duration = -1;
				return;

			case 21:
				switch (navigation) {
					case 0: // HORSE
						horseIndex = enc1.value;
						DrawSelectHorse(horseNames, horseCnt, horseIndex, selHorsePosX, selHorsePosY);
						break;
					case 1:	// DURATION
						duration = enc1.value;
						DrawSelectDuration(duration * 2, selDurationPosX, selDurationPosY);
						break;
					case 2:	// CAPACITY
						capacity = enc1.value;
						DrawSelectCapacity(capacity * 10, selCapacityPosX, selCapacityPosY);
						break;
					default:
						break;
				}
			default:
				break;
		}
	}

	return;
}

void OpenSettingsWindow() {

}

void DrowNavigation(int x, int y, int xLast, int yLast) {
	SetDefaultFont(true);
	myGLCD.print("    ", xLast, yLast);
	myGLCD.print(">>> ", x, y);
	delay(256);
}

void DrawSelectHorse(String* list, int lenght, int selectedIndex, int x, int y) {
	if (selectedIndex < lenght) {
		ClearScreen(x, y, x + 280, y + 16);
		SetDefaultFont(false);
		myGLCD.print(list[selectedIndex], x, y);
		SetDefaultFont(true);
	}
}

void DrawSelectCapacity(int value, int x, int y) {
	ClearScreen(x, y, x + 200, y + 16);
	SetDefaultFont(false);
	myGLCD.print(String(value) + " %", x, y);
	SetDefaultFont(true);
}

void DrawSelectDuration(int value, int x, int y) {
	ClearScreen(x, y, x + 200, y + 16);
	SetDefaultFont(false);
	myGLCD.print(String(value) + " min", x, y);
	SetDefaultFont(true);
}

void DrawStringList(String* list, int skip, int count, int x, int y, int selectedIndex) {
	int size = 15;

	SetDefaultFont(true);

	for (int i = 0; i < count; i++) {
		if (i == selectedIndex) {
			SetDefaultFont(false);
			myGLCD.print(list[skip + i], x + i * size, y);
			SetDefaultFont(true);

			continue;
		}

		myGLCD.print(list[skip + i], x + i * size, y);
	}
}

int IsPressed() {

	if (digitalRead(SW_1)) {
		return 1;
	}

	if (digitalRead(SW_2)) {
		return 2;
	}

	if (digitalRead(SW_3)) {
		return 3;
	}

	if (digitalRead(SW_4)) {
		return 4;
	}

	if (digitalRead(SW_5)) {
		return 5;
	}

	if (digitalRead(SW_6)) {
		return 6;
	}


	if (PINE & (1 << ENC1_SW)) {
		return 11;
	}


	if (digitalRead(ENC2_SW)) {
		return 12;
	}


	if (UpdateEncoder(&enc1)) {
		return 21;
	}

	if (UpdateEncoder(&enc2)) {
		return 22;
	}

	return 0;
}

int UpdateEncoder(Encoder* enc) {
	int aState = digitalRead(enc->pinA);
	int retVal = 0;

	if (aState != enc->a_last) {
		if (digitalRead(enc->pinB) != aState) {
			enc->value++;
		} else {
			enc->value--;
		}

		retVal = 1;

		if (enc->value >= enc->maxValue) {
			enc->value = enc->maxValue;
		} else {
			if (enc->value < enc->minValue) {
				enc->value = enc->minValue;
			}
		}
	}
	enc->a_last = aState;

	return retVal;
}

void DrawIntro() {
	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 798, 464);

	for (int i = 0; i < 10000; i++) {
		myGLCD.setColor(random(255), random(255), random(255));
		myGLCD.drawPixel(2 + random(796), 16 + random(447));
	}

	delay(500);

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(1, 15, 798, 464);

	myGLCD.setColor(255, 255, 255);
	myGLCD.setBackColor(0, 0, 0);
	myGLCD.print("*** WELCOME ***", CENTER, 467 / 2);
}

void ClearScreen() {
	myGLCD.setColor(255, 255, 255);
	myGLCD.fillRect(1, 1, 798, 464);
	SetDefaultFont(true);
}

void ClearScreen(int x1, int y1, int x2, int y2) {
	myGLCD.setColor(255, 255, 255);
	myGLCD.fillRect(x1, y1, x2, y2);
	SetDefaultFont(true);
}

void SetDefaultFont(bool mode) {
	if (mode) {
		myGLCD.setBackColor(255, 255, 255);
		myGLCD.setColor(130, 3, 185);
	} else {
		myGLCD.setBackColor(130, 3, 185);
		myGLCD.setColor(255, 255, 255);
	}
}

void DrawMainMenu() {
	SetLCDText(1);
	
	ClearScreen();

	DrawOptions();
}

void DrawOptions() {
	SetDefaultFont(true);

	int X = myGLCD.getDisplayXSize() - 1;
	int Y = myGLCD.getDisplayYSize() - 3;

	int size = (Y - 2) / 6;

	for (int i = 0; i < 7; i++) {
		int x1 = X - size * 3;
		int y1 = Y - i * size;

		int x2 = x1 + size * 3;
		int y2 = y1 + size;

		myGLCD.drawRect(x1, y1, x2, y2);
	}

	int textX = X - 225;
	int textY = -42;


	myGLCD.print(lcdText.opt_1, textX, textY += size);
	myGLCD.print(lcdText.opt_2, textX, textY += size);
	myGLCD.print(lcdText.opt_3, textX, textY += size);
	myGLCD.print(lcdText.opt_4, textX, textY += size);
	myGLCD.print(lcdText.opt_5, textX, textY += size);
	myGLCD.print(lcdText.opt_6, textX, textY += size);
}