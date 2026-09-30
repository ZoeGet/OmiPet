#include "sys.h"
#include "delay.h"
#include "tftlcd.h"
#include "pic.h"

int sec=55,min=59,hour=23;
u16 count;
int flg=1;

#define JTAG_SWD_DISABLE   0X02
#define SWD_ENABLE         0X01
#define JTAG_SWD_ENABLE    0X00	

void JTAG_Set(u8 mode)
{
	u32 temp;
	temp=mode;
	temp<<=25;
	RCC->APB2ENR|=1<<0;     //开启辅助时钟	   
	AFIO->MAPR&=0XF8FFFFFF; //清除MAPR的[26:24]
	AFIO->MAPR|=temp;       //设置jtag模式
} 

int main(void)
{
	SystemInit();	
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	JTAG_Set(JTAG_SWD_DISABLE);     
	JTAG_Set(SWD_ENABLE);  
	delay_ms(10);	


	TFTLCD_Init();
//	TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,WHITE);			

	while(1)
	{
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,WHITE);			//
		delay_ms(1000);

		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLACK);		//
		delay_ms(1000);
	 
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,RED);		//
		delay_ms(1000);
 
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GREEN);		//
		delay_ms(1000);
 
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BLUE);		//
		delay_ms(1000);
 
		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,YELLOW);		//
		delay_ms(1000);

		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,BRRED);		//
		delay_ms(1000);

		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,GRAY);		//
		delay_ms(1000);

		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,CYAN);		//
		delay_ms(1000);

		TFTLCD_Fill(0,0,TFTLCD_W,TFTLCD_H,LIGHTBLUE);		//
		delay_ms(1000);
	}
}

