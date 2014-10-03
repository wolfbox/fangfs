#pragma once

#include <stdlib.h>
#include <stdio.h>

static inline void __fail(const char* file, const char* func, int line, const char* msg) {
	fprintf(stderr, "Assertion failed at %s:%s:%d: %s\n", file, func, line, msg);
	exit(1);
}

#define verify(cond) ((cond)? (void)0 : __fail(__FILE__, __FUNCTION__, __LINE__, #cond))

#define do_test() (printf("Running %s...\n", __FUNCTION__))
