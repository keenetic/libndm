#ifndef __NDM_STRING_H__
#define __NDM_STRING_H__

#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

char *ndm_string_dup(
		const char *const s) NDM_ATTR_WUR;

char *ndm_string_ndup(
		const char *const s,
		const size_t size) NDM_ATTR_WUR;

#ifdef __cplusplus
}
#endif

#endif	/* __NDM_STRING_H__ */

