#define PWM_MIN_DUTY      80
#define PWM_START_DUTY    200
 
#include <stdint.h>
 
void AH_BL();
void AH_CL();
void BH_CL();
void BH_AL();
void CH_AL();
void CH_BL();
void bldc_move();
 
uint8_t bldc_step = 0;
uint16_t motor_speed, i, j;
 
void Interrupt()
{
  // BEMF debounce
  int8_t j;
  for(j = 0; j < 10; j++) {
    if(bldc_step & 1) {
      if(!C1OUT_bit)    j -= 1;
    }
    else {
      if(C1OUT_bit)     j -= 1;
    }
  }
  bldc_move();
  C1ON_bit = 1;      // clear the mismatch condition
  C1IF_bit = 0;      // Clear comparator 1 interrupt flag bit
}
 
void bldc_move()        // BLDC motor commutation function
{
  switch(bldc_step){
    case 0:
      AH_BL();
      CM1CON0 = 0xA2;   // Sense BEMF C (pin RA3 positive, RB3 negative)
      break;
    case 1:
      AH_CL();
      CM1CON0 = 0xA1;   // Sense BEMF B (pin RA3 positive, RA1 negative)
      break;
    case 2:
      BH_CL();
      CM1CON0 = 0xA0;   // Sense BEMF A (pin RA3 positive, RA0 negative)
      break;
    case 3:
      BH_AL();
      CM1CON0 = 0xA2;   // Sense BEMF C (pin RA3 positive, RB3 negative)
      break;
    case 4:
      CH_AL();
      CM1CON0 = 0xA1;   // Sense BEMF B (pin RA3 positive, RA1 negative)
      break;
    case 5:
      CH_BL();
      CM1CON0 = 0xA0;   // Sense BEMF A (pin RA3 positive, RA0 negative)
      break;
  }
  bldc_step++;
  if(bldc_step >= 6)
    bldc_step = 0;
}
 
// set PWM1 duty cycle function
void set_pwm_duty(uint16_t pwm_duty)
{
  CCP1CON = ((pwm_duty << 4) & 0x30) | 0x0C;
  CCPR1L  = pwm_duty >> 2;
}
 
// main function
void main()
{
 
  ANSEL = 0x10;     // configure AN4 (RA5) pin as analog
  PORTD = 0;
  TRISD = 0;
  // ADC module configuration
  ADCON0 = 0xD0;    // select analog channel 4 (AN4)
  ADFM_bit = 0;
 
  INTCON = 0xC0;   // enable global and peripheral interrupts
  C1IF_bit = 0;    // clear analog coparator interrupt flag bit
 
  // PWM
  CCP1CON = 0x0C;  // configure CCP1 module as PWM with single output & clear duty cycle 2 LSBs
  CCPR1L  = 0;     // clear duty cycle 8 MSBs
  // Timer2 module configuration for PWM frequency of 19.53kHz & 10-bit resolution
  TMR2IF_bit = 0;  // clear Timer2 interrupt flag bit
  T2CON = 0x04;    // enable Timer2 module with presacler = 1
  PR2   = 0xFF;    // Timer2 preload value = 255
 
  // Motor start
  set_pwm_duty(PWM_START_DUTY);                 // Set PWM duty cycle
  i = 5000;
  while(i > 100)
  {
    j = i;
    while(j--) ;
    bldc_move();
    i = i - 50;
  }
  
  ADON_bit = 1;
  C1IE_bit = 1;   // enable analog coparator interrupt
 
  while(1)
  {
    GO_DONE_bit = 1;  // start analog-to-digital conversion
    delay_ms(50);     // wait 50 ms
    
    motor_speed = (ADRESH << 2) | (ADRESL >> 6); // read ADC registers
    
    if(motor_speed < PWM_MIN_DUTY)
      motor_speed = PWM_MIN_DUTY;
    set_pwm_duty(motor_speed);             // set PWM duty cycle
    
  }
  
}
 
void AH_BL()
{
  CCP1CON = 0;        // PWM off
  PORTD   = 0x08;
  PSTRCON = 0x08;     // PWM output on pin P1D (RD7), others OFF
  CCP1CON = 0x0C;     // PWM on
}
 
void AH_CL()
{
  PORTD = 0x04;
}
 
void BH_CL()
{
  CCP1CON = 0;        // PWM off
  PORTD   = 0x04;
  PSTRCON = 0x04;     // PWM output on pin P1C (RD6), others OFF
  CCP1CON = 0x0C;     // PWM on
}
 
void BH_AL()
{
  PORTD = 0x10;
}
 
void CH_AL()
{
  CCP1CON = 0;        // PWM off
  PORTD   = 0x10;
  PSTRCON = 0x02;     // PWM output on pin P1B (RD5), others OFF
  CCP1CON = 0x0C;     // PWM on
}
 
void CH_BL()
{
  PORTD = 0x08;
}