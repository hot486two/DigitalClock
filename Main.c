
#include "2450addr.h"
#include "my_lib.h"
#include "option.h"
#include "num.h"
#include "menu.h" // �޴� �̹���
#include "set.h" // ���� �̹���
#include "snum.h" // small number �̹���
#include "btn.h" // ��ư �̹���
#include "und.h" // ����â �̹���
#include "alarm.h" // �˶��ð� �̹���

#define BLACK	0x0000
#define WHITE	0xFFFE
#define BLUE	0x001F
#define GREEN	0x03E0
#define RED		0x7C00

#define  NonPal_Fb   ((volatile unsigned short(*)[480]) FRAME_BUFFER)

enum Mode{Clock, Alarm, Stopwatch, Timer}; //Mode ���� ����

int Temp_hour, Temp_min, Temp_sec; // ����ð� ������ ǥ�õǴ� �ӽ� ����
int Alarm_hour, Alarm_min, Alarm_sec; // �˶� ���� ����
int Swatch_hour, Swatch_min, Swatch_sec, Swatch_msec; //SW ���� ����
int T_Swatch_hour, T_Swatch_min, T_Swatch_sec; // SW ���� 1�ʴ���, 10�� ���� �� ���ڸ� ������ ��� ��Ű�� ���� �񱳰� ���� ����(SW print �ӵ� ���)
int THour, TMin, TSec; // Timer ����
int Hour, Min, Sec; // Clock ����
int rank; // laptime ����

void Print_Temp(void); // Clock ������ ȭ�� ��� �Լ�
void Print_Timer(void); // Timer ��� �Լ�
void Print_Clock(void); // Clock ��� �Լ�
void Print_Alarm(void); // Alarm ��� �Լ�

void Print_Stopwatch(void); // SW Menu Ŭ���� ��� �Լ�

void Print_Stopwatch_ms(void); // SW ������ �� ���� ��� �Լ�
void Print_Stopwatch_s1(void);
void Print_Stopwatch_s10(void);
void Print_Stopwatch_m1(void);
void Print_Stopwatch_m10(void);
void Print_Stopwatch_h1(void);
void Print_Stopwatch_h10(void);

void Clear_SW_Allvalue(void); // SW reset 

int Check_AlarmValue(void); // ����ð��� �˶��ð� �� �Լ�

void Tick_Init(void);
void Timer0_ISR(void) __attribute__ ((interrupt ("IRQ")));//Stopwatch
// void Timer1_ISR(void) __attribute__ ((interrupt ("IRQ")));//Alarm
void Timer2_ISR(void) __attribute__ ((interrupt ("IRQ")));//Countdown timer
void Touch_ISR(void) __attribute__ ((interrupt ("IRQ")));//Touch
void Tick_ISR(void) __attribute__ ((interrupt ("IRQ")));
void Get_touch(int x, int y);

void Check_ring(void); // alarm check
void Countdown(void); // timer ���� ���� �Լ�
void Clean(void); // ȭ�� clean

volatile  int ADC_x, ADC_y;
volatile  int Touch_Pressed; // TOUCH ����


int Status; // ���� mode ����

// ���� FLAG

int conf; // ���� ��ư flag
int TimerStart_flag; //timer ���� flag
int SWStart_flag; // SW ���� flag
int AlarmStart_flag; // Alarm, ������ ���� flag

//Print flag
int ClockPrint_flag = 1; // ���ý� Clock ȭ�� ���
int AlarmPrint_flag = 0;
int SWPrint_flag = 0;
int TimerPrint_flag = 0;






void Timer0_ISR(void) //Stopwatch
{
	/* �ش� ���ͷ�Ʈ Masking */ 

	rINTMSK1 |= (1<<10);	
	rSRCPND1 |= (1<<10);
	rINTPND1 |= (1<<10);	

	// SW ���� �˰���
	// SW �� �ѳ��� ���¿��� �ٸ� ���� ���ٽÿ� SW print�� �����ϸ�
	// �ð� ����� ����ȭ �Ͽ� SW �ӵ� ����(10msec���� �ð� ��� �ð��� ��� ������ SW �ð��� �������� ��Ȳ�� ����)
	
	if(SWPrint_flag ==1) // flag on �ÿ��� ��� (print�� ���� �ʾƵ� background ������ ����ؼ� ���� ���� ��Ű�� ���ؼ�)
	{
		Print_Stopwatch_ms();
	}
	
	if(Swatch_msec==99)
	{
		Swatch_msec=-1;
		Swatch_sec++;
		if(SWPrint_flag ==1)
		{
			Print_Stopwatch_s1();
		}
		if(T_Swatch_sec != Swatch_sec/10)
		{
			if(SWPrint_flag ==1)
			{
				Print_Stopwatch_s10();
			}
			T_Swatch_sec = Swatch_sec/10;
		}
	}
	if(Swatch_sec==60)
	{
		Swatch_sec=0;	
		Swatch_min++;
		if(SWPrint_flag ==1)
		{
			Print_Stopwatch_m1();
		}
		if(T_Swatch_min != Swatch_min/10)
		{
			if(SWPrint_flag ==1)
			{
				Print_Stopwatch_m10();
			}
			T_Swatch_min = Swatch_min/10;
		}
		
	}
	
	if(Swatch_min==60)
	{
		Swatch_min=0;
		Swatch_hour++;
		
		if(SWPrint_flag ==1)
		{
			Print_Stopwatch_h1();
		}
		if(T_Swatch_hour != Swatch_hour/10)
		{
			if(SWPrint_flag ==1)
			{
				Print_Stopwatch_h10();
			}
			T_Swatch_hour = Swatch_hour/10;
		}
		
	}
	Swatch_msec++; // SW msec ������ ����
		
	/* �ش� ���ͷ�Ʈ UnMasking */
	rINTMSK1 &= ~(1<<10);
}

void Timer2_ISR(void) // count down Timer
{
	/* �ش� ���ͷ�Ʈ Masking */ 

	rINTMSK1 |= (1<<12);	
	/* TODO : Pending Register Clear */
	rSRCPND1 |= (1<<12);
	rINTPND1 |= (1<<12);	

	// Uart_Send_String("Timer2 ISR\n");
	
	// Timer ���� �˰���
	// ������ Timer �ð� ������ ���� ���� 
	
	if(TimerStart_flag == 1)
	{
		TSec--;
		Countdown();
	}
	if(TimerPrint_flag ==1) // Mode ����� print ����
	{
		Print_Timer();
	}
	/* �ش� ���ͷ�Ʈ UnMasking */
	if(TimerStart_flag ==1)
	{
		rINTMSK1 &= ~(1<<12);
	}
	
	
}

void Tick_ISR(void)
{
	rINTMSK1 |= (1<<8);
	
	rSRCPND1 |= (1<<8);
	rINTPND1 |= (1<<8);
	
	// �ð� ������ �ð� register �� ȣ��
	Hour = (rBCDHOUR/16)*10 + (rBCDHOUR%16);
	Min = (rBCDMIN/16)*10 + (rBCDMIN%16);
	Sec = (rBCDSEC/16)*10 + (rBCDSEC%16);
	
	
	Check_ring(); // �˶��ð� ��ġ check
		
	if(ClockPrint_flag == 1) // �ٸ� mode�� clock print ����
	{
		Print_Clock();
	}
	
	rINTMSK1 &= ~(1<<8);
}

void Touch_ISR(void)
{
	/* ���ͷ�Ʈ ������� ���� on Touch */
	rINTSUBMSK |= (0x1<<9);
	rINTMSK1 |= (0x1<<31);	
	
	/* TO DO: Pendng Clear on Touch */	
	rSUBSRCPND |= (0x1<<9);
	rSRCPND1 |= (0x1<<31);
	rINTPND1 |= (0x1<<31);
	
	if(rADCTSC & 0x100)
	{
		rADCTSC &= (0xff); 
		Touch_Pressed = 0;
		// Uart_Send_String("Detect Stylus Up Interrupt Signal \n");
	}
	
	else
	{
		
		/* TO DO : Stylus Down, YM_out Enable, YP_out Disable, XM_out Disable, XP_out disable
		 * 		   XP Pull-up Disable, Auto Sequential measurement of X/Y, No operation mode */
		rADCTSC =(0<<8)|(1<<7)|(1<<6)|(0<<5)|(1<<4)|(1<<3)|(1<<2)|(0);	

		/* TO DO : ENABLE_START */		
		rADCCON |=(1);
		
		/* wait until End of A/D Conversion */
		while(!(rADCCON & (1<<15))); /* ��ȯ�� ���������� ��ٸ� */
		
		/*store X-Position & Y-Position Conversion data value to ADC_x, ADC_y */
		ADC_x = (rADCDAT0 & 0x3ff); /* x��ǥ ��ȯ ������ ���� &���� == x��ǥ ������ */
		ADC_y = (rADCDAT1 & 0x3ff); /* y��ǥ ��ȯ ������ ���� &���� == y��ǥ ������ */
		
		Touch_Pressed = 1;
		
		/* TO DO : change to Waiting for interrupt mode 
		 *		   Stylus Up, YM_out Enable, YP_out Disable, XM_out Disable, XP_out disable
		 * 		   XP Pull-up Ensable, Normal ADC conversion, Waiting for interrupt mode */		
		rADCTSC =(1<<8)|(1<<7)|(1<<6)|(1<<4)|(0<<3)|(0<<2)|(3);
	}
		
	Get_touch(ADC_x-150, ADC_y-330);
	/* ���ͷ�Ʈ �ٽ� ���  on Touch */
	rINTSUBMSK &= ~(0x1<<9);
	rINTMSK1 &= ~(0x1<<31);
	
}

void Get_touch(int x, int y)
{
	if(Touch_Pressed)
	{
		int i,j;
		
		
		if((x>=0 && x<=150) && (y>=270 && y<=330)) // CLOCK
		{
			Clean();
			Status = Clock;
			Uart_Printf("CLOCK\n");
			Led_Display(Clock+1);
			SWPrint_flag = 0;
			TimerPrint_flag = 0;
			ClockPrint_flag =1;
			
			
			
		}
		else if((x>=0 && x<=150) && (y>=180 && y<=260)) // ALARM
		{		
			Clean();	
			Status = Alarm;
			Uart_Printf("ALARM \n");
			Led_Display(Alarm+1);
			
			SWPrint_flag = 0;
			TimerPrint_flag = 0;
			ClockPrint_flag = 0;
			
			
			Print_Alarm();
			
			
			
		}
		else if((x>=0 && x<=150) && (y>=90 && y<=170))  // STOPWATCH
		{		
			Clean();	
			Status = Stopwatch;
			Uart_Printf("STOP WATCH \n");
			Led_Display(Stopwatch+1);
			
			SWPrint_flag = 1;
			ClockPrint_flag = 0;
			TimerPrint_flag = 0;
			
			Print_Stopwatch();
			Print_Stopwatch_ms();
						
			if(SWStart_flag)
			{
				Lcd_Draw_BMP(0, 218, und3);
			}
			else
			{
				Lcd_Draw_BMP(0, 218, und2);
			}
			
			
		}
		else if((x>=0 && x<=150) && (y>=0 && y<=80))  // TIMER
		{
			Clean();
			Status = Timer;
			Uart_Printf("TIMER \n");
			Led_Display(Timer+1);
			
			TimerPrint_flag = 1;
			ClockPrint_flag = 0;
			SWPrint_flag = 0;
			
			
			Print_Timer();
			
			if(TimerStart_flag)
			{
				
				Lcd_Draw_BMP(0, 218, und4);
			}
			
			
			
			
		}
		else if((x>=150 && x<=260) && (y>=240 && y<=350) && (conf == 0) && (Status != Stopwatch)) //���� ���� ������
		{ 
			conf = 1;
			Uart_Printf("SETTING \n");
			Lcd_Draw_BMP(14, 50, upbtn);
			Lcd_Draw_BMP(122, 50, upbtn);
			Lcd_Draw_BMP(230, 50, upbtn);
			Lcd_Draw_BMP(14, 154, dwbtn);
			Lcd_Draw_BMP(122, 154, dwbtn);
			Lcd_Draw_BMP(230, 154, dwbtn);
			
			if(Status == Clock)
			{
				ClockPrint_flag =0;
				Temp_hour = Hour;
				Temp_min = Min;
				Temp_sec = Sec;
				Print_Temp();
				
				Lcd_Draw_BMP(0, 218, und1);
			}
			
			if(Status==Alarm)
			{
				
				Lcd_Draw_BMP(0, 218, und1);
			}
			
			if(Status == Timer)
			{
				Lcd_Draw_BMP(0, 218, und2);
			}
			
		}
		
		else if((x>=150 && x<=260) && (y>=240 && y<=350) && (conf == 1))
		{ //���� �ٽ� ������
			
			Clean();
			
			
			if(Status == Clock)
			{
				ClockPrint_flag =1;
				// Print_Temp();
			}	
			
			conf = 0;
			
		}
		
		else if((x>=180 && x<=455) && (y>=0 && y<=90) && ((conf == 1)||(Status == Stopwatch)))//�����ϴ� ��ư������
		{ 
			
			if(Status == Clock)
			{
				Clean();
				ClockPrint_flag =1;
				// Print_Temp();
				conf = 0;
			}
			else if(Status == Alarm)
			{
				Clean();
				AlarmStart_flag = 0;
				Alarm_hour = 0;
				Alarm_min = 0;
				Alarm_sec = 0;
				Print_Alarm();
			}	
			else if(Status == Timer) // Timer reset
			{
				THour = 0;
				TMin = 0;
				TSec = 0;
				Print_Timer();
			}
			
			if((Status == Stopwatch)&& (SWStart_flag == 0)) // Stopwatch reset
			{
				rTCON |= 0;
				Clear_SW_Allvalue();
				Print_Stopwatch();
				Print_Stopwatch_ms();
				rank = 0;
				
			}
			else if((Status == Stopwatch)&& (SWStart_flag == 1)) // Stopwatch laptime
			{
				Uart_Printf("%d�� laptime : \n", rank+1);
				
				Uart_Printf("%d%d : %d%d : %d%d : %d%d\n",Swatch_hour/10, Swatch_hour%10,Swatch_min/10,Swatch_min%10, Swatch_sec/10,Swatch_sec%10, Swatch_msec/10,Swatch_msec%10);
				rank++;
			}
			
			
		}
		
		else if((x>=455 && x<=730) && (y>=0 && y<=90) && ((conf == 1)||(Status == Stopwatch)||(TimerStart_flag==1)))//�����ϴ� ��ư������
		{ 
			
			// Clean();
			
			if(Status == Clock)  // Clode mode�� ������ ����ð� input
			{
				Clean();
				rBCDHOUR 	= (rBCDHOUR & ~(0x3F))|((Temp_hour/10)<<4)|((Temp_hour%10)<<0);
				rBCDMIN 	= (rBCDMIN & ~(0x7F))|((Temp_min/10)<<4)|((Temp_min%10)<<0);
				rBCDSEC 	= (rBCDSEC & ~(0x7F))|((Temp_sec/10)<<4)|((Temp_sec%10)<<0);
				ClockPrint_flag =1;
				// Print_Temp();
			}
			else if(Status== Alarm) // Alarm mode�� �˶� set
			{
				Clean();
				AlarmStart_flag = 1;
			}
			
			if((Status == Timer)&&(TimerStart_flag == 0)) // Timer mode�� timer Start
			{
				Clean(); // ȭ��ǥ, ����ȭ�� clear
				TimerStart_flag = 1;
				TimerPrint_flag = 1;
				if(TimerStart_flag)
				{
					pISR_TIMER2 = (unsigned int)Timer2_ISR;
					rINTMSK1 &= ~(1<<12);
					Timer2_Delay(1000);
				}
				Lcd_Draw_BMP(0, 218, und4);
				
				for(j=200; j<272; j++) //�ϴܿ������� ���������� ä���
				{		
					for(i=180; i<360; i++)
						NonPal_Put_Pixel(i,j,BLACK);
				}
				
			}
			else if((Status == Timer)&&(TimerStart_flag == 1)) // Timer Stop
			{
				Clean();
				TimerStart_flag = 0;
				rINTMSK1 |= (1<<12);
				rSRCPND1 |= (1<<12);
				rINTPND1 |= (1<<12);
				
			}
			
			if((Status == Stopwatch) && (SWStart_flag==0)) // Stopwatch Start
			{
				SWStart_flag = 1;
				if(SWStart_flag)
				{
					pISR_TIMER0 = (unsigned int)Timer0_ISR;
					rINTMSK1 &= ~(1<<10);
					// Print_Stopwatch();
					Timer0_Delay(10);
				}
				
				Lcd_Draw_BMP(0, 218, und3);
			}
			else if((Status == Stopwatch) && (SWStart_flag == 1)) // Stopwatch Stop
			{
				SWStart_flag = 0;
				rINTMSK1 |= (1<<10);
				 rSRCPND1 |= (1<<10);
				 rINTPND1 |= (1<<10);
				 Lcd_Draw_BMP(0, 218, und2);
			}
			
			
			conf=0;
		}
		
		else if((x>=580 && x<=710) && (y>=230 && y<=290) && (conf == 1))//(�������ʴ����� ���¿���)btn1 ������
		{ 
			
			//�̰��� �ð� ���� �ö󰡰� �����
			
			if(Status == Clock)
			{
				Temp_hour++;
				if(Temp_hour==24)
				{
					Temp_hour=0;
				}
				Print_Temp();
			}
			
			else if(Status == Alarm)
			{
				Alarm_hour++;
				if(Alarm_hour==24)
				{
					Alarm_hour=0;
				}
				Print_Alarm();
			}
			
			else if(Status == Timer)
			{
				THour++;
				if(THour==24)
				{
					THour=0;
				}
				Print_Timer();
			}
			
		}
		else if((x>=420 && x<=560) && (y>=230 && y<=290) && (conf == 1))//(�������ʴ����� ���¿���)btn2 ������
		{ 
			
			//�̰��� �ð� ���� �ö󰡰� �����
			if(Status == Clock)
			{
				Temp_min++;
				if(Temp_min==60)
				{
					Temp_min=0;
				}
				Print_Temp();
			}
			else if(Status == Alarm)
			{
				Alarm_min++;
				if(Alarm_min==60)
				{
					Alarm_min=0;
				}
				Print_Alarm();
			}
			else if(Status == Timer)
			{
				TMin++;
				if(TMin==60)
				{
					TMin=0;
				}
				Print_Timer();
			}
			
		}
		else if((x>=270 && x<=410) && (y>=230 && y<=290) && (conf == 1))//(�������ʴ����� ���¿���)btn3 ������
		{ 
			
			//�̰��� �ð� ���� �ö󰡰� �����
			if(Status == Clock)
			{
				Temp_sec++;
				if(Temp_sec==60)
				{
					Temp_sec=0;
				}
				Print_Temp();
			}
			else if(Status == Alarm)
			{
				Alarm_sec++;
				if(Alarm_sec==60)
				{
					Alarm_sec=0;
				}
				Print_Alarm();
			}
			
			else if(Status == Timer)
			{
				TSec++;
				if(TSec==60)
				{
					TSec=0;
				}
				Print_Timer();
			}
		}
		else if((x>=580 && x<=710) && (y>=100 && y<=160) && (conf == 1))//(�������ʴ����� ���¿���)btn4 ������
		{ 
			
			//�̰��� �ð� ���� �������� �����
			if(Status == Clock)
			{
				Temp_hour--;
				if(Temp_hour<0)
				{
					Temp_hour=23;
				}
				Print_Temp();
			}
			else if(Status == Alarm)
			{
				Alarm_hour--;
				if(Alarm_hour<0)
				{
					Alarm_hour=23;
				}
				Print_Alarm();
			}
			else if(Status == Timer)
			{
				THour--;
				if(THour<0)
				{
					THour=23;
				}
				Print_Timer();
			}
		}
		else if((x>=420 && x<=560) && (y>=100 && y<=160) && (conf == 1)) //(�������ʴ����� ���¿���)btn5 ������
		{ 
			
			//�̰��� �ð� ���� �������� �����
			if(Status == Clock)
			{
				Temp_min--;
				if(Temp_min<0)
				{
					Temp_min=59;
				}
				Print_Temp();
			}
			else if(Status == Alarm)
			{
				Alarm_min--;
				if(Alarm_min<0)
				{
					Alarm_min=59;
				}
				Print_Alarm();
			}
			
			if(Status == Timer)
			{
				TMin--;
				if(TMin<0)
				{
					TMin=59;
				}
				Print_Timer();
			}
		}
		else if((x>=270 && x<=410) && (y>=100 && y<=160) && (conf == 1)) //(�������ʴ����� ���¿���)btn6 ������
		{
			
			//�̰��� �ð� ���� �������� �����
			if(Status == Clock)
			{
				Temp_sec--;
				if(Temp_sec<0)
				{
					Temp_sec=59;
				}
				Print_Temp();
			}
			else if(Status == Alarm)
			{
				Alarm_sec--;
				if(Alarm_sec<0)
				{
					Alarm_sec=59;
				}
				Print_Alarm();
			}
			else if(Status == Timer)
			{
				TSec--;
				if(TSec<0)
				{
					TSec=59;
				}
				Print_Timer();
			}
		}
		else if((x>=670 && x<=730) && (y>=290 && y<=350) && (AlarmStart_flag == 1)) // �˶��ð� ������
		{
			AlarmStart_flag = 0;
			
			rTCON &= ~(1<<8);
			
			rINTPND1 |=(1<<11);
			rINTMSK1 |=(1<<11);
			
			for(j=0; j<44; j++) //Alarm icon �����
			{		
				for(i=0; i<40; i++)
					NonPal_Put_Pixel(i,j,BLACK);
			}
			
		}
	}
}

void Tick_Init(void)
{
	rTICNT |= 0x81;
	rRTCCON  = (rRTCCON  & ~(0xF))| 0x1;
	
	rBCDHOUR 	= rBCDHOUR & ~(0x3F);
    rBCDMIN 	= rBCDMIN & ~(0x7F);
    rBCDSEC 	= rBCDSEC & ~(0x7F);
	
	rINTMSK1 &= ~(1<<8);
	
	pISR_TICK = (unsigned int)Tick_ISR;
	
}

void Countdown(void)
{
	// Timer ���� �˰���
	
	
	if(TSec<0)
	{
		TSec = 59;
		TMin--;
	}
	if(TMin<0)
	{
		TMin = 59;
		THour--;
	}
	if(THour<0)
	{
		TimerPrint_flag = 0;
		TimerStart_flag = 0;
		AlarmStart_flag = 1;
		rINTMSK1 |= (1<<12);

		THour = 0;
		TMin = 0;
		TSec = 0;
		
	}
}

void Print_Temp(void)
{
	int i;
	int num_arr[] = {Temp_hour/10, Temp_hour%10, Temp_min/10, Temp_min%10, Temp_sec/10, Temp_sec%10};
	
	for(i=0; i<6; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Alarm(void)
{
	int i;
	int num_arr[] = {Alarm_hour/10, Alarm_hour%10, Alarm_min/10, Alarm_min%10, Alarm_sec/10, Alarm_sec%10};
	
	for(i=0; i<6; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch(void)
{
	int i;
	int num_arr[] = {Swatch_hour/10, Swatch_hour%10, Swatch_min/10, Swatch_min%10, Swatch_sec/10, Swatch_sec%10};
	
	for(i=0; i<6; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_ms(void)
{
	int i;
	int num_arr[] = {Swatch_msec/10, Swatch_msec%10};
	
	for(i = 0; i<2; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP(300+(12*i), 120, snum0); 
				break;
			case 1: 
				Lcd_Draw_BMP(300+(12*i), 120, snum1); 
				break;
			case 2: 
				Lcd_Draw_BMP(300+(12*i), 120, snum2); 
				break;
			case 3: 
				Lcd_Draw_BMP(300+(12*i), 120, snum3); 
				break;
			case 4: 
				Lcd_Draw_BMP(300+(12*i), 120, snum4); 
				break;
			case 5: 
				Lcd_Draw_BMP(300+(12*i), 120, snum5); 
				break;
			case 6: 
				Lcd_Draw_BMP(300+(12*i), 120, snum6); 
				break;
			case 7: 
				Lcd_Draw_BMP(300+(12*i), 120, snum7); 
				break;
			case 8: 
				Lcd_Draw_BMP(300+(12*i), 120, snum8); 
				break;
			case 9: 
				Lcd_Draw_BMP(300+(12*i), 120, snum9); 
				break;
		}
	}	
}

void Print_Stopwatch_s1(void)
{
	int i;
	int num_arr[] = {Swatch_sec/10, Swatch_sec%10};
	
	for(i=1; i<2; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_s10(void)
{
	int i;
	int num_arr[] = {Swatch_sec/10, Swatch_sec%10};
	
	for(i=0; i<1; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + ((i+4)* 36) + ((i+4)/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_m1(void)
{
	int i;
	int num_arr[] = {Swatch_min/10, Swatch_min%10};
	
	for(i=1; i<2; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_m10(void)
{
	int i;
	int num_arr[] = {Swatch_min/10, Swatch_min%10};
	
	for(i=0; i<1; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + ((i+2)* 36) + ((i+2)/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_h1(void)
{
	int i;
	int num_arr[] = {Swatch_hour/10, Swatch_hour%10};
	
	for(i=1; i<2; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Stopwatch_h10(void)
{
	int i;
	int num_arr[] = {Swatch_hour/10, Swatch_hour%10};
	
	for(i=0; i<1; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Timer(void)
{
	int i;
	int num_arr[] = {THour/10, THour%10, TMin/10, TMin%10, TSec/10, TSec%10};
	Uart_Printf("TIMER \n");
	for(i=0; i<6; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Print_Clock(void)
{
	int i;
	int num_arr[] = {Hour/10, Hour%10, Min/10, Min%10, Sec/10, Sec%10};
	// Uart_Printf("CLOCK \n");
	for(i=0; i<6; i++)
	{
		switch(num_arr[i])
		{
			case 0: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num0); 
				break;
			case 1: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num1); 
				break;
			case 2: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num2); 
				break;
			case 3: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num3); 
				break;
			case 4: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num4); 
				break;
			case 5: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num5); 
				break;
			case 6: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num6); 
				break;
			case 7: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num7); 
				break;
			case 8: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num8); 
				break;
			case 9: 
				Lcd_Draw_BMP((14 + (i* 36) + (i/2)*36), 80, num9); 
				break;
		}
	}	
}

void Clear_SW_Allvalue(void)
{
	Swatch_hour=0;
	Swatch_min=0;
	Swatch_sec=0;
	Swatch_msec=0;
}

void Clean(void)
{
	int i, j;
	for(j=120; j<138; j++) //�и��� �����
	{		
		for(i=300; i<324; i++)
			NonPal_Put_Pixel(i,j,BLACK);
	}
	
	for(j=200; j<272; j++) //�ϴܹ�ư���� ���������� ä���
		{		
			for(i=0; i<360; i++)
				NonPal_Put_Pixel(i,j,BLACK);
		}

	for(j=50; j<68; j++) //�� ȭ��ǥ �����
	{		
		for(i=14; i<302; i++)
			NonPal_Put_Pixel(i,j,BLACK);
	}
	
	for(j=154; j<172; j++) //�Ʒ� ȭ��ǥ �����
	{		
		for(i=14; i<302; i++)
			NonPal_Put_Pixel(i,j,BLACK);
	}
}

void Check_ring(void)
{
	if( (AlarmStart_flag) && Check_AlarmValue())
	{
		//�ð�׸������� ���� �ڵ��߰�
		//Make_ring();
		Uart_Printf("!!!!!!!!!!!!!!!!!!!!ALARM!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		Uart_Printf("!!!!!!!!!!!!!!!!!!!!ALARM!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		// AlarmStart_flag=0;
		Lcd_Draw_BMP(0, 0, alarm);
		Timer1_Delay(1000);
		//Lcd_Draw_BMP(5, 5, test); 
		
	}
	
	if(TimerStart_flag&&(THour ==0)&&(TMin==0)&&(TSec==0))
	{
		Lcd_Draw_BMP(0, 0, alarm);
		Timer1_Delay(1000);
	}
}

int Check_AlarmValue(void)//�ð��� �˶����� ������ ���� ��ġ�ϴ��� üũ.
{					   //��ġ�ϸ�1, �ƴϸ� 0 ����
	if( Hour==Alarm_hour && Min==Alarm_min && Sec==Alarm_sec)
		return 1;
	else
		return 0;
}

void Main(void)
{	
	
	Uart_Init(115200);	
	Touch_Init();
	Lcd_Port_Init();
	NonPal_Lcd_Init();
	Timer_Init();
	Led_Init();

	Uart_Send_Byte('\n');
	Uart_Send_Byte('A');	
	Uart_Send_String("Hello LCD Test...!!!\n");
	
	
	Led_Display(Clock+1);
	
	int i,j;
	for(j=0; j<272; j++)
	{
		for(i=0; i<360; i++)
		{
			NonPal_Put_Pixel(i,j,BLACK);
		}
	}
	
	//�޴� ���
	Lcd_Draw_BMP(320, 0, set);
	Lcd_Draw_BMP(360, 0, menu);
		
	Lcd_Draw_BMP(88, 80, numdb);
	Lcd_Draw_BMP(190, 80, numdb);
	
	Tick_Init();
	
	
	/* TO DO : ���ͷ�Ʈ ���Ϳ� Touch_ISR �Լ� ��� */
	pISR_ADC = (unsigned int)Touch_ISR;
	
	/* TO DO :  ���ͷ�Ʈ ��� on Touch */
	rINTSUBMSK &= ~(0x1<<9);
	rINTMSK1 &= ~(0x1<<31);	
	
	Uart_Send_String("Hello LCD Test...!!!\n");	
		
	Uart_Send_String("Test Finished...!!!\n");
}	


