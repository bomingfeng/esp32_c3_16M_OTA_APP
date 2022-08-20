/***********************************************************************************************
文件名	SegDynamicScan.c
日期	2014.4.13
作者	张诗星	陈东辉
修订	2014.4.13
文件说明	LED数码管动态扫描支持
修订说明	初始版本
2014.04.13	张诗星	初始版本
***********************************************************************************************/
/*头文件--------------------------------------------------------------------------------------*/
#include "Seg7Font.h"
#include "SegDynamicScan.h"

//#include ""


/*全局变量------------------------------------------------------------------------------------*/
//显示缓冲区
unsigned char segDisBuff[SEG_DIGIT_NUM];

const unsigned char SegDigCode[10]=
{SEG7_CODE_0,SEG7_CODE_1,SEG7_CODE_2,SEG7_CODE_3,SEG7_CODE_4,
SEG7_CODE_5,SEG7_CODE_6,SEG7_CODE_7,SEG7_CODE_8,SEG7_CODE_9};
/*接口函数------------------------------------------------------------------------------------*/
//有8位口的CPU可以用宏定义此函数
void SegDyn_OutputData(unsigned char Index)					
{	
	//段控制
	//P1 = segDisBuff[Index];	
	
	gpio_set_level(17,segDisBuff[Index] & 0x01);
	gpio_set_level(4,segDisBuff[Index] & 0x02);
	gpio_set_level(16,segDisBuff[Index] & 0x04);
	gpio_set_level(32,segDisBuff[Index] & 0x08);
	gpio_set_level(33,segDisBuff[Index] & 0x10);
	gpio_set_level(14,segDisBuff[Index] & 0x20);
	gpio_set_level(12,segDisBuff[Index] & 0x40);
	gpio_set_level(26,segDisBuff[Index] & 0x80);
	
	
/*
	gpio_set_level(17,1);//a
	gpio_set_level(16,1);//c
	gpio_set_level(4,1);//b
	gpio_set_level(12,1);//g
	gpio_set_level(14,1);//f
	gpio_set_level(32,1);//d
	gpio_set_level(33,1);//E
	gpio_set_level(26,1);//DP
*/
	//位控制
	//P0 = ~(1<<Index);		
	switch (Index)	
	{
		case 0:
			gpio_set_level(21,1);gpio_set_level(19,0);gpio_set_level(18,0);gpio_set_level(5,0);
			break;
		case 1:
			gpio_set_level(21,0);gpio_set_level(19,1);gpio_set_level(18,0);gpio_set_level(5,0);
			break;
		case 2:
			gpio_set_level(21,0);gpio_set_level(19,0);gpio_set_level(18,0);gpio_set_level(5,1);
			break;
		case 3:
			gpio_set_level(21,0);gpio_set_level(19,0);gpio_set_level(18,1);gpio_set_level(5,0);
			break;
		case 4:
		
			break;
		case 5:
		
			break;
		case 6:
		
			break;
		case 7:

			break;
		default:

			break;
	}
}


//用户接口
void SegDynamicScan(void)
{
	static unsigned char scanIndex=0;		

	//消隐
	SegDyn_Hidden();
	//输出数据
	SegDyn_OutputData(scanIndex);

	if (++scanIndex>(SEG_DIGIT_NUM-1))
		scanIndex=0;
}