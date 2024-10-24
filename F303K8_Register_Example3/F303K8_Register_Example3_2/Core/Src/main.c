#include "stm32f3xx.h"
#include <stdio.h>

enum {
	USART_OK,
	USART_NG
};

void System_Clock_Init(void);
void TIM6_Init(void);
void USART2_Init(unsigned int BaudRate);
void GPIO_Init(void);

void delay_ms(uint16_t ms);

void USART2_Transmit(uint8_t *transmit_buf, unsigned int data_size);
uint8_t USART2_Receive(uint8_t *receive_buf, unsigned int data_size, unsigned int timeout_ms);

int main(void)
{
	/*各ペリフェラルの初期化*/
	System_Clock_Init();
	TIM6_Init();
	USART2_Init(115200);
	GPIO_Init();

	while (1)
	{
		uint8_t transmit_buf1[50] = "Received 3 characters\n\r";
		uint8_t transmit_buf2[50] = "Can't received\n\r";
		uint8_t receive[10];

		if (USART2_Receive(receive, 3, 1000) == USART_OK) {
			USART2_Transmit(transmit_buf1, sizeof(transmit_buf1));
		} else {
			USART2_Transmit(transmit_buf2, sizeof(transmit_buf2));
		}
	}

	return 0;
}

void System_Clock_Init(void)
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

void USART2_Init(unsigned int BaudRate)
{
	/*USART2にクロックを供給*/
	RCC -> APB1ENR |= (1 << 17);

	/*ボーレートの設定*/
	USART2 -> CR1 &= (~(1 << 15));			//オーバーサンプリングを16倍に
	USART2 -> BRR = 32000000 / BaudRate;	//ボーレートを115200に設定

	/*USART2を有効にする*/
	USART2 -> CR1 |= (1 << 3);				//TXを有効に
	USART2 -> CR1 |= (1 << 2);				//RXを有効に
	USART2 -> CR1 |= (1 << 0);				//USART2を有効に
}

void GPIO_Init(void)
{
	/*GPIOA、Bにクロックを供給*/
	RCC -> AHBENR |= (1 << 17);
	RCC -> AHBENR |= (1 << 18);

	/*PortA*/
	/*GPIOのモード設定*/
	GPIOA -> MODER &= (~(1 << 4));
	GPIOA -> MODER |= (1 << 5);
	GPIOA -> MODER &= (~(1 << 30));
	GPIOA -> MODER |= (1 << 31);

	/*オルタネート機能を設定*/
	GPIOA -> AFR[0] |= (0b0111 << 8);
	GPIOA -> AFR[1] |= (0b0111 << 28);

	/*PortB*/
	/*GPIOのモード設定*/
	GPIOB -> MODER |= (1 << 0);
	GPIOB -> MODER &= (~(1 << 1));
}

void delay_ms(uint16_t ms)
{
	TIM6 -> SR &= (~(1 << 0));				//更新フラグをクリア
	TIM6 -> CNT = 0;						//カウンタを0にリセット
	TIM6 -> ARR = ms - 1;					//遅延時間をセット
	TIM6 -> CR1 |= (1 << 0);				//TIM6を有効に

	/*ARRの値に到達するまで待機*/
	while (!(TIM6 -> SR & (1 << 0)));

	TIM6 -> CR1 &= (~(1 << 0));				//TIM6を無効に
}

void USART2_Transmit(uint8_t *transmit_buf, unsigned int data_size)
{
	/*要求されたデータの数だけループする*/
	for (int i = 0; i < data_size; i++) {
		/*送信データレジスタがエンプティになるまで待機*/
		while (!(USART2 -> ISR & (1 << 7)));

		/*データ送信レジスタにデータを入力*/
		USART2 -> TDR = transmit_buf[i];

		/*データが送信されるまで待機*/
		while (!(USART2 -> ISR & (1 << 6)));
	}
}

uint8_t USART2_Receive(uint8_t *receive_buf, unsigned int data_size, unsigned int timeout_ms)
{
	/*delay_ms関数と同様に*/
	TIM6 -> SR &= (~(1 << 0));
	TIM6 -> CNT = 0;
	TIM6 -> ARR = timeout_ms - 1;
	TIM6 -> CR1 |= (1 << 0);

	/*要求されたデータの数だけループする*/
	for (int i = 0; i < data_size; i++) {
		/*データ受信するまで待機*/
		while (!(USART2 -> ISR & (1 << 5))) {
			if (TIM6 -> SR & (1 << 0)) {
				/*関数から抜け出す*/
				TIM6 -> CR1 &= (~(1 << 0));
				return USART_NG;
			}
		}

		receive_buf[i] = USART2 -> RDR;
	}

	TIM6 -> CR1 &= (~(1 << 0));

	return USART_OK;
}
