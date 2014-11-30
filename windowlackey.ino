#include <Wire.h>
#include <BH1750FVI.h>
#include <EEPROM.h>

#define swpin 10
#define motApin 7
#define motBpin 9
#define swON (digitalRead(swpin)==LOW)
#define CLOSETIME 16300
#define OPENTIME 16000
#define driveopen() digitalWrite(motApin,LOW); digitalWrite(motBpin,HIGH)
#define driveclose() digitalWrite(motBpin,LOW); digitalWrite(motApin,HIGH)
#define drivestop() digitalWrite(motBpin,LOW); digitalWrite(motApin,LOW)
#define hours(x) ((unsigned long int)(1000)*60*60*(x))
#define addr 500
#define EEPROMclosed 180
#define EEPROMopen (EEPROMclosed+1)
#define avgsize 64

BH1750FVI eye;
boolean blindsareopen;
boolean switchhasbeenon;
unsigned long int lastdawn=(unsigned long int)0-hours(22);
unsigned long int lastdusk=lastdawn;
word dawnbright = 55;
word duskbright = 40;
word warnbright = 15;
word previous[avgsize];
boolean daytime;
byte idx = 0;

void setup() {
  Wire.begin();
  eye.begin(BH_Singh);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  pinMode(motApin,OUTPUT);
  digitalWrite(motApin,LOW);
  pinMode(motBpin,OUTPUT);
  digitalWrite(motBpin,LOW);
  pinMode(swpin, INPUT_PULLUP);
  
  for(byte i=0; i<avgsize; i++){
    eye.startSingleSample(BH_Singh);
    while(!eye.sampleIsFresh());
    previous[i] = eye.getLightLevel();
  }
  
  daytime = ( previous[avgsize] > ((dawnbright+duskbright)/2));
  byte savedstatus=EEPROM.read(addr);
  boolean poweronasopen;
  switchhasbeenon=swON;
  if( (savedstatus==EEPROMopen) || (savedstatus==EEPROMclosed) ){
    poweronasopen=(savedstatus-EEPROMclosed);
  }
  else{
    poweronasopen=swON;
  }
  if(poweronasopen){
    blindsareopen=true;
    if(!daytime){
      closeblinds();
    }
  }
  else{
    blindsareopen=false;
    if(daytime){
      openblinds();
    }
  }
  
}

void loop() {

  processBrightness();
  processSwitch();
  
}

void openblinds(){
  if(blindsareopen) return;
  unsigned long int started, current, elapsed;
  started=millis();
  driveopen();
  do{
    current=millis();
    elapsed=current-started;
  }while(elapsed<OPENTIME);
  drivestop();
  blindsareopen=true;
  EEPROM.write(addr,EEPROMopen);
}

void closeblinds(){
  if(!blindsareopen) return;
  unsigned long int started, current, elapsed;
  started=millis();
  driveclose();
  do{
    current=millis();
    elapsed=current-started;
  }while(elapsed<CLOSETIME);
  drivestop();
  blindsareopen=false;
  EEPROM.write(addr,EEPROMclosed);
}

void processBrightness(){
  
  eye.startSingleSample(BH_Singh);
  while(!eye.sampleIsFresh());
  previous[idx] = eye.getLightLevel();
  
  unsigned long int sum = 0;
  for(byte i=0; i<avgsize; i++) sum += (unsigned long int)previous[i];
  sum = sum/avgsize;
  
  if(daytime){
    if(previous[idx] < duskbright+warnbright) digitalWrite(13, HIGH);
    else digitalWrite(13, LOW);
    unsigned long int timesincedusk = millis()-lastdusk;
    if(timesincedusk > hours(20)){
      if((word)sum < duskbright){
        lastdusk=millis();
        daytime = false;
        closeblinds();
      }
    }
  }
  else{
    if(previous[idx] > dawnbright-warnbright) digitalWrite(13, HIGH);
    else digitalWrite(13, LOW);
    unsigned long int timesincedawn = millis()-lastdawn;
    if(timesincedawn > hours(20)){
      if((word)sum > dawnbright){
        lastdawn=millis();
        daytime = true;
        openblinds();
      }
    }
  }
  
  idx++;
  if(idx >= avgsize) idx=0;
  
}

void processSwitch(){
  if(switchhasbeenon){
    if(!swON){
      closeblinds();
      switchhasbeenon=false;
    }
  }
  else{
    if(swON){
      switchhasbeenon=true;
      openblinds();
    }
  }
}
