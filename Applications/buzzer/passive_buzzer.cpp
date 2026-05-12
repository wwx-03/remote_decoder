#include "passive_buzzer.hpp"

PassiveBuzzer::PassiveBuzzer(GPIO_TypeDef *gpiox, uint16_t pin) 
		: gpio_(gpiox)
		, pin_(pin) {
	GPIO_InitTypeDef init{};
	init.Pin = pin;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(gpiox, &init);

	timer_.Create(+[](void *args) {
		auto *self = static_cast<PassiveBuzzer *>(args);
		self->TimerCallback();
	}, this, 1);
}

PassiveBuzzer::~PassiveBuzzer() {
	timer_.Delete();
	HAL_GPIO_DeInit(gpio_, pin_);
}

void PassiveBuzzer::Single(uint32_t duration) {
	state_ = kStateSingle;
	beepDuration_ = duration;
	count_ = 0;
	duration_ = 0;
	timer_.Start();
}

void PassiveBuzzer::Repeat(uint32_t beepDuration, uint32_t dumbDuration, uint32_t times) {
	state_ = kStateRepeat;
	beepDuration_ = beepDuration;
	dumbDuration_ = dumbDuration;
	times_ = times;
	count_ = 0;
	duration_ = 0;
	isBeep_ = true;
	timer_.Start();
}

void PassiveBuzzer::UpdatePinState() {
	if (state_ != kStateIdle) {
		TogglePinState();
	}
}

void PassiveBuzzer::TimerCallback() {
	if (kStateIdle == state_) {
		return;
	}

	if (kStateSingle == state_) {
		if (++duration_ >= beepDuration_) {
			state_ = kStateIdle;
			timer_.Stop();
		}
	} else if (kStateRepeat == state_) {
		if (isBeep_) {
			if (++duration_ >= beepDuration_) {
				isBeep_ = false;
				duration_ = 0;
			}
		} else {
			if (++duration_ >= dumbDuration_) {
				if (++count_ >= times_) {
					state_ = kStateIdle;
					timer_.Stop();
				} else {
					isBeep_ = true;
					duration_ = 0;
				}
			}
		}
	}
}
