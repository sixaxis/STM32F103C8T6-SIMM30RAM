#include <math.h>

#define PIN13          ((unsigned short)0x2000)
#define TX_PIN         ((unsigned short)0x1)

/* GPIOA REGISTERS */ 
#define GPIOA_CRL      (*((volatile unsigned long *)0x40010800))
#define GPIOA_CRH      (*((volatile unsigned long *)0x40010804))	
#define GPIOA_IDR      (*((volatile unsigned long *)0x40010808))
#define GPIOA_ODR      (*((volatile unsigned long *)0x4001080C))
#define GPIOA_BSRR     (*((volatile unsigned long *)0x40010810))	
#define GPIOA_BRR      (*((volatile unsigned long *)0x40010814))	

/* GPIOB REGISTERS */
#define GPIOB_CRL      (*((volatile unsigned long *)0x40010C00))
#define GPIOB_CRH      (*((volatile unsigned long *)0x40010C04))
#define GPIOB_IDR      (*((volatile unsigned long *)0x40010C08))
#define GPIOB_ODR      (*((volatile unsigned long *)0x40010C0C))
#define GPIOB_BSRR     (*((volatile unsigned long *)0x40010C10))	
#define GPIOB_BRR      (*((volatile unsigned long *)0x40010C14))	

/* GPIOC REGISTERS */
#define GPIOC_CRL      (*((volatile unsigned long *)0x40011C00))
#define GPIOC_CRH      (*((volatile unsigned long *)0x40011004))
#define GPIOC_IDR      (*((volatile unsigned long *)0x40011C08))
#define GPIOC_ODR      (*((volatile unsigned long *)0x40011C0C))
#define GPIOC_BSRR     (*((volatile unsigned long *)0x40011010))
#define GPIOC_BRR      (*((volatile unsigned long *)0x40011014))

/* TIM1 REGISTERS */
#define TIM1_CR1       (*((volatile unsigned long *)0x40012C00))
#define TIM1_DIER	   (*((volatile unsigned long *)0x40012C0C))
#define TIM1_SR        (*((volatile unsigned long *)0x40012C10))
#define TIM1_PSC       (*((volatile unsigned long *)0x40012C28))
#define TIM1_ARR 	   (*((volatile unsigned long *)0x40012C2C))

/* RCC REGISTERS */
#define RCC_APB2ENR    (*((volatile unsigned long *)0x40021018))
#define RCC_APB1ENR    (*((volatile unsigned long *)0x4002101C))

/* NVIC REGISTERS */
#define NVIC_ISER      (*((volatile unsigned long *)0xE000E100))

#define RAM_RAS         ((unsigned short)0x100)  
#define RAM_W           ((unsigned short)0x200)  
#define RAM_CAS         ((unsigned short)0x400)  
                                               
// RAM_RAS A8
// RAM_W   A9
// RAM_CAS A10

// RAM_DATA A0-A7
// RAM_ADDR B3-B13 
// PIN13    -Debug LED PC13
 

void ram_init();
void PutStr(char *str);
void send_debug(char s);
unsigned char test_ram();
int number_digits (int n);
void sendIntToPort(long n);
void _delay_ms(unsigned int del);
void _delay_us(unsigned int del);
unsigned char ram_read(unsigned int row, unsigned int col);
void ram_write(unsigned int row, unsigned int col, unsigned char val);

int main(void)
{
   ram_init();
   unsigned char data[4];
   PutStr("STM32F103C8T6 SIMM30 RAM TEST");

 while(1)
 {
		 
   ram_write(0xFE,0xFE,200);
   ram_write(0xF0,0xF0,'B');
   ram_write(0x0F,0x0F,255);

   GPIOC_BSRR = PIN13;
	_delay_ms(1000); 
	 
	 data[0] = ram_read(0x0F,0x0F);
	 data[1] = ram_read(0xFE,0xFE);
	 data[2] = ram_read(0xF0,0xF0);
	 data[3] = ram_read(0xFF,0xFF); //FF == FE
	 
	 if(data[0] == 255 && data[1] == 200 && data[2] == 'B' && data[3] == 200)
	 while(1)
	{
     GPIOC_BRR = PIN13;
     test_ram();
    }
	 	
 } 
}

unsigned char test_ram()
{

	unsigned long falses = 0;
	unsigned long oks = 0;
	
	
	for (int i=0;i<2047;i++)  
	{ 
	
	 for (int j=0;j<256;j++)
	 {
	  ram_write(i, j, j);
	  
	  if(ram_read(i, j) == j)
	   oks++;
	  else
	   falses++;	
	 }
    
	}
	  PutStr("TEST FINISH");
	  PutStr("OKS");
      sendIntToPort(oks);
	  PutStr("FAILS");
      sendIntToPort(falses);
//while(1);

_delay_ms(10000);
	return 0;
}

void TIM1_UP_IRQHandler(void)  
{
  TIM1_SR &= 0xFE; 	
	
	unsigned int cycles = 4096;

	do{
		GPIOA_BRR = RAM_CAS;	/* CAS = 0 */
		GPIOA_BRR = RAM_RAS;	/* RAS = 0 */
		GPIOA_BSRR = RAM_CAS;	/* CAS = 1 */
		GPIOA_BSRR = RAM_RAS;	/* RAS = 1 */
	}while(--cycles);
	
}


unsigned char ram_read(unsigned int row, unsigned int col) 
 {
    unsigned char buf;
	TIM1_DIER   &= 0xFFFE; //TIM1 OFF				
	GPIOB_BSRR = row<<3;   // SET ROW TO ADDRESS PINS
	GPIOA_BRR  = RAM_RAS;  // RAS = 0 
    GPIOB_ODR &= 0xF807;   // CLEAR ADDR 
	GPIOB_BSRR = col<<3;   // SET COL TO ADDRESS PINS
	GPIOA_BRR = RAM_CAS;   // CAS = 0 
	GPIOA_BSRR = RAM_CAS;  // CAS = 1 
	GPIOA_BSRR = RAM_RAS;  // RAS = 1 	
    buf = GPIOA_IDR;       // READ DATA
    GPIOB_ODR &= 0xF807;   // CLEAR ADDR 
    TIM1_DIER   |= 1;      //TIM1 ON
	return buf;
 }


void ram_write(unsigned int row, unsigned int col, unsigned char val) 
{

    TIM1_DIER   &= 0xFFFE;    // TIM1 OFF
	GPIOA_CRL  = 0x33333333;  // OUT push-pull	
	GPIOB_BSRR = row<<3;      // SET ROW TO ADDRESS PINS
	GPIOA_BRR  = RAM_RAS;	  // RAS = 0 
	GPIOA_BSRR = val;         // SET DATA 
	GPIOA_BRR = RAM_W;	      // WE = 0 
	GPIOB_ODR &= 0xF807;      // CLEAR ADDR 
	GPIOB_BSRR = col<<3;      // SET COL TO ADDRESS PINS
	GPIOA_BRR = RAM_CAS;	  // CAS = 0 
	GPIOA_BSRR = RAM_CAS;	  // CAS = 1 
	GPIOA_BSRR = RAM_W;	      // WE = 1 
	GPIOA_BSRR = RAM_RAS;	  // RAS = 1 
	GPIOA_CRL =  0x88888888;  // IN pull-down
	GPIOA_ODR &= 0xFF00;      // ENABLE pull-down
	GPIOB_ODR &= 0xF807;      // CLEAR ADDR
	TIM1_DIER   |= 1;         // TIM1 ON
}

void ram_init() {
	
	unsigned char t;
	
	RCC_APB2ENR |= 0x0000081C;   //ENABLE GPIOs Ñ,B,A AND TIMER1  
	GPIOB_CRL &= 0x00000FF0; 
	GPIOB_CRL |= 0x33333003;     // GPIOB 3-13,0 OUT Push-pull (address)  
	GPIOB_CRH &= 0xFF000000;
	GPIOB_CRH |= 0x00333333;		
 	GPIOA_CRH &= 0xFFFFF000;     // 
	GPIOA_CRH |= 0x00000333;     // GPIOA 8-10 OUT Push-pull (controll)	
	GPIOC_CRH &= 0xFF0FFFFF;     // 
	GPIOC_CRH |= 0x00300000;     // GPIOC 13 OUT Push-pull (debug)
	
	/*INTERRUPT 100 P/S */
	TIM1_PSC     = 17999;        //DEVIDER - 72000000/18000 = 4000 tics p sec
	TIM1_ARR     = 50;           //MACH REGISTER
	TIM1_CR1    |= 1;            //ENABLE TIMER
	 
	/*SETUP INTERRUPT CONTROLLER*/
	NVIC_ISER    = 0x2000000; 	  //ENABLE TIM1 INTERRUPT 
	
	GPIOA_BSRR = RAM_CAS;	      // CAS = 1 
	GPIOA_BSRR = RAM_RAS;	      // RAS = 1 
	GPIOA_BSRR = RAM_W;	          // W = 1 
		
	/* INIT SIMM RAM */
	_delay_ms(200);
	for(t = 0; t < 8; t++) TIM1_UP_IRQHandler();
	
	TIM1_DIER   |= 1;             // TIM1 ON
}


void _delay_ms(unsigned int del)
{
 del = del*7650;
 while(del--);
}

void _delay_us(unsigned int del)
{
 del = del*10+250;
 while(del--);
}
	

void sendIntToPort(long n) 
{ 

    if (n < 0) 
	{ 
      send_debug('-'); 
      n = -n; 
    }   
 
    if (n/10) 
    sendIntToPort(n/10); 
  
    send_debug(n%10 + '0'); 
} 
  


void PutStr(char *str)
{ 
   char ch;
   while (*str)
   {
      ch = *str++;
      send_debug(ch);
   }
   send_debug('\n');
   send_debug('\r');
}

void send_debug(char s)              //SOFT usart RX (9600 BAUD)
 {
     TIM1_DIER   &= 0xFFFE;          // TIM1 OF	 
	 unsigned int count = 0x1;

	 GPIOB_BRR = TX_PIN;
     _delay_us(105);

     while(count!=0x100)
     {
      if(s&count)
      {
       GPIOB_BSRR = TX_PIN;
	   _delay_us(105);
      }
      else
      {
		GPIOB_BRR = TX_PIN;
	    _delay_us(105);
      }
      count = count<<1;
     }

     GPIOB_BSRR = TX_PIN;
     _delay_us(105);
     TIM1_DIER   |= 0x1;             // TIM1 ON
 }
