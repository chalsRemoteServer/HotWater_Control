
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
#define version "0.1.8"
#define TIMER1_LED  13
#define BOBINA_SOLENOIDE_GAS_LED 12//Solenoide que abre la llave de gas
#define SWITCH_ENERGIA_PRINC_LED 11//switch que activa la chispa alto voltaje para que encienda el gas
#define SWITCH_RESIST_HOTWAT_LED 10//Resistencia que calienta la agua en la regadera
#define POWER_CONTROL_LED         9//Controla la base del MOSFET que alimenta los transistores que activan los reles

#define ON  1
#define OFF 0

volatile boolean timer1_out = HIGH;
volatile boolean timer2_out = HIGH;
volatile boolean timer3_out = HIGH;
volatile boolean timer4_out = HIGH;


unsigned char count1,count2,count3,estado;
struct _Control_Salida{
  int time1;//en cuando dura el estado deseado
  int state;//estado deseado.
  }swHV;


// Timer1 interrupt-----------------------------------------------
ISR (TIMER1_COMPA_vect) {
  digitalWrite(TIMER1_LED, timer1_out);
  timer1_out = !timer1_out;

  if(swHV.time1>0){
      if(--swHV.time1==0){
             digitalWrite(SWITCH_ENERGIA_PRINC_LED,!swHV.state);}}

  
}//fin de insterrupcion del timer1-----------------------------


void setup() {
  pinMode(TIMER1_LED, OUTPUT);
  pinMode(BOBINA_SOLENOIDE_GAS_LED,OUTPUT);
  pinMode(SWITCH_RESIST_HOTWAT_LED,OUTPUT);
  pinMode(SWITCH_ENERGIA_PRINC_LED,OUTPUT);
  pinMode(POWER_CONTROL_LED,OUTPUT);
  delay(3000);
  setupTimer();
  setTimer1(3);
  digitalWrite(BOBINA_SOLENOIDE_GAS_LED,0);
  digitalWrite(SWITCH_RESIST_HOTWAT_LED,0);
  digitalWrite(SWITCH_ENERGIA_PRINC_LED,0);
  digitalWrite(POWER_CONTROL_LED,1);//Encender Fuente de Reles
}//fin setup---------------------------------

void loop() {
  switch(estado){
    case 1:if(Switch_de_Alto_Voltaje(ON,3))estado++;break; //3 segundos
    case 2:if(Bobina_de_Gas(ON))estado++;break;//encender Bobina de Gas
    case 3:if(Monitor_Temperatura())estado++;break;//cuenta el tiempo que tarda en llegar la interrupcion del rele de temp.
    case 4:Ventilador(ON);estado++;break;
    case 5:if(Monitor_Temperatura2())estado++;break;
    case 6:break;//estado de Error
    default:estado=1;break;}//fin de switch loop
}//fin loop---------------------------------------------------------

//Enciende o apaga el control de alto voltaje de la chispa durante
//el tiempo que se indique, time1=0 es infinito.
//state: ON||OFF
unsigned char Switch_de_Alto_Voltaje(unsigned char state,int time1){
switch(state){//codigo de seguridad
  case 1:break;
  case 0:break;
  default:state=0;break;}
  digitalWrite(SWITCH_ENERGIA_PRINC_LED,state);
if(time1>0){swHV.time1=time1*3;swHV.state=state;}//por 3 es para que sea un seg aprox
else{swHV.time1=0;}
return 1;
}//fin de encender o apagar el apto voltaje-----------------------------


//enciende la Bobina de GAS y monitorea la corriente
//que comprueba que haya bobina
/* Si la lectura de la corriente de la bobina de Gas no es
   buena se activa una Alarma de error un LEd y un Buzzer */
unsigned char Bobina_de_Gas(int estado){
static unsigned char state,v;//
unsigned char ret=0;
  switch(state){
    case 1:digitalWrite(BOBINA_SOLENOIDE_GAS_LED,ON);state++;break;
    case 2:if(Lectura_de_Corriente_de_Bobina_de_Gas(&v));state++break;
    case 3:if(v) ret=1; else ret=0;state++;break;
    default:state=1;break;}///fin switch

return ret;  
}//fin de encender la bobina de gas LP-----------------------------------



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
