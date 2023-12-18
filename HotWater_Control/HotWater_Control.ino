/*  Si la alarma de Temperatura indica clima caliente
    Solo va funcionar la regadera electrica
    Si indica clima Frio va ha funcionar los 2, regadera y boiler
    si esque no hay alguna alarma de error de hardware.

*/
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
#define version "0.1.19"
#define TIMER1_LED  13
#define BOBINA_SOLENOIDE_GAS_LED 12//Solenoide que abre la llave de gas
#define SWITCH_ENERGIA_PRINC_LED 11//switch que activa la chispa alto voltaje para que encienda el gas
#define SWITCH_RESIST_HOTWAT_LED 10//Resistencia que calienta la agua en la regadera
#define POWER_CONTROL_LED         9//Controla la base del MOSFET que alimenta los transistores que activan los reles
#define ALARMA_FALTA_GAS_LED      8
#define ALARMA_BOBINA_FALLA_LED   7
#define ALARMA_SOBREVOLTAJE_LED   6
#define ALARMA_RESIST_REGAD_LED   5
#define VENTILADOR_LED            4
#define SENS_CORRIENTE_BOBINA_IN  A1
#define SENS_CORRIENTE_RESIST_IN  A2
#define SENS_OPTICO_DE_LLAMA_IN   A3
#define CAL_SENS_OPTICO_IN        A4//Calibracion sens. optico

#define INT0 0 //pin(2) en Nano-Arduino  Sensa si la temperatura se sobrepaso
#define INT1 1 //pin(3) en Nano-Arduino

#define MAXIMO_CORRIENTE_BOBINA   200  
#define MAXIMO_CORRIENTE_RESIST   1000

#define ON  1
#define OFF 0

#define ALARMA_FALTA_GAS    0x01
#define ALARMA_BOBINA_FALLA 0x02
#define ALARMA_SOBREVOLTAJE 0x04
#define ALARMA_RESIST_REGAD 0x08

#define HAY_LLAMA 0x33 //indica que hay una llama 



volatile boolean timer1_out = HIGH;
volatile boolean timer2_out = HIGH;
volatile boolean timer3_out = HIGH;
volatile boolean timer4_out = HIGH;


unsigned char count1,count2,count3,estado;
unsigned char DiscountTime,AlarmaTemp=0; //cuenta el tiempo de monitoreo de espera de interrupcion de temperatura
unsigned char vecesEncendido;
unsigned char AlarmaStatus;//guarda las alarmas que hay activadas

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
  
  if(DiscountTime>0){DiscountTime--;}
}//fin de insterrupcion del timer1-----------------------------

/* interrupcion externa INT0 pin-2 en nano-arduino* 
  recibe la señal del relevador de alarma de temperatura para
  indicar que la temperatura ambiental paso de su
  threshold/umbral por lo tanto solo encendemos la
  regadera no el Boiler */
void IRQ_INT0_EXTERNA(){
    AlarmaTemp=0xAA; 
}//fin de interrupcion externa INT0 PIN2  ++++++++++++++++++++++++++++




void setup() {
  pinMode(TIMER1_LED, OUTPUT);
  pinMode(BOBINA_SOLENOIDE_GAS_LED,OUTPUT);
  pinMode(SWITCH_RESIST_HOTWAT_LED,OUTPUT);
  pinMode(SWITCH_ENERGIA_PRINC_LED,OUTPUT);
  pinMode(POWER_CONTROL_LED,OUTPUT);
  pinMode(ALARMA_FALTA_GAS_LED,OUTPUT);
  pinMode(ALARMA_BOBINA_FALLA_LED,OUTPUT);
  pinMode(ALARMA_SOBREVOLTAJE_LED,OUTPUT);
  pinMode(ALARMA_RESIST_REGAD_LED,OUTPUT);
  pinMode(VENTILADOR_LED,OUTPUT);

  delay(3000);
  setupTimer();
  setTimer1(3);
  digitalWrite(BOBINA_SOLENOIDE_GAS_LED,0);
  digitalWrite(SWITCH_RESIST_HOTWAT_LED,0);
  digitalWrite(SWITCH_ENERGIA_PRINC_LED,0);
  digitalWrite(ALARMA_FALTA_GAS_LED,0);
  digitalWrite(ALARMA_BOBINA_FALLA_LED,0);
  digitalWrite(ALARMA_SOBREVOLTAJE_LED,0);
  digitalWrite(ALARMA_RESIST_REGAD_LED,0);
  digitalWrite(VENTILADOR_LED,0);
  
  
  digitalWrite(POWER_CONTROL_LED,0);//Encender Fuente de Reles
  attachInterrupt(INT0,IRQ_INT0_EXTERNA,RISING); 

  //digitalWrite(POWER_CONTROL_LED,1);//Al fin de todo Encender Fuente de Reles
}//fin setup---------------------------------

//AlarmaTemp indica que se activo el Rele del Sensor Ambiental que indica que se paso el
//Umbral de la Temperatura Fria a la Caliente
void loop() {
  static int delay2;
  static unsigned char status;
  switch(estado){
    case 1:delay2=100;
           if(AlarmaTemp!=0xAA)AlarmaTemp=0;estado++;break;//0xAA indica que se activo 
    case 2:if(--delay2<3)estado++;break;//para darle oportunidad al Sensor ambiental que active su rele
    case 3:digitalWrite(POWER_CONTROL_LED,1);//Al fin de todo Encender Fuente de Reles
    case 4:if(AlarmaTemp==0xAA)estado=10;else{estado++;}break;//muy caliente el Ambiente solo enciende la resistencia de regadera
    case 5:if(Switch_de_Alto_Voltaje(ON,3))estado++;break; //3 segundos
    case 6:if(Bobina_de_Gas(ON)){estado++;}break;//encender Bobina de Gas
    case 7:estado++;break;//delay
    case 8:if(Monitor_Temperatura_v2(&status))estado++;break;//cuenta el tiempo que tarda en llegar la interrupcion del rele de temp.
    case 9:if(status==HAY_LLAMA) estado++; break;        
    case 10:if(Resistencia_Regadera(ON))estado++;break;
    case 11:if(!(AlarmaStatus&0x07)) estado++;else estado=9;break;//NO HAY LA 3 PRIMERAS ALARMAS
    case 12:if(Monitor_Temperatura2_v2())estado++;break;
    case 13:break;//estado de Error
    default:estado=1;break;}//fin de switch loop
}//fin loop---------------------------------------------------------



void Ventilador(unsigned char estado){
 switch(estado){
  case 0: 
  case 1:digitalWrite(VENTILADOR_LED,estado);break;
  default:break;}
}//fin de ventilador++++++++++++++++++++++++++++++++++++

unsigned char  Resistencia_Regadera(unsigned char estado){
static unsigned char state;
unsigned char ret=0;
 if(!estado){digitalWrite(SWITCH_RESIST_HOTWAT_LED,OFF);ret=1;}
 else{switch(state){
          case 1:digitalWrite(SWITCH_RESIST_HOTWAT_LED,ON);state++;break; 
          case 2:Lectura_de_Corriente_de_Resist_Reg();state++;break;
          case 3:ret=1;state++;break;
          default:state=1;break;}}
return ret;
}//fin de ventilador++++++++++++++++++++++++++++++++++++

unsigned char Lectura_de_Corriente_de_Resist_Reg(void){
 if(analogRead(SENS_CORRIENTE_BOBINA_IN)<MAXIMO_CORRIENTE_BOBINA){
         Encender_Alarma(ALARMA_BOBINA_FALLA);}
}//fin de leer corriente de ressistencia 

unsigned char Lectura_de_Corriente_de_Bobina_de_Gas(void){
 if(analogRead(SENS_CORRIENTE_BOBINA_IN)<MAXIMO_CORRIENTE_RESIST){
         Encender_Alarma(ALARMA_RESIST_REGAD);} 
  }//fin de leeer la corriente de bobina de gas+++++++++++++





void Encender_Alarma(unsigned char alarma){
//unsigned char ret=0;  
   switch(alarma){
     case ALARMA_FALTA_GAS:AlarmaStatus|=alarma;
              digitalWrite(ALARMA_FALTA_GAS_LED,ON);break;
     case ALARMA_BOBINA_FALLA:AlarmaStatus|=alarma;
              digitalWrite(ALARMA_BOBINA_FALLA_LED,ON);break;
     case ALARMA_SOBREVOLTAJE:AlarmaStatus|=alarma;
              digitalWrite(POWER_CONTROL_LED,OFF);//Al fin de todo Encender Fuente de Reles
              digitalWrite(ALARMA_SOBREVOLTAJE_LED,ON);break;
     case ALARMA_RESIST_REGAD:AlarmaStatus|=alarma;
              digitalWrite(ALARMA_RESIST_REGAD_LED,ON);break;
     default:break;}
}//fin de encender Alarmar------------------------------


//enciende la Bobina de GAS y monitorea la corriente
//que comprueba que haya bobina
/* Si la lectura de la corriente de la bobina de Gas no es
   buena se activa una Alarma de error un LEd y un Buzzer */
unsigned char Bobina_de_Gas(int estado){
static unsigned char state,v;//
unsigned char ret=0;
  if(estado==0){digitalWrite(BOBINA_SOLENOIDE_GAS_LED,OFF);ret=1;}
  else{switch(state){
          case 1:digitalWrite(BOBINA_SOLENOIDE_GAS_LED,ON);state++;break;
          case 2:if(Lectura_de_Corriente_de_Bobina_de_Gas());state++;break;
          case 3:if(v) ret=1; else ret=0;state++;break;
          default:state=1;break;}}///fin switch

return ret;  
}//fin de encender la bobina de gas LP-----------------------------------



/* funcion que  monitorea la llama de Gas 
   Sino se detecta LLama 
   se enciende la chipa durante 1 segundo 3 veces,
   y se enciende la bobina de gas, si no se detecta la LLAMA
   se enciende la alarma del Falta de Gas, y se apaga la bobina
   si se detecta la  LLAMA, se sale del subprograma exitosamente*/
unsigned char Monitor_Temperatura_v2(unsigned char *status){
unsigned char ret=0;
static unsigned char estado;
const unsigned char TIEMPO_DE_ESPERA_SENSOR=12;//3 SEGUNDOS
const unsigned char TIME_WAIT=4;//tiempo de encendido chispa
const int UMBRAL_DETECCION_LUMBRE=1000;
int a,b;
static int delay1;
   switch(estado){
     case 1:AlarmaTemp=0;estado++;break;
     case 2:a=analogRead(SENS_OPTICO_DE_LLAMA_IN);
            b=analogRead(CAL_SENS_OPTICO_IN);
            if(a>b){estado=20;}else{estado++;}break;
     case 3:DiscountTime=TIME_WAIT;
            vecesEncendido=3;estado++;break;
     case 4:estado++;break;//encender Bobina Encendida
     case 5:if(Switch_de_Alto_Voltaje(ON,1)){
                estado++;delay1=5;}break;
     case 6:if(delay1==0)estado++;break; //delay de 100mseg
     case 7:a=analogRead(SENS_OPTICO_DE_LLAMA_IN);
            b=analogRead(CAL_SENS_OPTICO_IN);
            if(a>b){estado=20;}
            else{if(DiscountTime==0){
                    if(vecesEncendido>0){vecesEncendido--;
                                   DiscountTime=TIME_WAIT;
                                  estado=5;}
                    else{estado++;}}
                 else{estado=7;}} 
            break;       
     case 8:Bobina_de_Gas(OFF);estado++;break;
     case 9:Encender_Alarma(ALARMA_FALTA_GAS);estado=20;break;
     case 20:ret=1;estado++;break;
     default:estado=1;break;}//fin switch++++++++++   
return ret;  
}//-fin de monitor de temperatura..................................



/* si llegamos hasta a qui es porque no hubo Alarmas de Gas
 ni Sobrevoltaje, ni Alarma de Bobina Fallando. Por lo que 
 esta Funcion Monitorea que no haya sobrevoltaje, que la Bobina
   Funcione y que La temperatura no baje/ que la llama nose apague.*/
unsigned char Monitor_Temperatura2_v2(void){
unsigned char ret=0;
static unsigned char estado;
const unsigned char TIEMPO_DE_ESPERA_SENSOR=12;//3 SEGUNDOS
const unsigned char TIME_WAIT=4;//tiempo de encendido chispa
const int UMBRAL_DETECCION_LUMBRE=500;//Umbral de luz para detectar llama
   switch(estado){
     case 1:DiscountTime=TIEMPO_DE_ESPERA_SENSOR;AlarmaTemp=0;
            estado++;break;
     case 2:if(analogRead(SENS_OPTICO_DE_LLAMA_IN)<UMBRAL_DETECCION_LUMBRE){
                  Encender_Alarma(ALARMA_FALTA_GAS); }
            estado++;break;
     case 3:Lectura_de_Corriente_de_Bobina_de_Gas();
            vecesEncendido=3;estado++;break;
     case 4:estado++;break;//encender Bobina de Gas
     case 5:if(Switch_de_Alto_Voltaje(ON,1))estado++;break;
     case 6:if(AlarmaTemp){estado=20;}
            else{if(DiscountTime==0){
                    if(vecesEncendido>0){vecesEncendido--;
                                   DiscountTime=TIME_WAIT;
                                  estado=5;}
                    else{estado++;}}
                 else{estado=6;}} 
            break;       
     case 7:Bobina_de_Gas(OFF);estado++;break;
     case 8:Encender_Alarma(ALARMA_FALTA_GAS);estado=20;break;
     case 20:ret=1;estado++;break;
     default:estado=1;break;}//fin switch++++++++++   
return ret;  
}//-fin de monitor de temperatura..................................








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
