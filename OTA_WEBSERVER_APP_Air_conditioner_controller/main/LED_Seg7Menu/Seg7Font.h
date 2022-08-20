/***********************************************************************************************
文件名	Seg7Font.h
日期	2014.4.13
作者	张诗星	陈东辉
修订	2014.4.13
文件说明	LED数码管取模支持
修订说明	初始版本
2014.04.13	张诗星	初始版本
***********************************************************************************************/
#ifndef _LIB_SEG7_FONT_H_
#define _LIB_SEG7_FONT_H_

/*头文件--------------------------------------------------------------------------------------*/

//#include ""

/*用户配置------------------------------------------------------------------------------------*/

//共阳极性配置
#define SEG7_01_COM_ANODE		0	/*0共阴 1共阳*/

//7段数码管引脚配置
#define SEG7_IO_A		0			/*段A*/
#define SEG7_IO_B		1			/*段B*/
#define SEG7_IO_C		2			/*段C*/
#define SEG7_IO_D		3			/*段D*/
#define SEG7_IO_E		4			/*段E*/
#define SEG7_IO_F		5			/*段F*/
#define SEG7_IO_G		6			/*段G*/
#define SEG7_IO_DP		7			/*段DP*/

#define SEG7_IO_A_Rev		3			/*段A*/
#define SEG7_IO_B_Rev		4			/*段B*/
#define SEG7_IO_C_Rev		5			/*段C*/
#define SEG7_IO_D_Rev		0			/*段D*/
#define SEG7_IO_E_Rev		1			/*段E*/
#define SEG7_IO_F_Rev		2			/*段F*/
#define SEG7_IO_G_Rev		7			/*段G*/
#define SEG7_IO_DP_Rev		6			/*段DP*/
/*宏常量--------------------------------------------------------------------------------------*/

//字模定义
#if SEG7_01_COM_ANODE==0	/*共阴*/
#define SEG7_CODE_NULL 0
#define SEG7_CODE_0 ((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))		/*0*/				
#define SEG7_CODE_1 ((1<<SEG7_IO_B)|(1<<SEG7_IO_C))						/*1*/
#define SEG7_CODE_2	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))						/*2*/	
#define SEG7_CODE_3	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_G))						/*3*/
#define SEG7_CODE_4	((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*4*/
#define SEG7_CODE_5	((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))						/*5*/
#define SEG7_CODE_6	((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*6*/
#define SEG7_CODE_7	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C))		/*7*/
#define SEG7_CODE_8	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*8*/
#define SEG7_CODE_9	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*9*/
#define SEG7_CODE_A	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*A*/	
#define SEG7_CODE_B	((1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*b*/		
#define SEG7_CODE_C	((1<<SEG7_IO_A)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))					/*C*/
#define SEG7_CODE_D	((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))	/*d*/
#define SEG7_CODE_E	((1<<SEG7_IO_A)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*E*/
#define SEG7_CODE_F	((1<<SEG7_IO_A)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))					/*F*/
#define SEG7_CODE_G	((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*G*/
#define SEG7_CODE_H	((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*H*/
#define SEG7_CODE_L	((1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))									/*L*/
#define SEG7_CODE_N	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*n*/
#define SEG7_CODE_U	((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*u*/
#define SEG7_CODE_P	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*p*/
#define SEG7_CODE_O	((1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))					/*o*/
#define SEG7_CODE_HL	((1<<SEG7_IO_A))		/*-*/
#define SEG7_CODE_ML	((1<<SEG7_IO_G))		/*-*/	
#define SEG7_CODE_LL	((1<<SEG7_IO_D))		/*_*/
#define SEG7_CODE_DP	(1<<SEG7_IO_DP)			/*.*/


#define SEG7_CODE_0_Rev ((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))		/*0*/				
#define SEG7_CODE_1_Rev ((1<<SEG7_IO_E)|(1<<SEG7_IO_F))						/*1*/
#define SEG7_CODE_2_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))						/*2*/	
#define SEG7_CODE_3_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_G))						/*3*/
#define SEG7_CODE_4_Rev	((1<<SEG7_IO_C)|(1<<SEG7_IO_G)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*4*/
#define SEG7_CODE_5_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))						/*5*/
#define SEG7_CODE_6_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_B)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*6*/
#define SEG7_CODE_7_Rev	((1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))		/*7*/
#define SEG7_CODE_8_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*8*/
#define SEG7_CODE_9_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_E)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*9*/
#define SEG7_CODE_C_Rev	((1<<SEG7_IO_A)|(1<<SEG7_IO_D)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C))					/*C*/


#define SEG7_CODE_0_Rev_Rev ((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_B_Rev)|(1<<SEG7_IO_C_Rev)	\
					|(1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev))		/*0*/				
#define SEG7_CODE_1_Rev_Rev ((1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev))	//((1<<SEG7_IO_B)|(1<<SEG7_IO_C))      		/*1*/
#define SEG7_CODE_2_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_B_Rev)|(1<<SEG7_IO_D_Rev)	\
					|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_G_Rev))						/*2*/	
#define SEG7_CODE_3_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev)	\
					|(1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_G_Rev))						/*3*/
#define SEG7_CODE_4_Rev_Rev	((1<<SEG7_IO_C_Rev)|(1<<SEG7_IO_G_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev))	/*4*/
#define SEG7_CODE_5_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_C_Rev)|(1<<SEG7_IO_D_Rev)	\
					|(1<<SEG7_IO_F_Rev)|(1<<SEG7_IO_G_Rev))						/*5*/
#define SEG7_CODE_6_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_C_Rev)|(1<<SEG7_IO_D_Rev)	\
					|(1<<SEG7_IO_B_Rev)|(1<<SEG7_IO_F_Rev)|(1<<SEG7_IO_G_Rev))		/*6*/
#define SEG7_CODE_7_Rev_Rev	((1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev))		/*7*/
#define SEG7_CODE_8_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_B_Rev)|(1<<SEG7_IO_C_Rev)	\
					|(1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_F_Rev)|(1<<SEG7_IO_G_Rev))	/*8*/
#define SEG7_CODE_9_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_E_Rev)|(1<<SEG7_IO_C_Rev)	\
					|(1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_F_Rev)|(1<<SEG7_IO_G_Rev))		/*9*/
#define SEG7_CODE_C_Rev_Rev	((1<<SEG7_IO_A_Rev)|(1<<SEG7_IO_D_Rev)|(1<<SEG7_IO_B_Rev)|(1<<SEG7_IO_C_Rev))					/*C*/
#define SEG7_CODE_DP_Rev_Rev	(1<<SEG7_IO_DP_Rev)			/*.*/


#else				/*共阳*/

#define SEG7_CODE_NULL 0
#define SEG7_CODE_0 ~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))		/*0*/				
#define SEG7_CODE_1 ~((1<<SEG7_IO_B)|(1<<SEG7_IO_C))					/*1*/
#define SEG7_CODE_2	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))						/*2*/	
#define SEG7_CODE_3	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_G))						/*3*/
#define SEG7_CODE_4	~((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*4*/
#define SEG7_CODE_5	~((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))						/*5*/
#define SEG7_CODE_6	~((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*6*/
#define SEG7_CODE_7	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C))		/*7*/
#define SEG7_CODE_8	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*8*/
#define SEG7_CODE_9	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_D)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*9*/
#define SEG7_CODE_A	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)	\
					|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))		/*A*/	
#define SEG7_CODE_B	~((1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*b*/		
#define SEG7_CODE_C	~((1<<SEG7_IO_A)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))					/*C*/
#define SEG7_CODE_D	~((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))	/*d*/
#define SEG7_CODE_E	~((1<<SEG7_IO_A)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*E*/
#define SEG7_CODE_F	~((1<<SEG7_IO_A)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))					/*F*/
#define SEG7_CODE_G	~((1<<SEG7_IO_A)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*G*/
#define SEG7_CODE_H	~((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*H*/
#define SEG7_CODE_L	~((1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))									/*L*/
#define SEG7_CODE_N	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*n*/
#define SEG7_CODE_U	~((1<<SEG7_IO_B)|(1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F))	/*u*/
#define SEG7_CODE_P	~((1<<SEG7_IO_A)|(1<<SEG7_IO_B)|(1<<SEG7_IO_E)|(1<<SEG7_IO_F)|(1<<SEG7_IO_G))	/*p*/
#define SEG7_CODE_O	~((1<<SEG7_IO_C)|(1<<SEG7_IO_D)|(1<<SEG7_IO_E)|(1<<SEG7_IO_G))					/*o*/
#define SEG7_CODE_HL	~((1<<SEG7_IO_A))		/*-*/
#define SEG7_CODE_ML	~((1<<SEG7_IO_G))		/*-*/	
#define SEG7_CODE_LL	~((1<<SEG7_IO_D))		/*_*/
#define SEG7_CODE_DP	~(1<<SEG7_IO_DP)		/*.*/
#endif

#endif
