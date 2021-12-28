unsigned int secs = 0;
unsigned int mins = 0;
unsigned int hrs = 12;
const int interruptPin1 = 2; 
const int interruptPin2 = 3;
volatile byte state  = LOW;			// interrupt states
volatile byte state1 = LOW;
volatile byte state2 = LOW;
volatile byte state3 = LOW;
volatile byte state4 = LOW;
char *pin13 = (char *) 0x25;
#include <Adafruit_NeoPixel.h>
#define LED_PIN     7
#define LED_COUNT  24
#define BRIGHTNESS 50 

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t green = strip.Color(0, 64, 0);

//pins for bitshift registers
const int dataPin  = 11;      //    DS - data serial
const int latchPin = 8;      // ST_CP - storage register, latch clock pin
const int clockPin = 12;       // SH_CP - shift register clock pin

int buzzer = 9;        // I/O pin for controlling the buzzer

void updateShiftRegister(int a, int b){	//add byte to register
  	digitalWrite(latchPin, LOW);
  	int c = (a << 4)|b;
  	shiftOut(dataPin, clockPin, LSBFIRST, c);
  	digitalWrite(latchPin, HIGH);
}
void alarmbell(){
  tone(buzzer,1500);
  delay(10);
  noTone(buzzer);
}
void clockroutine(){			//logic for clock w/ secs
	secs ++;
  	if (secs >= 60){
    	secs = 0;
    	mins ++;
    }
   	setroutine();
}

void setroutine(){				//logic for clock (excluding secs)
  	if (mins >= 60){
    	mins = 0;
    	hrs ++;
  	}
  	if (hrs == 12 && mins == 0 && secs == 0){	// AM/PM toggle
  		*pin13 ^= 0b0100000;
    }
  	if (hrs >= 13){
    	hrs = 1;
    }
  	strip.clear();
  	int minHand = mins * (2.0/5.0)+1;
  	int hrHand  = ((2* hrs)+1)%24;
  	if (mins >= 30){
    	hrHand++;
    }
  	int secHand = secs * (2.0/5.0)+1;
  	if (minHand == hrHand){
      	strip.setPixelColor((minHand + 23)%24, strip.Color(0, 0, 255));
      	strip.setPixelColor((minHand + 2)%24, strip.Color(0, 0, 255));
      	strip.setPixelColor(((hrHand) % 24), strip.Color(0, 255, 0));
    	strip.setPixelColor(((hrHand + 1)%24), strip.Color(0, 255, 0));
    }
  	else if (minHand == hrHand + 1 || minHand == (hrHand + 23)%24){
      	strip.setPixelColor(minHand, strip.Color(0, 0, 255));
      	strip.setPixelColor(minHand + 2, strip.Color(0, 0, 255));
    	strip.setPixelColor(((hrHand) % 24), strip.Color(0, 255, 0));
    	strip.setPixelColor(((hrHand + 2)%24), strip.Color(0, 255, 0));
  	}
    else{
      	strip.setPixelColor(minHand %24, strip.Color(0, 0, 255));
      	strip.setPixelColor((minHand + 1)%24, strip.Color(0, 0, 255));
      	strip.setPixelColor(((hrHand) % 24), strip.Color(0, 255, 0));
    	strip.setPixelColor(((hrHand + 1)%24), strip.Color(0, 255, 0));
    }
  	for (int i = 0; i < 25; i++){
      	if (strip.getPixelColor(i)== 0){
        	strip.setPixelColor(i, strip.Color(255, 0, 255));
        }
    }
  	strip.setPixelColor((secHand)%24, strip.Color(255, 255, 0));
    strip.show();
  	setvalue(secs);
  	setvalue(mins);
  	setvalue(hrs);
}

void setvalue(int time){			//set panel time
 	int tens = (time/10)%10;
 	int ones = time%10;
	updateShiftRegister(tens, ones);
}
void hourcount(){
  	hrs ++;
  	if (hrs == 12)	// AM/PM toggle
  		*pin13 ^= 0b0100000;
  	if (hrs == 13)
    	hrs = 1;
    setvalue(secs);
  	setvalue(mins);
  	setvalue(hrs);
}
void pciSetup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{    	
  	unsigned long wait = 400;
  	int secs1 = secs;
    int mins1 = mins;
    int hrs1 = hrs;
  	state4 = digitalRead(10);
  	while (state4 == LOW){
      	state2 = digitalRead(A5);
      	state = digitalRead(interruptPin1);
      	while (state2 == LOW){
   			hourcount();
      		delay(12000);
      		state2 = digitalRead(A5);
    	}
  		while (state == LOW){
      		mins++;
      		//secs = 0;
      		setroutine();
      		delay(wait);
      		state = digitalRead(interruptPin1);
      		if (wait > 50)
          		wait -= 50;
       		if (hrs == 12 && mins == 0 && secs != 0)	// AM/PM toggle  
  				*pin13 ^= 0b0100000;					//if secs == 0											//avoid doing twice
 		}
      	state4 = digitalRead(10);
	}
  	mins = mins1;
  	secs = secs1;
  	hrs = hrs1;
}
 
ISR (PCINT1_vect){ // handle pin change interrupt for A0 to A5 here
  	state2 = digitalRead(A5);
  	while (state2 == LOW){
   		hrs ++;
  		if (hrs == 12)	// AM/PM toggle
  			*pin13 ^= 0b0100000;
  		if (hrs == 13)
    		hrs = 1;
        setvalue(secs);
  		setvalue(mins);
  		setvalue(hrs);
      	delay(12000);
      	state2 = digitalRead(A5);
	}
}  
 
ISR (PCINT2_vect) // handle pin change interrupt for D0 to D7 here
{
    state3 = digitalRead(6);
  	if (state3 == LOW){
      	secs = 0;
      	mins = 0;
      	hrs = 12;
		setvalue(secs);
  		setvalue(mins);
  		setvalue(hrs);
      	delay(7000);
    }   	
} 

void setup()
{	
  	Serial.begin(9600);
  	int i;
  	digitalWrite(6,HIGH); 
   	for (i=A0; i<=A5; i++)
      	digitalWrite(i,HIGH); 
  	 pciSetup(A5);
     pciSetup(6);
  	
  	strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
	strip.show(); 
	strip.setBrightness(BRIGHTNESS);  
  
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin,  OUTPUT);  
    pinMode(clockPin, OUTPUT);
    pinMode(interruptPin1, INPUT_PULLUP);		//button interrupts
  	pinMode(interruptPin2, INPUT_PULLUP);
  	attachInterrupt(0, setClock, CHANGE);
  	attachInterrupt(1, AMPM, CHANGE);
  	setvalue(secs);								//initialize clock (12:00)
  	setvalue(mins);
  	setvalue(hrs);
  	*pin13 ^= 0b0100000;
  
  	//set timer1 interrupt at 1Hz
  	TCCR1A = 0;// set entire TCCR1A register to 0
  	TCCR1B = 0;// same for TCCR1B
  	TCNT1  = 0;//initialize counter value to 0
  	// set compare match register for 1hz increments
  	OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  	// turn on CTC mode
  	TCCR1B |= (1 << WGM12);
  	// Set CS12 and CS10 bits for 1024 prescaler
  	TCCR1B |= (1 << CS12) | (1 << CS10);  
  	// enable timer compare interrupt
  	TIMSK1 |= (1 << OCIE1A);
  	sei();//allow interrupts
}

ISR(TIMER1_COMPA_vect){			//timer1 interrupt 1Hz 	
  	clockroutine();				//run clock every 1hz
}
void AMPM(){							//toggle AM/PM button
  state1 = digitalRead(interruptPin2);
  if (state1 == LOW){
    *pin13 ^= 0b0100000;
  }
  delay(30);
}
void setClock(){						//Minute button (can use to set clock)
  	unsigned long wait = 400;
  	state = digitalRead(interruptPin1);
  	while (state == LOW){
      	mins++;
      	//secs = 0;
      	setroutine();
      	delay(wait);
      	state = digitalRead(interruptPin1);
      	if (wait > 50){
          	wait -= 50;
        }
       	if (hrs == 12 && mins == 0 && secs != 0){	// AM/PM toggle  
  			*pin13 ^= 0b0100000;					//if secs == 0
    	}											//avoid doing twice
 	}
}
void loop(){}
