#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ndm/sys.h>
#include <ndm/caps.h>

#include <pwd.h>
#include <grp.h>

#include <sys/syscall.h>
#include <sys/prctl.h>

#if !defined(capset)
long capset(cap_user_header_t hdrp, const cap_user_data_t datap)
{
    return syscall(SYS_capset, hdrp, datap);
}
#endif

static inline
bool ndm_keep_caps(void)
{
	return (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) != -1);
}

static inline
bool ndm_set_uid(const char *const user)
{
	struct group *grp;
	struct passwd *pwd;

	if (geteuid() != 0) {
		return true;
	}

	errno = 0;
	pwd = getpwnam(user);

	if (pwd == NULL) {
		return false;
	}

	errno = 0;
	grp = getgrnam(user);

	if (grp == NULL) {
		return false;
	}

	errno = 0;

	if (setgid(grp->gr_gid) == -1 ||
		setuid(pwd->pw_uid) == -1) {
		return false;
	}

	return true;
}

static inline
bool ndm_set_caps(const uint32_t caps)
{
	struct __user_cap_header_struct header = {
		.version = _LINUX_CAPABILITY_VERSION_3,
	};
	struct __user_cap_data_struct c[2];

	memset(&c, 0, sizeof(c));

	c[0].effective |= caps;
	c[0].permitted |= caps;
	c[0].inheritable |= caps;

	if (capset(&header, c) != 0 ||
		prctl(PR_SET_KEEPCAPS, 0, 0, 0, 0) == -1) {
		return false;
	}

	return true;
}

bool ndm_drop_privileges(const char *const user, const uint32_t caps)
{
	if (caps != 0 && !ndm_keep_caps()) {
		return false;
	}

	if (!ndm_set_uid(user)) {
		return false;
	}

	if (caps != 0 && !ndm_set_caps(caps)) {
		return false;
	}

	return true;
}
