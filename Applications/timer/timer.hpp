#pragma once

#include <stdint.h>
#include <algorithm>

#include <functional/functional>

class Timer {
public:
	Timer() = default;
	Timer(custom::function<void (void *)> function, void *args, uint32_t period);
	~Timer();
	void Create(custom::function<void (void *)> function, void *args, uint32_t period);
	void Start();
	void Stop();
	void ChangePeriod(uint32_t period);
	void ChangeCount(uint32_t count);
	void Delete();

private:
	Timer *next_ = nullptr;
	std::pair<custom::function<void (void *)>, void *> callback_;
	uint32_t period_{};
	uint32_t count_{};
	bool state_{};

	friend class TimerManager;
};

class TimerManager {
public:
	static TimerManager& GetInstance() {
		static TimerManager instance;
		return instance;
	}

	void Running();

private:
	Timer *head_ = nullptr;

	TimerManager() = default;
	~TimerManager() = default;
	TimerManager(const TimerManager&) = delete;
	TimerManager& operator=(const TimerManager&) = delete;

	friend class Timer;
};
