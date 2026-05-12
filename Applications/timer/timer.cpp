#include "timer.hpp"

void TimerManager::Running() {
	if (!head_) {
		return;
	}

	for (Timer *current = head_; current; current = current->next_) {
		// Check if the timer is active
		if (!current->state_) {
			continue;
		}
		// Do something with current timer
		if (++current->count_ >= current->period_) {
			current->count_ = 0;
			// Timer has reached its period, perform the desired action
			auto &[function, args] = current->callback_;
			function(args);
		}
	}
}

Timer::Timer(custom::function<void (void *)> function, void *args, uint32_t period) {
	Create(function, args, period);
}

Timer::~Timer() {
	Delete();
}

void Timer::Create(custom::function<void (void *)> function, void *args, uint32_t period) {
	callback_ = {function, args};
	period_ = period;
	count_ = 0;
	state_ = false;

	auto &timerManager = TimerManager::GetInstance();
	if (!timerManager.head_) {
		timerManager.head_ = this;
	} else {
		Timer *current = timerManager.head_;
		while (current->next_) {
			current = current->next_;
		}
		current->next_ = this;
	}
}

void Timer::Start() {
	state_ = true;
}

void Timer::Stop() {
	state_ = false;
}

void Timer::ChangePeriod(uint32_t period) {
	period_ = period;
}

void Timer::ChangeCount(uint32_t count) {
	count_ = count;
}

void Timer::Delete() {
	auto &timerManager = TimerManager::GetInstance();
	if (timerManager.head_ == this) {
		timerManager.head_ = next_;
	} else {
		Timer *current = timerManager.head_;
		while (current && current->next_ != this) {
			current = current->next_;
		}
		if (current) {
			current->next_ = next_;
		}
	}
}
