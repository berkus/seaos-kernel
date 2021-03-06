#ifndef VSPRINTF_H
#define VSPRINTF_H

#include <vargs.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
void kprintf(const char *fmt, ...);
void printk(int l, const char *fmt, ...);

#endif

