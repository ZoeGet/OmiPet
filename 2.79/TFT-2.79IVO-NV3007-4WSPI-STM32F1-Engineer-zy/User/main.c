/********************金逸晨电子**************************
*****************0.99圆屏 4线SPI TFT FOR STM32F103*************
*****STM32F103驱动**************************
//              GND   电源地
//              VCC   3.3v电源
//              BLK   PB4（背光电源）
//              SCL   PB5（SCLK）
//              SDA   PB6（MOSI）
//              RES   PB7
//              DC    PB8
//              CS    PB9
********************************************************/

#include "stm32f10x.h"
#include "led.h"
#include "key.h"  
//#include "bmp.h"  

/**********SPI引脚分配，连接TFT屏，更具实际情况修改*********/

#define TFT_COLUMN_OFFSET 12	//列偏移
#define TFT_LINE_OFFSET 0   //行偏移
#define USE_HORIZONTAL 0    //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_COL 142	
#define LCD_ROW 428
#else
#define LCD_COL 428
#define LCD_ROW 142
#endif


/**********SPI引脚分配，连接TFT屏，更具实际情况修改*********/
#define LCD_SCLK_Clr() GPIO_ResetBits(GPIOB,GPIO_Pin_5)//SCL=SCLK
#define LCD_SCLK_Set() GPIO_SetBits(GPIOB,GPIO_Pin_5)

#define LCD_MOSI_Clr() GPIO_ResetBits(GPIOB,GPIO_Pin_6)//SDA=MOSI
#define LCD_MOSI_Set() GPIO_SetBits(GPIOB,GPIO_Pin_6)

#define LCD_RES_Clr()  GPIO_ResetBits(GPIOB,GPIO_Pin_7)//RES
#define LCD_RES_Set()  GPIO_SetBits(GPIOB,GPIO_Pin_7)

#define LCD_DC_Clr()   GPIO_ResetBits(GPIOB,GPIO_Pin_8)//DC
#define LCD_DC_Set()   GPIO_SetBits(GPIOB,GPIO_Pin_8)
 		     
#define LCD_CS_Clr()   GPIO_ResetBits(GPIOB,GPIO_Pin_9)//CS
#define LCD_CS_Set()   GPIO_SetBits(GPIOB,GPIO_Pin_9)

#define LCD_BLK_Clr()  GPIO_ResetBits(GPIOB,GPIO_Pin_4)//BLK
#define LCD_BLK_Set()  GPIO_SetBits(GPIOB,GPIO_Pin_4)


/*****************颜色数据**************************/
#define WHITE            0xFFFF
#define BLACK            0x0000   
#define BLUE             0x001F  
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define GREEN            0x07E0

unsigned int page = 0;
unsigned int autoplay = 0;

const unsigned char gImage_gary[344] = { /* 0X00,0X10,0XAC,0X00,0X01,0X00,0X01,0X1B, */
0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XDF,0XFF,0XDF,0XFF,0XDF,0XFF,0XBE,0XF7,0XBE,0XF7,
0X9E,0XF7,0X9E,0XF7,0X9E,0XF7,0X7D,0XEF,0X7D,0XEF,0X5D,0XEF,0X5D,0XEF,0X5D,0XEF,
0X3C,0XE7,0X3C,0XE7,0X3C,0XE7,0X1C,0XE7,0X1C,0XE7,0XFB,0XDE,0XFB,0XDE,0XFB,0XDE,
0XDB,0XDE,0XDB,0XDE,0XDB,0XDE,0XBA,0XD6,0XBA,0XD6,0X9A,0XD6,0X9A,0XD6,0X9A,0XD6,
0X79,0XCE,0X79,0XCE,0X79,0XCE,0X59,0XCE,0X59,0XCE,0X38,0XC6,0X38,0XC6,0X38,0XC6,
0X18,0XC6,0X18,0XC6,0X18,0XC6,0XF7,0XBD,0XF7,0XBD,0XD7,0XBD,0XD7,0XBD,0XD7,0XBD,
0XB6,0XB5,0XB6,0XB5,0XB6,0XB5,0X96,0XB5,0X96,0XB5,0X75,0XAD,0X75,0XAD,0X75,0XAD,
0X55,0XAD,0X55,0XAD,0X55,0XAD,0X34,0XA5,0X34,0XA5,0X14,0XA5,0X14,0XA5,0X14,0XA5,
0XF3,0X9C,0XF3,0X9C,0XF3,0X9C,0XD3,0X9C,0XD3,0X9C,0XB2,0X94,0XB2,0X94,0XB2,0X94,
0X92,0X94,0X92,0X94,0X92,0X94,0X71,0X8C,0X71,0X8C,0X51,0X8C,0X51,0X8C,0X51,0X8C,
0X30,0X84,0X30,0X84,0X30,0X84,0X10,0X84,0X10,0X84,0X10,0X84,0XEF,0X7B,0XEF,0X7B,
0XEF,0X7B,0XCF,0X7B,0XCF,0X7B,0XCF,0X7B,0XAE,0X73,0XAE,0X73,0X8E,0X73,0X8E,0X73,
0X8E,0X73,0X6D,0X6B,0X6D,0X6B,0X6D,0X6B,0X4D,0X6B,0X4D,0X6B,0X2C,0X63,0X2C,0X63,
0X2C,0X63,0X0C,0X63,0X0C,0X63,0X0C,0X63,0XEB,0X5A,0XEB,0X5A,0XCB,0X5A,0XCB,0X5A,
0XCB,0X5A,0XAA,0X52,0XAA,0X52,0XAA,0X52,0X8A,0X52,0X8A,0X52,0X69,0X4A,0X69,0X4A,
0X69,0X4A,0X49,0X4A,0X49,0X4A,0X49,0X4A,0X28,0X42,0X28,0X42,0X08,0X42,0X08,0X42,
0XE7,0X39,0XE7,0X39,0XE7,0X39,0XC7,0X39,0XC7,0X39,0XA6,0X31,0XA6,0X31,0XA6,0X31,
0X86,0X31,0X86,0X31,0X86,0X31,0X65,0X29,0X65,0X29,0X45,0X29,0X45,0X29,0X45,0X29,
0X24,0X21,0X24,0X21,0X24,0X21,0X04,0X21,0X04,0X21,0XE3,0X18,0XE3,0X18,0XE3,0X18,
0XC3,0X18,0XC3,0X18,0XC3,0X18,0XA2,0X10,0XA2,0X10,0XA2,0X10,0X82,0X10,0X82,0X10,
0X61,0X08,0X61,0X08,0X61,0X08,0X61,0X08,0X41,0X08,0X41,0X08,0X20,0X00,0X20,0X00,
0X20,0X00,0X00,0X00,0X00,0X00,0X00,0X00,};

void delay_us(unsigned int _us_time)
{       
  unsigned char x=0;
  for(;_us_time>0;_us_time--)
  {
    x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;
	  x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;
	  
  }
}

void delay_ms(unsigned int _ms_time)
  {
    unsigned int i,j;
    for(i=0;i<_ms_time;i++)
    {
    for(j=0;j<900;j++)
      {;}
    }
  }


void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能B端口时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOB
 	GPIO_SetBits(GPIOB, GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9);
}


/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{	
	u8 i;
	LCD_CS_Clr();
	for(i=0;i<8;i++)
	{			  
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}	
  LCD_CS_Set();	
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
	LCD_Writ_Bus(dat>>8);
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+TFT_LINE_OFFSET);
		LCD_WR_DATA(y2+TFT_LINE_OFFSET);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+0);
		LCD_WR_DATA(y2+0);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+TFT_LINE_OFFSET);
		LCD_WR_DATA(x2+TFT_LINE_OFFSET);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(y2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2c);//储存器写
	}
	else
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
}

void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化GPIO
	
	LCD_RES_Clr();//复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(100);
	
	LCD_BLK_Set();//打开背光
  delay_ms(100);
		
	// NV3007 init
	
	LCD_WR_REG(0x36);
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x60);
	else LCD_WR_DATA8(0xA0);

//NV3006A1N IVO2.6 
LCD_WR_REG(0xff);
LCD_WR_DATA8(0xa5);	
LCD_WR_REG(0x9a);
LCD_WR_DATA8(0x08);
LCD_WR_REG(0x9b);
LCD_WR_DATA8(0x08);	
LCD_WR_REG(0x9c);
LCD_WR_DATA8(0xb0);	
LCD_WR_REG(0x9d);
LCD_WR_DATA8(0x16);
LCD_WR_REG(0x9e);
LCD_WR_DATA8(0xc4);
LCD_WR_REG(0x8f);
LCD_WR_DATA8(0x55);
LCD_WR_DATA8(0x04);
LCD_WR_REG(0x84);
LCD_WR_DATA8(0x90);
LCD_WR_REG(0x83);
LCD_WR_DATA8(0x7b);
LCD_WR_REG(0x85);
LCD_WR_DATA8(0x33);
LCD_WR_REG(0x60);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0x70);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0x61);
LCD_WR_DATA8(0x02);
LCD_WR_REG(0x71);
LCD_WR_DATA8(0x02);
LCD_WR_REG(0x62);
LCD_WR_DATA8(0x04);
LCD_WR_REG(0x72);
LCD_WR_DATA8(0x04);
LCD_WR_REG(0x6c);
LCD_WR_DATA8(0x29);
LCD_WR_REG(0x7c);
LCD_WR_DATA8(0x29);
LCD_WR_REG(0x6d);
LCD_WR_DATA8(0x31);
LCD_WR_REG(0x7d);
LCD_WR_DATA8(0x31);
LCD_WR_REG(0x6e);
LCD_WR_DATA8(0x0f);
LCD_WR_REG(0x7e);
LCD_WR_DATA8(0x0f);
LCD_WR_REG(0x66);
LCD_WR_DATA8(0x21);
LCD_WR_REG(0x76);
LCD_WR_DATA8(0x21);
LCD_WR_REG(0x68);
LCD_WR_DATA8(0x3A);
LCD_WR_REG(0x78);
LCD_WR_DATA8(0x3A);
LCD_WR_REG(0x63);
LCD_WR_DATA8(0x07);
LCD_WR_REG(0x73);
LCD_WR_DATA8(0x07);
LCD_WR_REG(0x64);
LCD_WR_DATA8(0x05);
LCD_WR_REG(0x74);
LCD_WR_DATA8(0x05);
LCD_WR_REG(0x65);
LCD_WR_DATA8(0x02);
LCD_WR_REG(0x75);
LCD_WR_DATA8(0x02);
LCD_WR_REG(0x67);
LCD_WR_DATA8(0x23);
LCD_WR_REG(0x77);
LCD_WR_DATA8(0x23);
LCD_WR_REG(0x69);
LCD_WR_DATA8(0x08);
LCD_WR_REG(0x79);
LCD_WR_DATA8(0x08);
LCD_WR_REG(0x6a);
LCD_WR_DATA8(0x13);
LCD_WR_REG(0x7a);
LCD_WR_DATA8(0x13);
LCD_WR_REG(0x6b);
LCD_WR_DATA8(0x13);
LCD_WR_REG(0x7b);
LCD_WR_DATA8(0x13);
LCD_WR_REG(0x6f);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0x7f);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0x50);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0x52);
LCD_WR_DATA8(0xd6);
LCD_WR_REG(0x53);
LCD_WR_DATA8(0x08);
LCD_WR_REG(0x54);
LCD_WR_DATA8(0x08);
LCD_WR_REG(0x55);
LCD_WR_DATA8(0x1e);
LCD_WR_REG(0x56);
LCD_WR_DATA8(0x1c);
//goa map_sel
LCD_WR_REG(0xa0);
LCD_WR_DATA8(0x2b);
LCD_WR_DATA8(0x24);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xa1);
LCD_WR_DATA8(0x87);
LCD_WR_REG(0xa2);
LCD_WR_DATA8(0x86);
LCD_WR_REG(0xa5);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xa6);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xa7);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xa8);
LCD_WR_DATA8(0x36);
LCD_WR_REG(0xa9);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xaa);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xB9);
LCD_WR_DATA8(0x85);
LCD_WR_REG(0xBA);
LCD_WR_DATA8(0x84);
LCD_WR_REG(0xBB);
LCD_WR_DATA8(0x83);
LCD_WR_REG(0xBC);
LCD_WR_DATA8(0x82);
LCD_WR_REG(0xBD);
LCD_WR_DATA8(0x81);
LCD_WR_REG(0xBE);
LCD_WR_DATA8(0x80);
LCD_WR_REG(0xBF);
LCD_WR_DATA8(0x01);
LCD_WR_REG(0xC0);
LCD_WR_DATA8(0x02);
LCD_WR_REG(0xc1);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xc2);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xc3);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xc4);
LCD_WR_DATA8(0x33);
LCD_WR_REG(0xc5);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xc6);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xC8);
LCD_WR_DATA8(0x33);
LCD_WR_DATA8(0x33);
LCD_WR_REG(0xC9);
LCD_WR_DATA8(0x68);
LCD_WR_REG(0xCA);
LCD_WR_DATA8(0x69);
LCD_WR_REG(0xCB);
LCD_WR_DATA8(0x6a);
LCD_WR_REG(0xCC);
LCD_WR_DATA8(0x6b);
LCD_WR_REG(0xCD);
LCD_WR_DATA8(0x33);
LCD_WR_DATA8(0x33); 
LCD_WR_REG(0xCE);
LCD_WR_DATA8(0x6c);
LCD_WR_REG(0xCF);
LCD_WR_DATA8(0x6d);
LCD_WR_REG(0xD0);
LCD_WR_DATA8(0x6e);
LCD_WR_REG(0xD1);
LCD_WR_DATA8(0x6f);
LCD_WR_REG(0xAB);
LCD_WR_DATA8(0x03);
LCD_WR_DATA8(0x67);
LCD_WR_REG(0xAC);
LCD_WR_DATA8(0x03);
LCD_WR_DATA8(0x6b);
LCD_WR_REG(0xAD);
LCD_WR_DATA8(0x03);
LCD_WR_DATA8(0x68);
LCD_WR_REG(0xAE);
LCD_WR_DATA8(0x03);
LCD_WR_DATA8(0x6c);
LCD_WR_REG(0xb3);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xb4);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xb5);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xB6);
LCD_WR_DATA8(0x32);
LCD_WR_REG(0xB7);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xB8);
LCD_WR_DATA8(0x7e);
LCD_WR_REG(0xe0);
LCD_WR_DATA8(0x00);
LCD_WR_REG(0xe1);
LCD_WR_DATA8(0x03);
LCD_WR_DATA8(0x0f);
LCD_WR_REG(0xe2);
LCD_WR_DATA8(0x04);
LCD_WR_REG(0xe3);
LCD_WR_DATA8(0x01);
LCD_WR_REG(0xe4);
LCD_WR_DATA8(0x0e);
LCD_WR_REG(0xe5);
LCD_WR_DATA8(0x01);
LCD_WR_REG(0xe6);
LCD_WR_DATA8(0x19);
LCD_WR_REG(0xe7);
LCD_WR_DATA8(0x10);
LCD_WR_REG(0xe8);
LCD_WR_DATA8(0x10);
LCD_WR_REG(0xea);
LCD_WR_DATA8(0x12);
LCD_WR_REG(0xeb);
LCD_WR_DATA8(0xd0);
LCD_WR_REG(0xec);
LCD_WR_DATA8(0x04);
LCD_WR_REG(0xed);
LCD_WR_DATA8(0x07);
LCD_WR_REG(0xee);
LCD_WR_DATA8(0x07);
LCD_WR_REG(0xef);
LCD_WR_DATA8(0x09);
LCD_WR_REG(0xf0);
LCD_WR_DATA8(0xd0);
LCD_WR_REG(0xf1);
LCD_WR_DATA8(0x0e);

//LCD_WR_REG(0xF9);
//LCD_WR_DATA8(0x17); 
//LCD_WR_REG(0xf2);
//LCD_WR_DATA8(0x2e);
//LCD_WR_DATA8(0x1b);
//LCD_WR_DATA8(0x0b);
//LCD_WR_DATA8(0x20);
//LCD_WR_REG(0xF9);
LCD_WR_DATA8(0x17); 
LCD_WR_REG(0xf2);
LCD_WR_DATA8(0x2c);
LCD_WR_DATA8(0x1b);
LCD_WR_DATA8(0x0b);
LCD_WR_DATA8(0x20);
////1 dot
LCD_WR_REG(0xe9);
LCD_WR_DATA8(0x29);
LCD_WR_REG(0xec);
LCD_WR_DATA8(0x04);
//TE
LCD_WR_REG(0x35);
LCD_WR_DATA8(0x00); 
LCD_WR_REG(0x44);
LCD_WR_DATA8(0x00);
LCD_WR_DATA8(0x10); 
LCD_WR_REG(0x46);
LCD_WR_DATA8(0x10); 
LCD_WR_REG(0xff);
LCD_WR_DATA8(0x00); 
LCD_WR_REG(0x3a);
LCD_WR_DATA8(0x05); 
LCD_WR_REG(0x11); 
delay_ms(220); 
LCD_WR_REG(0x29); 
delay_ms(200);


} 




/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{          
	u16 i,j; 
	LCD_Address_Set(xsta,ysta,xend-1,yend-1);//设置显示范围
	for(i=ysta;i<yend;i++)
	{													   	 	
		for(j=xsta;j<xend;j++)
		{
			LCD_WR_DATA(color);
		}
	} 					  	    
}




/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标
                length 图片长度
                width  图片宽度
                pic[]  图片数组    
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(u16 x,u16 y,u16 length,u16 width,const u8 pic[])
{
	u16 i,j;
	u32 k=0;
	LCD_Address_Set(x,y,x+length-1,y+width-1);
	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			LCD_WR_DATA8(pic[k*2]);
			LCD_WR_DATA8(pic[k*2+1]);
			k++;
		}
	}			
}



void TFT_Page_bar()
{
	unsigned long color[8] = { 0xFFFF, 0x0000, 0x001F, 0XF81F, 0XFFE0, 0X07FF, 0xF800, 0x07E0 };
	
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围


			for(int j=0;j<7;j++)
			{
				for(int k=0;k<LCD_COL*53;k++)
				{
					LCD_WR_DATA(color[j]);
				}
			}

				for(int k=0;k<LCD_COL*57;k++)
				{
					LCD_WR_DATA(color[7]);
				}

}


void TFT_Page_bar2()
{
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
	
	int height = 10;

			for(int j=0;j<LCD_ROW;j=j+height)
			{
				for(int k=0;k<LCD_COL*height;k++)
				{
					LCD_WR_DATA8(gImage_gary[j+1]);
					LCD_WR_DATA8(gImage_gary[j]);
				}
			}
}


void TFT_Page_border()
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c, r;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围

	
	for(r=0;r<row;r++)
	{
		for(c=0;c<col;c++)
		{
			if(c==0 || r==0 || c == col-1 || r == row-1)
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
			//delay_ms(50);
		}
	}
}
	

void TFT_Page_border2()
{
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
	
	int radio = 50;
	
	int a,b,c,d;
	for(int i = 0; i<radio; i++)
	{
		for(int j = 0; j<LCD_COL; j++)
		{
			a = 0; b = radio - i; c = LCD_COL - 1 - radio + i; d = LCD_COL - 1;
			if( (i==a && j<=b) || (i==c && j>=d) )
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
		}
	}
}



/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_Address_Set(x,y,x,y);//设置光标位置 
	LCD_WR_DATA(color);
} 


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      返回值：  无
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color, int place)
{
	int a,b;
	a=0;b=r;	  
	while(a<=b)
	{
		if(place == 1)
		{
		LCD_DrawPoint(x0-b,y0-a,color);             //3       1    
		LCD_DrawPoint(x0-a,y0-b,color);             //2         2    
		}
		if(place == 2)
		{
		LCD_DrawPoint(x0+a,y0-b,color);             //5	3
		LCD_DrawPoint(x0+b,y0-a,color);             //0           4
		}
		if(place == 3)
		{
		LCD_DrawPoint(x0+b,y0+a,color);             //4               5
		LCD_DrawPoint(x0+a,y0+b,color);             //6 	6
		}
		if(place == 4)
		{
		LCD_DrawPoint(x0-a,y0+b,color);             //1                7
		LCD_DrawPoint(x0-b,y0+a,color);             //7	8
		}
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}



void TFT_Page_net()
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c, r;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
	
	for(c=0;c<col;c++)
	{
		for(r=0;r<row;r++)
		{
			if(r%2 == 0)
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
		}
	}
}	


void TFT_Page_cell()
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c = 0;
	int r = 0;

	int c2 = 0;
	int r2 = 0;

	int c3 = 0;
	int r3 = 0;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围

	for(c=0;c<col;c++)
	{
		for(r=0;r<row;r++)
		{
			c2 = (int)(c/40);
			r2 = (int)(r/43);
			c3 = c2 % 2;
			r3 = r2 % 2;

			if(c3 == r3){
				LCD_WR_DATA(0xFF);
			}else{
				LCD_WR_DATA(0x0000);
			}			
		}
	}
}
	
void Picture_display()
{
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);

	long i,j, a = 0;
		
		a = 0;
		for(i=0;i<LCD_COL;i++)
		{
			for(j=0;j<LCD_ROW;j++)
			{
//				LCD_Writ_Bus(gImage_color[a+1]);
//				LCD_Writ_Bus(gImage_color[a]);
				a=a+2;
			}
		}
	
}


int main(void)
{
	delay_ms(100);
	
	LCD_Init();
	
	int speeds = 3000;

	LED_GPIO_Config(); //LED 端口初始化   	
  Key_GPIO_Config();//按键端口初始化

	page = 0;
	
  while(1)
  {
		if( Key_Scan(GPIOB,GPIO_Pin_12) == KEY_OFF  )	 //判断KEY1是否按下
		{
			LED1( OFF );
			page++;
			if(page > 5){ page = 1; }

			if(page == 1){
				LCD_Fill(0,0,LCD_COL,LCD_ROW,BLACK);
			}else if(page == 2){
				LCD_Fill(0,0,LCD_COL,LCD_ROW,RED);
			}else if(page == 3){
				LCD_Fill(0,0,LCD_COL,LCD_ROW,WHITE);
			}else if(page == 4){
				TFT_Page_border();
			}else if(page == 5){
				TFT_Page_bar();
			}else if(page == 6){
				TFT_Page_bar2();
			}else if(page == 7){
				LCD_Fill(0,0,LCD_COL,LCD_ROW,GREEN);
				LCD_Fill(0,0,LCD_COL,LCD_ROW,BLUE);
			}else if(page == 8){
				//Picture_display();
			}else{
				page = 1;
			}
			delay_ms(speeds);
		}else{
			LED1( ON );
		}
	}		

}



