/*
 * lcd.c
 *
 *  Created on: 2026年5月16日
 *      Author: kout
 */
#include "lcd.h"
#include "stdlib.h"
#include "font.h"
#include "text.h"

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;
//////////////////////////////////////////////////////////////////////////////////

//2.4寸/2.8寸  LCD 液晶驱动


//********************************************************************************

//////////////////////////////////////////////////////////////////////////////////

//LCD的画笔颜色和背景色

u16 POINT_COLOR=0x0000; //画笔颜色
u16 BACK_COLOR=0xFFFF;  //背景色



//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;


//本驱动程序使用的是MCU硬件SPI接口驱动


/******************************************************************************

  接口定义，请根据接线修改并修改相应IO初始化--驱动板子和IO要对应

    #define LCD_SDA         //PB15      //数据输入线
    #define LCD_SCL         //PB13      //时钟线
    #define LCD_CS          //PB12      //片选
    #define LCD_SDO/RST     //PB14      //数据输出/复位
    #define LCD_RS          //PC5       //命令/数据切换
    #define LCD_BLK         //PB1         //背光控制


*******************************************************************************/


/******************************************************************************/

//函数： void SPI2_Init(void)

//函数功能：以下是 硬件SPI模块的初始化代码，配置成主机模式
//SPI口初始化
//这里针是对SPI2的初始化

/******************************************************************************/


void SPI2_Init(void)
{
    /* 软件SPI不需要初始化硬件SPI外设
       这里只设置GPIO默认状态 */

    LCD_CS_SET;      // 片选拉高
    LCD_SCL_SET;     // 时钟空闲高电平，对应 CPOL=1
    LCD_SDA_SET;     // 数据线默认拉高
}



/******************************************************************************/

//函数： void LCD_GPIO_Init(void)

//函数功能：驱动液晶IO初始化配置
//  #define LCD_SDA         //601      //数据输入线
//  #define LCD_SCL         //602      //时钟线
//  #define LCD_CS          //603      //片选
//  #define LCD_SDO/RST     //604      //数据输出/复位
//  #define LCD_RS          //605       //命令/数据切换
//  #define LCD_BLK         //715         //背光控制

/******************************************************************************/
void LCD_GPIO_Init(void)
{
    /* 如果工程启动流程中已经打开了 ioport，这里可不重复打开
       否则需要确认系统里执行过 R_IOPORT_Open() */

    SPI2_Init();          // 软件SPI初始化（设置默认电平）

    LCD_RST_SET;
    LCD_CS_SET;
    LCD_RS_SET;
    LCD_BLK_CLR;          // 先关背光也行，看你的需求

    LCD_HardwareRest();   // 硬复位

    LCD_BLK_On;           // 开启背光
}

static void LCD_SPI_Delay(void)
{
    /* 原1us延时导致SPI时钟仅~330kHz, 刷新一帧需~4秒.
     * 改为短NOP循环, SPI时钟提升到~5-10MHz, 刷新<0.5秒 */
    for (volatile uint32_t d = 0; d < 3; d++) {
        __NOP();
    }
}


/********************************************************************/
//函数：u8 SPI2_ReadWriteByte(u8 TxData)
//函数功能： SPI读写函数 ： 使用操作寄存器方式，速度更快

//SPI1 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节





//*******************************************************************/

u8 SPI2_ReadWriteByte(u8 TxData)
{
    u8 i;

    for (i = 0; i < 8; i++)
    {
        LCD_SCL_CLR;
        LCD_SPI_Delay();

        if (TxData & 0x80)
        {
            LCD_SDA_SET;
        }
        else
        {
            LCD_SDA_CLR;
        }

        LCD_SPI_Delay();
        TxData <<= 1;

        LCD_SCL_SET;
        LCD_SPI_Delay();
    }

    return 0;
}


//********************************************************************/

//函数：void LCD_WR_REG(u16 regval)
//函数功能：
//写寄存器函数
//regval:寄存器值




//*******************************************************************/

void LCD_WR_REG(u16 regval)
{
    LCD_CS_CLR;
    LCD_RS_CLR;
    SPI2_ReadWriteByte((u8)(regval & 0x00FF));
    LCD_CS_SET;
}



//*******************************************************************/

//函数：void LCD_WR_DATA8(u8 data)   //写8位数据
//函数功能：
//液晶屏--写8位数据函数




//*******************************************************************/


void LCD_WR_DATA8(u8 data)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI2_ReadWriteByte(data);
    LCD_CS_SET;
}



//*******************************************************************/
//函数：void LCD_WR_DATA16(u16 data)   //写16位数据
//函数功能：写LCD数据
//输入参数：
//data:要写入的值




//*******************************************************************/


void LCD_WR_DATA16(u16 data)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI2_ReadWriteByte((u8)(data >> 8));
    SPI2_ReadWriteByte((u8)data);
    LCD_CS_SET;
}



//*******************************************************************/

//函数：void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)  //写液晶屏寄存器

//函数功能：写寄存器

//输入参数:
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据




//*******************************************************************/

void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{
    LCD_WR_REG(LCD_Reg);           //写入要写的寄存器序号
    LCD_WR_DATA8((u8)LCD_RegValue);//写入数据
}


//*******************************************************************/
//函数：void LCD_WriteRAM_Prepare(void)
//函数功能：开始写GRAM  命令




//*******************************************************************/

void LCD_WriteRAM_Prepare(void)
{
    LCD_WR_REG(lcddev.wramcmd);
}



//*******************************************************************/
//函数：LCD_WriteRAM(u16 RGB_Code)
//函数功能：写入点阵颜色值
//输入参数:
//RGB_Code:颜色值




//*******************************************************************/

void LCD_WriteRAM(u16 RGB_Code)
{
    LCD_WR_DATA16( RGB_Code );//写十六位GRAM
}





//********************************************************************************/
//函数：void LCD_DisplayOn(void)
//函数功能：
//LCD开启显示




//*******************************************************************/
void LCD_DisplayOn(void)
{
    LCD_WR_REG(0X29);   //开启显示
}


//*******************************************************************/
//函数：void LCD_DisplayOff(void)
//函数功能：
//LCD关闭显示




//*******************************************************************/

void LCD_DisplayOff(void)
{
    LCD_WR_REG(0X28);   //关闭显示

}

//********************************************************************************/
//函数：void LCD_SoftRest(void)
//函数功能：给屏幕发命令，执行软复位命令
//LCD开启显示




//*******************************************************************/
void LCD_SoftRest(void)
{
    LCD_WR_REG(0x01);   //发送软复位命令
    LCD_Delay_ms(50);      // delay 50 ms
}


//********************************************************************************/
//函数：void LCD_SoftRest(void)
//函数功能：给屏幕发命令，执行软复位命令
//LCD开启显示




//*******************************************************************/
// ✅ 正确
void LCD_HardwareRest(void)
{
    LCD_RST_SET;            // 先确保RST为高
    LCD_Delay_ms(10);
    LCD_RST_CLR;            // 拉低复位
    LCD_Delay_ms(50);
    LCD_RST_SET;            // 拉高，结束复位  ← 这里必须是 RST 不是 RS
    LCD_Delay_ms(120);      // ILI9341 复位后需要等 120ms
}



//*******************************************************************/
//函数：void LCD_SetCursor(u16 Xpos, u16 Ypos)
//函数功能：设置光标位置
//输入参数：
//Xpos:横坐标
//Ypos:纵坐标




//*******************************************************************/
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{

        LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA16(Xpos);

        LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA16(Ypos);


}





//*******************************************************************/
//函数：void LCD_DrawPoint(u16 x,u16 y)
//函数功能：画点
//输入参数：
//x,y:坐标
//POINT_COLOR:此点的颜色




//*******************************************************************/

void LCD_DrawPoint(u16 x,u16 y)
{
    LCD_SetCursor(x,y);     //设置光标位置
    LCD_WriteRAM_Prepare(); //开始写入GRAM
    LCD_WR_DATA16(POINT_COLOR);
}


//*******************************************************************/
//函数：void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)
//函数功能：快速画点
//输入参数：
//x,y:坐标
//color:颜色




//*******************************************************************/

void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)
{
        LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA16(x);

    LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA16(y);

      LCD_WR_REG(lcddev.wramcmd);
      LCD_WR_DATA16(color); //写入16位颜色
}



//*******************************************************************/
//函数：void LCD_Scan_Dir(u8 dir)
//函数功能：设置LCD的自动扫描方向
//输入参数：

//默认设置为L2R_U2D,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向




//*******************************************************************/

void LCD_Scan_Dir(u8 dir)
{
    u16 regval=0;

    //扫描方向定义--扫描方式有不同规格，可能定义不左右和上下的参照方向不同，总结方式，只有一下八种

    switch(dir)
        {
            case L2R_U2D://从左到右,从上到下
                regval|=(0<<7)|(0<<6)|(0<<5);
                break;
            case L2R_D2U://从左到右,从下到上
                regval|=(1<<7)|(0<<6)|(0<<5);
                break;
            case R2L_U2D://从右到左,从上到下
                regval|=(0<<7)|(1<<6)|(0<<5);
                break;
            case R2L_D2U://从右到左,从下到上
                regval|=(1<<7)|(1<<6)|(0<<5);
                break;
            case U2D_L2R://从上到下,从左到右
                regval|=(0<<7)|(0<<6)|(1<<5);
                break;
            case U2D_R2L://从上到下,从右到左
                regval|=(0<<7)|(1<<6)|(1<<5);
                break;
            case D2U_L2R://从下到上,从左到右
                regval|=(1<<7)|(0<<6)|(1<<5);
                break;
            case D2U_R2L://从下到上,从右到左
                regval|=(1<<7)|(1<<6)|(1<<5);
                break;
        }



        LCD_WriteReg(0x36, regval);  //扫描方向


}




/**************************************************************************/
//函数：void LCD_Display_Dir(u8 dir)
//函数功能：设置LCD的显示方向及像素参数

//输入参数：

//设置LCD显示方向

////dir:   0,竖屏  正
//         1,竖屏  反
//         2,横屏  左
//         3,横屏  右




//*************************************************************************/
void LCD_Display_Dir(u8 dir)
{

    u8 SCAN_DIR;

    if(dir==0)               //竖屏  正
    {
        lcddev.dir=0;        //竖屏
        lcddev.width=240;
        lcddev.height=320;

        lcddev.wramcmd=0X2C;
        lcddev.setxcmd=0X2A;
        lcddev.setycmd=0X2B;

    SCAN_DIR=L2R_U2D; //选择扫描方向

    }

else if (dir==1)             //竖屏  反
    {
        lcddev.dir=0;        //竖屏
        lcddev.width=240;
        lcddev.height=320;

        lcddev.wramcmd=0X2C;
        lcddev.setxcmd=0X2A;
        lcddev.setycmd=0X2B;

    SCAN_DIR=R2L_D2U; //选择扫描方向
    }


    else if (dir==2)            //横屏
    {
        lcddev.dir=1;        //横屏
        lcddev.width=320;
        lcddev.height=240;

        lcddev.wramcmd=0X2C;
        lcddev.setxcmd=0X2A;
        lcddev.setycmd=0X2B;

    SCAN_DIR=U2D_R2L; //选择扫描方向

    }
 else if (dir==3)                 //横屏
    {
        lcddev.dir=1;           //横屏
        lcddev.width=320;
        lcddev.height=240;

        lcddev.wramcmd=0X2C;
        lcddev.setxcmd=0X2A;
        lcddev.setycmd=0X2B;

    SCAN_DIR=D2U_L2R; //选择扫描方向

    }
 else //设置默认为竖屏--正
 {
      lcddev.dir=0;      //竖屏
        lcddev.width=240;
        lcddev.height=320;

        lcddev.wramcmd=0X2C;
        lcddev.setxcmd=0X2A;
        lcddev.setycmd=0X2B;

    SCAN_DIR=L2R_U2D; //选择扫描方向

 }


//////以下设置，为窗口参数设置，设置了全屏的显示范围

            LCD_WR_REG(lcddev.setxcmd);

      LCD_WR_DATA16(0);
      LCD_WR_DATA16(lcddev.width-1);

            LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA16(0);
      LCD_WR_DATA16(lcddev.height-1);


 /////设置屏幕显示--扫描方向

       LCD_Scan_Dir(SCAN_DIR);  //设置屏幕显示--扫描方向

}


/**************************************************************************/
//函数：void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
//函数功能：设置LCD的显示窗口

//设置窗口,并自动设置画点坐标到窗口左上角(sx,sy).
//sx,sy:窗口起始坐标(左上角)
//width,height:窗口宽度和高度,必须大于0!!
//窗体大小:width*height.




//*************************************************************************/

void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{


      width=sx+width-1;
      height=sy+height-1;

        LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA16(sx);      //设置 X方向起点
    LCD_WR_DATA16(width);   //设置 X方向终点

        LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA16(sy);      //设置 Y方向起点
    LCD_WR_DATA16(height);  //设置 Y方向终点


}


/*******************************************************************************/
//函数：void LCD_Init(void)
//函数功能：初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶!!!




/*******************************************************************************/

void LCD_Init(void)
{

    LCD_GPIO_Init();        //初始化驱动 I/O接口

      LCD_SoftRest();        //软复位

      LCD_Delay_ms(50);      // delay 50 ms

        LCD_WR_REG(0xCF);
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0xD9);
        LCD_WR_DATA8(0X30);

        LCD_WR_REG(0xED);
        LCD_WR_DATA8(0x64);
        LCD_WR_DATA8(0x03);
        LCD_WR_DATA8(0X12);
        LCD_WR_DATA8(0X81);

        LCD_WR_REG(0xE8);
        LCD_WR_DATA8(0x85);
        LCD_WR_DATA8(0x10);
        LCD_WR_DATA8(0x78);

        LCD_WR_REG(0xCB);
        LCD_WR_DATA8(0x39);
        LCD_WR_DATA8(0x2C);
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0x34);
        LCD_WR_DATA8(0x02);

        LCD_WR_REG(0xF7);
        LCD_WR_DATA8(0x20);

        LCD_WR_REG(0xEA);
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0x00);

        LCD_WR_REG(0xC0);     //Power control
        LCD_WR_DATA8(0x21);   //VRH[5:0]

        LCD_WR_REG(0xC1);     //Power control
        LCD_WR_DATA8(0x12);   //SAP[2:0];BT[3:0]

        LCD_WR_REG(0xC5);      //VCM control
        LCD_WR_DATA8(0x32);      //3F
        LCD_WR_DATA8(0x3C);      //3C

        LCD_WR_REG(0xC7);     //VCM control2
        LCD_WR_DATA8(0xC1);

        LCD_WR_REG(0x36);     // Memory Access Control
        LCD_WR_DATA8(0x00);

        LCD_WR_REG(0x3A);
        LCD_WR_DATA8(0x55);

        LCD_WR_REG(0xB1);     //帧速率控制
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0x18);

        LCD_WR_REG(0xB6);    // Display Function Control
        LCD_WR_DATA8(0x0A);
        LCD_WR_DATA8(0xA2);

        LCD_WR_REG(0xF2);    // 3Gamma Function Disable
        LCD_WR_DATA8(0x00);

        LCD_WR_REG(0x26);    //Gamma curve selected
        LCD_WR_DATA8(0x01);

        LCD_WR_REG(0xE0);    //Set Gamma
        LCD_WR_DATA8(0x0F);
        LCD_WR_DATA8(0x20);
        LCD_WR_DATA8(0x1E);
        LCD_WR_DATA8(0x09);
        LCD_WR_DATA8(0x12);
        LCD_WR_DATA8(0x0B);
        LCD_WR_DATA8(0x50);
        LCD_WR_DATA8(0xBA);
        LCD_WR_DATA8(0x44);
        LCD_WR_DATA8(0x09);
        LCD_WR_DATA8(0x14);
        LCD_WR_DATA8(0x05);
        LCD_WR_DATA8(0x23);
        LCD_WR_DATA8(0x21);
        LCD_WR_DATA8(0x00);


        LCD_WR_REG(0xE1);    //Set Gamma
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0x19);
        LCD_WR_DATA8(0x19);
        LCD_WR_DATA8(0x00);
        LCD_WR_DATA8(0x12);
        LCD_WR_DATA8(0x07);
        LCD_WR_DATA8(0x2D);
        LCD_WR_DATA8(0x28);
        LCD_WR_DATA8(0x3F);
        LCD_WR_DATA8(0x02);
        LCD_WR_DATA8(0x0A);
        LCD_WR_DATA8(0x08);
        LCD_WR_DATA8(0x25);
        LCD_WR_DATA8(0x2D);
        LCD_WR_DATA8(0x0F);


        LCD_WR_REG(0x11); //Exit Sleep

        LCD_Delay_ms(120);

        LCD_WR_REG(0x29); //display on

      LCD_Display_Dir(LCD_DIR_Mode);    //选择--屏幕显示方式

      LCD_BLK_On;                   //点亮背光

      LCD_Clear(WHITE);



}


//////////////////以下函数是屏幕显示图形的简单驱动函数/////////////////////////////////////////////////



/*******************************************************************************/
//函数：void LCD_Clear(u16 color)
//函数功能：全屏清屏填充函数
//输入参数：
//color:要清屏的填充色




/*******************************************************************************/

void LCD_Clear(u16 color)
{
    u32 index=0;
    u32 totalpoint;

    LCD_Set_Window(0,0,lcddev.width,lcddev.height);//设置全屏窗口

    totalpoint=lcddev.width * lcddev.height;            //得到总点数

    LCD_SetCursor(0x00,0x00);   //设置光标位置

    LCD_WriteRAM_Prepare();             //开始写入GRAM

    for(index=0;index<totalpoint;index++)
    {
        LCD_WR_DATA16(color);
    }
}


/*******************************************************************************/
//函数：void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)

//函数功能：区域填充函数--填充单个颜色
//输入参数：
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色




/*******************************************************************************/

void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{
        u32 i;
        u32 xlen=0;

 //设置窗口
        LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA16(sx);//设置 X方向起点
      LCD_WR_DATA16(ex-1);//设置 X方向终点


        LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA16(sy);//设置 Y方向起点
      LCD_WR_DATA16(ey-1);//设置 Y方向终点


      LCD_WR_REG(lcddev.wramcmd);

        xlen=(ex-sx)*(ey-sy);//计算出总共需要写入的点数

        LCD_WriteRAM_Prepare();                 //开始写入GRAM

        for(i=0;i<xlen;i++)
        {
            LCD_WR_DATA16(color);   //显示颜色
        }


//恢复全屏窗口

//以下设置，为窗口参数设置，设置了全屏的显示范围

            LCD_WR_REG(lcddev.setxcmd);
            LCD_WR_DATA16(0);             //设置 X方向起点
            LCD_WR_DATA16(lcddev.width-1);//设置 X方向终点

            LCD_WR_REG(lcddev.setycmd);
            LCD_WR_DATA16(0);              //设置 Y方向起点
            LCD_WR_DATA16(lcddev.height-1);//设置 Y方向终点

}


/*******************************************************************************/
//函数：void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)

//函数功能：区域填充函数
//输入参数：
//在指定区域内填充指定颜色块
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)
//color:要填充的颜色




/*******************************************************************************/

void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
{

        u32 i;
        u32 xlen=0;

 //设置窗口
        LCD_WR_REG(lcddev.setxcmd);
      LCD_WR_DATA16(sx);//设置 X方向起点
      LCD_WR_DATA16(ex-1);//设置 X方向终点

        LCD_WR_REG(lcddev.setycmd);
      LCD_WR_DATA16(sy);//设置 Y方向起点
      LCD_WR_DATA16(ey-1);//设置 Y方向终点

      LCD_WR_REG(lcddev.wramcmd);

        xlen=(ex-sx)*(ey-sy);//计算出总共需要写入的点数

        LCD_WriteRAM_Prepare();                 //开始写入GRAM

        for(i=0;i<xlen;i++)
        {
            LCD_WR_DATA16(*color);  //显示颜色
        }


//恢复全屏窗口

//以下设置，为窗口参数设置，设置了全屏的显示范围

            LCD_WR_REG(lcddev.setxcmd);
            LCD_WR_DATA16(0);             //设置 X方向起点
            LCD_WR_DATA16(lcddev.width-1);//设置 X方向终点

            LCD_WR_REG(lcddev.setycmd);
            LCD_WR_DATA16(0);              //设置 Y方向起点
            LCD_WR_DATA16(lcddev.height-1);//设置 Y方向终点


}



/*******************************************************************************/
//函数：void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
//函数功能：画线
//输入参数：
//x1,y1:起点坐标
//x2,y2:终点坐标
//Color;线条颜色




/*******************************************************************************/

void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 Color)
{
    u16 t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;

    delta_x=x2-x1; //计算坐标增量
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;


    if(delta_x>0)incx=1; //设置单步方向
    else if(delta_x==0)incx=0;//垂直线
    else
     {
            incx=-1;
            delta_x=-delta_x;
         }


    if(delta_y>0)incy=1;
    else if(delta_y==0)incy=0;//水平线
    else
     {
           incy=-1;
           delta_y=-delta_y;
         }


    if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
    else distance=delta_y;

    for(t=0;t<=distance;t++ )//画线输出
    {
        LCD_Fast_DrawPoint(uRow,uCol,Color);//画点 --使用输入颜色参数
        xerr+=delta_x ;
        yerr+=delta_y ;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}

/*******************************************************************************/
//函数：void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
//函数功能：画矩形
//输入参数：
//(x1,y1),(x2,y2):矩形的对角坐标
//Color;线条颜色




/*******************************************************************************/

void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 Color)
{
    LCD_DrawLine(x1,y1,x2,y1,Color);
    LCD_DrawLine(x1,y1,x1,y2,Color);
    LCD_DrawLine(x1,y2,x2,y2,Color);
    LCD_DrawLine(x2,y1,x2,y2,Color);
}

/*******************************************************************************/
//函数：void LCD_Draw_Circle(u16 x0,u16 y0,u8 r)
//函数功能：在指定位置画一个指定大小的圆
//输入参数：
//(x,y):中心点
//r    :半径
//Color;线条颜色




/*******************************************************************************/

void LCD_Draw_Circle(u16 x0,u16 y0,u8 r, u16 Color)
{
    int a,b;
    int di;
    a=0;b=r;
    di=3-(r<<1);             //判断下个点位置的标志
    while(a<=b)
    {
        LCD_Fast_DrawPoint(x0+a,y0-b,Color);             //5
        LCD_Fast_DrawPoint(x0+b,y0-a,Color);             //0
        LCD_Fast_DrawPoint(x0+b,y0+a,Color);             //4
        LCD_Fast_DrawPoint(x0+a,y0+b,Color);             //6
        LCD_Fast_DrawPoint(x0-a,y0+b,Color);             //1
        LCD_Fast_DrawPoint(x0-b,y0+a,Color);
        LCD_Fast_DrawPoint(x0-a,y0-b,Color);             //2
    LCD_Fast_DrawPoint(x0-b,y0-a,Color);             //7
        a++;
        //使用Bresenham算法画圆
        if(di<0)di +=4*a+6;
        else
        {
            di+=10+4*(a-b);
            b--;
        }
    }
}


/*******************************************************************************/
//函数：LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u16 color,u8 mode)
//函数功能：在指定位置显示一个字符
//输入参数：
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//color,字符颜色
//mode:叠加方式(1)还是非叠加方式(0)




/*******************************************************************************/

void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u16 color,u8 mode)
{
  u8 temp,t1,t;
    u16 y0=y;
    u8 csize=(size/8+((size%8)?1:0))*(size/2);      //得到字体一个字符对应点阵集所占的字节数
    //设置窗口
    num=num-' ';//得到偏移后的值
    for(t=0;t<csize;t++)
    {
        if(size==12)temp=asc2_1206[num][t];         //调用1206字体
        else if(size==16)temp=asc2_1608[num][t];    //调用1608字体
        else if(size==24)temp=asc2_2412[num][t];    //调用2412字体
        else return;                                //没有的字库
        for(t1=0;t1<8;t1++)
        {
            if(temp&0x80)LCD_Fast_DrawPoint(x,y,color);
            else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);

            temp<<=1;
            y++;
            if(y>=lcddev.height)return;     //超区域了
            if((y-y0)==size)
            {
                y=y0;
                x++;
                if(x>=lcddev.width)return;  //超区域了
                break;
            }
        }
    }
}



/*******************************************************************************/
//函数：u32 LCD_Pow(u8 m,u8 n)
//函数功能：m^n函数
//输入参数：两个8位数据
//返回值:m^n次方.




/*******************************************************************************/
u32 LCD_Pow(u8 m,u8 n)
{
    u32 result=1;
    while(n--)result*=m;
    return result;
}


/*******************************************************************************/
//函数：void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color)
//函数功能：显示数字,高位为0,则不显示
//输入参数：

//x,y :起点坐标
//num:数值(0~4294967295);
//len :数字的位数
//size:字体大小
//color:颜色


//返回值:无




/*******************************************************************************/

void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color)
{
    u8 t,temp;
    u8 enshow=0;
    for(t=0;t<len;t++)
    {
        temp=(num/LCD_Pow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                LCD_ShowChar(x+(size/2)*t,y,' ',size,color,0);
                continue;
            }else enshow=1;

        }
        LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,color,0);
    }
}



/*******************************************************************************/
//函数：void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color,u8 mode)
//函数功能：显示数字,高位为0,还是显示
//输入参数：
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);
//len:长度(即要显示的位数)
//size:字体大小
//color:颜色
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.




/*******************************************************************************/

void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u16 color,u8 mode)
{
    u8 t,temp;
    u8 enshow=0;
    for(t=0;t<len;t++)
    {
        temp=(num/LCD_Pow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,color,mode&0X01);
                else LCD_ShowChar(x+(size/2)*t,y,' ',size,color,mode&0X01);
                continue;
            }else enshow=1;

        }
        LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,color,mode&0X01);
    }
}

/*******************************************************************************/
//函数：void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u16 color,u8 *p)
//函数功能：显示字符串
//输入参数：
//x,y:起点坐标
//width,height:区域大小
//size:字体大小
//color:颜色
//*p:字符串起始地址




/*******************************************************************************/


void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u16 color,u8 *p)
{
    u8 x0=x;
    width+=x;
    height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        LCD_ShowChar(x,y,*p,size,color,0);
        x+=size/2;
        p++;
    }
}







//****************************************************************************************/
//函数：void DisplayButtonDown(u16 x1,u16 y1,u16 x2,u16 y2)
//功能描述: 在屏幕显示一凸起的按钮框
//输    入: u16 x1,y1,x2,y2 按钮框左上角和右下角坐标
//输    出: 无





/****************************************************************************************/

void DisplayButtonDown(u16 x1,u16 y1,u16 x2,u16 y2)
{

    LCD_DrawLine(x1,y1, x2,y1,GRAY2);  //H

    LCD_DrawLine(x1+1,y1+1,x2,y1+1,GRAY1);  //H

    LCD_DrawLine(x1,  y1,  x1,y2,GRAY2);    //V

    LCD_DrawLine(x1+1,y1+1,x1+1,y2,GRAY1);  //V

    LCD_DrawLine(x1, y2, x2, y2,WHITE);     //H
    LCD_DrawLine(x2, y1, x2, y2,WHITE);     //V
}

//****************************************************************************************/
//函数：void DisplayButtonUp(u16 x1,u16 y1,u16 x2,u16 y2)
//功能描述: 在屏幕显示一凹下的按钮框
//输    入: u16 x1,y1,x2,y2 按钮框左上角和右下角坐标
//输    出: 无



/****************************************************************************************/
void DisplayButtonUp(u16 x1,u16 y1,u16 x2,u16 y2)
{

    LCD_DrawLine(x1,  y1,  x2,y1,WHITE);    //H
    LCD_DrawLine(x1,  y1,  x1,y2,WHITE);    //V

    LCD_DrawLine(x1+1,y2-1,x2,y2-1,GRAY1);  //H

    LCD_DrawLine(x1,  y2,  x2,y2,GRAY2);    //H

    LCD_DrawLine(x2-1,y1+1,x2-1,y2,GRAY1);  //V

  LCD_DrawLine(x2  ,y1  ,x2,y2,GRAY2);    //V


}



/*******************************************************************************/
//函数：void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, u16 *image)
//函数功能：在指定位置显示图像
//输入参数：
//x,y: 图像起始坐标（左上角）
//width,height: 图像宽度和高度
//image: 图像数据缓冲区指针（RGB565格式）
/*******************************************************************************/
void LCD_ShowImage(u16 x, u16 y, u16 width, u16 height, u16 *image)
{
    u32 i;
    u32 total_pixels = (u32)width * height;

    // 1. 设置显示窗口
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA16(x);
    LCD_WR_DATA16(x + width - 1);

    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA16(y);
    LCD_WR_DATA16(y + height - 1);

    // 2. 发送内存写命令（之后一直保持 CS 低）
    LCD_WR_REG(lcddev.wramcmd);     // 这个函数内部会短暂拉低 CS 再拉高，但没关系，
                                     // 命令已进入控制器，我们马上再次拉低 CS

    // 3. 手动控制 CS 和 RS，一次性连续发送所有像素数据
    LCD_CS_CLR;                     // 拉低 CS
    LCD_RS_SET;                     // RS 为高，表示数据

    for(i = 0; i < total_pixels; i += 2)
    {
        /* YVYU 格式: Y在低字节, V在高字节(前), U在高字节(后) */
        int y0  = image[i]     & 0xFF;
        int v   = (image[i]   >> 8) & 0xFF;
        int y1  = image[i+1]   & 0xFF;
        int u   = (image[i+1] >> 8) & 0xFF;

        /* ITU-R BT.601 YUV→RGB 定点公式 */
        int c0 = y0 - 16;
        int c1 = y1 - 16;
        int d  = u - 128;
        int e  = v - 128;

        int r0 = (298 * c0 + 409 * e + 128) >> 8;
        int g0 = (298 * c0 - 100 * d - 208 * e + 128) >> 8;
        int b0 = (298 * c0 + 516 * d + 128) >> 8;
        int r1 = (298 * c1 + 409 * e + 128) >> 8;
        int g1 = (298 * c1 - 100 * d - 208 * e + 128) >> 8;
        int b1 = (298 * c1 + 516 * d + 128) >> 8;

        /* clamp [0,255] */
        if(r0 < 0) r0 = 0; if(r0 > 255) r0 = 255;
        if(g0 < 0) g0 = 0; if(g0 > 255) g0 = 255;
        if(b0 < 0) b0 = 0; if(b0 > 255) b0 = 255;
        if(r1 < 0) r1 = 0; if(r1 > 255) r1 = 255;
        if(g1 < 0) g1 = 0; if(g1 > 255) g1 = 255;
        if(b1 < 0) b1 = 0; if(b1 > 255) b1 = 255;

        /* 打包为 RGB565 (B→R, R→G, G→B 通道校正) */
        u16 px0 = (u16)(((b0 >> 3) << 11) | ((r0 >> 3) << 5) | (g0 >> 2));
        u16 px1 = (u16)(((b1 >> 3) << 11) | ((r1 >> 3) << 5) | (g1 >> 2));

        SPI2_ReadWriteByte((u8)(px0 >> 8));
        SPI2_ReadWriteByte((u8)(px0));
        SPI2_ReadWriteByte((u8)(px1 >> 8));
        SPI2_ReadWriteByte((u8)(px1));
    }

    LCD_CS_SET;                     // 全部数据发完，拉高 CS

    // 4. 恢复全屏窗口（防止影响后续绘图）
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA16(0);
    LCD_WR_DATA16(lcddev.width - 1);

    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA16(0);
    LCD_WR_DATA16(lcddev.height - 1);
}






