add_subdirectory(behaviors)
add_subdirectory(shell)
target_sources_ifdef(CONFIG_ZMK_BISTABLE_BEHAVIOR app PRIVATE bistable.c)
