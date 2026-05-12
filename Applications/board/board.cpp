#include "board.hpp"

#include "button/gpio_button.hpp"
#include "buzzer/passive_buzzer.hpp"
#include "display/tube_display.hpp"
#include "relay/gpio_relay.hpp"
#include "remote_decoder/ic_remote_decoder.hpp"
#include "storage/internal_flash.hpp"

Board::Board() {
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	TIM14_Init();
}

Board::~Board() {
	HAL_TIM_Base_Stop_IT(&htim14_);
	HAL_TIM_Base_DeInit(&htim14_);
}

Button *Board::GetButton(size_t index) {
	static GpioButton buttons[config::kNumberOfButtons]{
		GpioButton(GPIOA, GPIO_PIN_6),
		GpioButton(GPIOA, GPIO_PIN_7),
		GpioButton(GPIOB, GPIO_PIN_7),
	};
	return &buttons[index];
}

Buzzer *Board::GetBuzzer() {
	static PassiveBuzzer buzzer(GPIOA, GPIO_PIN_5);
	return &buzzer;
}

Display *Board::GetDisplay() {
	return &TubeDisplay::GetInstance();
}

Relay *Board::GetRelay() {
	static GpioRelay<GPIOA_BASE, GPIO_PIN_1> relay;
	return &relay;
}

RemoteDecoder *Board::GetRemoteDecoder() {
	static IcRemoteDecoder remoteDecoder(TIM1, TIM_CHANNEL_3, GPIOB, GPIO_PIN_5);
	return &remoteDecoder;
}

Storage *Board::GetStorage() {
	return &InternalFlash::GetInstance();
}

void Board::TIM14_Init() {
	htim14_.Instance = TIM14;
	htim14_.Init.Prescaler = 24 - 1;
	htim14_.Init.Period = 350 - 1;
	htim14_.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim14_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim14_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	htim14_.Init.RepetitionCounter = 0;
	HAL_TIM_Base_Init(&htim14_);
	HAL_TIM_Base_Start_IT(&htim14_);
}

extern "C" void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
	GPIO_InitTypeDef init{};

	if (htim->Instance == TIM1) {
		__HAL_RCC_TIM1_CLK_ENABLE();
		
		init.Mode = GPIO_MODE_AF_PP;
		init.Pull = GPIO_NOPULL;
		init.Speed = GPIO_SPEED_FREQ_HIGH;
		init.Pin = GPIO_PIN_5;
		init.Alternate = GPIO_AF2_TIM1;
		HAL_GPIO_Init(GPIOB, &init);
		
		HAL_NVIC_SetPriority(TIM1_CC_IRQn, 3, 0);
		HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
	} else if (htim->Instance == TIM14) {
		__HAL_RCC_TIM14_CLK_ENABLE();

		HAL_NVIC_SetPriority(TIM14_IRQn, 4, 0);
		HAL_NVIC_EnableIRQ(TIM14_IRQn);
	}
}

extern "C" void HAL_IncTick(void) {
	extern __IO uint32_t uwTick;
	uwTick += uwTickFreq;
	auto &timerManager = TimerManager::GetInstance();
	timerManager.Running();
}

extern "C" void TIM1_CC_IRQHandler(void) {

	auto &board = Board::GetInstance();
	auto *remoteDecoder = static_cast<IcRemoteDecoder *>(board.GetRemoteDecoder());

	HAL_TIM_IRQHandler(remoteDecoder->GetTimerHandle());
}

extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    auto &board = Board::GetInstance();
    auto *remoteDecoder = static_cast<IcRemoteDecoder *>(board.GetRemoteDecoder());
    if (htim == remoteDecoder->GetTimerHandle()) {
        remoteDecoder->ParseFrame();
    }
}

extern "C" void TIM14_IRQHandler(void) {
	static uint8_t update = 0;
	auto &board = Board::GetInstance();
	auto *display = static_cast<TubeDisplay *>(board.GetDisplay());
	auto *buzzer = static_cast<PassiveBuzzer *>(board.GetBuzzer());

	if (TIM14->SR & TIM_FLAG_UPDATE) {
		if (TIM14->DIER & TIM_IT_UPDATE) {
			TIM14->SR = ~TIM_FLAG_UPDATE;
			update = (update + 1) & 0x01;
			if (update) {
				display->Refresh();
			}
			buzzer->UpdatePinState();
		}
	}
}
