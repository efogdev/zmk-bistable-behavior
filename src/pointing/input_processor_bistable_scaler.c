#define DT_DRV_COMPAT zmk_input_processor_bistable_scaler

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <drivers/input_processor.h>
#include <zephyr/logging/log.h>

#include <zmk_bistable_behavior/bistable.h>

#if IS_ENABLED(CONFIG_ZMK_RUNTIME_CONFIG)
#include <zmk_runtime_config/runtime_config.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct bscaler_config {
    uint8_t type;
    uint32_t slot0;
    uint32_t slot1;
    uint32_t default_coef;
    const char *key_s0_mult;
    const char *key_s0_div;
    const char *key_s1_mult;
    const char *key_s1_div;
    size_t codes_len;
    uint16_t codes[];
};

struct bscaler_data {
    int32_t c_s0_mult;
    int32_t c_s0_div;
    int32_t c_s1_mult;
    int32_t c_s1_div;
    uint32_t last_refresh;
    bool initialized;
};

#if IS_ENABLED(CONFIG_ZMK_RUNTIME_CONFIG)
static void zrc_cache_refresh_if_due(const struct device *dev, const uint32_t now) {
    const struct bscaler_config *cfg = dev->config;
    struct bscaler_data *data = dev->data;

    if (likely(data->initialized) &&
        (now - data->last_refresh) < CONFIG_ZMK_INPUT_PROCESSOR_BISTABLE_SCALER_ZRC_POLL_MS) {
        return;
    }

    if (cfg->slot0 == 0) {
        data->c_s0_mult = zrc_get(cfg->key_s0_mult);
        data->c_s0_div = zrc_get(cfg->key_s0_div);
    }
    if (cfg->slot1 == 0) {
        data->c_s1_mult = zrc_get(cfg->key_s1_mult);
        data->c_s1_div = zrc_get(cfg->key_s1_div);
    }

    data->last_refresh = now;
    data->initialized = true;
}
#else
static inline void zrc_cache_refresh_if_due(const struct device *dev, const uint32_t now) {
    ARG_UNUSED(dev);
    ARG_UNUSED(now);
}
#endif

static int scale_val(struct input_event *event, const uint32_t mul, const uint32_t div,
                     struct zmk_input_processor_state *state) {
    int16_t value_mul = event->value * (int16_t)mul;
    if (state && state->remainder) {
        value_mul += *state->remainder;
    }

    const int16_t scaled = value_mul / (int16_t)div;
    if (state && state->remainder) {
        *state->remainder = value_mul - (scaled * (int16_t)div);
    }

    event->value = scaled;
    return ZMK_INPUT_PROC_CONTINUE;
}

static int bscaler_handle_event(const struct device *dev, struct input_event *event,
                                const uint32_t param1, const uint32_t param2,
                                struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    const struct bscaler_config *cfg = dev->config;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    bool relevant = false;
    for (size_t i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            relevant = true;
            break;
        }
    }
    if (!relevant) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    const uint8_t slot = zbs_get_slot();
    const uint32_t packed = slot ? cfg->slot1 : cfg->slot0;

    uint32_t mult;
    uint32_t div;
    if (packed != 0) {
        mult = packed >> 16;
        div = packed & 0xFFFF;
    } else {
#if IS_ENABLED(CONFIG_ZMK_RUNTIME_CONFIG)
        zrc_cache_refresh_if_due(dev, (uint32_t)k_uptime_get());
        const struct bscaler_data *data = dev->data;
        mult = (uint32_t)(slot ? data->c_s1_mult : data->c_s0_mult);
        div = (uint32_t)(slot ? data->c_s1_div : data->c_s0_div);
#else
        const uint32_t other = slot ? cfg->slot0 : cfg->slot1;
        const uint32_t fallback = other ? other : cfg->default_coef;
        mult = fallback >> 16;
        div = fallback & 0xFFFF;
#endif
    }

    if (div == 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    return scale_val(event, mult, div, state);
}

static struct zmk_input_processor_driver_api bscaler_api = {
    .handle_event = bscaler_handle_event,
};

#define BSCALER_DEFAULT(n, other_slot)                                                             \
    (DT_INST_PROP(n, other_slot) ? DT_INST_PROP(n, other_slot) : DT_INST_PROP_OR(n, default_coef, 0))

#define BSCALER_INST(n)                                                                            \
    BUILD_ASSERT(DT_INST_PROP(n, slot0) != 0 || DT_INST_PROP(n, slot1) != 0 ||                     \
                     DT_INST_PROP_OR(n, default_coef, 0) != 0,                                    \
                 "bistable-scaler: set at least one of slot0/slot1, or default-coef when both "    \
                 "slots are runtime");                                                             \
    static struct bscaler_data data_##n = {                                                        \
        .c_s0_mult = BSCALER_DEFAULT(n, slot1) >> 16,                                              \
        .c_s0_div = BSCALER_DEFAULT(n, slot1) & 0xFFFF,                                             \
        .c_s1_mult = BSCALER_DEFAULT(n, slot0) >> 16,                                              \
        .c_s1_div = BSCALER_DEFAULT(n, slot0) & 0xFFFF,                                             \
    };                                                                                             \
    static const struct bscaler_config config_##n = {                                              \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .slot0 = DT_INST_PROP(n, slot0),                                                           \
        .slot1 = DT_INST_PROP(n, slot1),                                                           \
        .default_coef = DT_INST_PROP_OR(n, default_coef, 0),                                     \
        .key_s0_mult = DT_INST_PROP(n, zrc_prefix) "/s0_mult",                                     \
        .key_s0_div = DT_INST_PROP(n, zrc_prefix) "/s0_div",                                       \
        .key_s1_mult = DT_INST_PROP(n, zrc_prefix) "/s1_mult",                                     \
        .key_s1_div = DT_INST_PROP(n, zrc_prefix) "/s1_div",                                       \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = DT_INST_PROP(n, codes),                                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &data_##n, &config_##n, POST_KERNEL,                      \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &bscaler_api);

DT_INST_FOREACH_STATUS_OKAY(BSCALER_INST)

#if IS_ENABLED(CONFIG_ZMK_RUNTIME_CONFIG)
#define BSCALER_REG(n)                                                                             \
    if (DT_INST_PROP(n, slot0) == 0) {                                                             \
        const uint32_t d = BSCALER_DEFAULT(n, slot1);                                              \
        zrc_register(DT_INST_PROP(n, zrc_prefix) "/s0_mult", d >> 16, 1, 255);                   \
        zrc_register(DT_INST_PROP(n, zrc_prefix) "/s0_div", d & 0xFFFF, 1, 255);                 \
    }                                                                                              \
    if (DT_INST_PROP(n, slot1) == 0) {                                                             \
        const uint32_t d = BSCALER_DEFAULT(n, slot0);                                              \
        zrc_register(DT_INST_PROP(n, zrc_prefix) "/s1_mult", d >> 16, 1, 255);                   \
        zrc_register(DT_INST_PROP(n, zrc_prefix) "/s1_div", d & 0xFFFF, 1, 255);                 \
    }

static int bscaler_register_runtime_params(void) {
    DT_INST_FOREACH_STATUS_OKAY(BSCALER_REG)
    return 0;
}
SYS_INIT(bscaler_register_runtime_params, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif
