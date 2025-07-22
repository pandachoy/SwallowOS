#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char* my_itoa(int value, char* str, int base)
{
	if (base < 2 || base > 32)
	{
		printf("Wrong radix!\n");
		return str;
	}
	char* ret = str;
	if (value == 0)
	{
		*str++ = '0';
		*str = '\0';
		return ret;
	}
	if (base == 10 && value < 0)
	{
		value = -value;
		*str++ = '-';
	}
	char* start = str;
	// 从右到左依次将数字的每一位存储起来
	size_t num = value;
	while (num != 0)
	{
		if (num % base < 10)
		{
			*str++ = '0' + (char)(num % base);
		}
		else
		{
			*str++ = 'a' + (char)(num % base - 10);
		}
		num /= base;
	}
	*str = '\0';
	// 倒置字符串
	for (char* left = start, *right = str - 1; left < right; left++, right--)
	{
		char tmp = *left;
		*left = *right;
		*right = tmp;
	}
	return ret;
}

static bool print(const char *data, size_t length) {
    const unsigned char *bytes = (const unsigned char*) data;
    for (size_t i = 0; i < length; ++i)
        if (putchar(bytes[i]) == EOF)
            return false;
    return true;
}

int printf(const char*restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = 0;
    while (*format != 0) {
        size_t maxrem = INT_MAX - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') /* two % */
                format++;
            size_t amount = 1;
            while (format[amount] && format[amount] != '%')
                amount++;
            if (maxrem < amount) {
                // TODO: set errno -to EOVERFLOW.
                return -1;
            }
            if (!print(format, amount))
                return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char *format_begun_at = format++;

        if (*format == 'c') {
            format++;
            char c = (char) va_arg(parameters, int /* char promotes to int */);
            if (!maxrem) {
                // TODO: Set errno to EOVERFLOW
                return -1;
            }
            if (!print(&c, sizeof(c)))
                return -1;
            written++;
        } else if (*format == 's') {
            format++;
            const char *str = va_arg(parameters, const char*);
            size_t len = strlen(str);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW
                return -1;
            }
            if (!print(str, len))
                return -1;
            written += len;
        } else if (*format == 'd') {
            format++;
            char str[21] = {0};
            int num = (char) va_arg(parameters, int);
            my_itoa(num, str, 10);
            size_t len = strlen(str);
            if (!print(str, strlen(str)))
                return -1;
            written += len;
        } else {
            format = format_begun_at;
            size_t len = strlen(format);
            if (maxrem < len) {
                // TODO: Set errno to EOVERFLOW.
                return -1;
            }
            if (!print(format, len))
                return -1;
            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
}