#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ndm/net.h>
#include <ndm/ip_sockaddr.h>
#include <ndm/poll.h>

#define NDM_NET_DOMAIN_MIN_LEN_			1
#define NDM_NET_DOMAIN_MAX_LEN_			253

#define NDM_NET_SUBDOMAIN_MAX_LEN_		63

#define NDM_NET_NDN_RPC_PORT_			54321
#define NDM_NET_NDN_RPC_TIMEOUT_		5000 // ms

#define NDM_NET_ANSWER_NOT_RES_			"not resolved"
#define NDM_NET_ANSWER_POLL_			"poll "

bool ndm_net_is_domain_name(const char *const name)
{
	// ^([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.) ->
	// -> ([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))$

	size_t name_size = strlen(name);
	bool valid = false;

	if (name_size >= NDM_NET_DOMAIN_MIN_LEN_ &&
		name_size <= NDM_NET_DOMAIN_MAX_LEN_)
	{
		size_t i = (size_t) -1;

		do {
			/* A subdomain name should start with
			 * an alphanumeric character. */

			++i;

			if (!isalnum(name[i++])) {
				valid = false;
			} else {
				/* the subdomain name should contain
				 * only alphanumeric characters and '-' symbols.
				 * It should end with an alphanumeric character
				 * and to be shorter than @c SUBDOMAIN_MAX_LEN_. */

				const size_t s = i - 1;

				while (
					i < name_size &&
					(isalnum(name[i]) || name[i] == '-'))
				{
					++i;
				}

				valid =
					isalnum(name[i - 1]) &&
					(name[i] == '.' || i == name_size) &&
					i - s <= NDM_NET_SUBDOMAIN_MAX_LEN_;
			}
		} while (valid && i < name_size);
	}

	return valid;
}

static int ndm_net_fill_addrinfo_(
		const char *node,
		struct addrinfo *prev,
		struct addrinfo **res)
{
	struct ndm_ip_sockaddr_t sa = NDM_IP_SOCKADDR_ANY;

	if (node == NULL ||
		!ndm_ip_sockaddr_pton(node, &sa) ||
		ndm_ip_sockaddr_is_equal(&sa, &NDM_IP_SOCKADDR_ANY))
	{
		return EAI_NONAME;
	}

	struct addrinfo *r = (struct addrinfo *) malloc(sizeof(*r));

	if (r == NULL) {
		return EAI_MEMORY;
	}

	memset(r, 0, sizeof(*r));

	r->ai_family = ndm_ip_sockaddr_family(&sa);
	r->ai_addrlen =
		r->ai_family == PF_INET ?
			sizeof(struct sockaddr_in) :
			sizeof(struct sockaddr_in6);

	struct sockaddr *s = (struct sockaddr *) malloc(sizeof(*s));

	if (s == NULL)
	{
		free(r);
		return EAI_MEMORY;
	}

	memset(s, 0, sizeof(*s));
	memcpy(s, &sa, r->ai_addrlen);

	r->ai_addr = s;
	r->ai_next = prev;
	*res = r;

	return 0;
}

int ndm_net_getaddrinfo(
		const char *node,
		const char *service,
		const struct addrinfo *hints,
		struct addrinfo **res)
{
	if (res == NULL) {
		errno = EFAULT;

		return EAI_SYSTEM;
	}

	*res = NULL;

	struct addrinfo dh;

	if (node != NULL && node[0] == '*' && node[1] == '\0') {
		node = NULL;
	}

	if (service != NULL && service[0] == '*' && service[1] == '\0') {
		service = NULL;
	}

	if (node == NULL && service == NULL) {
		return EAI_NONAME;
	}

	if (hints == NULL) {
		memset(&dh, 0, sizeof(dh));
		if (PF_UNSPEC != 0) {
			dh.ai_family = PF_UNSPEC;
		}
		hints = &dh;
	}

	if (hints->ai_flags & ~(AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST|
			AI_ADDRCONFIG|AI_V4MAPPED|AI_NUMERICSERV|AI_ALL)) {
		return EAI_BADFLAGS;
	}

	if ((hints->ai_flags & AI_CANONNAME) && node == NULL) {
		return EAI_BADFLAGS;
	}

	if (service != NULL) {
		return getaddrinfo(node, service, hints, res);
	}

	if (node != NULL) {
		const int exit_code = ndm_net_fill_addrinfo_(node, NULL, res);

		if (exit_code == 0) {
			return 0;
		}

		if (exit_code != EAI_NONAME) {
			return exit_code;
		}
	}

	if (node == NULL || !ndm_net_is_domain_name(node)) {
		return EAI_NONAME;
	}

	if (hints != NULL &&
		!(hints->ai_family == PF_UNSPEC || hints->ai_family == PF_INET))
	{
		return EAI_FAMILY;
	}

	char buffer[16 * 1024];

	/* request is "resolv-conf a <fqdn>" */

	const int req_size = snprintf(buffer, sizeof(buffer),
			"resolv-conf a %s", node);

	if (req_size < 0) {
		return EAI_SYSTEM;
	}

	if (req_size > (sizeof(buffer) - 1)) {
		errno = ENOMEM;

		return EAI_SYSTEM;
	}

	const int sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sockfd < 0) {
		return EAI_SYSTEM;
	}

	int flags = 0;

	if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1) {
		const int err = errno;

		close(sockfd);
		errno = err;

		return EAI_SYSTEM;
	}

	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		const int err = errno;

		close(sockfd);
		errno = err;

		return EAI_SYSTEM;
	}

	struct sockaddr_in ndnp_addr;

	memset(&ndnp_addr, 0, sizeof(ndnp_addr));

	ndnp_addr.sin_family = AF_INET;
	ndnp_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // loopback
	ndnp_addr.sin_port = htons(NDM_NET_NDN_RPC_PORT_);

	if (sendto(sockfd, &buffer, (size_t)req_size, 0,
			(const struct sockaddr *)&ndnp_addr, sizeof(ndnp_addr)) < 0)
	{
		const int err = errno;

		close(sockfd);
		errno = err;

		return EAI_SYSTEM;
	}

	struct pollfd pfd =
	{
		.fd = sockfd,
		.events = POLLIN,
		.revents = 0
	};

	const ssize_t n = ndm_poll(&pfd, 1, NDM_NET_NDN_RPC_TIMEOUT_);
	const int err_poll = errno;

	if (n < 0) {
		close(sockfd);
		errno = err_poll;

		return EAI_SYSTEM;
	}

	if (n == 0) {
		close(sockfd);
		errno = ETIMEDOUT;

		return EAI_SYSTEM;
	}

	if (!(pfd.revents & POLLIN)) {
		close(sockfd);
		errno = ETIMEDOUT;

		return EAI_SYSTEM;
	}

	if (pfd.revents & POLLHUP || pfd.revents & POLLERR) {
		close(sockfd);
		errno = ECONNRESET;

		return EAI_SYSTEM;
	}

	if (pfd.revents & POLLNVAL) {
		close(sockfd);
		errno = EBADF;

		return EAI_SYSTEM;
	}

	socklen_t ndnp_addr_len = 0;
	const ssize_t answ = recvfrom(sockfd, &buffer, sizeof(buffer) - 1, 0,
			(struct sockaddr *)&ndnp_addr, &ndnp_addr_len);
	const int err_answ = errno;

	close(sockfd);

	if (ndnp_addr_len != sizeof(ndnp_addr)) {
		errno = EIO;

		return EAI_SYSTEM;
	}

	if (answ < 0) {
		errno = err_answ;

		return EAI_SYSTEM;
	}

	if (answ == 0) {
		errno = EIO;

		return EAI_SYSTEM;
	}

	buffer[answ] = '\0';

	/* One of three answers can be "not resolved" */

	if (!strncmp(buffer, NDM_NET_ANSWER_NOT_RES_, (size_t)answ)) {
		return EAI_NONAME;
	}

	/* Second answer can be "poll <hex-token>" */

	if (!strncmp(buffer, NDM_NET_ANSWER_POLL_, (size_t)answ)) {
		return EAI_AGAIN;
	}

	/* Third answer is a list of "<fqdn> <a> <address>" */

	char *sptr = NULL;
	char *token = NULL;
	unsigned int step = 0;

	for (token = strtok_r(buffer, " ", &sptr), step = 0;
		 token != NULL;
		 ++step, token = strtok_r(NULL, " ", &sptr))
	{
		if (step % 3 != 2) {
			continue;
		}

		char ab[NDM_IP_SOCKADDR_LEN];
		char *pab = ab;
		char *pt = token;

		memset(ab, 0, sizeof(ab));

		/* strtok_r() often leaves delimiters, so strip it */

		while (*pt != '\0') {
			if (!isspace(*pt) && (pab < ab + NDM_IP_SOCKADDR_LEN)) {
				*pab = *pt;
				++pab;
			}
			++pt;
		}

		const int exit_code = ndm_net_fill_addrinfo_(ab, *res, res);

		if (exit_code == 0) {
			continue;
		}

		ndm_net_freeaddrinfo(*res);

		*res = NULL;

		return exit_code;
	}

	return *res == NULL ? EAI_NONAME : 0;
}

void ndm_net_freeaddrinfo(struct addrinfo *res)
{
	while (res != NULL) {
		struct addrinfo *curr = res;

		res = res->ai_next;

		free(curr->ai_addr);
		free(curr);
	}
}

const char *ndm_net_gai_strerror(int errcode)
{
	return gai_strerror(errcode);
}
