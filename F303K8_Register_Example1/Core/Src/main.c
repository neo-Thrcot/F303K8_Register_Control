#include "stm32f3xx.h"

int main(void)
{
	/*GPIOAとGPIOBにクロックを供給*/
	RCC -> AHBENR |= (1 << 17);
	RCC -> AHBENR |= (1 << 18);

	/*PA0を入力に設定*/
	GPIOA -> MODER &= (~(1 << 0));
	GPIOA -> MODER &= (~(1 << 1));

	/*PB0を出力に設定*/
	GPIOB -> MODER |= (1 << 0);
	GPIOB -> MODER &= (~(1 << 1));

	while(1)
	{
		/*PA0の状態をstateに代入*/
		uint8_t state = (GPIOA -> IDR & (1 << 0));

		/*stateの値を出力に反映*/
		GPIOB -> ODR = state;
	}

	return 0;
}
