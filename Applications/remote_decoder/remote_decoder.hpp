#pragma once

#include <algorithm>
#include <functional/functional>

class RemoteDecoder {
public:
	virtual ~RemoteDecoder() = default;
	virtual void Start() = 0;

	void OnReleased(custom::function<void (void *, uint32_t)> function, void *args) {
		releasedCallback_ = {function, args};
	}

protected:
	std::pair<custom::function<void (void *, uint32_t)>, void *> releasedCallback_;
};
