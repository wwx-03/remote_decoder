#pragma once

#include "main.h"

#include "button.hpp"
#include "timer/timer.hpp"

class GpioButton : public Button {
public:
	GpioButton(GPIO_TypeDef *gpiox, uint16_t pin);
	~GpioButton();

private:
	GPIO_TypeDef *gpio_;
	uint16_t pin_;
	Timer timer_;
	uint32_t pressedDuration_{};
	bool state_{};

	__attribute__((__always_inline__)) 
	bool ReadState() { return (gpio_->IDR & pin_) != 0; }

	void TimerCallback();
};
