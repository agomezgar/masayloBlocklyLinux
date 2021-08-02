//-- Otto Humanoid V10

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include <pins_arduino.h>
#endif
#include "Otto9Humanoidapp.h"

///////////////////////////////////////////////////////
void Otto9Humanoidapp::initHUMANOID(int YL, int YR, int RL, int RR,int LA, int RA, bool load_calibration, int NoiseSensor, int Buzzer, int USTrigger, int USEcho) {
  isHUMANOID = true;
  servo_pins[0] = YL;
  servo_pins[1] = YR;
  servo_pins[2] = RL;
  servo_pins[3] = RR;
  servo_pins[4] = LA;
  servo_pins[5] = RA;

  attachServos();
  isOttoResting=false;

  if (load_calibration) {
    for (int i = 0; i < 6; i++) {
      int servo_trim = EEPROM.read(i);
      if (servo_trim > 128) servo_trim -= 256;
      servo[i].SetTrim(servo_trim);
    }
  }
  
  for (int i = 0; i < 6; i++) servo_position[i] = 90;

  us.init(USTrigger, USEcho);  //US sensor init with the pins:
  pinBuzzer = Buzzer; //Buzzer & noise sensor pins: 
  pinNoiseSensor = NoiseSensor;

  pinMode(Buzzer,OUTPUT);
  pinMode(NoiseSensor,INPUT);
}

//////////////////////////////////////////////////////////
void Otto9Humanoidapp::initMATRIX(int DIN, int CS, int CLK, int rotate){
ledmatrix.init( DIN, CS, CLK, 1, rotate);   // set up Matrix display
}
void Otto9Humanoidapp::matrixIntensity(int intensity){
ledmatrix.setIntensity(intensity);
}

///////////////////////////////////////////////////////////////////
//-- ATTACH & DETACH FUNCTIONS ----------------------------------//
///////////////////////////////////////////////////////////////////
void Otto9Humanoidapp::attachServos(){

    servo[0].attach(servo_pins[0]);
    servo[1].attach(servo_pins[1]);
    servo[2].attach(servo_pins[2]);
    servo[3].attach(servo_pins[3]);
    servo[4].attach(servo_pins[4]);
    servo[5].attach(servo_pins[5]);

}

void Otto9Humanoidapp::detachServos(){

    servo[0].detach();
    servo[1].detach();
    servo[2].detach();
    servo[3].detach();
    servo[4].detach();
    servo[5].detach();
}

///////////////////////////////////////////////////////////////////
//-- OSCILLATORS TRIMS ------------------------------------------//
///////////////////////////////////////////////////////////////////
void Otto9Humanoidapp::setTrims(int YL, int YR, int RL, int RR, int LA, int RA) {
  servo[0].SetTrim(YL);
  servo[1].SetTrim(YR);
  servo[2].SetTrim(RL);
  servo[3].SetTrim(RR);
  servo[4].SetTrim(LA);
  servo[5].SetTrim(RA);

}

void Otto9Humanoidapp::saveTrimsOnEEPROM() {
  
  for (int i = 0; i < 6; i++){ 
      EEPROM.write(i, servo[i].getTrim());
  } 
      
}

///////////////////////////////////////////////////////////////////
//-- BASIC MOTION FUNCTIONS -------------------------------------//
///////////////////////////////////////////////////////////////////
void Otto9Humanoidapp::_moveServos(int time, int  servo_target[]) {

 attachServos();
  if(getRestState()==true){
        setRestState(false);
  }

  if(time>10){
    for (int i = 0; i < 6; i++) increment[i] = ((servo_target[i]) - servo_position[i]) / (time / 10.0);
    final_time =  millis() + time;

    for (int iteration = 1; millis() < final_time; iteration++) {
      partial_time = millis() + 10;
      for (int i = 0; i < 6; i++) servo[i].SetPosition(servo_position[i] + (iteration * increment[i]));
      while (millis() < partial_time); //pause
    }
  }
  else{
    for (int i = 0; i < 6; i++) servo[i].SetPosition(servo_target[i]);
  }
  for (int i = 0; i < 6; i++) servo_position[i] = servo_target[i];
}


void Otto9Humanoidapp::oscillateServos(int A[6], int O[6], int T, double phase_diff[6], float cycle=1){

  for (int i=0; i<6; i++) {
	servo[i].SetO(O[i]);
    servo[i].SetA(A[i]);
    servo[i].SetT(T);
    //servo[i].SetPh(phase_diff[i]);
	servo[i].SetPh(phase_diff[i]);
  }
  double ref=millis();
   for (double x=ref; x<=T*cycle+ref; x=millis()){
     for (int i=0; i<6; i++){
        servo[i].refresh();
     }
  }
}

void Otto9Humanoidapp::_execute(int A[6], int O[6], int T, double phase_diff[6], float steps = 1.0){

  attachServos();
  if(getRestState()==true){
        setRestState(false);
  }

  int cycles=(int)steps;    

  //-- Execute complete cycles
  if (cycles >= 1) 
    for(int i = 0; i < cycles; i++) 
      oscillateServos(A,O, T, phase_diff);
      
  //-- Execute the final not complete cycle    
  oscillateServos(A,O, T, phase_diff,(float)steps-cycles);
}

///////////////////////////////////////////////////////////////////
//-- HOME = Otto at rest position -------------------------------//
///////////////////////////////////////////////////////////////////
void Otto9Humanoidapp::home(){

  if(isOttoResting==false){ //Go to rest position only if necessary

    int homes[6]={90, 90, 90, 90, 90, 90}; //All the servos at rest position
    _moveServos(500,homes);   //Move the servos in half a second

    detachServos();
    isOttoResting=true;
  }
}

bool Otto9Humanoidapp::getRestState(){

    return isOttoResting;
}

void Otto9Humanoidapp::setRestState(bool state){

    isOttoResting = state;
}

///////////////////////////////////////////////////////////////////
//-- PREDETERMINED MOTION SEQUENCES -----------------------------//
///////////////////////////////////////////////////////////////////
//-- Otto movement: Jump
//--  Parameters:
//--    steps: Number of steps
//--    T: Period
//---------------------------------------------------------
void Otto9Humanoidapp::jump(float steps, int T){

    set_A[0] = 90;
	set_A[1] = 90;
	set_A[2] = 160;
	set_A[3] = 20;
	set_A[4] = 160;
	set_A[5] = 20;
	
  _moveServos(T,set_A);
  
    set_A[0] = 90;
	set_A[1] = 90;
	set_A[2] = 90;
	set_A[3] = 90;
	set_A[4] = 90;
	set_A[5] = 90;
	
  _moveServos(T,set_A);
}

//---------------------------------------------------------
//-- Otto gait: Walking  (forward or backward)    
//--  Parameters:
//--    * steps:  Number of steps
//--    * T : Period
//--    * Dir: Direction: FORWARD / BACKWARD
//---------------------------------------------------------
void Otto9Humanoidapp::walk(float steps, int T, int dir){

  //-- Oscillator parameters for walking
  //-- Hip sevos are in phase
  //-- Feet servos are in phase
  //-- Hip and feet are 90 degrees out of phase
  //--      -90 : Walk forward
  //--       90 : Walk backward
  //-- Feet servos also have the same offset (for tiptoe a little bit)
  
  	set_A[0] = 30;
	set_A[1] = 30;
	set_A[2] = 20;
	set_A[3] = 20;
	set_A[4] = 20;
	set_A[5] = 20;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = 4;
	set_O[3] = -4;
	set_O[4] = 0;
	set_O[5] = 0;
	
    set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = DEG2RAD(dir * -90);
	set_phase_diff[3] = DEG2RAD(dir * -90);
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;

  _execute(set_A, set_O, T, set_phase_diff, steps);  
}

//---------------------------------------------------------
//-- Otto gait: Turning (left or right)
//--  Parameters:
//--   * Steps: Number of steps
//--   * T: Period
//--   * Dir: Direction: LEFT / RIGHT
//---------------------------------------------------------
void Otto9Humanoidapp::turn(float steps, int T, int dir){

  //-- Same coordination than for walking (see Otto::walk)
  //-- The Amplitudes of the hip's oscillators are not igual
  //-- When the right hip servo amplitude is higher, the steps taken by
  //--   the right leg are bigger than the left. So, the robot describes an 
  //--   left arc

   set_A[0] = 30;
   set_A[1] = 30;
   set_A[2] = 20;
   set_A[3] = 20;
   set_A[4] = 15;
   set_A[5] = 15;
   
   	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = 4;
	set_O[3] = -4;
	set_O[4] = 0;
	set_O[5] = 0;

	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = DEG2RAD(-90);
	set_phase_diff[3] = DEG2RAD(-90);
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;

    
  if (dir == LEFT) {  
	  set_A[0] = 30; //-- Left hip servo
	  set_A[1] = 10; //-- Right hip servo
  }
  else {
	  set_A[0] = 10;
	  set_A[1] = 30;
  }
    
  _execute(set_A, set_O, T, set_phase_diff, steps);
}

//---------------------------------------------------------
//-- Otto gait: Lateral bend
//--  Parameters:
//--    steps: Number of bends
//--    T: Period of one bend
//--    dir: RIGHT=Right bend LEFT=Left bend
//---------------------------------------------------------
void Otto9Humanoidapp::bend (int steps, int T, int dir){

  	//Time of the bend movement. Fixed parameter to avoid falls
	int T2 = 800;

	//Bend movement
	for (int i = 0; i<steps; i++)
	{
		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = (dir == -1) ? 180-35 : 62;
		set_A[3] = (dir == -1) ? 180-60 : 35;
		set_A[4] = 20;
		set_A[5] = 60;

		_moveServos(T2 / 2, set_A);

		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = (dir == -1) ? 180 - 105 : 62;
		set_A[3] = (dir == -1) ? 180 - 60 : 105;
		set_A[4] = 60;
		set_A[5] = 20;

		_moveServos(T2 / 2, set_A);
		delay(T*0.8);

		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = 90;
		set_A[3] = 90;
		set_A[4] = 90;
		set_A[5] = 90;

		_moveServos(500, set_A);
	}
}

//---------------------------------------------------------
//-- Otto gait: Shake a leg
//--  Parameters:
//--    steps: Number of shakes
//--    T: Period of one shake
//--    dir: RIGHT=Right leg LEFT=Left leg
//---------------------------------------------------------
void Otto9Humanoidapp::shakeLeg (int steps,int T,int dir){

  //This variable change the amount of shakes
	const int numberLegMoves = 2;

	//Time of the bend movement. Fixed parameter to avoid falls
	const int T2 = 1000;
	//Time of one shake, constrained in order to avoid movements too fast.            
	T = T - T2;
	T = max(T, 200 * numberLegMoves);

	for (int j = 0; j<steps; j++)
	{
		//Bend movement
		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = (dir == -1) ? 180 - 35 : 58;
		set_A[3] = (dir == -1) ? 180 - 58 : 35;
		set_A[4] = 90;
		set_A[5] = 90;

		_moveServos(T2 / 2, set_A);
		
		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = (dir == -1) ? 180 - 120 : 58;
		set_A[3] = (dir == -1) ? 180 - 58 : 120;
		set_A[4] = 100;
		set_A[5] = 80;

		_moveServos(T2 / 2, set_A);
		
		//Shake movement
		for (int i = 0; i<numberLegMoves; i++)
		{
			set_A[0] = 90;
			set_A[1] = 90;
			set_A[2] = (dir == -1) ? 180 - 60 : 58;
			set_A[3] = (dir == -1) ? 180 - 58 : 60;
			set_A[4] = 80;
			set_A[5] =100;
			
			_moveServos(T / (2 * numberLegMoves), set_A);
			
			set_A[0] = 90;
			set_A[1] = 90;
			set_A[2] = (dir == -1) ? 180 - 120 : 58;
			set_A[3] = (dir == -1) ? 180 - 58 : 120;
			set_A[4] = 100;
			set_A[5] = 80;
			_moveServos(T / (2 * numberLegMoves), set_A);
		}
		
		set_A[0] = 90;
		set_A[1] = 90;
		set_A[2] = 90;
		set_A[3] = 90;
		set_A[4] = 90;
		set_A[5] = 90;
		_moveServos(500, set_A); //Return to home position
		
	}
	delay(T);
}

//---------------------------------------------------------
//-- Otto movement: up & down
//--  Parameters:
//--    * steps: Number of jumps
//--    * T: Period
//--    * h: Jump height: SMALL / MEDIUM / BIG 
//--              (or a number in degrees 0 - 90)
//---------------------------------------------------------
void Otto9Humanoidapp::updown(float steps, int T, int h){

  //-- Both feet are 180 degrees out of phase
  //-- Feet amplitude and offset are the same
  //-- Initial phase for the right foot is -90, so that it starts
  //--   in one extreme position (not in the middle)

  	set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = h;
	set_A[5] = h;
		
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = h;
	set_O[3] = -h;
	set_O[4] = h;
	set_O[5] = -h;
	
	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = DEG2RAD(-90);
	set_phase_diff[3] = DEG2RAD(90);
	set_phase_diff[4] = DEG2RAD(-90);
	set_phase_diff[5] = DEG2RAD(90);

  _execute(set_A, set_O, T, set_phase_diff, steps); 
}
//---------------------------------------------------------
//-- Otto movement: Hands up
//---------------------------------------------------------

void Otto9Humanoidapp::armsup(){
  
   	set_A[0] = 90;
	set_A[1] = 90;
	set_A[2] = 90;
	set_A[3] = 90;
	set_A[4] = 20;
	set_A[5] = 160;
    _moveServos(750,set_A);   //Move the servos

}

void Otto9Humanoidapp::armsdown(){
  
   	set_A[0] = 90;
	set_A[1] = 90;
	set_A[2] = 90;
	set_A[3] = 90;
	set_A[4] = 160;
	set_A[5] = -20;
    _moveServos(750,set_A);   //Move the servos in half a second

}

///////////////////////////////////////////////////
void Otto9Humanoidapp::armwave(int dir){
//-- Wave , either left or right

 if(dir==-1)      
  {
    set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = 0;
	set_A[3] = 0;
	set_A[4] = 30;
	set_A[5] = 0;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = 0;
	set_O[3] = 0;
	set_O[4] = -30;
	set_O[5] = -40;
  
  	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = 0;
	set_phase_diff[3] = 0;
	set_phase_diff[4] = DEG2RAD(0);
	set_phase_diff[5] = 0;

  _execute(set_A, set_O, 500, set_phase_diff, 5); 
  }
  if(dir==1)      
  {
    set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = 0;
	set_A[3] = 0;
	set_A[4] = 0;
	set_A[5] = 30;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = 0;
	set_O[3] = 0;
	set_O[4] = 40;
	set_O[5] = 60;
	
	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = 0;
	set_phase_diff[3] = 0;
	set_phase_diff[4] = 0;
	set_phase_diff[5] = DEG2RAD(0);

  _execute(set_A, set_O, 500, set_phase_diff, 5); 
  }  
}

//---------------------------------------------------------
//-- Otto movement: swinging side to side
//--  Parameters:
//--     steps: Number of steps
//--     T : Period
//--     h : Amount of swing (from 0 to 50 aprox)
//---------------------------------------------------------
void Otto9Humanoidapp::swing(float steps, int T, int h){

  //-- Both feets are in phase. The offset is half the amplitude
  //-- It causes the robot to swing from side to side

    set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = h;
	set_A[5] = h;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] =  h/2;
	set_O[3] = -h/2;
	set_O[4] = h;
	set_O[5] = -h;
	
	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = DEG2RAD(0);
	set_phase_diff[3] = DEG2RAD(0);
	set_phase_diff[4] = DEG2RAD(0);
	set_phase_diff[5] = DEG2RAD(0);
  
  _execute(set_A, set_O, T, set_phase_diff, steps); 
}

//---------------------------------------------------------
//-- Otto movement: swinging side to side without touching the floor with the heel
//--  Parameters:
//--     steps: Number of steps
//--     T : Period
//--     h : Amount of swing (from 0 to 50 aprox)
//---------------------------------------------------------
void Otto9Humanoidapp::tiptoeSwing(float steps, int T, int h){

  //-- Both feets are in phase. The offset is not half the amplitude in order to tiptoe
  //-- It causes the robot to swing from side to side

    set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = h;
	set_A[5] = h;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = h;
	set_O[3] = -h;
	set_O[4] = h;
	set_O[5] = -h;

	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] = 0;
	set_phase_diff[3] = 0;
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;
  
  _execute(set_A, set_O, T, set_phase_diff, steps); 
}

//---------------------------------------------------------
//-- Otto gait: Jitter 
//--  Parameters:
//--    steps: Number of jitters
//--    T: Period of one jitter 
//--    h: height (Values between 5 - 25)   
//---------------------------------------------------------
void Otto9Humanoidapp::jitter(float steps, int T, int h){

  //-- Both feet are 180 degrees out of phase
  //-- Feet amplitude and offset are the same
  //-- Initial phase for the right foot is -90, so that it starts
  //--   in one extreme position (not in the middle)
  //-- h is constrained to avoid hit the feets
  h=min(25,h);
  
    set_A[0] = h;
	set_A[1] = h;
	set_A[2] = 0;
	set_A[3] = 0;
	set_A[4] = 0;
	set_A[5] = 0;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = 0;
	set_O[3] = 0;
	set_O[4] = 0;
	set_O[5] = 0;
	
	set_phase_diff[0] = DEG2RAD(-90);
	set_phase_diff[1] = DEG2RAD(90);
	set_phase_diff[2] = 0;
	set_phase_diff[3] = 0;
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;
  
  _execute(set_A, set_O, T, set_phase_diff, steps); 
}

//---------------------------------------------------------
//-- Otto gait: Ascending & turn (Jitter while up&down)
//--  Parameters:
//--    steps: Number of bends
//--    T: Period of one bend
//--    h: height (Values between 5 - 15) 
//---------------------------------------------------------
void Otto9Humanoidapp::ascendingTurn(float steps, int T, int h){

  //-- Both feet and legs are 180 degrees out of phase
  //-- Initial phase for the right foot is -90, so that it starts
  //--   in one extreme position (not in the middle)
  //-- h is constrained to avoid hit the feets
  h=min(13,h);
  
    set_A[0] = h;
	set_A[1] = h;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = 40;
	set_A[5] = 40;
	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = h+4;
	set_O[3] = -h+4;
	set_O[4] = 0;
	set_O[5] = 0;

	set_phase_diff[0] = DEG2RAD(-90);
	set_phase_diff[1] = DEG2RAD(90);
	set_phase_diff[2] =  DEG2RAD(-90);
	set_phase_diff[3] =  DEG2RAD(90);
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;
  
  _execute(set_A, set_O, T, set_phase_diff, steps); 
}

//---------------------------------------------------------
//-- Otto gait: Moonwalker. Otto moves like Michael Jackson
//--  Parameters:
//--    Steps: Number of steps
//--    T: Period
//--    h: Height. Typical valures between 15 and 40
//--    dir: Direction: LEFT / RIGHT
//---------------------------------------------------------
void Otto9Humanoidapp::moonwalker(float steps, int T, int h, int dir){

  //-- This motion is similar to that of the caterpillar robots: A travelling
  //-- wave moving from one side to another
  //-- The two Otto's feet are equivalent to a minimal configuration. It is known
  //-- that 2 servos can move like a worm if they are 120 degrees out of phase
  //-- In the example of Otto, the two feet are mirrored so that we have:
  //--    180 - 120 = 60 degrees. The actual phase difference given to the oscillators
  //--  is 60 degrees.
  //--  Both amplitudes are equal. The offset is half the amplitud plus a little bit of
  //-   offset so that the robot tiptoe lightly
 
    set_A[0] = 0;
	set_A[1] = 0;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = h;
	set_A[5] = h;

    set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] =h / 2 + 2;
	set_O[3] = -h / 2 - 2;
	set_O[4] = -h;
	set_O[5] = h;
	
	const int phi = -dir * 90;
	set_phase_diff[0] = 0;
	set_phase_diff[1] = 0;
	set_phase_diff[2] =  DEG2RAD(phi);
	set_phase_diff[3] =  DEG2RAD(-60 * dir + phi);
	set_phase_diff[4] = DEG2RAD(phi);
	set_phase_diff[5] = DEG2RAD(phi);

	_execute(set_A, set_O, T, set_phase_diff, steps);  
}

//----------------------------------------------------------
//-- Otto gait: Crusaito. A mixture between moonwalker and walk
//--   Parameters:
//--     steps: Number of steps
//--     T: Period
//--     h: height (Values between 20 - 50)
//--     dir:  Direction: LEFT / RIGHT
//-----------------------------------------------------------
void Otto9Humanoidapp::crusaito(float steps, int T, int h, int dir){

    set_A[0] = 25;
	set_A[1] = 25;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = 0;
	set_A[5] = 0;

	
	set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = h/2+ 4;
	set_O[3] = -h/2 - 4;
	set_O[4] = 0;
	set_O[5] = 0;

	set_phase_diff[0] = 90;
	set_phase_diff[1] = 90;
	set_phase_diff[2] =  DEG2RAD(0);
	set_phase_diff[3] =  DEG2RAD(-60 * dir);
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;
	
  _execute(set_A, set_O, T, set_phase_diff, steps); 
}


//---------------------------------------------------------
//-- Otto gait: Flapping
//--  Parameters:
//--    steps: Number of steps
//--    T: Period
//--    h: height (Values between 10 - 30)
//--    dir: direction: FOREWARD, BACKWARD
//---------------------------------------------------------
void Otto9Humanoidapp::flapping(float steps, int T, int h, int dir){

    set_A[0] = 12;
	set_A[1] = 12;
	set_A[2] = h;
	set_A[3] = h;
	set_A[4] = 0;
	set_A[5] = 0;

	
    set_O[0] = 0;
	set_O[1] = 0;
	set_O[2] = h - 10;
	set_O[3] = -h + 10;
	set_O[4] = 0;
	set_O[5] = 0;
	
	set_phase_diff[0] = DEG2RAD(0);
	set_phase_diff[1] = DEG2RAD(180);
	set_phase_diff[2] =  DEG2RAD(-90 * dir);
	set_phase_diff[3] =  DEG2RAD(90 * dir);
	set_phase_diff[4] = 0;
	set_phase_diff[5] = 0;

  _execute(set_A, set_O, T, set_phase_diff, steps); 
}

///////////////////////////////////////////////////////////////////
//-- SENSORS FUNCTIONS  -----------------------------------------//
///////////////////////////////////////////////////////////////////

//---------------------------------------------------------
//-- Otto getDistance: return Otto's ultrasonic sensor measure
//---------------------------------------------------------
float Otto9Humanoidapp::getDistance(){

  return us.read();
}

//---------------------------------------------------------
//-- Otto getNoise: return Otto's noise sensor measure
//---------------------------------------------------------
int Otto9Humanoidapp::getNoise(){

  int noiseLevel = 0;
  int noiseReadings = 0;
  int numReadings = 2;  

    noiseLevel = analogRead(pinNoiseSensor);

    for(int i=0; i<numReadings; i++){
        noiseReadings += analogRead(pinNoiseSensor);
        delay(4); // delay in between reads for stability
    }

    noiseLevel = noiseReadings / numReadings;

    return noiseLevel;
}

///////////////////////////////////////////////////////////////////
//-- MOUTHS & ANIMATIONS ----------------------------------------//
///////////////////////////////////////////////////////////////////
void Otto9Humanoidapp::setLed(byte X, byte Y, byte value){
  ledmatrix.setDot( X,  Y, value);
}

// EXAMPLE putAnimationMouth(dreamMouth,0);
void Otto9Humanoidapp::putAnimationMouth(unsigned long int aniMouth, int index){

ledmatrix.writeFull(PROGMEM_getAnything (&Gesturetable[aniMouth][index]));
}

//EXAMPLE putMouth(smile);
void Otto9Humanoidapp::putMouth(unsigned long int mouth, bool predefined){
  if (predefined){
// Here a direct entry into the Progmem Mouthttable is used!!

ledmatrix.writeFull(PROGMEM_getAnything(&Mouthtable[mouth]));
  }
  else{
    ledmatrix.writeFull(mouth);
  }
}

void Otto9Humanoidapp::clearMouth(){

  ledmatrix.clearMatrix();
}
// // limited characters are : CAPITALS A to Z   NUMBERS 0 to 9    'SPACE'  : ; < >  = @ 
void Otto9Humanoidapp::writeText(const char * s, byte scrollspeed){
 int a ;
 int b ;
  for(a = 0; s[a] != '\0'; a++){
    b = a +1 ;
    if (b > 9 ) b = 9; // only maximum of nine characters allowed
  }
  for(int charNUMBER = 0; charNUMBER <b; charNUMBER++){
      if ((* s < 48) || (* s > 91)) {
        if (* s == 32){
          ledmatrix.sendChar (44, charNUMBER, b, scrollspeed);
        }
        else
        {
          ledmatrix.sendChar (43, charNUMBER, b, scrollspeed);
        }
     }
      else
      {
      ledmatrix.sendChar ((* s - 48), charNUMBER, b, scrollspeed);
     }
  * s++;
  }
}

///////////////////////////////////////////////////////////////////
//-- SOUNDS -----------------------------------------------------//
///////////////////////////////////////////////////////////////////

void Otto9Humanoidapp::_tone (float noteFrequency, long noteDuration, int silentDuration){

    // tone(10,261,500);
    // delay(500);

      if(silentDuration==0){silentDuration=1;}

      tone(Otto9Humanoidapp::pinBuzzer, noteFrequency, noteDuration);
      delay(noteDuration);       //milliseconds to microseconds
      noTone(pinBuzzer);
      delay(silentDuration);
}

void Otto9Humanoidapp::bendTones (float initFrequency, float finalFrequency, float prop, long noteDuration, int silentDuration){

  //Examples:
  //  bendTones (880, 2093, 1.02, 18, 1);
  //  bendTones (note_A5, note_C7, 1.02, 18, 0);

  if(silentDuration==0){silentDuration=1;}

  if(initFrequency < finalFrequency)
  {
      for (int i=initFrequency; i<finalFrequency; i=i*prop) {
          _tone(i, noteDuration, silentDuration);
      }

  } else{

      for (int i=initFrequency; i>finalFrequency; i=i/prop) {
          _tone(i, noteDuration, silentDuration);
      }
  }
}

void Otto9Humanoidapp::sing(int songName){
  switch(songName){

    case S_connection:
      _tone(note_E5,50,30);
      _tone(note_E6,55,25);
      _tone(note_A6,60,10);
    break;

    case S_disconnection:
      _tone(note_E5,50,30);
      _tone(note_A6,55,25);
      _tone(note_E6,50,10);
    break;

    case S_buttonPushed:
      bendTones (note_E6, note_G6, 1.03, 20, 2);
      delay(30);
      bendTones (note_E6, note_D7, 1.04, 10, 2);
    break;

    case S_mode1:
      bendTones (note_E6, note_A6, 1.02, 30, 10);  //1318.51 to 1760
    break;

    case S_mode2:
      bendTones (note_G6, note_D7, 1.03, 30, 10);  //1567.98 to 2349.32
    break;

    case S_mode3:
      _tone(note_E6,50,100); //D6
      _tone(note_G6,50,80);  //E6
      _tone(note_D7,300,0);  //G6
    break;

    case S_surprise:
      bendTones(800, 2150, 1.02, 10, 1);
      bendTones(2149, 800, 1.03, 7, 1);
    break;

    case S_OhOoh:
      bendTones(880, 2000, 1.04, 8, 3); //A5 = 880
      delay(200);

      for (int i=880; i<2000; i=i*1.04) {
           _tone(note_B5,5,10);
      }
    break;

    case S_OhOoh2:
      bendTones(1880, 3000, 1.03, 8, 3);
      delay(200);

      for (int i=1880; i<3000; i=i*1.03) {
          _tone(note_C6,10,10);
      }
    break;

    case S_cuddly:
      bendTones(700, 900, 1.03, 16, 4);
      bendTones(899, 650, 1.01, 18, 7);
    break;

    case S_sleeping:
      bendTones(100, 500, 1.04, 10, 10);
      delay(500);
      bendTones(400, 100, 1.04, 10, 1);
    break;

    case S_happy:
      bendTones(1500, 2500, 1.05, 20, 8);
      bendTones(2499, 1500, 1.05, 25, 8);
    break;

    case S_superHappy:
      bendTones(2000, 6000, 1.05, 8, 3);
      delay(50);
      bendTones(5999, 2000, 1.05, 13, 2);
    break;

    case S_happy_short:
      bendTones(1500, 2000, 1.05, 15, 8);
      delay(100);
      bendTones(1900, 2500, 1.05, 10, 8);
    break;

    case S_sad:
      bendTones(880, 669, 1.02, 20, 200);
    break;

    case S_confused:
      bendTones(1000, 1700, 1.03, 8, 2); 
      bendTones(1699, 500, 1.04, 8, 3);
      bendTones(1000, 1700, 1.05, 9, 10);
    break;

    case S_fart1:
      bendTones(1600, 3000, 1.02, 2, 15);
    break;

    case S_fart2:
      bendTones(2000, 6000, 1.02, 2, 20);
    break;

    case S_fart3:
      bendTones(1600, 4000, 1.02, 2, 20);
      bendTones(4000, 3000, 1.02, 2, 20);
    break;

  }
}

///////////////////////////////////////////////////////////////////
//-- GESTURES ---------------------------------------------------//
///////////////////////////////////////////////////////////////////

void Otto9Humanoidapp::playGesture(int gesture){
 int gesturePOSITION[6];
  
  switch(gesture){

    case OttoHappy: 
        _tone(note_E5,50,30);
        putMouth(smile);
        sing(S_happy_short);
        swing(1,800,20); 
        sing(S_happy_short);

        home();
        putMouth(happyOpen);
    break;


    case OttoSuperHappy:
        putMouth(happyOpen);
        sing(S_happy);
        putMouth(happyClosed);
        tiptoeSwing(1,500,20);
        putMouth(happyOpen);
        sing(S_superHappy);
        putMouth(happyClosed);
        tiptoeSwing(1,500,20); 

        home();  
        putMouth(happyOpen);
    break;


    case OttoSad: 
        putMouth(sad);
        gesturePOSITION[0] = 110;//int sadPos[6]=      {110, 70, 20, 160, 90, 90};
        gesturePOSITION[1] = 70;
         gesturePOSITION[2] = 20;
          gesturePOSITION[3] = 160;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(700, gesturePOSITION);     
        bendTones(880, 830, 1.02, 20, 200);
        putMouth(sadClosed);
        bendTones(830, 790, 1.02, 20, 200);  
        putMouth(sadOpen);
        bendTones(790, 740, 1.02, 20, 200);
        putMouth(sadClosed);
        bendTones(740, 700, 1.02, 20, 200);
        putMouth(sadOpen);
        bendTones(700, 669, 1.02, 20, 200);
        putMouth(sad);
        delay(500);

        home();
        delay(300);
        putMouth(happyOpen);
    break;


    case OttoSleeping:
    gesturePOSITION[0] = 100;//int bedPos[6]=      {100, 80, 60, 120, 90, 90};
        gesturePOSITION[1] = 80;
         gesturePOSITION[2] = 60;
          gesturePOSITION[3] = 120;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(700, gesturePOSITION);     
        for(int i=0; i<4;i++){
          putAnimationMouth(dreamMouth,0);
          bendTones (100, 200, 1.04, 10, 10);
          putAnimationMouth(dreamMouth,1);
          bendTones (200, 300, 1.04, 10, 10);  
          putAnimationMouth(dreamMouth,2);
          bendTones (300, 500, 1.04, 10, 10);   
          delay(500);
          putAnimationMouth(dreamMouth,1);
          bendTones (400, 250, 1.04, 10, 1); 
          putAnimationMouth(dreamMouth,0);
          bendTones (250, 100, 1.04, 10, 1); 
          delay(500);
        } 

        putMouth(lineMouth);
        sing(S_cuddly);

        home();  
        putMouth(happyOpen);
    break;


    case OttoFart:
    gesturePOSITION[0] = 90;// int fartPos_1[6]=   {90, 90, 145, 122, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 145;
          gesturePOSITION[3] = 122;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(500,gesturePOSITION);
        delay(300);     
        putMouth(lineMouth);
        sing(S_fart1);  
        putMouth(tongueOut);
        delay(250);
        gesturePOSITION[0] = 90;// int fartPos_2[6]=   {90, 90, 80, 122, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 80;
          gesturePOSITION[3] = 122;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(500,gesturePOSITION);
        delay(300);
        putMouth(lineMouth);
        sing(S_fart2); 
        putMouth(tongueOut);
        delay(250);
        gesturePOSITION[0] = 90;// int fartPos_3[6]=   {90, 90, 145, 80, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 145;
          gesturePOSITION[3] = 80;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(500,gesturePOSITION);
        delay(300);
        putMouth(lineMouth);
        sing(S_fart3);
        putMouth(tongueOut);    
        delay(300);

        home(); 
        delay(500); 
        putMouth(happyOpen);
    break;


    case OttoConfused:
    gesturePOSITION[0] = 110;//int confusedPos[6]= {110, 70, 90, 90, 90, 90};
        gesturePOSITION[1] = 70;
         gesturePOSITION[2] = 90;
          gesturePOSITION[3] = 90;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300, gesturePOSITION); 
        putMouth(confused);
        sing(S_confused);
        delay(500);

        home();  
        putMouth(happyOpen);
    break;


    case OttoLove:
        putMouth(heart);
        sing(S_cuddly);
        crusaito(2,1500,15,1);

        home(); 
        sing(S_happy_short);  
        putMouth(happyOpen);
    break;


    case OttoAngry: 
    gesturePOSITION[0] = 90;//int angryPos[6]=    {90, 90, 70, 110, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 70;
          gesturePOSITION[3] = 110;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300, gesturePOSITION); 
        putMouth(angry);

        _tone(note_A5,100,30);
        bendTones(note_A5, note_D6, 1.02, 7, 4);
        bendTones(note_D6, note_G6, 1.02, 10, 1);
        bendTones(note_G6, note_A5, 1.02, 10, 1);
        delay(15);
        bendTones(note_A5, note_E5, 1.02, 20, 4);
        delay(400);
        gesturePOSITION[0] = 110;//int headLeft[6]=    {110, 110, 90, 90, 90, 90};
        gesturePOSITION[1] = 110;
         gesturePOSITION[2] = 90;
          gesturePOSITION[3] = 90;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(200, gesturePOSITION); 
        bendTones(note_A5, note_D6, 1.02, 20, 4);
        gesturePOSITION[0] = 70;//int headRight[6]=   {70, 70, 90, 90, 90, 90};
        gesturePOSITION[1] = 70;
         gesturePOSITION[2] = 90;
          gesturePOSITION[3] = 90;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(200, gesturePOSITION); 
        bendTones(note_A5, note_E5, 1.02, 20, 4);

        home();  
        putMouth(happyOpen);
    break;


    case OttoFretful: 
        putMouth(angry);
        bendTones(note_A5, note_D6, 1.02, 20, 4);
        bendTones(note_A5, note_E5, 1.02, 20, 4);
        delay(300);
        putMouth(lineMouth);

        for(int i=0; i<4; i++){
          gesturePOSITION[0] = 90;//int fretfulPos[6]=  {90, 90, 90, 110, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 90;
          gesturePOSITION[3] = 110;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
          _moveServos(100, gesturePOSITION);   
          home();
        }

        putMouth(angry);
        delay(500);

        home();  
        putMouth(happyOpen);
    break;


    case OttoMagic:

        //Initial note frecuency = 400
        //Final note frecuency = 1000
        
        // Reproduce the animation four times
        for(int i = 0; i<4; i++){ 

          int noteM = 400; 

            for(int index = 0; index<6; index++){
              putAnimationMouth(adivinawi,index);
              bendTones(noteM, noteM+100, 1.04, 10, 10);    //400 -> 1000 
              noteM+=100;
            }

            clearMouth();
            bendTones(noteM-100, noteM+100, 1.04, 10, 10);  //900 -> 1100

            for(int index = 0; index<6; index++){
              putAnimationMouth(adivinawi,index);
              bendTones(noteM, noteM+100, 1.04, 10, 10);    //1000 -> 400 
              noteM-=100;
            }
        } 
 
        delay(300);
        putMouth(happyOpen);
    break;


    case OttoWave:
        
        // Reproduce the animation four times
        for(int i = 0; i<2; i++){ 

            int noteW = 500; 

            for(int index = 0; index<10; index++){
              putAnimationMouth(wave,index);
              bendTones(noteW, noteW+100, 1.02, 10, 10); 
              noteW+=101;
            }
            for(int index = 0; index<10; index++){
              putAnimationMouth(wave,index);
              bendTones(noteW, noteW+100, 1.02, 10, 10); 
              noteW+=101;
            }
            for(int index = 0; index<10; index++){
              putAnimationMouth(wave,index);
              bendTones(noteW, noteW-100, 1.02, 10, 10); 
              noteW-=101;
            }
            for(int index = 0; index<10; index++){
              putAnimationMouth(wave,index);
              bendTones(noteW, noteW-100, 1.02, 10, 10); 
              noteW-=101;
            }
        }    

        clearMouth();
        delay(100);
        putMouth(happyOpen);
    break;

    case OttoVictory:
        
        putMouth(smallSurprise);
        //final pos   = {90,90,150,30}
        for (int i = 0; i < 60; ++i){
          int pos[]={90,90,90+i,90-i, 90, 90};  
          _moveServos(10,pos);
          _tone(1600+i*20,15,1);
        }

        putMouth(bigSurprise);
        //final pos   = {90,90,90,90}
        for (int i = 0; i < 60; ++i){
          int pos[]={90,90,150-i,30+i, 90, 90};  
          _moveServos(10,pos);
          _tone(2800+i*20,15,1);
        }

        putMouth(happyOpen);
        //SUPER HAPPY
        //-----
        tiptoeSwing(1,500,20);
        sing(S_superHappy);
        putMouth(happyClosed);
        tiptoeSwing(1,500,20); 
        //-----

        home();
        clearMouth();
        putMouth(happyOpen);

    break;

    case OttoFail:
        putMouth(sadOpen);
         gesturePOSITION[0] = 90;//int bendPos_1[6]=   {90, 90, 70, 35, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 70;
          gesturePOSITION[3] = 35;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300,gesturePOSITION);
        _tone(900,200,1);
        putMouth(sadClosed);
        gesturePOSITION[0] = 90;//int bendPos_2[6]=   {90, 90, 55, 35, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 55;
          gesturePOSITION[3] = 35;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300,gesturePOSITION);
        _tone(600,200,1);
        putMouth(confused);
        gesturePOSITION[0] = 90;//int bendPos_3[6]=   {90, 90, 42, 35, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 42;
          gesturePOSITION[3] = 35;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300,gesturePOSITION);
        _tone(300,200,1);
        gesturePOSITION[0] = 90;//int bendPos_4[6]=   {90, 90, 34, 35, 90, 90};
        gesturePOSITION[1] = 90;
         gesturePOSITION[2] = 34;
          gesturePOSITION[3] = 35;
           gesturePOSITION[4] = 90;
            gesturePOSITION[5] = 90;
        _moveServos(300,gesturePOSITION);
        putMouth(xMouth);

        detachServos();
        _tone(150,2200,1);
        
        delay(600);
        clearMouth();
        putMouth(happyOpen);
        home();

    break;

  }
}        
