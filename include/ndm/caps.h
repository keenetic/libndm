#ifndef __NDM_CAPS_H__
#define __NDM_CAPS_H__

#include <stdint.h>
#include <stdbool.h>
#include "sys.h"

#include <linux/capability.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ndm_drop_privileges(const char *const user, const uint32_t caps);

#ifdef __cplusplus
}
#endif

#endif	/* __NDM_CAPS_H__ */

