#include "tube_display.hpp"

TubeDisplay::TubeDisplay() {
	timer_.Create(+[](void *args) {
		auto *self = static_cast<TubeDisplay *>(args);
		self->OnBlinkTick();
	}, this, 1);

	Initialize();
}

TubeDisplay::~TubeDisplay() {
	timer_.Delete();
}

void TubeDisplay::Initialize() {
	number_ = 0;
	scanIndex_ = 0;
	circleTickDivider_ = 0;
	status_ = {1, 1, 1, 1};
	mode_ = Mode::kDigit;
	blinkDigitPtr_ = nullptr;
	blinkDigitCache_ = 0;
	lastBlinkDigit_ = 0;
	blinkCounter_ = 0;
	brightMs_ = 0;
	darkMs_ = 0;
	timer_.Stop();
	TurnAllOff();
}

void TubeDisplay::DisplayNumber(uint16_t number) {
	number_ = number;
}

void TubeDisplay::BindBlinkDigit(const uint8_t *digitPtr) {
	blinkDigitPtr_ = digitPtr;
	if (blinkDigitPtr_ != nullptr) {
		blinkDigitCache_ = (*blinkDigitPtr_) & (kDigitCount - 1);
	}
}

void TubeDisplay::Blink(uint8_t digit, uint32_t brightMs, uint32_t darkMs) {
	blinkDigitCache_ = digit & (kDigitCount - 1);
	brightMs_ = brightMs;
	darkMs_ = darkMs;
	blinkCounter_ = 0;
	lastBlinkDigit_ = ResolveBlinkDigit();
	timer_.Start();
}

void TubeDisplay::Unblink() {
	SetDigitEnabled(lastBlinkDigit_, true);
	timer_.Stop();
	blinkCounter_ = 0;
}

void TubeDisplay::EnterLearningScreen() {
	mode_ = Mode::kCircle;
	scanIndex_ = 0;
	circleTickDivider_ = 0;
	TurnAllOff();
}

void TubeDisplay::ExitLearningScreen() {
	mode_ = Mode::kDigit;
	scanIndex_ = 0;
	TurnAllOff();
}

void TubeDisplay::SetAllEnabled(bool enabled) {
	status_ = enabled ? DigitStatus{1, 1, 1, 1} : DigitStatus{0, 0, 0, 0};
	if (!enabled) {
		TurnAllOff();
	}
}

void TubeDisplay::SetDigitEnabled(uint8_t digit, bool enabled) {
	switch (digit & (kDigitCount - 1)) {
		case 0: status_.d1 = enabled; break;
		case 1: status_.d2 = enabled; break;
		case 2: status_.d3 = enabled; break;
		case 3: status_.d4 = enabled; break;
	}
}

void TubeDisplay::Refresh() {
	if (status_.NoneEnabled()) {
		TurnAllOff();
		return;
	}

	if (mode_ == Mode::kDigit) {
		RefreshDigitMode();
	} else {
		RefreshCircleMode();
	}
}

void TubeDisplay::ConfigureOutputPushPull(GPIO_TypeDef *p_gpio, uint32_t positive,
		GPIO_TypeDef *n_gpio, uint32_t negative) {
	uint32_t position = __builtin_ctz(positive);

	p_gpio->MODER &= ~(0x03UL << (2 * position));
	p_gpio->MODER |= (0x01UL << (2 * position));
	p_gpio->OTYPER &= ~positive;
	p_gpio->OSPEEDR &= ~(0x03UL << (2 * position));
	p_gpio->OSPEEDR |= (0x02UL << (2 * position));

	position = __builtin_ctz(negative);
	n_gpio->MODER &= ~(0x03UL << (2 * position));
	n_gpio->MODER |= (0x01UL << (2 * position));
	n_gpio->OTYPER &= ~negative;
	n_gpio->OSPEEDR &= ~(0x03UL << (2 * position));
	n_gpio->OSPEEDR |= (0x02UL << (2 * position));
}

void TubeDisplay::ConfigureHighImpedance(GPIO_TypeDef *positiveGpio, uint32_t positive,
		GPIO_TypeDef *negativeGpio, uint32_t negative) {
	uint32_t position = __builtin_ctz(positive);
	positiveGpio->MODER &= ~(0x03UL << (2 * position));
	positiveGpio->PUPDR &= ~(0x03UL << (2 * position));

	position = __builtin_ctz(negative);
	negativeGpio->MODER &= ~(0x03UL << (2 * position));
	negativeGpio->PUPDR &= ~(0x03UL << (2 * position));
}

template <TubeDisplay::LineAction Action>
inline void TubeDisplay::ApplyLine(const Line &line) {
	if constexpr (Action == LineAction::kTurnOn) {
		ConfigureOutputPushPull(line.positiveGpio, line.positivePin, line.negativeGpio, line.negativePin);
		line.positiveGpio->BSRR = line.positivePin;
		line.negativeGpio->BRR = line.negativePin;
	} else {
		ConfigureHighImpedance(line.positiveGpio, line.positivePin, line.negativeGpio, line.negativePin);
	}
}

void TubeDisplay::TurnAllOff() {
	for (const auto &line : kScanLines) {
		ApplyLine<LineAction::kTurnOff>(line);
	}
}

void TubeDisplay::NextDigitScan() {
	if (++scanIndex_ >= kScanCount) {
		scanIndex_ = 0;
	}
}

bool TubeDisplay::CurrentScanEnabled() const {
	if (scanIndex_ < 7) {
		return status_.d1;
	}
	if (scanIndex_ < 14) {
		return status_.d2;
	}
	if (scanIndex_ >= 16 && scanIndex_ < 23) {
		return status_.d3;
	}
	if (scanIndex_ >= 23) {
		return status_.d4;
	}
	return true;
}

uint8_t TubeDisplay::ResolveBlinkDigit() {
	if (blinkDigitPtr_ != nullptr) {
		blinkDigitCache_ = (*blinkDigitPtr_) & (kDigitCount - 1);
	}
	return blinkDigitCache_;
}

void TubeDisplay::OnBlinkTick() {
	const uint8_t digit = ResolveBlinkDigit();

	if (lastBlinkDigit_ != digit) {
		SetDigitEnabled(lastBlinkDigit_, true);
		lastBlinkDigit_ = digit;
	}

	++blinkCounter_;

	if (blinkCounter_ < brightMs_) {
		SetDigitEnabled(digit, true);
	} else if (blinkCounter_ < (brightMs_ + darkMs_)) {
		SetDigitEnabled(digit, false);
	} else {
		blinkCounter_ = 0;
	}
}

void TubeDisplay::RefreshDigitMode() {
	TurnAllOff();

	const uint8_t digitIndex = kScanDigitIndex[scanIndex_];
	if (digitIndex == kInvalidDigit) {
		DriveCurrentDigitLine();
		return;
	}

	const uint8_t number = ExtractDigit(digitIndex);
	const uint8_t segment = kScanSegmentIndex[scanIndex_];

	if ((kSegmentMap[number] & (1U << segment)) != 0U) {
		DriveCurrentDigitLine();
	} else {
		NextDigitScan();
	}
}

void TubeDisplay::RefreshCircleMode() {
	if (++circleTickDivider_ < 70) {
		return;
	}

	circleTickDivider_ = 0;
	if (++scanIndex_ >= kCircleScanCount) {
		scanIndex_ = 0;
	}

	TurnAllOff();
	ApplyLine<LineAction::kTurnOn>(kScanLines[kCircleLineIndex[scanIndex_]]);
}

void TubeDisplay::DriveCurrentDigitLine() {
	if (CurrentScanEnabled()) {
		ApplyLine<LineAction::kTurnOn>(kScanLines[scanIndex_]);
	}
	NextDigitScan();
}

uint8_t TubeDisplay::ExtractDigit(uint8_t digitIndex) const {
	uint16_t value = number_;
	for (uint8_t i = 0; i < digitIndex; ++i) {
		value /= 10;
	}
	return static_cast<uint8_t>(value % 10);
}

