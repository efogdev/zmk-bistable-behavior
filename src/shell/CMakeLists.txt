target_sources_ifdef(CONFIG_ZMK_BISTABLE_BEHAVIOR_SHELL app PRIVATE bistable_shell.c)
set_source_files_properties(bistable_shell.c PROPERTIES COMPILE_FLAGS "-Os")
