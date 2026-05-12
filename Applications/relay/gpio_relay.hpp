#pragma once

#include <cstdio>

#include "main.h"
#include "relay.hpp"

namespace gpio_relay {

namespace detail {
	void Initialize(GPIO_TypeDef *gpio, uint16_t pin);
}

template <uint32_t GpioBase, uint16_t Pin>
class GpioRelay : public Relay {
public:
	GpioRelay() {
		gpio_relay::detail::Initialize(reinterpret_cast<GPIO_TypeDef *>(GpioBase), Pin);
	}

	~GpioRelay() {
		HAL_GPIO_DeInit(reinterpret_cast<GPIO_TypeDef *>(GpioBase), Pin);
	}

	void SetState(bool state) override {
		reinterpret_cast<GPIO_TypeDef *>(GpioBase)->BSRR = state ? Pin : (Pin << 16U);
	}
};

}

using gpio_relay::GpioRelay;
