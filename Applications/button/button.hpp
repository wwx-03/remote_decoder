#pragma once

#include <algorithm>
#include <functional/functional>
#include <manager/manager>
#include "config.hpp"

class Button {
public:
	virtual ~Button() = default;

	void OnReleased(custom::function<void (void *, uint32_t)> function, void *args) {
		releasedCallback_ = {function, args};
	}

	void OnPressed(custom::function<void (void *, uint32_t)> function, void *args) {
		pressedCallback_ = {function, args};
	}

protected:
	std::pair<custom::function<void (void *)>, void *> clickedCallback_;
	std::pair<custom::function<void (void *, uint32_t)>, void *> releasedCallback_;
	std::pair<custom::function<void (void *, uint32_t)>, void *> pressedCallback_;
};
