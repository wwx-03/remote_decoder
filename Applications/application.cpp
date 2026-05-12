/**
 * @note 至少需要三个按键，分别是上、下、确认。
 * @note 其中按键需要支持长按不放触发，长按放开触发以及短按触发这三种功能。
 * 
 * @note 确认按键：短按事件有两种情况：
 * @note 	1.设备空闲时进入工作模式；
 * @note 	2.设备设置模式时切换位数。
 * @note 确认按键：长按不放大于'x'毫秒触发事件，进入学习模式。
 * @note 确认按键：长按不放小于'y'毫秒触发事件，进入设置模式。
 * 
 * @note 上、下按键：短按事件有两种情况：
 * @note 	1.设备空闲时无效；
 * @note 	2.设备设置模式时增加/减少当前位数的值。
 * 
 * @note 设备有4中状态，分别是空闲、设置、学习、工作。
 * @note 空闲状态时：屏幕显示持续时间，表示工作模式下继电器开启的时间。
 * @note 设置状态时：屏幕闪烁显示当前正在设置的位数，按下确认键切换位数，按下上/下键增加/减少当前位数的值。
 * @note 学习状态时：屏幕显示特定光效（具体由硬件配合软件决定），此时按下遥控器按键即可学习，学习完成进入空闲状态，屏幕恢复显示持续时间。
 * @note 工作状态时：屏幕倒计时，倒计时结束、按下确认按键或者遥控器的确认按键即可退出工作模式。
 */

#include "application.hpp"

#include <log/log>

#include "board/board.hpp"

namespace {
	// event
	static constexpr inline uint32_t kEventIdUpButtonTriggered = (1U << 0);
	static constexpr inline uint32_t kEventIdEnterButtonClicked = (1U << 1);
	static constexpr inline uint32_t kEventIdEnterButtonPressing = (1U << 2);
	static constexpr inline uint32_t kEventIdDownButtonTriggered = (1U << 3);
	static constexpr inline uint32_t kEventIdCounterTimerTriggered = (1U << 4);
	static constexpr inline uint32_t kEventIdFrameReceived = (1U << 5);
	static constexpr inline uint32_t kEventIdEnterSetting = (1U << 6);
	static constexpr inline uint32_t kEventIdEnterLearning = (1U << 7);

	// button
	static constexpr inline uint32_t kMaxButtonClickedDuration = 1000;
	static constexpr inline uint32_t kButtonPressingTriggeredPeriod = 100;

	// display
	static constexpr inline uint32_t kBlinkBrightDuration = 500;
	static constexpr inline uint32_t kBlinkDarkDuration = 200;

	static constexpr inline uint32_t kMaxWaitInSettingMode = 15;
	static constexpr inline uint32_t kMaxWaitInLearningMode = 30;

	static constexpr inline uint32_t kSettingDigitCount = 4;
	static constexpr inline const uint32_t kDigitToNumber[kSettingDigitCount] = {1, 10, 60, 3600};

	static constexpr inline uint32_t kMaxTime = 9 * 3600 + 9 * 60 + 59;

	static constexpr inline const char *kStateToString[] = {
		"Unknown",
		"Idle",
		"Working",
		"Setting",
		"Learning",
	};

	static uint32_t NumberToTimeFormat(uint32_t number) {
		uint32_t hours = number / 3600;
		uint32_t minutes = (number % 3600) / 60;
		uint32_t seconds = number % 60;
		return hours * 1000 + minutes * 100 + seconds;
	}
}

Application::Application() {

	LOG_INIT();

	counterTimer_.Create(+[](void *args) {
		auto *self = static_cast<Application *>(args);
		self->eventId_ |= kEventIdCounterTimerTriggered;
	}, this, 1000);

	LoadStoredData();
}

void Application::Start() {

	auto &board = Board::GetInstance();

	auto *upButton = board.GetButton(config::kUpButtonIndex);
	upButton->OnReleased(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration < kMaxButtonClickedDuration) {
			self->eventId_ |= kEventIdUpButtonTriggered;
		}
	}, this);

	upButton->OnPressed(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration >= kMaxButtonClickedDuration && duration % kButtonPressingTriggeredPeriod == 0) {
			self->eventId_ |= kEventIdUpButtonTriggered;
		}
	}, this);

	auto *enterButton = board.GetButton(config::kEnterButtonIndex);
	enterButton->OnReleased(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration <= kMaxButtonClickedDuration) {
			self->eventId_ |= kEventIdEnterButtonClicked;
		} else if (duration <= 3000) {
			self->eventId_ |= kEventIdEnterSetting;
		}
		self->eventId_ &= ~kEventIdEnterButtonPressing;
	}, this);

	enterButton->OnPressed(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration > 5000) {
			if (!(self->eventId_ & kEventIdEnterButtonPressing)) {
				self->eventId_ |= kEventIdEnterButtonPressing;
				self->eventId_ |= kEventIdEnterLearning;
			}
		}
	}, this);

	auto *downButton = board.GetButton(config::kDownButtonIndex);
	downButton->OnReleased(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration < kMaxButtonClickedDuration) {
			self->eventId_ |= kEventIdDownButtonTriggered;
		}
	}, this);

	downButton->OnPressed(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		if (duration >= kMaxButtonClickedDuration && duration % kButtonPressingTriggeredPeriod == 0) {
			self->eventId_ |= kEventIdDownButtonTriggered;
		}
	}, this);

	auto *remoteDecoder = board.GetRemoteDecoder();
	remoteDecoder->OnReleased(+[](void *args, uint32_t frame) {
		auto *self = static_cast<Application *>(args);
		self->frameRxd_ = frame;
		self->eventId_ |= kEventIdFrameReceived;
	}, this);

	remoteDecoder->Start();
	SetState(kDeviceStateIdle);

	auto *buzzer = board.GetBuzzer();
	buzzer->Single(100);
	
	while (1) {

		if (eventId_ & kEventIdUpButtonTriggered) {
			eventId_ &= ~kEventIdUpButtonTriggered;
			UpButtonTriggeredEventHandler();
		}

		if (eventId_ & kEventIdEnterButtonClicked) {
			eventId_ &= ~kEventIdEnterButtonClicked;
			EnterButtonClickedEventHandler();
		}

		if (eventId_ & kEventIdDownButtonTriggered) {
			eventId_ &= ~kEventIdDownButtonTriggered;
			DownButtonTriggeredEventHandler();
		}

		if (eventId_ & kEventIdCounterTimerTriggered) {
			eventId_ &= ~kEventIdCounterTimerTriggered;
			CounterTimerTriggeredEventHandler();
		}

		if (eventId_ & kEventIdFrameReceived) {
			eventId_ &= ~kEventIdFrameReceived;
			FrameRxdEventHandler();
		}

		if (eventId_ & kEventIdEnterSetting) {
			eventId_ &= ~kEventIdEnterSetting;
			EnterButtonPressedEventHandler();
		}

		if (eventId_ & kEventIdEnterLearning) {
			eventId_ &= ~kEventIdEnterLearning;
			EnterButtonLongPressedEventHandler();
		}
	}
}

void Application::LoadStoredData() {
	auto &board = Board::GetInstance();
	auto *storage = board.GetStorage();

	config::StoredData data{};
	storage->Load(config::kStoredDataAddress, reinterpret_cast<uint8_t *>(&data), sizeof(data));

	if (data.valid_ == config::kStoredDataValid) {
		time_ = data.time_;
		for (size_t i = 0; i < data.numOfValidFrames_; ++i) {
			frames_[i] = data.frames_[i];
		}
		numOfValidFrames_ = data.numOfValidFrames_;
		LOGI("Loaded stored data: time: %lu, numOfValidFrames: %u\r\n", time_, numOfValidFrames_);
	} else {
		LOGW("No valid stored data found\r\n");
	}
}

void Application::SaveStoredData() {
	auto &board = Board::GetInstance();
	auto *storage = board.GetStorage();

	config::StoredData data{};
	data.time_ = time_;
	for (size_t i = 0; i < numOfValidFrames_; ++i) {
		data.frames_[i] = frames_[i];
	}
	data.numOfValidFrames_ = numOfValidFrames_;
	data.valid_ = config::kStoredDataValid;

	storage->Save(config::kStoredDataAddress, reinterpret_cast<const uint8_t *>(&data), sizeof(data));
}

void Application::SetState(DeviceState state) {
	if (state_ != state) {
		LOGI("State changed: %s -> %s\r\n", kStateToString[state_], kStateToString[state]);
		state_ = state;
		OnStateChanged();
	}
}

void Application::OnStateChanged() {

	auto &board = Board::GetInstance();
	auto *display = board.GetDisplay();
	auto *relay = board.GetRelay();

	switch (state_) {
		case kDeviceStateIdle:
			counter_ = time_;
			digit_ = 0;
			relay->SetState(false);
			counterTimer_.Stop();
			display->ExitLearningScreen();
			display->Unblink();
			display->DisplayNumber(NumberToTimeFormat(time_));
			break;
		case kDeviceStateWorking:
			relay->SetState(true);
			counterTimer_.Start();
			display->DisplayNumber(NumberToTimeFormat(counter_));
			break;
		case kDeviceStateSetting:
			counter_ = kMaxWaitInSettingMode;
			counterTimer_.Start();
			display->DisplayNumber(NumberToTimeFormat(time_));
			display->Blink(digit_, kBlinkBrightDuration, kBlinkDarkDuration);
			break;
		case kDeviceStateLearning:
			counter_ = kMaxWaitInLearningMode;
			counterTimer_.Start();
			display->EnterLearningScreen();
			break;
		case kDeviceStateUnknown:
			break;
	}

	if (dirty_) {
		dirty_ = false;
		SaveStoredData();
	}
}

void Application::EnterButtonClickedEventHandler() {
	auto &board = Board::GetInstance();
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateWorking);
	} else if (state_ == kDeviceStateWorking) {
		SetState(kDeviceStateIdle);
	} else if (state_ == kDeviceStateSetting) {
		// 切换位数
		if (++digit_ >= kSettingDigitCount) {
			digit_ = 0;
		}
		dirty_ = true;
		auto *display = board.GetDisplay();
		display->Blink(digit_, kBlinkBrightDuration, kBlinkDarkDuration);
	}
	auto *buzzer = board.GetBuzzer();
	buzzer->Single(25);
}

void Application::EnterButtonPressedEventHandler() {
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateSetting);
	} else if (state_ == kDeviceStateSetting) {
		SetState(kDeviceStateIdle);
	}
	auto &board = Board::GetInstance();
	auto *buzzer = board.GetBuzzer();
	buzzer->Repeat(25, 25, 2);
}

void Application::EnterButtonLongPressedEventHandler() {
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateLearning);
	} else if (state_ == kDeviceStateSetting) {
		SetState(kDeviceStateIdle);
	}
	auto &board = Board::GetInstance();
	auto *buzzer = board.GetBuzzer();
	buzzer->Repeat(25, 25, 3);
}

void Application::UpButtonTriggeredEventHandler() {
	if (state_ == kDeviceStateSetting) {
		// 增加当前位数的值
		if (time_ + kDigitToNumber[digit_] <= kMaxTime) {
			time_ += kDigitToNumber[digit_];
		} else {
			time_ = kMaxTime;
		}
		counter_ = kMaxWaitInSettingMode;
		dirty_ = true;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->DisplayNumber(NumberToTimeFormat(time_));
		auto *buzzer = board.GetBuzzer();
		buzzer->Single(25);
	}
}

void Application::DownButtonTriggeredEventHandler() {
	if (state_ == kDeviceStateSetting) {
		// 减少当前位数的值
		if (time_ >= kDigitToNumber[digit_]) {
			time_ -= kDigitToNumber[digit_];
		} else {
			time_ = 0;
		}
		counter_ = kMaxWaitInSettingMode;
		dirty_ = true;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->DisplayNumber(NumberToTimeFormat(time_));
		auto *buzzer = board.GetBuzzer();
		buzzer->Single(25);
	}
}

void Application::CounterTimerTriggeredEventHandler() {
	if (counter_ > 0) {
		--counter_;
		if (state_ == kDeviceStateWorking) {
			auto &board = Board::GetInstance();
			auto *display = board.GetDisplay();
			display->DisplayNumber(NumberToTimeFormat(counter_));
		}
	} else {
		SetState(kDeviceStateIdle);
	}
}

void Application::FrameRxdEventHandler() {
	LOGI("Frame received: %lu, address: %lu, key: %lu\r\n", frameRxd_, frameRxd_ & 0x0FFFFF, frameRxd_ >> 20);
	auto &board = Board::GetInstance();
	auto *buzzer = board.GetBuzzer();
	if (state_ == kDeviceStateIdle) {
		if (IsValidFrame(frameRxd_)) {
			SetState(kDeviceStateWorking);
			buzzer->Single(25);
		}
	} else if (state_ == kDeviceStateWorking) {
		if (IsValidFrame(frameRxd_)) {
			SetState(kDeviceStateIdle);
			buzzer->Single(25);
		}
	} else if (state_ == kDeviceStateLearning) {
		if (numOfValidFrames_ < config::kMaxFrameMembers) {
			frames_[numOfValidFrames_++] = frameRxd_;
		} else {
			// 缓冲区满，平移数据并覆盖最旧的帧
			for (size_t i = 1; i < config::kMaxFrameMembers; ++i) {
				frames_[i - 1] = frames_[i];
			}
			frames_[config::kMaxFrameMembers - 1] = frameRxd_;
			// numOfValidFrames_ 保持为 config::kMaxFrameMembers
		}
		dirty_ = true;
		SetState(kDeviceStateIdle);
	}
}

bool Application::IsValidFrame(uint32_t frame) {
	for (size_t i = 0; i < numOfValidFrames_; ++i) {
		if (frames_[i] == frame) {
			return true;
		}
	}
	return false;
}
