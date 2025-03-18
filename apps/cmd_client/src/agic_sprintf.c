#include <stdarg.h>
#include <string.h>
#include <limits.h>

void itoa(int num, char* str)
{
	if (num == INT_MIN) {
		strcpy(str, "-2147483648");
		return;
	}
	if (num < 0) {
		*str = '-';
		str++;
		num = -num;
	}
	if (num == 0) {
		*str = '0';
		str++;
		*str = '\0';
		return;
	}
	int temp = num;
	int count = 0;
	while (temp > 0) {
		temp /= 10;
		count++;
	}
	str[count] = '\0';
	temp = num;
	while (count > 0) {
		count--;
		str[count] = (temp % 10) + '0';
		temp /= 10;
	}
}

void uitoa(unsigned int num, char* str)
{
	if (num == 0) {
		*str = '0';
		str++;
		*str = '\0';
		return;
	}
	int temp = num;
	int count = 0;
	while (temp > 0) {
		temp /= 10;
		count++;
	}
	str[count] = '\0';
	temp = num;
	while (count > 0) {
		count--;
		str[count] = (temp % 10) + '0';
		temp /= 10;
	}
}

void printchar(char** out, char c)
{
	**out = c;
	(*out)++;
}

int prints(char** out, const char* string)
{
	int pc = 0;
	while (*string != '\0') {
		printchar(out, *string);
		string++;
		pc++;
	}
	return pc;
}

int printi(char** out, int i)
{
	char buf[20];
	itoa(i, buf);
	return prints(out, buf);
}

static int print(char** out, const char* format, va_list args)
{
	int pc = 0;
	while (*format != '\0') {
		if (*format != '%') {
			printchar(out, *format);
			pc++;
			format++;
		} else {
			format++;
			if (*format == 's') {
				char* str = va_arg(args, char*);
				pc += prints(out, str);
				format++;
			} else if (*format == 'd') {
				int num = va_arg(args, int);
				pc += printi(out, num);
				format++;
			} else if (*format == 'c') {
				int ch = va_arg(args, int);
				printchar(out, (char)ch);
				pc++;
				format++;
			} else if (*format == 'u') {
				unsigned int num = va_arg(args, unsigned int);
				char buf[20];
				uitoa(num, buf);
				pc += prints(out, buf);
				format++;
			} else {
				printchar(out, '%');
				pc++;
				printchar(out, *format);
				pc++;
				format++;
			}
		}
	}
	return pc;
}

int agic_sprintf(char* buffer, const char* format, ...) {
	va_list args;
	va_start(args, format);
	int pc = print(&buffer, format, args);
	*buffer = '\0';
	va_end(args);
	return pc;
}