#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <zmk_bistable_behavior/bistable.h>

LOG_MODULE_DECLARE(zmk_bistable_behavior, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_SHELL) && IS_ENABLED(CONFIG_ZMK_BISTABLE_BEHAVIOR_SHELL)

static int cmd_slot(const struct shell *sh, size_t argc, char **argv) {
    shell_print(sh, "bistable slot: %d", zbs_get_slot());
    return 0;
}

static int cmd_set(const struct shell *sh, size_t argc, char **argv) {
    if (argc < 2) {
        shell_print(sh, "Usage: bistable set <0|1>");
        return -EINVAL;
    }

    char *endptr;
    long val = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0' || (val != 0 && val != 1)) {
        shell_print(sh, "Error: slot must be 0 or 1");
        return -EINVAL;
    }

    const int rc = zbs_set_slot((uint8_t)val);
    if (rc == 0) {
        shell_print(sh, "bistable slot: %d", (int)val);
    } else {
        shell_print(sh, "Error: %d", rc);
    }
    return rc;
}

static int cmd_toggle(const struct shell *sh, size_t argc, char **argv) {
    const int rc = zbs_toggle_slot();
    if (rc == 0) {
        shell_print(sh, "bistable slot: %d", zbs_get_slot());
    } else {
        shell_print(sh, "Error: %d", rc);
    }
    return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bistable,
    SHELL_CMD(slot,   NULL, "Show current bistable slot", cmd_slot),
    SHELL_CMD(set,    NULL, "Set bistable slot (0 or 1)", cmd_set),
    SHELL_CMD(toggle, NULL, "Toggle bistable slot",       cmd_toggle),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bistable, &sub_bistable, "Bistable behavior control", NULL);

#endif
