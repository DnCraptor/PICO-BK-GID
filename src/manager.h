#pragma once
#include <inttypes.h>
#include <stdbool.h>

int if_manager(bool force);
void notify_image_insert_action(uint8_t drivenum, char *pathname);
bool handleScancode(uint32_t ps2scancode);
void reset(uint8_t cmd);

extern bool SD_CARD_AVAILABLE;
