#include "ic_remote_decoder.hpp"

namespace {
	static constexpr inline uint32_t kMinBit0Duration = 500;
	static constexpr inline uint32_t kMaxBit0Duration = 1200;
	static constexpr inline uint32_t kMinBit1Duration = 700;
	static constexpr inline uint32_t kMaxBit1Duration = 1300;
	static constexpr inline uint32_t kMinSyncDuration = 5600;
	static constexpr inline uint32_t kMaxSyncDuration = 16000;

}

IcRemoteDecoder::IcRemoteDecoder(TIM_TypeDef *timx, uint32_t channel, GPIO_TypeDef *gpiox, uint16_t pin)
		: channel_(channel)
		, gpio_(gpiox)
		, pin_(pin) {
	htim_.Instance = timx;
	htim_.Init.Prescaler = 24 - 1;
	htim_.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim_.Init.Period = 0xFFFF;
	htim_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Base_Init(&htim_);

	TIM_IC_InitTypeDef configIC{};
	configIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
	configIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
	configIC.ICPrescaler = TIM_ICPSC_DIV1;
	configIC.ICFilter = 0;
	HAL_TIM_IC_ConfigChannel(&htim_, &configIC, channel);

	timer_.Create(+[](void *args) {
		auto *self = static_cast<IcRemoteDecoder *>(args);
		self->TimerCallback();
	}, this, 100);
}

IcRemoteDecoder::~IcRemoteDecoder() {
	HAL_TIM_IC_Stop_IT(&htim_, channel_);
	HAL_TIM_IC_DeInit(&htim_);
	HAL_TIM_Base_DeInit(&htim_);
}

void IcRemoteDecoder::Start() {
	HAL_TIM_IC_Start_IT(&htim_, channel_);
}

void IcRemoteDecoder::ParseFrame() {
	uint32_t now = HAL_TIM_ReadCapturedValue(&htim_, channel_);
	const uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim_);
	uint32_t us = (now >= lastCaptureValue_) ? (now - lastCaptureValue_) : (arr + 1 + now - lastCaptureValue_);

	lastCaptureValue_ = now;

	bool state = ReadPinState();

	if (state) {
		bit0Duration_ = us;
		parseComplete_ = true;
	} else {
		bit1Duration_ = us;
	}

	if (!parseComplete_) {
		return;
	}

	parseComplete_ = false;

	if (kMinBit0Duration < bit0Duration_ && bit0Duration_ < kMaxBit0Duration) {
		frameCache_ = 0;
		bitCount_ = 0;
		parsing_ = true;
	} else if (parsing_) {
		if (kMinBit0Duration < bit0Duration_ && bit0Duration_ < kMaxBit0Duration) {
			frameCache_ &= ~(1 << bitCount_);
			++bitCount_;
		} else if (kMinBit1Duration < bit1Duration_ && bit1Duration_ < kMaxBit1Duration) {
			frameCache_ |= (1 << bitCount_);
			++bitCount_;
		} else {
			frameCache_ = 0;
			bitCount_ = 0;
			parsing_ = false;
		}
	}

	if (bitCount_ >= 24) {
		currentFrame_ = frameCache_;
		bitCount_ = 0;
		parsing_ = false;
		if (lastFrame_ == currentFrame_) {
			timer_.ChangeCount(0);
			timer_.Start();
		}
		lastFrame_ = currentFrame_;
	}
}

void IcRemoteDecoder::TimerCallback() {
	auto &[function, args] = releasedCallback_;
	function(args, currentFrame_);
	timer_.Stop();
}
