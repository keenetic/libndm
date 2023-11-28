#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ndm/ip_sockaddr.h>
#include <ndm/net.h>
#include "test.h"

int main(const int argc, const char* argv[])
{
	size_t i;
	char test_domain[] = "a~a.com";

	for (i = 1; i < 256; ++i) {
		test_domain[1] = (char) i;

		NDM_TEST(
			((isalnum((int) i) || i == '-' || i == '.') &&
			 ndm_net_is_domain_name(test_domain)) ||
			(!(isalnum((int) i) || i == '-' || i == '.') &&
			 !ndm_net_is_domain_name(test_domain)));
	}

	NDM_TEST(!ndm_net_is_domain_name(""));
	NDM_TEST(!ndm_net_is_domain_name(" "));
	NDM_TEST(!ndm_net_is_domain_name("     "));
	NDM_TEST(!ndm_net_is_domain_name("domain.ru."));
	NDM_TEST(!ndm_net_is_domain_name("domain-.ru"));
	NDM_TEST(!ndm_net_is_domain_name("domain--.ru"));
	NDM_TEST(!ndm_net_is_domain_name("-domain.ru"));
	NDM_TEST(!ndm_net_is_domain_name("-domain..ru"));
	NDM_TEST(!ndm_net_is_domain_name("domain.-.ru"));
	NDM_TEST(!ndm_net_is_domain_name("domain.test-.ru"));
	NDM_TEST(!ndm_net_is_domain_name(".domain.ru"));
	NDM_TEST(!ndm_net_is_domain_name("-.domain.ru"));
	NDM_TEST(!ndm_net_is_domain_name("-.domain-a.ru"));
	NDM_TEST(!ndm_net_is_domain_name("t-.domain-a.ru"));
	NDM_TEST(!ndm_net_is_domain_name("-.ni"));
	NDM_TEST(!ndm_net_is_domain_name(
		"very-long-long-long-long-long"
		"long-long-long-long-long-domainname.005.com"));
	NDM_TEST(!ndm_net_is_domain_name("domain-0.ru  "));
	NDM_TEST(!ndm_net_is_domain_name("  a0-domain.ru"));
	NDM_TEST(!ndm_net_is_domain_name(" 005.com  "));
	NDM_TEST(!ndm_net_is_domain_name("   ru.shortn   "));

	NDM_TEST(ndm_net_is_domain_name("domain"));
	NDM_TEST(ndm_net_is_domain_name("a.a"));
	NDM_TEST(ndm_net_is_domain_name(
		"not-so-long-long-long-long-long-"
		"long-long-long-long-domain-name.005.com"));
	NDM_TEST(ndm_net_is_domain_name("ru.veryverylonglonglonglonglongname"));
	NDM_TEST(ndm_net_is_domain_name("ru.longna"));
	NDM_TEST(ndm_net_is_domain_name("test-d.domain-a.ru"));
	NDM_TEST(ndm_net_is_domain_name("te--st------d.do----main-a.ru"));
	NDM_TEST(ndm_net_is_domain_name("domain-0.ru"));
	NDM_TEST(ndm_net_is_domain_name("a0-domain.ru"));
	NDM_TEST(ndm_net_is_domain_name("005.com"));
	NDM_TEST(ndm_net_is_domain_name("ru.shortn"));
	NDM_TEST(ndm_net_is_domain_name("ru.ru"));
	NDM_TEST(ndm_net_is_domain_name("a.ni"));

	if (argc == 2) {
		struct addrinfo hints;
		struct addrinfo *res;
		struct ndm_ip_sockaddr_t sa;
		char buf[NDM_IP_SOCKADDR_LEN];

		memset(&hints, 0, sizeof(hints));

		NDM_TEST(ndm_net_getaddrinfo(argv[1], NULL, &hints, &res) == 0);

		while (res != NULL) {
			struct addrinfo *ai = res;

			NDM_TEST(ai->ai_family == AF_INET || ai->ai_family == AF_INET6);

			if (ai->ai_family == AF_INET) {
				ndm_ip_sockaddr_assign(&sa, (struct sockaddr_in *)ai->ai_addr);
			} else
			if (ai->ai_family == AF_INET6) {
				ndm_ip_sockaddr_assign6(&sa, (struct sockaddr_in6 *)ai->ai_addr);
			}

			printf("%s %s\n",
				ai->ai_family == AF_INET ? "A" : "AAAA",
				ndm_ip_sockaddr_ntop(&sa, buf, sizeof(buf)));

			res = res->ai_next;
		}

		ndm_net_freeaddrinfo(res);

	}

	return NDM_TEST_RESULT;
}

