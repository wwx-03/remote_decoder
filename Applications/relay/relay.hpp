#pragma once

class Relay {
public:
	virtual ~Relay() = default;
	virtual void SetState(bool state) = 0;
};
