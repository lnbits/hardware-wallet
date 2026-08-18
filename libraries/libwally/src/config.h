#ifndef LIBWALLYCORE_CONFIG_H
#define LIBWALLYCORE_CONFIG_H

/* Every Bowser target is little-endian. Keep access conservative across the
 * Xtensa ESP32 and RISC-V ESP32-C6 rather than relying on unaligned loads. */
#define HAVE_UNALIGNED_ACCESS 0

/* GCC supports the empty compiler barrier libwally uses to keep secret-memory
 * clearing from being optimized away. */
#define HAVE_INLINE_ASM 1

#include "ccan_config.h"

#endif /* LIBWALLYCORE_CONFIG_H */

