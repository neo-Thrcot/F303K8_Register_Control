#include "stm32f3xx.h"

void System_Clock_Init(void);
void TIM6_Init(void);
void USART2_Init(unsigned int BaudRate);
void GPIO_Init(void);

void delay_ms(uint16_t ms);

void USART2_Transmit(uint8_t *transmit_buf, unsigned int data_size);
uint16_t USART2_Receive(uint8_t *receive_buf, unsigned int data_size);

int main(void)
{
	/*各ペリフェラルの初期化*/
	System_Clock_Init();
	TIM6_Init();
	USART2_Init(115200);
	GPIO_Init();

	while (1)
	{
		uint8_t message_buf[20] = {0};

		if (USART2_Receive(message_buf, 1)) {
			USART2_Transmit(message_buf, 1);
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
	/*GPIOAにクロックを供給*/
	RCC -> AHBENR |= (1 << 17);

	/*PA2、PA15をオルタネートモードに*/
	GPIOA -> MODER &= (~(1 << 4));
	GPIOA -> MODER |= (1 << 5);
	GPIOA -> MODER &= (~(1 << 30));
	GPIOA -> MODER |= (1 << 31);

	/*オルタネート機能を設定*/
	GPIOA -> AFR[0] |= (0b0111 << 8);
	GPIOA -> AFR[1] |= (0b0111 << 28);
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

uint16_t USART2_Receive(uint8_t *receive_buf, unsigned int data_size)
{
	uint16_t receive_data_size = 0;				//データ受信の数をカウント

	/*要求されたデータの数だけループする*/
	for (int i = 0; i < data_size; i++) {
		if (USART2 -> ISR & (1 << 5)) {
			/*データを受信した場合に実行*/
			receive_buf[i] = USART2 -> RDR;		//配列にデータを格納

			receive_data_size++;
		} else {
			/*データを受信していなければ0を代入*/
			receive_buf[i] = 0x00;
		}
	}

	return receive_data_size;
}
