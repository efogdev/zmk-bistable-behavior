#define DT_DRV_COMPAT zmk_behavior_bistable_toggle

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk_bistable_behavior/bistable.h>

LOG_MODULE_DECLARE(zmk_bistable_behavior, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_bistable_toggle_binding_pressed(struct zmk_behavior_binding *binding,
                                              const struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    switch (binding->param1) {
    case 0:
        return zbs_set_slot(0);
    case 1:
        return zbs_set_slot(1);
    default:
        return zbs_toggle_slot();
    }
}

static int behavior_bistable_toggle_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

static const struct behavior_driver_api behavior_bistable_toggle_driver_api = {
    .binding_pressed = on_bistable_toggle_binding_pressed,
};

#define BISTABLE_TOGGLE_INST(n)                                                                    \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_bistable_toggle_init, NULL, NULL, NULL,                    \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_bistable_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BISTABLE_TOGGLE_INST)

#endif
