#include "string.h"
#include "assert.h"
#include "common.h"

/*
 * use putchar_func to print a string
 *   @putchar_func is a function pointer to print a character
 *   @format is the control format string (e.g. "%d + %d = %d")
 *   @data is the address of the first variable argument
 * please implement it.
 */

void printd(void (*putchar_func)(char), int data);
// void printc(char ch);
void prints(void (*putchar_func)(char), char *str);
void printx(void (*putchar_func)(char), unsigned int data);
int

vfprintf(void (*putchar_func)(char), const char *format, void **data)
{

	int ccount = 0; // 统计字符个数
	// int arg_addr=(int)&format+2;  //获得参数地址
	int arg_addr = (int)data;
	while (*format)
	{
		if ((*format) == '%')
		{
			format++;
			if ((*format) == 'd')
			{
				format++;
				printd(putchar_func, *(int *)(arg_addr));

				arg_addr += 4;
			}
			else if ((*format) == 's')
			{
				format++;
				prints(putchar_func, *(char **)arg_addr);
				// arg_addr+=2;
				arg_addr += 4; // 32 位指针是 4 字节
			}
			else if ((*format) == 'x')
			{
				format++;
				putchar_func(48);
				putchar_func('x');

				printx(putchar_func, *(unsigned int *)(arg_addr));
				arg_addr += 4; // 32 位指针是 4 字节
			}
			else
			{
				if (*format == '\n')
					putchar_func('\r');
				putchar_func(*format++);
			}
		}
		else
			putchar_func(*format++);
		ccount++;
	}

	return ccount;
	// 返回打印的字节数
}

/*void printc(char ch)
{

	serial_printc(ch);
*/
void prints(void (*putchar_func)(char), char *str)
{
	for (; *str; str++)

		putchar_func(*str);
}
void printd(void (*putchar_func)(char), int data)
{
	int mod;
	// 负数处理
	if (data < 0)
	{
		putchar_func('-');
		data = -1 * data;
	}
	// 逐位打印，递归调用
	mod = data % 10;
	data = data / 10;
	if (data == 0)
	{
		putchar_func(48 + mod);
	}
	else
	{
		printd(putchar_func, data);
		putchar_func(48 + mod);
	}
}
void printx(void (*putchar_func)(char), unsigned int data)
{
	// 逐位处理，递归调用
	unsigned int mod;
	mod = data % 16;
	data = data / 16;
	if (data == 0)
	{
		if (mod == 10)
			putchar_func('A');
		else if (mod == 11)
			putchar_func('B');
		else if (mod == 12)
			putchar_func('C');
		else if (mod == 13)
			putchar_func('D');
		else if (mod == 14)
			putchar_func('E');
		else if (mod == 15)
			putchar_func('F');
		else
			putchar_func(48 + mod);
	}
	else
	{
		printx(putchar_func, data);
		if (mod == 10)
			putchar_func('A');
		else if (mod == 11)
			putchar_func('B');
		else if (mod == 12)
			putchar_func('C');
		else if (mod == 13)
			putchar_func('D');
		else if (mod == 14)
			putchar_func('E');
		else if (mod == 15)
			putchar_func('F');
		else
			putchar_func(48 + mod);
	}
}
