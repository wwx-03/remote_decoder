#pragma once

#include <stddef.h>
#include "main.h"

#ifdef __cplusplus

namespace config {

	static constexpr inline size_t kMaxButtonPressedCallbacks = 2;

	static constexpr inline size_t kNumberOfButtons = 3;
	static constexpr inline size_t kUpButtonIndex = 0;
	static constexpr inline size_t kEnterButtonIndex = 1;
	static constexpr inline size_t kDownButtonIndex = 2;

	static constexpr inline size_t kMaxFrameMembers = 4;

	static constexpr inline uint32_t Address(uint32_t sector, uint32_t page) { return FLASH_BASE + (sector << 12) + (page << 7); }
	static constexpr inline uint32_t kStoredDataAddress = Address(5, 31);
	static constexpr inline uint32_t kStoredDataValid = 0xA5;

	struct StoredData {
		uint32_t time_{};
		uint32_t frames_[kMaxFrameMembers]{};
		uint8_t numOfValidFrames_{};
		uint8_t valid_{}; // 0xA5表示数据有效，其它无效
	};

}

#endif /* __cplusplus */
