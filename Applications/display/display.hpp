#pragma once

#include <stdint.h>

class Display {
public:
	virtual ~Display() = default;
	virtual void DisplayNumber(uint16_t number) = 0;
	virtual void Blink(uint8_t digit, uint32_t brightMs, uint32_t darkMs) = 0;
	virtual void Unblink() = 0;
	virtual void EnterLearningScreen() = 0;
	virtual void ExitLearningScreen() = 0;
};
