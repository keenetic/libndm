#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ndm/sys.h>
#include <ndm/core.h>
#include <ndm/time.h>
#include <ndm/stdio.h>
#include <ndm/feedback.h>
#include "core.h"

#define NDM_FEEDBACK_WAIT_SLEEP_MSEC_				100		/* 0.1 sec. */

static
bool ndm_core_feedback_ve(
		const int64_t timeout_msec,
		const char *const argv[],
		const char *const env_argv[])
{
	bool done = false;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd >= 0) {
		int error = 0;
		struct sockaddr_un sa;
		const int n = snprintf(
			sa.sun_path, sizeof(sa.sun_path),
			"%s", NDM_CORE_FEEDBACK_SOCKET);

		if (n < 0 || n >= sizeof(sa.sun_path)) {
			errno = ENOBUFS;
		} else {
			sa.sun_family = AF_UNIX;

			if (connect(
					fd, (struct sockaddr *) &sa,
					(socklen_t) sizeof(sa)) == 0) {
				struct timespec intblock;
				struct timespec deadline;

				ndm_time_get_monotonic_plus_msec(
					&intblock, NDM_CORE_INTBLOCK_TIMEOUT);
				ndm_time_get_monotonic_plus_msec(
					&deadline, NDM_CORE_DEFAULT_TIMEOUT);

				done =
					ndm_core_send(fd, &intblock, &deadline, argv, env_argv) &&
					ndm_core_recv(fd, &intblock, &deadline);
			}
		}

		error = errno;
		close(fd);
		errno = error;
	}

	return done;
}

bool ndm_feedback(
		const int64_t timeout_msec,
		const char *const args[],
		const char *const env_format,
		...)
{
	va_list ap;
	int ret = -1;
	char *env;
	const char *env_fmt = (env_format == NULL) ? "" : env_format;
	bool answ = false;

	va_start(ap, env_format);
	ret = ndm_vasprintf(&env, env_fmt, ap);
	va_end(ap);

	if (ret < 0)
		return false;

	unsigned int i;
	size_t env_count = (ret > 0) ? 1u : 0u;
	const char **env_argv;

	for (i = 0; i < ret; i++) {
		if (env[i] == *NDM_FEEDBACK_ENV_SEPARATOR) {
			++env_count;
		}
	}

	++env_count; /* NULL terminator */

	if (env_count > SIZE_MAX / sizeof(char *)) {
		/* too many environment variables */
		env_argv = NULL;
		errno = ENOMEM;
	} else
	if ((env_argv = malloc(env_count * sizeof(char *))) != NULL) {
		int env_index = 0;
		char *env_start = env;
		bool valid_format = true;

		i = 0;

		while (i <= ret && valid_format) {
			if (env[i] == *NDM_FEEDBACK_ENV_SEPARATOR || i == ret) {
				if (env_start == &env[i] && i == ret) {
					/* skip last empty entry */
				} else {
					env[i] = '\0';
					env_argv[env_index++] = env_start;
					valid_format = (strchr(env_start, '=') != NULL);
					env_start = &env[i + 1];
				}
			}

			++i;
		}

		env_argv[env_index] = NULL;

		if (!valid_format) {
			/* no '=' symbol in an environment entry */
			errno = EINVAL;
		} else	{
			answ = ndm_feedback_ve(timeout_msec, args, env_argv);
		}

		free(env_argv);
	}

	free(env);

	return answ;
}

bool ndm_feedback_ve(
		const int64_t timeout_msec,
		const char *const argv[],
		const char *const env_argv[])
{
	return ndm_core_feedback_ve(timeout_msec, argv, env_argv);
}

