#include <Wire.h>
#include <BH1750FVI.h>
#include <EEPROM.h>

#define swpin 10    //pin that the override switch is connected to
#define motApin 7   //arduino pin for the motor driver 'A' input
#define motBpin 9   //arduino pin for the motor driver 'B' input
#define swON (digitalRead(swpin)==LOW) //a macro to read the switch (for easy polarity reversal)
#define CLOSETIME 16800  //how many milliseconds to drive when closing
#define OPENTIME 16500   //always open for a little less, to make sure it always gets all the way closed
#define driveopen() digitalWrite(motApin,LOW); digitalWrite(motBpin,HIGH)  //macros to drive motor
#define driveclose() digitalWrite(motBpin,LOW); digitalWrite(motApin,HIGH) //
#define drivestop() digitalWrite(motBpin,LOW); digitalWrite(motApin,LOW)   //
#define hours(x) ((unsigned long int)(1000)*60*60*(x))  //a macro to convert hours to ms
#define addrStat 400  //the EEPROM address to store the last known state of the blinds, for power outages
#define addrDawn 401  //the EEPROM address to store the last calibrated dawn light level (2 bytes)
#define addrDusk 403  //the EEPROM address to store the last calibrated dusk light level (2 bytes)
#define EEPROMclosed 180             //an arbitrary code for the EEPROM state
#define EEPROMopen (EEPROMclosed+1)  //
#define avgsize 64  //the window size for the moving average filter

BH1750FVI eye;            //create the light sensor object
boolean blindsareopen;    //stores the status of the blinds in memory
boolean switchhasbeenon;  //stores the last known switch state
unsigned long int lastdawn=(unsigned long int)0-hours(22); //the last dusk and dawn time in millis.
unsigned long int lastdusk=lastdawn;                       //they initialize as though they happened
                                                           //more than 22 hours before startup
word dawnbright = 55;  //brightness level for dawn trigger
word duskbright = 40;  //brightness level for dusk trigger
word warnbright = 15;  //warning window, for LED indicator to let you know they'll move soon
word previous[avgsize];//buffer of previous samples for moving average light level
byte idx = 0;          //index into 'previous' buffer
boolean daytime;       //stores whether or not it's currently daytime

void setup() {
  Wire.begin();                  //have to use Wire to use light sensor
  eye.begin(BH_Singh);           //start the light sensor in single-sample, half-scale mode
  pinMode(13,OUTPUT);            //set up pins
  digitalWrite(13,LOW);          //
  pinMode(motApin,OUTPUT);       //
  digitalWrite(motApin,LOW);     //
  pinMode(motBpin,OUTPUT);       //
  digitalWrite(motBpin,LOW);     //
  pinMode(swpin, INPUT_PULLUP);  //the override switch uses the internal pullups
  
  for(byte i=0; i<avgsize; i++){        //initialize the averaging buffer.
    eye.startSingleSample(BH_Singh);    //getting all these samples
    while(!eye.sampleIsFresh());        //will take a few seconds.
    previous[i] = eye.getLightLevel();  //
  }
  
  daytime = ( previous[avgsize] > ((dawnbright+duskbright)/2));  //initial guess at whether it's daytime
  byte savedstatus=EEPROM.read(addrStat); //see if there's a saved status in EEPROM
  boolean poweronasopen;              //stores the flag for whether powering on with blinds closed or open
  switchhasbeenon=swON;               //check to see if we've powered up with the switch 'on'
  if( (savedstatus==EEPROMopen) || (savedstatus==EEPROMclosed) ){
    poweronasopen=(savedstatus==EEPROMopen);   //if there's a valid code in EEPROM, use that for status
    dawnbright=EEPROM.read(addrDawn)<<8;       //also load both bytes of
    dawnbright+=EEPROM.read(addrDawn+1);       //dawn light level and
    duskbright=EEPROM.read(addrDusk)<<8;       //both bytes of
    duskbright+=EEPROM.read(addrDusk+1);       //dusk light level
  }                                            //
  else{                                        //otherwise, this is probably the first powerup,
    poweronasopen=swON;                        //so the user has indicated whether the blinds are open
                                               //or closed by setting the switch appropriately
    EEPROM.write(addrDusk, byte(duskbright>>8)); //also, make sure that we store
    EEPROM.write(addrDusk+1, byte(duskbright));  //the default dusk/dawn brightness
    EEPROM.write(addrDawn, byte(dawnbright>>8)); //levels into EEPROM for the future
    EEPROM.write(addrDawn+1, byte(dawnbright));  //(two bytes each, just in case)
  }
  if(poweronasopen){
    blindsareopen=true;                        //remember that the blinds are open
    if(!daytime){                              //but if they need to close,
      closeblinds();                           //then close them
      lastdawn=millis()-hours(19);             //prevent immediate close/open cycle at startup
    }
  }
  else{
    blindsareopen=false;                       //opposite of above
    if(daytime){                               //
      openblinds();                            //
      lastdusk=millis()-hours(19);             //
    }
  }
  
}

void loop() {

  processBrightness();   //all the code is below in functions
  processSwitch();       //
  
}

void openblinds(){
  if(blindsareopen) return;    //never try to open blinds that are already open
  unsigned long int started, current, elapsed; //timing variables
  started=millis();              //note the current time
  driveopen();                   //turn on the motor
  do{
    current=millis();            //keep checking the time
    elapsed=current-started;     //
  }while(elapsed<OPENTIME);      //and seeing whether the motor has run long enough
  drivestop();                   //turn off the motor
  blindsareopen=true;            //record in RAM that the blinds are now open
  EEPROM.write(addrStat,EEPROMopen); //record in EEPROM
}

void closeblinds(){ //same as above, but for different length of time and motor direction
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
  EEPROM.write(addrStat,EEPROMclosed);
}

void processBrightness(){
  
  eye.startSingleSample(BH_Singh);      //start a new light sample
  while(!eye.sampleIsFresh());          //wait for it to finish
  previous[idx] = eye.getLightLevel();  //replace a sample into the buffer at the current index
  
  unsigned long int sum = 0;
  for(byte i=0; i<avgsize; i++) sum += (unsigned long int)previous[i];
  sum = sum/avgsize;                    //create an average of the entire buffer window
  
  if(daytime){ //do the following if we're checking for it to get dark out
    if(previous[idx] < duskbright+warnbright) digitalWrite(13, HIGH); //light the warning LED if the most
    else digitalWrite(13, LOW);                                       //recent sample is almost dark
    unsigned long int timesincedusk = millis()-lastdusk; //calculate time since last dusk triggered
    if(timesincedusk > hours(20)){ //this forces dusk to be close to the same time of day as yesterday
      if((word)sum < duskbright){  //check the brightness only if it's been almost a whole day
        if(lastdusk==lastdawn) lastdawn=millis()-hours(19); //prevent immediate close/open cycle at startup
        lastdusk=millis();  //record the time, because this is today's dusk
        daytime = false;    //record that it is now night-time, to prevent this code from running for a while
        closeblinds();      //run the blinds closed
      }
    }
  }
  else{ //do the following if we're checking for it to get light out. same as above with different variables
    if(previous[idx] > dawnbright-warnbright) digitalWrite(13, HIGH);
    else digitalWrite(13, LOW);
    unsigned long int timesincedawn = millis()-lastdawn;
    if(timesincedawn > hours(20)){
      if((word)sum > dawnbright){
        if(lastdusk==lastdawn) lastdusk=millis()-hours(19);
        lastdawn=millis();
        daytime = true;
        openblinds();
      }
    }
  }
  
  idx++;                        //increase the buffer index because this sample round is over
  if(idx >= avgsize) idx=0;     //(circular buffer)
  
}

void processSwitch(){   //this is an edge trigger for the switch
  if(switchhasbeenon){  //so it only does something at the moment when you flip it
    if(!swON){
      closeblinds();
      if(swON){                          //if the switch is on now, it has been immediately turned back on
        eye.startSingleSample(BH_Singh); //which indicates that you're trying to train the closing time
        lastdusk = millis();             //so act as though this was a real dusk event
        daytime = false;                 //but also adjust the light trigger level
        while(!eye.sampleIsFresh());
        duskbright = (eye.getLightLevel()+duskbright)/2;
        EEPROM.write(addrDusk, duskbright>>8);
        EEPROM.write(addrDusk+1, byte(duskbright));
      }
      else switchhasbeenon=false;
    }
  }
  else{
    if(swON){
      openblinds();
      if(!swON){                         //training case (see above)
        eye.startSingleSample(BH_Singh); // start a new sample
        lastdawn = millis();             // record time
        daytime = true;                  // set daytime
        while(!eye.sampleIsFresh());     // wait for the light sample to be ready
        dawnbright = (eye.getLightLevel()+dawnbright)/2; //average the new trigger level with the old one
        EEPROM.write(addrDawn, byte(dawnbright>>8));
        EEPROM.write(addrDawn+1, byte(dawnbright));
      }
      else switchhasbeenon=true;
    }
  }
}
