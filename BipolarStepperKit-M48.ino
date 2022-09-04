/* 
Program for the CasuallyLoaded® Bipolar Stepper Motor Driver kit, Created 2022.
Compiled for use on the Atmega48P with 16Mhz external clock and 4.3v BOD. Each kit has 
this code preprogrammed into the included IC so its not necessary to do the upload yourself. 
For those looking to modify the code it has been annotated. 
*/

//create and define variables
#define DELAY_CYCLES(n) __builtin_avr_delay_cycles(n) //delay (1 = 62.5nS @16MHZ)
unsigned long RnMillis = 0;
unsigned long motionCheckMillis = 0;
unsigned long lastStpToggle = 0;
volatile int stepCount = 0;
volatile int spinDirection = 0;
volatile int stepSize = 0;
volatile int motorStepping = 0;
int MotorInMotion = 0;
int StepLEDstate = 0;
int LEDsActive = 0;
int stepSizeIndicatorSet = 0;
int spinIndicatorSet = 0;

void setup() {

  DDRB = B00111110; //sets the MOSFET, BJT and LED pins to be outputs
  DDRD = B11111111; //sets the MOSFET, BJT and LED pins to be outputs
  PORTB = B00000000; //sets all the outputs to be off
  PORTD = B00000000;

  PCICR = B00000011; //enables pin change interrupts
  PCMSK0 = B00000001; //sets the individual port pins to trigger the interrupts
  PCMSK1 = B00000110;

  PORTB |= B00100000; //start up led blink sequence
  delay(100);
  PORTB &= ~B00100000; //NOTE: |= B1 will set the bit to a 1 and &= ~B1 will set the bit to a 0 without impacting any other bits
  delay(50);
  PORTB |= B00011110;
  delay(50);
  PORTB &= ~B00011110;
  delay(50);
  PORTB |= B00111110;
  delay(50);
  PORTB &= ~B00111110;
  
  sei(); //makes sure all interrupts are enabled

}

void loop() { //since most of the step setting is done using interrupts the loop mainly keeps track of the indicator LEDs and if the motor is still spinning

  RnMillis = millis(); //saving millis to test if the driver is still receiving step inputs and to time the motion indicators blink sequence
  
  if ((motorStepping == 1) && (RnMillis - motionCheckMillis > 50)) { //if motor is spinning and its time for a motion check
    motorStepping = 0; //set a test variable that should be back to 1 by the next timeout
    MotorInMotion = 1; //confirms the motor is spinning
    motionCheckMillis = RnMillis; //records the most recent timeout 
  }
  else if ((motorStepping == 0) && (MotorInMotion == 1) && (RnMillis - motionCheckMillis > 50)) { //if the test variable has not been updated since the last timeout but the program still thinks the motor is spinning
    motorStepping = 0; //record that the motor is not stepping
    MotorInMotion = 0; //record that the motor is not in motion
  }

  if ((MotorInMotion == 1) && (LEDsActive == 0)) { //this controls if the indicator LEDs should be on or off
    LEDsActive = 1; //if the motor is spinning turn the LEDs on
  }
  else if ((MotorInMotion == 0) && (LEDsActive == 1)) { //if the motor is not spinning but the LEDs are still on
    PORTB &= ~B00111110; //turn the LEDs off
    StepLEDstate = 0; //update the respective variables
    spinIndicatorSet = 0;
    stepSizeIndicatorSet = 0;
    LEDsActive = 0; 
  }

  if ((LEDsActive == 1) && (StepLEDstate == 0) && (RnMillis - lastStpToggle > 100)) { //if the motion indicator is off and needs to blink on
    PORTB |= B00100000; //turn on the LED
    StepLEDstate = 1; //record the state
    lastStpToggle = RnMillis; //record the last blink time
  }
  else if ((LEDsActive == 1) && (StepLEDstate == 1) && (RnMillis - lastStpToggle > 100)) { //if the motion indicator is on and needs to blink off
    PORTB &= ~B00100000; //turn off the LED
    StepLEDstate = 0; //record the state
    lastStpToggle = RnMillis; //record the last blink time
  }

  if ((LEDsActive == 1) && (stepSize == 0) && (stepSizeIndicatorSet == 0)) { //sets the step size LEDs state based on the interrupt variables
    PORTB &= ~B00011000; //sets the LEDs
    PORTB |=  B00010000;
    stepSizeIndicatorSet = 1; //records that the LEDs are set so the function only runs once
  }
  if ((LEDsActive == 1) && (stepSize == 1) && (stepSizeIndicatorSet == 0)) { //sets the step size LEDs state based on the interrupt variables
    PORTB &= ~B00011000; //sets the LEDs
    PORTB |=  B00001000;
    stepSizeIndicatorSet = 1; //records that the LEDs are set so the function only runs once
  }

  if ((LEDsActive == 1) && (spinDirection == 0) && (spinIndicatorSet == 0)) { //sets the rotation direction LEDs state based on the interrupt variables
    PORTB &= ~B00000110; //sets the LEDs
    PORTB |=  B00000100;
    spinIndicatorSet = 1; //records that the LEDs are set so the function only runs once
  }
  if ((LEDsActive == 1) && (spinDirection == 1) && (spinIndicatorSet == 0)) { //sets the rotation direction LEDs state based on the interrupt variables
    PORTB &= ~B00000110; //sets the LEDs
    PORTB |=  B00000010;
    spinIndicatorSet = 1; //records that the LEDs are set so the function only runs once
  }
  
}

ISR (PCINT0_vect) { //step input interrupt vector

  if ((PINB & B00000001)){ //if the step input pin is in a high state
    stepCount++; //increase the step count
    if (stepCount > 8) { stepCount = 1; } //if the count goes over the max of 8 set it back to 1
    takeStep(stepCount, stepSize, spinDirection); //take a step with its size and the spin direction included
    motorStepping = 1; //record that the motor is stepping
    DELAY_CYCLES(1); //small delay
  }
  
}


ISR (PCINT1_vect) { //reverse and step-size interrupt vector
  
  if ((PINC & B00000010)){ //step size set to half steps
    stepSize = 1;
    stepSizeIndicatorSet = 0;
  }
  else if (!(PINC & B00000010)){ //step size set to full steps
    stepSize = 0;
    stepSizeIndicatorSet = 0;
  }

  if ((PINC & B00000100)) { //spin direction set in reverse
    spinDirection = 1;
    spinIndicatorSet = 0;
  }
  else if (!(PINC & B00000100)) { //spin direction set in forwards
    spinDirection = 0;
    spinIndicatorSet = 0;
  }
  
}

void takeStep (int stepNum, int halfStepsOn, int RotationDir) { //single function for setting all steps in all modes

  if (RotationDir == 0) { //full steps going forwards 
    if (halfStepsOn == 0) { 
      switch (stepNum){ //selects the correct step, there are only 4 full steps but for simplicity this section repeats once after step 4 to cover the whole 8 steps

        case 1:
        PORTD = B10011001; //sets the correct pins to control both full bridges
        break;

        case 2:
        PORTD = B10010110;
        break;

        case 3:
        PORTD = B01100110;
        break;
    
        case 4:
        PORTD = B01101001;
        break;

        case 5:
        PORTD = B10011001;
        break;

        case 6:
        PORTD = B10010110;
        break;

        case 7:
        PORTD = B01100110;
        break;
    
        case 8:
        PORTD = B01101001;
        break;
      
      }
    }
    else if (halfStepsOn == 1) { //half steps going forwards
      switch (stepNum){ //this sets all 8 steps
      
        case 1:
        PORTD = B00001001; //sets the correct pins to control both full bridges
        break;

        case 2:
        PORTD = B10011001;
        break;

        case 3:
        PORTD = B10010000;
        break;

        case 4:
        PORTD = B10010110;
        break;

        case 5:
        PORTD = B00000110;
        break;

        case 6:
        PORTD = B01100110;
        break;
    
        case 7:
        PORTD = B01100000;
        break;
    
        case 8:
        PORTD = B01101001;
        break;
      
      }
    }
  }
  else if (RotationDir == 1) { //full steps going in reverse
    if (halfStepsOn == 0) { 
      switch (stepNum){ //selects the correct step, there are only 4 full steps but for simplicity this section repeats once after step 4 to cover the whole 8 steps

        case 1:
        PORTD = B01101001; //sets the correct pins to control both full bridges
        break;

        case 2:
        PORTD = B01100110; 
        break;

        case 3:
        PORTD = B10010110; 
        break;

        case 4:
        PORTD = B10011001; 
        break;

        case 5:
        PORTD = B01101001; 
        break;

        case 6:
        PORTD = B01100110; 
        break;

        case 7:
        PORTD = B10010110; 
        break;

        case 8:
        PORTD = B10011001; 
        break;
    
      }
    }
    else if (halfStepsOn == 1) { //half steps going in reverse 
      switch (stepNum){ //this sets all 8 steps
    
        case 1:
        PORTD = B01101001; //sets the correct pins to control both full bridges
        break;

        case 2:
        PORTD = B01100000; 
        break;

        case 3:
        PORTD = B01100110; 
        break;

        case 4:
        PORTD = B00000110; 
        break;

        case 5:
        PORTD = B10010110; 
        break;

        case 6:
        PORTD = B10010000; 
        break;
    
        case 7:
        PORTD = B10011001; 
        break;

        case 8:
        PORTD = B00001001; 
        break;
    
      }
    } 
  }
}
