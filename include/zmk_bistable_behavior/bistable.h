#pragma once

#define ZBS_DEFAULT_SLOT_KEY "bst/default"

uint8_t zbs_get_slot(void);
int zbs_set_slot(uint8_t slot);
int zbs_toggle_slot(void);
