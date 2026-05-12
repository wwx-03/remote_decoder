#pragma once

#include "main.h"

#include "buzzer.hpp"
#include "timer/timer.hpp"

class PassiveBuzzer : public Buzzer {
public:
	PassiveBuzzer(GPIO_TypeDef *gpiox, uint16_t pin);
	~PassiveBuzzer();

	void Single(uint32_t duration) override;
	void Repeat(uint32_t beepDuration, uint32_t dumbDuration, uint32_t times) override;

	void UpdatePinState();

private:
	enum State : uint8_t {
		kStateIdle = 0,
		kStateSingle,
		kStateRepeat,
	};

	GPIO_TypeDef *gpio_;
	uint16_t pin_;
	Timer timer_;
	State state_{};
	uint32_t beepDuration_{};
	uint32_t dumbDuration_{};
	uint32_t times_{};
	uint32_t duration_{};
	uint32_t count_{};
	bool isBeep_{};

	void TimerCallback();
	__attribute__((__always_inline__))
	void TogglePinState() { gpio_->ODR ^= pin_; }
};
