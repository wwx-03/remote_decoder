#include "gpio_button.hpp"

namespace {
	static constexpr uint32_t kTimerPeriod = 20;
	static constexpr bool kPressedState = false;
}

GpioButton::GpioButton(GPIO_TypeDef *gpiox, uint16_t pin) 
		: gpio_(gpiox)
		, pin_(pin) {
	GPIO_InitTypeDef init{};
	init.Pin = pin;
	init.Mode = GPIO_MODE_INPUT;
	init.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(gpiox, &init);

	state_ = ReadState();

	timer_.Create(+[](void *args) {
		auto *self = static_cast<GpioButton *>(args);
		self->TimerCallback();
	}, this, kTimerPeriod);

	timer_.Start();
}

GpioButton::~GpioButton() {
	timer_.Delete();
	HAL_GPIO_DeInit(gpio_, pin_);
}


void GpioButton::TimerCallback() {
	bool currentState = ReadState();

	if (currentState == kPressedState && state_ == kPressedState) {
		pressedDuration_ += kTimerPeriod;
		auto &[function, args] = pressedCallback_;
		function(args, pressedDuration_);
	}

	if (currentState == !kPressedState && state_ == kPressedState) {
		auto &[function, args] = releasedCallback_;
		function(args, pressedDuration_);
		pressedDuration_ = 0;
	}
	
	state_ = currentState;
}
