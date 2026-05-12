#pragma once

#include "main.h"

#include "display.hpp"
#include "timer/timer.hpp"

//		*				*						*				*
//
//	*		*		*		*		*		*		*		*		*
//
//		*				*						*				*
//		
//	*		*		*		*		*		*		*		*		*
//
//		*				*						*				*



class TubeDisplay : public Display {
public:
	static TubeDisplay &GetInstance() {
		static TubeDisplay instance;
		return instance;
	}

	TubeDisplay();
	~TubeDisplay();

	void Initialize();
	void Refresh();

	void DisplayNumber(uint16_t number) override;
	void Blink(uint8_t digit, uint32_t brightMs, uint32_t darkMs) override;
	void Unblink() override;
	void EnterLearningScreen() override;
	void ExitLearningScreen() override;

	void BindBlinkDigit(const uint8_t *digitPtr);
	void SetAllEnabled(bool enabled);
	void SetDigitEnabled(uint8_t digit, bool enabled);


private:
	struct DigitStatus {
		uint8_t d1 : 1;
		uint8_t d2 : 1;
		uint8_t d3 : 1;
		uint8_t d4 : 1;

		bool NoneEnabled() const {
			return !(d1 | d2 | d3 | d4);
		}
	};

	enum class Mode : uint8_t {
		kDigit = 0,
		kCircle,
	};

	struct Line {
		GPIO_TypeDef *positiveGpio;
		uint32_t positivePin;
		GPIO_TypeDef *negativeGpio;
		uint32_t negativePin;
	};

	enum class LineAction : uint8_t {
		kTurnOn = 0,
		kTurnOff,
	};

	static constexpr size_t kDigitCount = 4;
	static constexpr size_t kScanCount = 30;
	static constexpr size_t kCircleScanCount = 12;
	static constexpr uint8_t kInvalidDigit = 0xFF;

	static constexpr uint8_t kSegmentMap[10] = {
		0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
		0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111,
	};

	static const inline Line kScanLines[kScanCount] = {
		{GPIOB, GPIO_PIN_4, GPIOA, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_2},
		{GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_3},
		{GPIOB, GPIO_PIN_3, GPIOB, GPIO_PIN_2},
		{GPIOB, GPIO_PIN_3, GPIOB, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_1},

		{GPIOB, GPIO_PIN_2, GPIOA, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_3, GPIOA, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_3, GPIOB, GPIO_PIN_1},
		{GPIOB, GPIO_PIN_3, GPIOB, GPIO_PIN_4},
		{GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_3},
		{GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_1},

		{GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_4},
		{GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_4},

		{GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_4},
		{GPIOB, GPIO_PIN_1, GPIOA, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_2},
		{GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_3},
		{GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_3},
		{GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_1},
		{GPIOB, GPIO_PIN_1, GPIOB, GPIO_PIN_0},

		{GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_0, GPIOA, GPIO_PIN_0},
		{GPIOB, GPIO_PIN_0, GPIOB, GPIO_PIN_2},
		{GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_4},
		{GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_3},
		{GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_1},
		{GPIOA, GPIO_PIN_0, GPIOB, GPIO_PIN_2},
	};

	static constexpr uint8_t kScanDigitIndex[kScanCount] = {
		0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1,
		kInvalidDigit, kInvalidDigit,
		2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3,
	};

	static constexpr uint8_t kScanSegmentIndex[kScanCount] = {
		0, 1, 2, 3, 4, 5, 6,
		0, 1, 2, 3, 4, 5, 6,
		0, 1,
		0, 1, 2, 3, 4, 5, 6,
		0, 1, 2, 3, 4, 5, 6,
	};

	static constexpr uint8_t kCircleLineIndex[kCircleScanCount] = {
		23, 16, 7, 0, 1, 2, 3, 10, 19, 26, 27, 28,
	};

	uint16_t number_ = 0;
	uint8_t scanIndex_ = 0;
	uint8_t circleTickDivider_ = 0;
	DigitStatus status_ = {1, 1, 1, 1};
	Mode mode_ = Mode::kDigit;
	Timer timer_;
	const uint8_t *blinkDigitPtr_ = nullptr;
	uint8_t blinkDigitCache_ = 0;
	uint8_t lastBlinkDigit_ = 0;
	uint32_t blinkCounter_ = 0;
	uint32_t brightMs_ = 0;
	uint32_t darkMs_ = 0;

	template <LineAction Action>
	static inline void ApplyLine(const Line &line);

	static inline void ConfigureOutputPushPull(GPIO_TypeDef *p_gpio, uint32_t positive,
			GPIO_TypeDef *n_gpio, uint32_t negative);
	static inline void ConfigureHighImpedance(GPIO_TypeDef *p_gpio, uint32_t positive,
			GPIO_TypeDef *n_gpio, uint32_t negative);

	TubeDisplay(const TubeDisplay &) = delete;
	TubeDisplay &operator=(const TubeDisplay &) = delete;

	void TurnAllOff();
	void NextDigitScan();
	bool CurrentScanEnabled() const;
	uint8_t ResolveBlinkDigit();
	void OnBlinkTick();
	void RefreshDigitMode();
	void RefreshCircleMode();
	void DriveCurrentDigitLine();
	uint8_t ExtractDigit(uint8_t digitIndex) const;
};
