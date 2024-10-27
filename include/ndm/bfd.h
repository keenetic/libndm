#ifndef __NDM_BFD_H__
#define __NDM_BFD_H__

#include <stdint.h>
#include <stdbool.h>
#include "attr.h"
#include "ip_sockaddr.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ndm_bfd_str(
		const char *const from,
		const uint16_t port,
		const char *const addr);

bool ndm_bfd_sa(
		const char *const from,
		const uint16_t port,
		const struct ndm_ip_sockaddr_t *const sa);

#ifdef __cplusplus
}
#endif

#endif	/* __NDM_BFD_H__ */

