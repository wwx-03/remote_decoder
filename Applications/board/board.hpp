#pragma once

#include "config.hpp"
#include "button/button.hpp"
#include "buzzer/buzzer.hpp"
#include "display/display.hpp"
#include "relay/relay.hpp"
#include "remote_decoder/remote_decoder.hpp"
#include "storage/storage.hpp"

class Board {
public:
	static Board& GetInstance() {
		static Board instance;
		return instance;
	}

	Button *GetButton(size_t index);
	Buzzer *GetBuzzer();
	Display *GetDisplay();
	Relay *GetRelay();
	RemoteDecoder *GetRemoteDecoder();
	Storage *GetStorage();

private:
	TIM_HandleTypeDef htim14_;

	Board();
	~Board();
	Board(const Board&) = delete;
	Board& operator=(const Board&) = delete;

	void TIM14_Init();
};
