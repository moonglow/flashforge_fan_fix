#include "inc/stm32f407xx.h"

#define _PIN_HI( PORT, PIN )        GPIO##PORT->BSRR = (1u<<PIN)
#define _PIN_LOW( PORT, PIN )       GPIO##PORT->BSRR = (0x10000u<<PIN)
#define _PIN_TOGGLE( PORT, PIN )do{\
uint32_t odr = GPIO##PORT->ODR;\
GPIO##PORT->BSRR = ((odr & (1u<<PIN)) << 16u) | (~odr & (1u<<PIN));\
}while(0)

#define PIN_HI( CONFIG ) _PIN_HI( CONFIG )
#define PIN_LOW( CONFIG ) _PIN_LOW( CONFIG )
#define PIN_TOGGLE( CONFIG ) _PIN_TOGGLE( CONFIG )

#define TIM_DEV     TIM9
#define TIM_BUS     RCC->APB2ENR
#define TIM_CLOCK   RCC_APB2ENR_TIM9EN
#define TIM_IRQN    TIM1_BRK_TIM9_IRQn
#define TIM_PERIOD  0xFFFFu

#define FAN_1 F, 5
#define FAN_2 E, 1 /* printer case fan */

void __attribute__((section (".patch_location"))) TIM1_BRK_TIM9_IRQHandler( void )
{
  uint32_t sr = TIM_DEV->SR;
  
  if( sr & TIM_SR_CC1IF )
  {
    PIN_LOW( FAN_1 );
    TIM_DEV->SR &= ~TIM_SR_CC1IF;
  }
  if( sr & TIM_SR_UIF )
  {
		PIN_HI( FAN_1 );
    TIM_DEV->SR &= ~TIM_SR_UIF;
  }
}

void __attribute__((section (".patch_location"))) hw_config_timer( void )
{
  /* TIM9 */
  TIM_BUS |= TIM_CLOCK;
  (void)TIM_BUS;

  TIM_DEV->PSC = (25-1); /* 168000000/25/65536 = ~102Hz */
  TIM_DEV->CR1 &= (uint16_t)(~TIM_CR1_CKD);
  /* no clock div */
  TIM_DEV->CR1 &= ~TIM_CR1_CKD;
  TIM_DEV->EGR |= TIM_EGR_UG;

  /* set low priority */
  NVIC_SetPriority( TIM_IRQN, (1UL << __NVIC_PRIO_BITS) - 1UL ); 
  NVIC_EnableIRQ( TIM_IRQN );
}

#define FAN_PWM_TO_PERC( _pwm ) ((_pwm*1000)/TIM_PERIOD)
#define FAN_MAX (900) /* 90% */
#define FAN_MIN (150) /* 15%  */

void __attribute__((section (".patch_location"))) timer_pwm_set( uint32_t value )
{
  if( !( TIM_BUS & TIM_CLOCK ) )
  {
    hw_config_timer();
  }

  if( ( FAN_PWM_TO_PERC( value ) >= FAN_MAX ) || ( FAN_PWM_TO_PERC( value ) <= FAN_MIN ) )
  {
    TIM_DEV->DIER &= ~( TIM_DIER_UIE | TIM_DIER_CC1IE );
    TIM_DEV->CR1 &= ~TIM_CR1_CEN;
    if( FAN_PWM_TO_PERC( value ) <= FAN_MIN )
      PIN_LOW( FAN_1 );
    else
      PIN_HI( FAN_1 );
  }
  else
  {
    TIM_DEV->CCR1 = value;
    if( !(TIM_DEV->CR1 & TIM_CR1_CEN ) )
    {
      TIM_DEV->DIER |= ( TIM_DIER_UIE | TIM_DIER_CC1IE );
      TIM_DEV->CR1 |= TIM_CR1_CEN;
    }
  }
}

int __attribute__((section (".patch_location"))) __atoi( char *s )
{
  int r = 0;
  while( (*s >= '0') && (*s <= '9') )
  {
    r = r*10 + (*s++ - '0');
  }
  return r;
}

int __attribute__((section (".patch_location")))  gcode_int_param( char t )
{
  /* firmware specific offsets */
  uint16_t *p_index_value = (void*)PRINTER_DOFFSET;
  char *p_data_buffer = (void*)PRINTER_DBUFFER; /* command buffer */

  /* get Gcode buffer pointer */
  char *s = &p_data_buffer[0x60*(*p_index_value)];

  while( *s )
  {
    if( *s++ == t )
      return __atoi( s );
  }
  return -1;
}

/* set fan speed */
void __attribute__((section (".patch_location"))) hook_m106( void *port, uint32_t pin )
{
  int speed = gcode_int_param( 'S' );

  /* if no param was found or param too hight */
  if( ( speed < 0 ) || ( speed > 255 ) )
    speed = 255;

  speed *= 257; /* from 0...255 => 0...65535 */
  timer_pwm_set( speed );
  return;
}

/* fan off */
void __attribute__((section (".patch_location"))) hook_m107( void *port, uint32_t pin )
{
  timer_pwm_set( 0 );
  return;
}

/* ............. for test purpose only ............. */
void SystemInit( void )
{
  SCB->VTOR = 0x8010000;//FLASH_BASE;
}

int main( void )
{
  for(;;);
}


