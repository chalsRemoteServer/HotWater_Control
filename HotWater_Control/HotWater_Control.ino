
/*
|  CS12  |  CS11  |  CS10  |  Description                       |
|:-------|:------:|:------:|:----------------------------------:|
|   0    |    0   |    0   |  No clock source(timer stop)       |
|   0    |    0   |    1   |  clk / 1                           |
|   0    |    1   |    0   |  clk / 8                           |
|   0    |    1   |    1   |  clk / 64                          |
|   1    |    0   |    0   |  clk / 256                         |
|   1    |    0   |    1   |  clk / 1024                        |
|   1    |    1   |    0   |  External clock source no T1 pin.  |
|   1    |    1   |    1   |  External clock source no T1 pin.  |
*/

#define TIMER1_LED  13
#define BOBINA_SOLENOIDE_GAS_LED 12//Solenoide que abre la llave de gas
#define SWITCH_ENERGIA_PRINC_LED 11//switch que activa la chispa alto voltaje para que encienda el gas
#define SWITCH_RESIST_HOTWAT_LED 10//Resistencia que calienta la agua en la regadera
#define POWER_CONTROL_LED         9//Controla la base del MOSFET que alimenta los transistores que activan los reles

volatile boolean timer1_out = HIGH;
volatile boolean timer2_out = HIGH;
volatile boolean timer3_out = HIGH;
volatile boolean timer4_out = HIGH;


unsigned char count1,count2,count3;


// Timer1 interrupt-----------------------------------------------
ISR (TIMER1_COMPA_vect) {
  digitalWrite(TIMER1_LED, timer1_out);
  timer1_out = !timer1_out;
 
  if(++count1>2){count1=0;
      digitalWrite(BOBINA_SOLENOIDE_GAS_LED,timer2_out);
      timer2_out=!timer2_out;}

  if(++count2>3){count2=0;
      digitalWrite(SWITCH_ENERGIA_PRINC_LED,timer3_out);
      timer3_out=!timer3_out;}

  if(++count3>4){count3=0;
      digitalWrite(SWITCH_RESIST_HOTWAT_LED,timer4_out);
      timer4_out=!timer4_out;}
  
}//fin de insterrupcion del timer1-----------------------------


void setup() {
  pinMode(TIMER1_LED, OUTPUT);
  pinMode(BOBINA_SOLENOIDE_GAS_LED,OUTPUT);
  pinMode(SWITCH_RESIST_HOTWAT_LED,OUTPUT);
  pinMode(SWITCH_ENERGIA_PRINC_LED,OUTPUT);
  delay(3000);
  setupTimer();
  setTimer1(3);
}//fin setup---------------------------------

void loop() {
}

void setupTimer() {
  cli();
  initTimer1();
  sei();
  }//fin de setup timer ---------------------------

void initTimer1() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 10000;
 // TCCR1B = bit(WGM12) | bit(CS12)| bit(CS10);  // WGM12 => CTC(Clear Timer on Compare Match), CS12 & CS10  => prescaler 1/1024
  TCCR1B = bit(WGM12) | bit(CS11)| bit(CS10);
  TIMSK1 = bit(OCIE1A);                        // OCIE1A => Timer1 compare match A interrupt
}//fin de initializaer timer 1--------------------------------


void setTimer1(float _time) {
  long cnt = 16000000 / 1024 * _time;  // cnt = clk / prescaler * time(s)
  if(cnt > 65535) {
    cnt = 65535;         // "timer1 16bit counter over."
  }
  OCR1A = cnt;           // Output Compare Register Timer1A
  TIMSK1 = bit(OCIE1A);
}//fin de set timer 1-------------------------------------

void stopTimer1(){
    TIMSK1 = 0;
}//fin de sstop timer---------------------------------------
