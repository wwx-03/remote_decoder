#include "gpio_relay.hpp"

void gpio_relay::detail::Initialize(GPIO_TypeDef *gpio, uint16_t pin) {
	GPIO_InitTypeDef init{};
	init.Pin = pin;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(gpio, &init);
}
