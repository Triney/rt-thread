/*
STM32F103    Cortex-M3
72M ÿLOOP 6������
36MÿLOOP  4������
24MûLOOP 3������
*/

#ifndef __STM32_DELAY_H__
#define __STM32_DELAY_H__

#include <stm32f10x.h>

//#define _delay_loops(n)    do{if(n!=0){__delay_loops(n);}}while(0)

/*
__asm void _delay_loops(unsigned long ulCount)	\
{
    subs    r0, #1;					\
    bne     _delay_loops;   			\
    bx       lr;     				\
}									
*/

#pragma push // �����������
#pragma O3 // ����Ż�
#pragma Ospace // Ϊ�ռ��Ż�
__forceinline void _delay_loops(register u32 volatile a){
        do{a--;}
        while(a);
        return;
}
#pragma pop // �ָ�����������O0�±��룬���������Ľ�������Ǹ���һ���ģ���Ϊ������ʲô�����±��룬�Ƕδ�����Ż��̶ȶ��ǹ̶�����ģ����Ҳ���Ҫ�������ã�
//���ڶ��룬�������ǻ��ڱ�Ҫ��ʱ���ڵ��ú���֮ǰ���� 1 - 3 �� NOP�ģ�

#define F_CPU 72000000L

#if F_CPU>48000000UL
#define  _CYCLES_PER_LOOP  10		//6
#elif F_CPU>24000000UL
#define  _CYCLES_PER_LOOP  4
#else
#define  _CYCLES_PER_LOOP  3
#endif

#define delay_us(A)\
  _delay_loops( (uint32_t) (( (double)(F_CPU) *((A)/1000000.0))/_CYCLES_PER_LOOP+0.5))

#define delay_ms(A)\
  _delay_loops( (uint32_t) (( (double)(F_CPU) *((A)/1000.0))/_CYCLES_PER_LOOP+0.5))

#define delay_s(A)\
  _delay_loops( (uint32_t) (( (double)(F_CPU) *((A)/1.0))/_CYCLES_PER_LOOP+0.5))

#endif
