/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Data format conversion
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"

#include <hal/hal.h>


#define FLAG_SIGNED           0x1
#define FLAG_64BIT            0x2
#define FLAG_FLOAT            0x4
#define FLAG_SPACE            0x10
#define FLAG_ZERO             0x20
#define FLAG_PLUS             0x40
#define FLAG_HEX              0x80
#define FLAG_OCT              0x100
#define FLAG_LARGE_DIGITS     0x200
#define FLAG_ALTERNATE        0x400
#define FLAG_NULLMARK         0x800
#define FLAG_MINUS            0x1000
#define FLAG_FIELD_WIDTH_STAR 0x2000
#define FLAG_DOUBLE           0x4000


#define GET_UNSIGNED(number, flags, args) \
	do { \
		if ((flags) & FLAG_64BIT) \
			(number) = va_arg((args), u64); \
		else \
			(number) = va_arg((args), u32); \
	} while (0)


#define GET_SIGNED(number, flags, args) \
	do { \
		if ((flags) & FLAG_64BIT) \
			(number) = va_arg((args), s64); \
		else \
			(number) = va_arg((args), s32); \
	} while (0)


typedef union {
	float f;
	u32 u;
} float_u32;


typedef union {
	double d;
	u64 u;
} double_u64;


static inline double lib_formatDoubleFromU64(u64 ui)
{
	double_u64 u = { .u = ui };
	return u.d;
}


static inline u64 lib_formatU64FromDouble(double d)
{
	double_u64 u = { .d = d };
	return u.u;
}


static inline float lib_formatFloatFromU32(u32 ui)
{
	float_u32 u = { .u = ui };
	return u.f;
}


static inline u32 lib_formatU32FromFloat(float f)
{
	float_u32 u = { .f = f };
	return u.u;
}


static void *lib_formatMemchr(const void *s, int c, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i, ++s) {
		if (*(char *)s == c)
			return (void *)s;
	}

	return NULL;
}


static float lib_formatModff(float x, float *integral_out)
{
	float high_int, low, low_int, frac;
	int h;

	if (x > 1048576.0f) {
		h = (int)(x / 1048576.0f);
		high_int = (float)h * 1048576.0f;
		low = x - high_int;

		low_int = (float)(int)low;
		frac = low - low_int;
		*integral_out = high_int * 1048576.0f + low_int;

		return frac;
	}

	low_int = (float)(int)x;
	frac = x - low_int;
	*integral_out = low_int;

	return frac;
}


static u32 lib_formatFracToU32(float frac, int float_frac_len, float *overflow)
{
	const u32 s_powers10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
	const u32 mult = s_powers10[float_frac_len];
	u32 ret;

	frac *= (float)mult;

	/* Ensure proper rounding */
	frac += 0.5f;
	ret = (u32)(s32)frac;

	if (ret >= mult) {
		*overflow += 1;
		ret -= mult;
	}

	return ret;
}


static void lib_formatSprintfNum(void *ctx, void (*feed)(void *, char), u64 num64, u32 flags, int min_number_len, int float_frac_len)
{
	const char *digits = (flags & FLAG_LARGE_DIGITS) ? "0123456789ABCDEF" : "0123456789abcdef";
	const char *prefix = (flags & FLAG_LARGE_DIGITS) ? "X0" : "x0", *s;
	u32 num32, num_high, frac32, frac_left, higher_part, lower_part;
	int pad_len, frac_len, int_len, temp, exp = 0, exps = 0, tz = 1;
	char tmp_buf[32], sign = 0, *tmp = tmp_buf;
	float f, integral, fracf, higher_part_f;
	long long frac, temp1, temp2;
	double d, exp_val;
	unsigned int i;

	if ((flags & FLAG_NULLMARK) && !num64) {
		s = "(nil)";

		for (i = 0; i < hal_strlen(s); ++i)
			feed(ctx, s[i]);
	}

	if (flags & FLAG_DOUBLE) {
		d = lib_formatDoubleFromU64(num64);

		/* Check sign */
		if ((num64 >> 63) & 1) {
			sign = '-';
			d = -d;
		}
		num64 = lib_formatU64FromDouble(d);

		/* Check zero */
		if (!(num64 & 0x7fffffffffffffff)) {
			if (sign)
				feed(ctx, sign);
			feed(ctx, '0');
			sign = 0;
			return;
		}

		/* Check special cases */
		if (((num64 >> 52) & 0x7ff) == 0x7ff) {
			if (!(num64 & 0xfffffffffffff)) {
				/* Infinity */
				s = "inf";
				if (sign)
					feed(ctx, sign);
			}
			else {
				s = "NaN";
			}

			for (i = 0; i < hal_strlen(s); ++i)
				feed(ctx, s[i]);
			sign = 0;
			return;
		}

		frac_len = float_frac_len ? float_frac_len : 1;
		int_len = 1;

		if ((num64 >> 52) < 0x3ff) {
			temp = ((int)((num64 >> 52) & 0x7ff) - 1023) / -4;
			exp_val = 10;

			for (i = 0; i < 9; ++i) {
				if ((temp >> i) & 1)
					d *= exp_val;
				exp_val *= exp_val;
			}
			temp1 = d;

			num64 = lib_formatU64FromDouble(d);
			/* Shift fraction left until it's >= 1*/
			while ((num64 >> 52) < 0x3ff) {
				++exp;
				d *= 10;
				num64 = lib_formatU64FromDouble(d);
			}
			exp = temp + exp;
		}

		if (!exp) {
			/* No shifting - number is larger than 1 */
			exps = 1;
			temp = ((int)((num64 >> 52) & 0x7ff) - 1023) / 4;
			exp_val = 10;

			for (i = 0; i < 9; ++i) {
				if ((temp >> i) & 1)
					d /= exp_val;
				exp_val *= exp_val;
			}

			num64 = lib_formatU64FromDouble(d);
			while ((num64 >> 52) >= 0x3ff) {
				++exp;
				d /= 10;
				num64 = lib_formatU64FromDouble(d);
			}

			d *= 10;
			--exp;
			exp = temp + exp;
			if (exp < float_frac_len) {
				int_len += exp;
				frac_len -= exp;
				while (exp--)
					d *= 10;
				exp = 0;
				frac = d;
			}
		}
		temp = frac_len;

		while (frac_len--)
			d *= 10;

		frac_len = temp - 1;
		frac = d;
		temp1 = frac / 10;

		/* Rounding */
		frac += 5;
		frac /= 10;
		temp2 = frac;

		while (frac_len--) {
			temp1 /= 10;
			temp2 /= 10;
		}

		frac_len = temp - 1;
		temp = int_len - 1;

		while (temp--) {
			temp1 /= 10;
			temp2 /= 10;
		}

		if (temp2 > temp1) {
			frac /= 10;
			if (exps == 1)
				++exp;
			else
				--exp;
		}

		if ((exp > frac_len) || (!exps & (exp >= 5))) {
			*tmp++ = digits[exp % 10];
			exp /= 10;
			if (!exp) {
				*tmp++ = '0';
			}
			else {
				while (exp) {
					*tmp++ = digits[exp % 10];
					exp /= 10;
				}
			}

			/* Check exp sign */
			if (exps)
				*tmp++ = '+';
			else
				*tmp++ = '-';
			*tmp++ = 'e';
		}

		while (frac_len--) {
			if (frac % 10 || !tz) {
				tz = 0;
				*tmp++ = digits[frac % 10];
			}
			frac /= 10;
		}

		if (exp) {
			tz = 0;
			*tmp++ = digits[frac % 10];
			frac /= 10;
			while (--exp)
				*tmp++ = '0';
		}

		if (!tz)
			*tmp++ = '.';

		while (int_len--) {
			*tmp++ = digits[frac % 10];
			frac /= 10;
		}
		/* We need num32 to be 0 and num64 to not be 0 so the rest of the function won't mess our conversion */
		num64 = 0x100000000;
	}

	if (flags & FLAG_FLOAT) {
		f = lib_formatFloatFromU32((u32)num64);

		if (f < 0) {
			sign = '-';
			f = -f;
		}
		fracf = lib_formatModff(f, &integral);

		/* Max decimal digits to fit into uint32 : 9 */
		if (float_frac_len > 9)
			float_frac_len = 9;

		frac32 = lib_formatFracToU32(fracf, float_frac_len, &integral);
		frac_left = float_frac_len;

		while (frac_left-- > 0) {
			*tmp++ = digits[frac32 % 10];
			frac32 /= 10;
		}

		if ((float_frac_len > 0) || (flags & FLAG_ALTERNATE))
			*tmp++ = '.';

		if (integral < 4294967296.0f) {
			num64 = (u32)integral;
		}
		else {
			higher_part_f = (integral / 4294967296.0f);
			higher_part = (u32)higher_part_f;
			lower_part = (u32)(integral - (higher_part * 4294967296.0f));
			num64 = (((u64)higher_part) << 32) | lower_part;
			flags |= FLAG_64BIT;
		}
	}

	num32 = (u32)num64;
	num_high = (u32)(num64 >> 32);

	if (flags & FLAG_SIGNED) {
		if (flags & FLAG_64BIT) {
			if ((s32)num_high < 0) {
				num64 = -(s64)num64;
				num32 = (u32)num64;
				num_high = (u32)(num64 >> 32);
				sign = '-';
			}
		}
		else {
			if ((s32)num32 < 0) {
				num32 = -(s32)num32;
				sign = '-';
			}
		}

		if (!sign) {
			if (flags & FLAG_SPACE)
				sign = ' ';
			else if (flags & FLAG_PLUS)
				sign = '+';
		}
	}

	if ((flags & FLAG_64BIT) && !num_high)
		flags &= ~FLAG_64BIT;

	if (!num64) {
		*tmp++ = '0';
	}
	else if (flags & FLAG_HEX) {
		if (flags & FLAG_64BIT) {
			for (i = 0; i < 8; ++i) {
				*tmp++ = digits[num32 & 0x0f];
				num32 >>= 4;
			}
			while (num_high) {
				*tmp++ = digits[num_high & 0x0f];
				num_high >>= 4;
			}
		}
		else {
			while (num32) {
				*tmp++ = digits[num32 & 0x0f];
				num32 >>= 4;
			}
		}
		if (flags & FLAG_ALTERNATE) {
			hal_memcpy(tmp, prefix, 2);
			tmp += 2;
		}
	}
	else if (flags & FLAG_OCT) {
		if (flags & FLAG_64BIT) {
			// 30 bits
			for (i = 0; i < 10; ++i) {
				*tmp++ = digits[num32 & 0x07];
				num32 >>= 3;
			}
			// 31, 32 bit from num32, bit 0 from num_high
			num32 |= (num_high & 0x1) << 2;
			*tmp++ = digits[num32 & 0x07];
			num_high >>= 1;

			while (num_high) {
				*tmp++ = digits[num_high & 0x07];
				num_high >>= 3;
			}
		}
		else {
			while (num32) {
				*tmp++ = digits[num32 & 0x07];
				num32 >>= 3;
			}
		}
		if (flags & FLAG_ALTERNATE)
			*tmp++ = '0';
	}
	else {
		if (flags & FLAG_64BIT) { /* TODO: optimize */
			while (num64) {
				*tmp++ = digits[num64 % 10];
				num64 /= 10;
			}
		}
		else {
			while (num32) {
				*tmp++ = digits[num32 % 10];
				num32 /= 10;
			}
		}
	}
	pad_len = min_number_len - (tmp - tmp_buf) - !!sign;

	/* Pad, if needed */
	if ((pad_len > 0) && !(flags & FLAG_ZERO)) {
		while (pad_len-- > 0)
			feed(ctx, ' ');
	}

	if (sign)
		feed(ctx, sign);

	/* Pad, if needed */
	if ((pad_len > 0) && (flags & FLAG_ZERO)) {
		while (pad_len-- > 0)
			feed(ctx, '0');
	}

	/* Copy reversed */
	while ((--tmp) >= tmp_buf)
		feed(ctx, *tmp);
}


void lib_formatParse(void *ctx, void (*feed)(void *, char), const char *format, va_list args)
{
	const char *s;
	u32 flags, min_number_len;
	int float_frac_len;
	unsigned int i, l;
	char *p, c, fmt;
	u64 number;

	for (;;) {
		fmt = *format++;

		if (!fmt)
			break;

		if (fmt != '%') {
			feed(ctx, fmt);
			continue;
		}

		fmt = *format++;
		if (!fmt) {
			feed(ctx, '%');
			break;
		}

		/* Precision, padding (set default to 6 digits) */
		flags = 0;
		min_number_len = 0;
		float_frac_len = -1;

		for (;;) {
			if (fmt == ' ')
				flags |= FLAG_SPACE;
			else if (fmt == '-')
				flags |= FLAG_MINUS;
			else if (fmt == '0')
				flags |= FLAG_ZERO;
			else if (fmt == '+')
				flags |= FLAG_PLUS;
			else if (fmt == '#')
				flags |= FLAG_ALTERNATE;
			else if (fmt == '*')
				flags |= FLAG_FIELD_WIDTH_STAR;
			else
				break;

			fmt = *format++;
		}
		if (!fmt)
			break;

		/* Leading number digits-cnt */
		while ((fmt >= '0') && (fmt <= '9')) {
			min_number_len = min_number_len * 10 + fmt - '0';
			fmt = *format++;
		}

		if (flags & FLAG_FIELD_WIDTH_STAR)
			min_number_len = va_arg(args, int);

		if (!fmt)
			break;

		/* Fractional number digits-cnt (only a single digit is acceptable in this impl.) */
		if (fmt == '.') {
			float_frac_len = 0;
			fmt = *format++;
			while ((fmt >= '0') && (fmt <= '9')) {
				float_frac_len = float_frac_len * 10 + fmt - '0';
				fmt = *format++;
			}
		}
		if (!fmt)
			break;

		if (fmt == '*') {
			float_frac_len = va_arg(args, int);
			fmt = *format++;

			if (!fmt)
				break;
		}

		if (fmt == 'l') {
			fmt = *format++;

			if (fmt == 'l') {
				flags |= FLAG_64BIT;
				fmt = *format++;
			}
		}
		if (!fmt)
			break;

		if (fmt == 'z') {
			fmt = *format++;
			if (sizeof(size_t) == sizeof(u64))
				flags |= FLAG_64BIT;
		}
		if (!fmt)
			break;

		number = 0;
		switch (fmt) {
			case 's':
				s = va_arg(args, char *);
				if (s == NULL)
					s = "(null)";

				if (min_number_len > 0) {
					p = lib_formatMemchr(s, 0, min_number_len);
					if (p != NULL)
						l = p - s;
					else
						l = min_number_len;
				}
				else {
					l = hal_strlen(s);
				}

				if (float_frac_len >= 0)
					l = min(l, float_frac_len);

				if ((l < min_number_len) && !(flags & FLAG_MINUS)) {
					for (i = 0; i < (min_number_len - l); ++i)
						feed(ctx, ' ');
				}

				for (i = 0; i < l; ++i)
					feed(ctx, s[i]);

				if ((l < min_number_len) && (flags & FLAG_MINUS)) {
					for (i = 0; i < (min_number_len - l); ++i)
						feed(ctx, ' ');
				}
				break;

			case 'c':
				c = (char)va_arg(args, int);
				feed(ctx, c);
				break;

			case 'p':
				flags |= (FLAG_HEX | FLAG_NULLMARK | FLAG_ZERO);
				if (sizeof(void *) == sizeof(u64))
					flags |= FLAG_64BIT;
				min_number_len = sizeof(void *) * 2;
				GET_UNSIGNED(number, flags, args);
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;

			case 'o':
				flags |= FLAG_OCT;
				GET_UNSIGNED(number, flags, args);
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;

			case 'X':
				flags |= FLAG_LARGE_DIGITS;
			case 'x':
				flags |= FLAG_HEX;
			case 'u':
				GET_UNSIGNED(number, flags, args);
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;

			case 'd':
			case 'i':
				flags |= FLAG_SIGNED;
				GET_SIGNED(number, flags, args);
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;

#if 0
			case 'g':
				flags |= FLAG_DOUBLE;
				float_frac_len = float_frac_len >= 0 ? float_frac_len : 6;
				number = lib_formatU64FromDouble((double)va_arg(args, double));
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;

			case 'f':
				flags |= FLAG_FLOAT;
				float_frac_len = (float_frac_len >= 0) ? float_frac_len : 6;
				number = lib_formatU32FromFloat((float)va_arg(args, double));
				lib_formatSprintfNum(ctx, feed, number, flags, min_number_len, float_frac_len);
				break;
#endif

			case '%':
				feed(ctx, '%');
				break;

			default:
				feed(ctx, '%');
				feed(ctx, fmt);
				break;
		}
	}
}
