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

#include "board/board.hpp"

namespace {
	static constexpr inline uint32_t kEventIdUpButtonTriggered = (1U << 0);
	static constexpr inline uint32_t kEventIdEnterButtonClicked = (1U << 1);
	static constexpr inline uint32_t kEventIdDownButtonTriggered = (1U << 2);
	static constexpr inline uint32_t kEventIdCounterTimerTriggered = (1U << 3);
	static constexpr inline uint32_t kEventIdFrameReceived = (1U << 4);

	static constexpr inline uint32_t kSettingDigitCount = 4;
	static constexpr inline const uint32_t kDigitToNumber[kSettingDigitCount] = {1, 60, 3600, 36000};

	static constexpr inline uint32_t kMaxTime = 9 * 3600 + 9 * 60 + 59;
}

Application::Application() {
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
		if (duration < 1000) {
			self->eventId_ |= kEventIdUpButtonTriggered;
		}
	}, this);

	upButton->OnPressed(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		self->eventId_ |= kEventIdUpButtonTriggered;
	}, this);

	auto *enterButton = board.GetButton(config::kEnterButtonIndex);
	enterButton->OnReleased(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		self->eventId_ |= kEventIdEnterButtonClicked;
	}, this);

	auto *downButton = board.GetButton(config::kDownButtonIndex);
	downButton->OnReleased(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		self->eventId_ |= kEventIdDownButtonTriggered;
	}, this);

	downButton->OnPressed(+[](void *args, uint32_t duration) {
		auto *self = static_cast<Application *>(args);
		self->eventId_ |= kEventIdDownButtonTriggered;
	}, this);

	auto *remoteDecoder = board.GetRemoteDecoder();
	remoteDecoder->OnReleased(+[](void *args, uint32_t frame) {
		auto *self = static_cast<Application *>(args);
		self->frameRxd_ = frame;
		self->eventId_ |= kEventIdFrameReceived;
	}, this);

	SetState(kDeviceStateIdle);

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
	}
}

void Application::LoadStoredData() {
	auto &board = Board::GetInstance();
	auto *storage = board.GetStorage();

	config::StoredData data{};
	storage->Load(config::kStoredDataAddress, reinterpret_cast<uint8_t *>(&data), sizeof(data));

	if (data.valid_ == 0xA5) {
		time_ = data.time_;
		for (size_t i = 0; i < data.numOfValidFrames_; ++i) {
			frames_[i] = data.frames_[i];
		}
		numOfValidFrames_ = data.numOfValidFrames_;
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
	data.valid_ = 0xA5;

	storage->Save(config::kStoredDataAddress, reinterpret_cast<const uint8_t *>(&data), sizeof(data));
}

void Application::SetState(DeviceState state) {
	if (state_ != state) {
		state_ = state;
		OnStateChanged();
	}
}

void Application::OnStateChanged() {

	auto &board = Board::GetInstance();
	auto *display = board.GetDisplay();

	switch (state_) {
		case kDeviceStateIdle:
			counter_ = time_;
			digit_ = 0;
			counterTimer_.Stop();
			display->ExitLearningScreen();
			display->DisplayNumber(time_);
			break;
		case kDeviceStateWorking:
			counterTimer_.Start();
			display->DisplayNumber(counter_);
			break;
		case kDeviceStateSetting:
			display->DisplayNumber(time_);
			display->Blink(digit_, 500, 500);
			break;
		case kDeviceStateLearning:
			counter_ = 30;
			counterTimer_.Start();
			display->EnterLearningScreen();
			break;
	}

	if (dirty_) {
		dirty_ = false;
		SaveStoredData();
	}
}

void Application::EnterButtonClickedEventHandler() {
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateWorking);
	} else if (state_ == kDeviceStateSetting) {
		// 切换位数
		if (++digit_ >= kSettingDigitCount) {
			digit_ = 0;
		}
		dirty_ = true;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->Blink(digit_, 500, 500);
	}
}

void Application::EnterButtonPressedEventHandler() {
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateSetting);
	} else if (state_ == kDeviceStateSetting) {
		SetState(kDeviceStateIdle);
	}
}

void Application::EnterButtonLongPressedEventHandler() {
	if (state_ == kDeviceStateIdle) {
		SetState(kDeviceStateLearning);
	} else if (state_ == kDeviceStateSetting) {
		SetState(kDeviceStateIdle);
	}
}

void Application::UpButtonTriggeredEventHandler() {
	if (state_ == kDeviceStateSetting) {
		// 增加当前位数的值
		if (time_ + kDigitToNumber[digit_] <= kMaxTime) {
			time_ += kDigitToNumber[digit_];
		} else {
			time_ = kMaxTime;
		}
		dirty_ = true;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->DisplayNumber(time_);
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
		dirty_ = true;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->DisplayNumber(time_);
	}
}

void Application::CounterTimerTriggeredEventHandler() {
	if (counter_ > 0) {
		--counter_;
		auto &board = Board::GetInstance();
		auto *display = board.GetDisplay();
		display->DisplayNumber(counter_);
	} else {
		SetState(kDeviceStateIdle);
	}
}

void Application::FrameRxdEventHandler() {
	if (state_ == kDeviceStateIdle) {
		for (auto &f : frames_) {
			if (f == frameRxd_) {
				SetState(kDeviceStateWorking);
				return;
			}
		}
	} else if (state_ == kDeviceStateLearning) {
		if (++numOfValidFrames_ >= config::kMaxFrameMembers) {
			numOfValidFrames_ = 0;
		}
		frames_[numOfValidFrames_] = frameRxd_;
		dirty_ = true;
		SetState(kDeviceStateIdle);
	}
}
