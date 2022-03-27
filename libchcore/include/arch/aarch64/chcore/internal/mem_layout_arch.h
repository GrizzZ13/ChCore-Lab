#pragma once

/* Base address to place libc.so when launching dynamicaly linked apps. */
#define LIBC_SO_LOAD_BASE 0x400000000000UL

#define MAIN_THREAD_STACK_BASE 0x500000000000UL
#define MAIN_THREAD_STACK_SIZE 0x800000UL

#define CHILD_THREAD_STACK_BASE \
        (MAIN_THREAD_STACK_BASE + MAIN_THREAD_STACK_SIZE)
#define CHILD_THREAD_STACK_SIZE 0x800000UL

#define MEM_AUTO_ALLOC_REGION      0x300000000000UL
#define MEM_AUTO_ALLOC_REGION_SIZE 0x100000000000UL