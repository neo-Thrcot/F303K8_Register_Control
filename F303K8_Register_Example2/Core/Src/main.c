#include "stm32f3xx.h"

void RCC_Init(void);

void TIM6_Init(void);

void GPIO_Init(void);

void delay_ms(unsigned long ms);

int main(void)
{
	/*各ペリフェラルの初期化*/
	RCC_Init();
	TIM6_Init();
	GPIO_Init();

	while (1)
	{
		GPIOA -> ODR |= (1 << 0);
		delay_ms(1000);

		GPIOA -> ODR &= (~(1 << 0));
		delay_ms(1000);

	}

	return 0;
}

void RCC_Init(void)
{
	/*PLLの設定*/
	RCC -> CFGR &= (~(1 << 16));		//PLLのクロック源をHSIに
	RCC -> CFGR |= (4 << 8);			//APB1のクロックを2分周
	RCC -> CFGR |= (15 << 18);			//PLLのクロックを16倍に逓倍

	/*Flash読み出しの遅延*/
	FLASH -> ACR |= (2 << 0);

	/*PLLを有効にし、有効になるまで待機*/
	RCC -> CR |= (1 << 24);
	while (!(RCC -> CR & (1 << 25)));

	/*システムクロックをPLLに変更し、有効になるまで待機*/
	RCC -> CFGR |= (2 << 0);
	while ((RCC -> CFGR & (3 << 2)) != (2 << 2));
}

void TIM6_Init(void)
{
	/*TIM6にクロックを供給*/
	RCC -> APB1ENR |= (1 << 4);

	/*分周比を設定し、TIM6の周波数を1KHzに*/
	TIM6 -> PSC = (64000000 / 1000) - 1;

	/*TIM6を無効に*/
	TIM6 -> CR1 &= (~(1 << 0));
}

void GPIO_Init(void)
{
	RCC -> AHBENR |= (1 << 17);

	GPIOA -> MODER |= (1 << 0);
	GPIOA -> MODER &= (~(1 << 1));
}

void delay_ms(unsigned long ms)
{
	TIM6 -> SR &= (~(1 << 0));				//更新フラグをクリア
	TIM6 -> CNT = 0;						//カウンタを0にリセット
	TIM6 -> ARR = ms - 1;					//遅延時間をセット
	TIM6 -> CR1 |= (1 << 0);				//TIM6を有効に

	/*ARRの値に到達するまで待機*/
	while (!(TIM6 -> SR & (1 << 0)));

	TIM6 -> CR1 &= (~(1 << 0));				//TIM6を無効に
}
