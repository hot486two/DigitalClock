/*
 * =====================================================================
 * NAME         : Led.c
 *
 * Descriptions : Main routine for S3C2440
 *
 * IDE          : GCC-4.1.0
 *
 * Modification
 *	  
 * =====================================================================
 */

#include "2450addr.h"
#include "my_lib.h"

//Function Declaration
void Led_Init();
void Led_Display(int data);

void Led_Init()
{
	/* TO DO : Init GPIO port connected to LED 
	 * LED1:LED2:LED3:LED4 = GPG4:GPG5:GPG6:GPG7 */
	rGPGDAT |= (0xF<<4);
	rGPGCON &= ~(0xFF<<8);
	rGPGCON |= (0x55<<8);
		
}

void Led_Display(int data)
{
	
	//** 삼항연산자를 사용할 경우 
	(data == 1)? (rGPGDAT &= ~(1<<4)):(rGPGDAT |= (1<<4));
	(data == 2)? (rGPGDAT &= ~(1<<5)):(rGPGDAT |= (1<<5)); 
	(data == 3)? (rGPGDAT &= ~(1<<6)):(rGPGDAT |= (1<<6)); 
	(data == 4)? (rGPGDAT &= ~(1<<7)):(rGPGDAT |= (1<<7)); 


}