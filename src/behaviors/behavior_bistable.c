#define DT_DRV_COMPAT zmk_behavior_bistable

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

#include <zmk_bistable_behavior/bistable.h>

LOG_MODULE_DECLARE(zmk_bistable_behavior, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_bistable_config {
    struct zmk_behavior_binding *bindings;
};

struct bistable_held_key {
    uint32_t position;
    uint8_t  slot;
};

static struct bistable_held_key held_keys[CONFIG_ZMK_BISTABLE_BEHAVIOR_MAX_HELD];

static struct bistable_held_key *find_held(uint32_t position) {
    for (int i = 0; i < CONFIG_ZMK_BISTABLE_BEHAVIOR_MAX_HELD; i++) {
        if (held_keys[i].position == position) {
            return &held_keys[i];
        }
    }
    return NULL;
}

static struct bistable_held_key *alloc_held(uint32_t position, uint8_t slot) {
    for (int i = 0; i < CONFIG_ZMK_BISTABLE_BEHAVIOR_MAX_HELD; i++) {
        if (held_keys[i].position == UINT32_MAX) {
            held_keys[i].position = position;
            held_keys[i].slot = slot;
            return &held_keys[i];
        }
    }
    return NULL;
}

static void free_held(struct bistable_held_key *key) {
    key->position = UINT32_MAX;
}

static int on_bistable_binding_pressed(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_bistable_config *cfg = dev->config;
    const uint8_t slot = zbs_get_slot();

    if (alloc_held(event.position, slot) == NULL) {
        LOG_ERR("No free held slot for position %d", event.position);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return zmk_behavior_invoke_binding(&cfg->bindings[slot], event, true);
}

static int on_bistable_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_bistable_config *cfg = dev->config;

    struct bistable_held_key *held = find_held(event.position);
    const uint8_t slot = held ? held->slot : zbs_get_slot();
    if (held) {
        free_held(held);
    }

    return zmk_behavior_invoke_binding(&cfg->bindings[slot], event, false);
}

static int behavior_bistable_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static const struct behavior_driver_api behavior_bistable_driver_api = {
    .binding_pressed  = on_bistable_binding_pressed,
    .binding_released = on_bistable_binding_released,
};

#define _TRANSFORM_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)
#define TRANSFORMED_BINDINGS(node) \
    {LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_ENTRY, (, ), DT_DRV_INST(node))}

#define BISTABLE_INST(n)                                                                           \
    static struct zmk_behavior_binding                                                             \
        behavior_bistable_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                  \
            TRANSFORMED_BINDINGS(n);                                                               \
    static const struct behavior_bistable_config behavior_bistable_config_##n = {                  \
        .bindings = behavior_bistable_config_##n##_bindings,                                       \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_bistable_init, NULL, NULL,                                 \
                            &behavior_bistable_config_##n, POST_KERNEL,                            \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &behavior_bistable_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BISTABLE_INST)

static int bistable_held_keys_init(void) {
    for (int i = 0; i < CONFIG_ZMK_BISTABLE_BEHAVIOR_MAX_HELD; i++) {
        held_keys[i].position = UINT32_MAX;
    }
    return 0;
}

SYS_INIT(bistable_held_keys_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
