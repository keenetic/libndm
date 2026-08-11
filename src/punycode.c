/**
 * Copyright (C) 2011 by Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <ndm/conv.h>
#include <ndm/net.h>
#include <ndm/punycode.h>

#include <stdio.h>
#include <errno.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

/* punycode parameters, see http://tools.ietf.org/html/rfc3492#section-5 */

#define BASE 36
#define TMIN 1
#define TMAX 26
#define SKEW 38
#define DAMP 700
#define INITIAL_N 128
#define INITIAL_BIAS 72

static uint32_t adapt_bias_(
		size_t delta,
		const size_t n_points,
		const int is_first)
{
	uint32_t k;

	delta /= is_first ? DAMP : 2;
	delta += delta / n_points;

	/* while delta > 455: delta /= 35 */
	for (k = 0; delta > ((BASE - TMIN) * TMAX) / 2; k += BASE) {
		delta /= (BASE - TMIN);
	}

	return (uint32_t)(k + (((BASE - TMIN + 1) * delta) / (delta + SKEW)));
}

static char encode_digit_(const size_t c)
{
	assert(c <= BASE - TMIN);

	if (c > 25) {
		return (char)c + 22; /* '0'..'9' */
	}

	return (char)c + 'a'; /* 'a'..'z' */
}

/* Encode as a generalized variable-length integer.
 * Returns number of bytes written. */
static size_t encode_var_int_(
		const size_t bias,
		const size_t delta,
		char *const dst,
		size_t dstlen)
{
	size_t i = 0;
	size_t k = BASE;
	size_t q = delta;
	size_t t = 0;

	while (i < dstlen) {
		if (k <= bias) {
			t = TMIN;
		} else
		if (k >= bias + TMAX) {
			t = TMAX;
		} else {
			t = k - bias;
		}

		if (q < t) {
			break;
		}

		dst[i++] = encode_digit_(t + (q - t) % (BASE - t));

		q = (q - t) / (BASE - t);
		k += BASE;
	}

	if (i < dstlen) {
		dst[i++] = encode_digit_(q);
	}

	return i;
}

static size_t decode_digit_(const uint32_t v)
{
	if (isdigit(v)) {
		return 26 + (v - '0');
	}

	if (islower(v)) {
		return v - 'a';
	}

	if (isupper(v)) {
		return v - 'A';
	}

	return SIZE_MAX;
}

size_t ndm_punycode_encode(
		const uint32_t *const src,
		const size_t srclen,
		char *const dst,
		size_t *const dstlen)
{
	size_t b, h;
	size_t delta, bias;
	size_t m, n;
	size_t si, di;

	for (si = 0, di = 0; si < srclen && di < *dstlen; si++) {
		if (src[si] < 0x80) {
			dst[di++] = (char)(src[si] & 0x7F);
		}
	}

	b = h = di;

	/* Write out delimiter if any basic code points were processed. */
	if (di > 0 && di < *dstlen) {
		dst[di++] = '-';
	}

	n = INITIAL_N;
	bias = INITIAL_BIAS;
	delta = 0;

	for (; h < srclen && di < *dstlen; n++, delta++) {
		/* Find next smallest non-basic code point. */
		for (m = SIZE_MAX, si = 0; si < srclen; si++) {
			if (src[si] >= n && src[si] < m) {
				m = src[si];
			}
		}

		if ((m - n) > (SIZE_MAX - delta) / (h + 1)) {
			/* OVERFLOW */

			goto fail;
		}

		delta += (m - n) * (h + 1);
		n = m;

		for (si = 0; si < srclen; si++) {
			if (src[si] < n) {
				if (++delta == 0) {
					/* OVERFLOW */

					goto fail;
				}
			} else
			if (src[si] == n) {
				di += encode_var_int_(bias, delta, &dst[di], *dstlen - di);
				bias = adapt_bias_(delta, h + 1, h == b);
				delta = 0;
				h++;
			}
		}
	}

fail:
	/* Tell the caller how many bytes were written to the output buffer. */
	*dstlen = di;

	/* Return how many Unicode code points were converted. */
	return si;
}

size_t ndm_punycode_decode(
		const char *const src,
		const size_t srclen,
		uint32_t *const dst,
		size_t *const dstlen)
{
	const char *p;
	size_t b, n, t;
	size_t i, k, w;
	size_t si, di;
	size_t digit;
	size_t org_i;
	size_t bias;

	/* Ensure that the input contains only ASCII characters. */
	for (si = 0; si < srclen; si++) {
		if (src[si] & 0x80) {
			*dstlen = 0;
			return 0;
		}
	}

	/* Reverse-search for delimiter in input. */
	for (p = src + srclen - 1; p > src && *p != '-'; p--)
	{
	}

	assert( p >= src );

	b = (size_t)(p - src);

	/* Copy basic code points to output. */
	di = min(b, *dstlen);

	for (i = 0; i < di; i++) {
		dst[i] = (unsigned char)src[i];
	}

	i = 0;
	n = INITIAL_N;
	bias = INITIAL_BIAS;

	for (si = b + (b > 0); si < srclen && di < *dstlen; di++) {
		org_i = i;

		for (w = 1, k = BASE; di < *dstlen; k += BASE) {
			digit = decode_digit_((uint32_t)src[si++]);

			if (digit == SIZE_MAX) {
				goto fail;
			}

			if (digit > (SIZE_MAX - i) / w) {
				/* OVERFLOW */

				goto fail;
			}

			i += digit * w;

			if (k <= bias) {
				t = TMIN;
			} else
			if (k >= bias + TMAX) {
				t = TMAX;
			} else {
				t = k - bias;
			}

			if (digit < t) {
				break;
			}

			if (w > SIZE_MAX / (BASE - t)) {
				/* OVERFLOW */

				goto fail;
			}

			w *= BASE - t;
		}

		bias = adapt_bias_(i - org_i, di + 1, org_i == 0);

		if (i / (di + 1) > SIZE_MAX - n) {
			/* OVERFLOW */

			goto fail;
		}

		n += i / (di + 1);
		i %= (di + 1);

		memmove(dst + i + 1, dst + i, (di - i) * sizeof(uint32_t));
		dst[i++] = (uint32_t)n;
	}

fail:
	/* Tell the caller how many bytes were written to the output buffer. */
	*dstlen = di;

	return si;
}

bool ndm_punycode_decode_utf8(
		const char* const src,
		const size_t src_len,
		char* dst)
{
	if (src_len > NDM_NET_DOMAIN_MAX_LEN) {
		return false;
	}

	ndm_conv_t cd = ndm_conv_open("UTF-8", "UTF-32");

	if (cd < 0) {
		return false;
	}

	char name[NDM_NET_DOMAIN_MAX_LEN + 1];

	memcpy(name, src, src_len);
	name[src_len] = '\0';

	size_t name_size = strlen(name);

	if (name_size < NDM_NET_DOMAIN_MIN_LEN ||
		name_size > NDM_NET_DOMAIN_MAX_LEN)
	{
		ndm_conv_close(cd);

		return false;
	}

	size_t off = 0;
	size_t i = (size_t) -1;
	bool valid = false;

	do {
		/* A subdomain name should start with
		 * an alphanumeric character. */

		++i;

		if (!isalnum(name[i++])) {
			ndm_conv_close(cd);

			return false;
		}

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

		const size_t l = i - s;

		if (!isalnum(name[i - 1]) ||
			!(name[i] == '.' || i == name_size) ||
			l > NDM_NET_SUBDOMAIN_MAX_LEN)
		{
			ndm_conv_close(cd);

			return false;
		}

		if (l > 4 &&
			name[s + 0] == 'x' &&
			name[s + 1] == 'n' &&
			name[s + 2] == '-' &&
			name[s + 3] == '-')
		{
			/*
			 * ACE encoded IDNA subdomain
			 */

			const size_t idna_len = l - 4;

			name[i] = '\0';

			uint32_t fqdn_utf32[NDM_NET_SUBDOMAIN_MAX_LEN + 1];
			size_t out = sizeof(fqdn_utf32) / sizeof(fqdn_utf32[0]);
			const size_t dec = ndm_punycode_decode(
				name + s + 4, idna_len, fqdn_utf32, &out);

			if (dec < idna_len) {
				ndm_conv_close(cd);

				return false;
			}

			char fqdn_out[4 * (NDM_NET_SUBDOMAIN_MAX_LEN + 1)];
			const char* inp = (char*)fqdn_utf32;
			size_t inb = out * sizeof(fqdn_utf32[0]);
			char* outp = fqdn_out;
			size_t outb = sizeof(fqdn_out) - 1;

			const size_t ret = ndm_conv(cd, &inp, &inb, &outp, &outb);

			if (ret == (size_t) -1 || inb > 0 || outb < 1) {
				ndm_conv_close(cd);

				return false;
			}

			const size_t out_len = sizeof(fqdn_out) - outb - 1;

			memcpy(dst + off, fqdn_out, out_len);
			dst[off + out_len] = '.';
			dst[off + out_len + 1] = '\0';
			off += (out_len + 1);
		} else
		{
			/*
			 * no punycode at this level
			 */

			memcpy(dst + off, name + s, l);
			dst[off + l] = '.';
			dst[off + l + 1] = '\0';
			off += (l + 1);
		}

		valid = true;

	} while (i < name_size && name[i + 1] != '\0');

	ndm_conv_close(cd);

	return valid;
}
