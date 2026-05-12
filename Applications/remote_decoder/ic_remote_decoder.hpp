#pragma once

#include "main.h"

#include "remote_decoder.hpp"
#include "timer/timer.hpp"

class IcRemoteDecoder : public RemoteDecoder {
public:
	IcRemoteDecoder(TIM_TypeDef *timx, uint32_t channel, GPIO_TypeDef *gpiox, uint16_t pin);
	~IcRemoteDecoder();

	void Start() override;

	void ParseFrame();
	TIM_HandleTypeDef *GetTimerHandle() { return &htim_; }

private:
	TIM_HandleTypeDef htim_;
	uint32_t channel_;
	GPIO_TypeDef *gpio_;
	uint16_t pin_;
	Timer timer_;
	uint32_t lastCaptureValue_{};
	uint32_t bit0Duration_{};
	uint32_t bit1Duration_{};
	uint32_t frameCache_{};
	uint32_t currentFrame_{};
	uint32_t lastFrame_{};
	uint8_t bitCount_{};
	bool isLearning_{};
	bool parseComplete_{};
	bool parsing_{};

	void TimerCallback();

	__attribute__((__always_inline__))
	bool ReadPinState() { return (gpio_->IDR & pin_) != 0; }
};
