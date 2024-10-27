#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ndm/sys.h>
#include <ndm/core.h>
#include <ndm/time.h>
#include <ndm/bfd.h>
#include "core.h"

static bool __ndm_core_udp_send(
		const uint16_t port,
		const int64_t timeout_msec,
		const char *const argv[])
{
	bool done = false;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd >= 0) {
		int error = 0;
		struct sockaddr_in sa;

		memset(&sa, 0, sizeof(struct sockaddr_in));

		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // loopback
		sa.sin_port = htons(port);

		if (connect(
				fd, (struct sockaddr *) &sa,
				(socklen_t) sizeof(sa)) == 0) {
			struct timespec intblock;
			struct timespec deadline;
			const char* env_arg[1];

			env_arg[0] = NULL;

			ndm_time_get_monotonic_plus_msec(
				&intblock, NDM_CORE_INTBLOCK_TIMEOUT);
			ndm_time_get_monotonic_plus_msec(
				&deadline, NDM_CORE_DEFAULT_TIMEOUT);

			done = ndm_core_send(fd, &intblock, &deadline, argv, env_arg);
		}

		error = errno;
		close(fd);
		errno = error;
	}

	return done;
}

bool ndm_bfd_str(
		const char *const from,
		const uint16_t port,
		const char *const addr)
{
	const char* args[] =
	{
		from,
		addr,
		NULL
	};

	return __ndm_core_udp_send(port, NDM_SYS_SLEEP_GRANULARITY_MSEC, args);
}

bool ndm_bfd_sa(
		const char *const from,
		const uint16_t port,
		const struct ndm_ip_sockaddr_t *const sa)
{
	char buf[NDM_IP_SOCKADDR_LEN];

	if (!ndm_ip_sockaddr_ntop(sa, buf, sizeof(buf))) {
		return false;
	}

	return ndm_bfd_str(from, port, buf);
}
