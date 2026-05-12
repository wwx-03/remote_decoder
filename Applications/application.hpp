#pragma once

#include <stdint.h>

#include "config.hpp"
#include "timer/timer.hpp"

enum DeviceState : uint8_t {
	kDeviceStateUnknown = 0,
	kDeviceStateIdle,
	kDeviceStateWorking,
	kDeviceStateSetting,
	kDeviceStateLearning,
};

class Application {
public:
	static Application& GetInstance() {
		static Application instance;
		return instance;
	}

	void Start();
	
private:
	volatile uint32_t eventId_{};
	DeviceState state_{kDeviceStateUnknown};
	Timer counterTimer_{};
	uint32_t time_{5 * 60};
	uint32_t counter_{};
	uint8_t digit_{};
	uint32_t frames_[config::kMaxFrameMembers]{};
	uint8_t numOfValidFrames_{};
	volatile uint32_t frameRxd_{};
	bool dirty_{};

	Application();
	~Application() = default;
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void LoadStoredData();
	void SaveStoredData();

	void SetState(DeviceState state);
	void OnStateChanged();

	void UpButtonTriggeredEventHandler();
	void EnterButtonClickedEventHandler();
	void EnterButtonPressedEventHandler();
	void EnterButtonLongPressedEventHandler();
	void DownButtonTriggeredEventHandler();
	void CounterTimerTriggeredEventHandler();
	void FrameRxdEventHandler();

	bool IsValidFrame(uint32_t frame);
};
