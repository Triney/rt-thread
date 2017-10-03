/*
STM32F103    Cortex-M3
72M 每LOOP 6个周期
36M每LOOP  4个周期
24M没LOOP 3个周期
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

#pragma push // 保存编译配置
#pragma O3 // 最高优化
#pragma Ospace // 为空间优化
__forceinline void _delay_loops(register u32 volatile a){
        do{a--;}
        while(a);
        return;
}
#pragma pop // 恢复编译设置在O0下编译，反汇编出来的结果和你那个是一样的，因为不管在什么条件下编译，那段代码的优化程度都是固定不变的，而且不需要函数调用，
//至于对齐，编译器是会在必要的时候在调用函数之前加上 1 - 3 个 NOP的，

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
