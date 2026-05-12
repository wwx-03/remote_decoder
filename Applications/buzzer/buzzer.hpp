#pragma once

#include <stdint.h>

class Buzzer {
public:
	virtual ~Buzzer() = default;

	virtual void Single(uint32_t duration) = 0;
	virtual void Repeat(uint32_t beepDuration, uint32_t dumbDuration, uint32_t times) = 0;
};
