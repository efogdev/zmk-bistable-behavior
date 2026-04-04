#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include <zmk_bistable_behavior/bistable.h>

LOG_MODULE_REGISTER(zmk_bistable_behavior, CONFIG_ZMK_LOG_LEVEL);

#define ZBS_NVS_KEY "bistable/slot"

static uint8_t current_slot;

uint8_t zbs_get_slot(void) {
    return current_slot;
}

int zbs_set_slot(uint8_t slot) {
    if (slot > 1) {
        return -EINVAL;
    }
    current_slot = slot;
    const int rc = settings_save_one(ZBS_NVS_KEY, &current_slot, sizeof(current_slot));
    if (rc != 0) {
        LOG_ERR("Failed to save bistable slot: %d", rc);
    } else {
        LOG_DBG("Slot set to %d", slot);
    }
    return rc;
}

int zbs_toggle_slot(void) {
    return zbs_set_slot(current_slot ^ 1u);
}

static int zbs_settings_load_cb(const char *name, size_t len,
                                settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "slot") != 0) {
        return 0;
    }
    if (len != sizeof(uint8_t)) {
        return 0;
    }
    uint8_t val;
    const int rd = read_cb(cb_arg, &val, sizeof(val));
    if (rd == sizeof(uint8_t) && val <= 1) {
        current_slot = val;
        LOG_DBG("Loaded slot = %d", val);
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(zbs_settings, "bistable",
                                NULL, zbs_settings_load_cb, NULL, NULL);
