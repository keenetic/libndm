#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <ndm/int.h>
#include <ndm/macro.h>
#include "test.h"

#define TEST_INT_STRLEN(t)											\
	do {															\
		char buf[sizeof(t)*CHAR_BIT];								\
		const int size = (NDM_INT_IS_SIGNED(t)) ?					\
			snprintf(buf, sizeof(buf), "%lli",						\
				(long long int) NDM_INT_MIN(t)) :					\
			snprintf(buf, sizeof(buf), "%llu",						\
				(unsigned long long int) NDM_INT_MIN(t));			\
																	\
		NDM_TEST_BREAK_IF(size < 0);								\
		NDM_TEST(NDM_INT_MAX_STRLEN(t) >= size);					\
	} while (false);

int main()
{
	char lb[NDM_INT_BUFSIZE];
	char c;
	short s;
	int i;
	long l;
	long long ll;
	unsigned char uc;
	unsigned short us;
	unsigned int ui;
	unsigned long ul;
	unsigned long long ull;

	NDM_TEST(NDM_INT_MIN(char) == CHAR_MIN);
	NDM_TEST(NDM_INT_MAX(char) == CHAR_MAX);

	NDM_TEST(NDM_INT_MIN(long) == LONG_MIN);
	NDM_TEST(NDM_INT_MAX(long) == LONG_MAX);

	NDM_TEST(
		NDM_INT_IS_SIGNED(time_t) ?
			(NDM_INT_MAX(time_t) <= LLONG_MAX) :
			(NDM_INT_MAX(time_t) <= ULLONG_MAX));

	TEST_INT_STRLEN(char);
	TEST_INT_STRLEN(unsigned char);
	TEST_INT_STRLEN(short);
	TEST_INT_STRLEN(unsigned short);
	TEST_INT_STRLEN(int);
	TEST_INT_STRLEN(unsigned int);
	TEST_INT_STRLEN(long int);
	TEST_INT_STRLEN(unsigned long int);

	TEST_INT_STRLEN(int64_t);
	TEST_INT_STRLEN(uint64_t);
	TEST_INT_STRLEN(intmax_t);
	TEST_INT_STRLEN(uintmax_t);

	NDM_TEST(!ndm_int_parse_char("", &c));
	NDM_TEST(!ndm_int_parse_char(" ", &c));
	NDM_TEST(!ndm_int_parse_char("0 ", &c));
	NDM_TEST(!ndm_int_parse_char(" 0", &c));
	NDM_TEST(!ndm_int_parse_char("999999999999999999999999999999999", &c));

	NDM_TEST(ndm_int_parse_char("1", &c) && c == 1);
	NDM_TEST(ndm_int_parse_char("-1", &c) && c == -1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", (int) CHAR_MAX) <= 0);
	NDM_TEST(ndm_int_parse_char(lb, &c) && c == CHAR_MAX);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", (int) CHAR_MIN) <= 0);
	NDM_TEST(ndm_int_parse_char(lb, &c) && c == CHAR_MIN);

	NDM_TEST(!ndm_int_parse_uchar("", &uc));
	NDM_TEST(!ndm_int_parse_uchar(" ", &uc));
	NDM_TEST(!ndm_int_parse_uchar("0 ", &uc));
	NDM_TEST(!ndm_int_parse_uchar(" 0", &uc));
	NDM_TEST(!ndm_int_parse_uchar("999999999999999999999999999999999", &uc));
	NDM_TEST(!ndm_int_parse_uchar("-1", &uc));

	NDM_TEST(ndm_int_parse_uchar("1", &uc) && uc == 1);

	NDM_TEST_BREAK_IF(snprintf(
		lb, sizeof(lb), "%u",
		(unsigned int) UCHAR_MAX) <= 0);
	NDM_TEST(ndm_int_parse_uchar(lb, &uc) && uc == UCHAR_MAX);

	NDM_TEST(!ndm_int_parse_short("", &s));
	NDM_TEST(!ndm_int_parse_short(" ", &s));
	NDM_TEST(!ndm_int_parse_short("0 ", &s));
	NDM_TEST(!ndm_int_parse_short(" 0", &s));
	NDM_TEST(!ndm_int_parse_short("999999999999999999999999999999999", &s));

	NDM_TEST(ndm_int_parse_short("1", &s) && s == 1);
	NDM_TEST(ndm_int_parse_short("-1", &s) && s == -1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", (int) SHRT_MAX) <= 0);
	NDM_TEST(ndm_int_parse_short(lb, &s) && s == SHRT_MAX);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", (int) SHRT_MIN) <= 0);
	NDM_TEST(ndm_int_parse_short(lb, &s) && s == SHRT_MIN);

	NDM_TEST(!ndm_int_parse_ushort("", &us));
	NDM_TEST(!ndm_int_parse_ushort(" ", &us));
	NDM_TEST(!ndm_int_parse_ushort("0 ", &us));
	NDM_TEST(!ndm_int_parse_ushort(" 0", &us));
	NDM_TEST(!ndm_int_parse_ushort(
		"999999999999999999999999999999999", &us));
	NDM_TEST(!ndm_int_parse_ushort("-1", &us));

	NDM_TEST(ndm_int_parse_ushort("1", &us) && us == 1);

	NDM_TEST_BREAK_IF(snprintf(
		lb, sizeof(lb), "%u",
		(unsigned int) USHRT_MAX) <= 0);
	NDM_TEST(ndm_int_parse_ushort(lb, &us) && us == USHRT_MAX);

	NDM_TEST(!ndm_int_parse_int("", &i));
	NDM_TEST(!ndm_int_parse_int(" ", &i));
	NDM_TEST(!ndm_int_parse_int("0 ", &i));
	NDM_TEST(!ndm_int_parse_int(" 0", &i));
	NDM_TEST(!ndm_int_parse_int("999999999999999999999999999999999", &i));

	NDM_TEST(ndm_int_parse_int("1", &i) && i == 1);
	NDM_TEST(ndm_int_parse_int("-1", &i) && i == -1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", INT_MAX) <= 0);
	NDM_TEST(ndm_int_parse_int(lb, &i) && i == INT_MAX);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%d", INT_MIN) <= 0);
	NDM_TEST(ndm_int_parse_int(lb, &i) && i == INT_MIN);

	NDM_TEST(!ndm_int_parse_uint("", &ui));
	NDM_TEST(!ndm_int_parse_uint(" ", &ui));
	NDM_TEST(!ndm_int_parse_uint("0 ", &ui));
	NDM_TEST(!ndm_int_parse_uint(" 0", &ui));
	NDM_TEST(!ndm_int_parse_uint(
		"999999999999999999999999999999999", &ui));
	NDM_TEST(!ndm_int_parse_uint("-1", &ui));

	NDM_TEST(ndm_int_parse_uint("1", &ui) && ui == 1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%u", UINT_MAX) <= 0);
	NDM_TEST(ndm_int_parse_uint(lb, &ui) && ui == UINT_MAX);

	NDM_TEST(!ndm_int_parse_long("", &l));
	NDM_TEST(!ndm_int_parse_long(" ", &l));
	NDM_TEST(!ndm_int_parse_long("0 ", &l));
	NDM_TEST(!ndm_int_parse_long(" 0", &l));
	NDM_TEST(!ndm_int_parse_long("999999999999999999999999999999999", &l));

	NDM_TEST(ndm_int_parse_long("1", &l) && l == 1);
	NDM_TEST(ndm_int_parse_long("-1", &l) && l == -1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%li", LONG_MAX) <= 0);
	NDM_TEST(ndm_int_parse_long(lb, &l) && l == LONG_MAX);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%li", LONG_MIN) <= 0);
	NDM_TEST(ndm_int_parse_long(lb, &l) && l == LONG_MIN);

	NDM_TEST(!ndm_int_parse_ulong("", &ul));
	NDM_TEST(!ndm_int_parse_ulong(" ", &ul));
	NDM_TEST(!ndm_int_parse_ulong("0 ", &ul));
	NDM_TEST(!ndm_int_parse_ulong(" 0", &ul));
	NDM_TEST(!ndm_int_parse_ulong("999999999999999999999999999999999", &ul));
	NDM_TEST(!ndm_int_parse_ulong("-1", &ul));

	NDM_TEST(ndm_int_parse_ulong("1", &ul) && ul == 1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%lu", ULONG_MAX) <= 0);
	NDM_TEST(ndm_int_parse_ulong(lb, &ul) && ul == ULONG_MAX);

	NDM_TEST(!ndm_int_parse_llong("", &ll));
	NDM_TEST(!ndm_int_parse_llong(" ", &ll));
	NDM_TEST(!ndm_int_parse_llong("0 ", &ll));
	NDM_TEST(!ndm_int_parse_llong(" 0", &ll));
	NDM_TEST(!ndm_int_parse_llong("999999999999999999999999999999999", &ll));

	NDM_TEST(ndm_int_parse_llong("1", &ll) && ll == 1);
	NDM_TEST(ndm_int_parse_llong("-1", &ll) && ll == -1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%lli", LLONG_MAX) <= 0);
	NDM_TEST(ndm_int_parse_llong(lb, &ll) && ll == LLONG_MAX);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%lli", LLONG_MIN) <= 0);
	NDM_TEST(ndm_int_parse_llong(lb, &ll) && ll == LLONG_MIN);

	NDM_TEST(!ndm_int_parse_ullong("", &ull));
	NDM_TEST(!ndm_int_parse_ullong(" ", &ull));
	NDM_TEST(!ndm_int_parse_ullong("0 ", &ull));
	NDM_TEST(!ndm_int_parse_ullong(" 0", &ull));
	NDM_TEST(!ndm_int_parse_ullong(
		"999999999999999999999999999999999", &ull));
	NDM_TEST(!ndm_int_parse_ullong("-1", &ull));

	NDM_TEST(ndm_int_parse_ullong("1", &ull) && ull == 1);

	NDM_TEST_BREAK_IF(snprintf(lb, sizeof(lb), "%llu", ULLONG_MAX) <= 0);
	NDM_TEST(ndm_int_parse_ullong(lb, &ull) && ull == ULLONG_MAX);

	return NDM_TEST_RESULT;
}

