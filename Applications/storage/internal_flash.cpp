#include "internal_flash.hpp"

#include <string.h>
#include <custom>

void InternalFlash::Save(uint32_t address, const uint8_t *data, size_t length) {
	HAL_FLASH_Unlock();

    size_t written = 0;
    while (written < length) {
        // 当前页起始地址
        uint32_t page_start = FLASH_BASE + (((address - FLASH_BASE) >> 7) << 7);

        // 页内偏移
        uint32_t offset = address - page_start;

        // 本次写入长度（最多填满一页）
        size_t chunk_len = custom::min((size_t)(128 - offset), length - written);

        // 先读出整页到 buffer
        uint8_t buffer[128];
        Load(page_start, buffer, 128);

        // 更新 buffer 中的数据
        memcpy(buffer + offset, data + written, chunk_len);

        // 擦除该页
        uint32_t error = 0;
        FLASH_EraseInitTypeDef init{};
        init.TypeErase = FLASH_TYPEERASE_PAGEERASE;
        init.PageAddress = page_start;
        init.NbPages = 1;
        if (HAL_FLASH_Erase(&init, &error) != HAL_OK) {
            goto exit;
        }

        // 写入整页（128字节）
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, page_start, (uint32_t *)buffer) != HAL_OK) {
            goto exit;
        }

        // 更新写入进度
        written += chunk_len;
        address += chunk_len;
    }

exit:
    HAL_FLASH_Lock();
}

void InternalFlash::Load(uint32_t address, uint8_t *data, size_t length) {
	if (!data || length == 0) {
		return;
	}
	if (address < FLASH_BASE || address + length > FLASH_END) { 
		return; 
	}
	memcpy(data, (void *)address, length);
}
