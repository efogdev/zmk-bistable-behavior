#pragma once

#define ZBS_SLOT_0  0
#define ZBS_SLOT_1  1
#define ZBS_TOG     2

#define ZBS_SCALE(mult, div) ((((mult) & 0xFFFF) << 16) | ((div) & 0xFFFF))
#define ZBS_RUNTIME 0
