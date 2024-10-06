#pragma once

#ifndef DEBUG

#define LogCreate
#define Log(message)
#define Logf(format, ...)
#define Error(message)
#define Errorf(format, ...)
#define ErrorExit(message)
#define ErrorExitf(format, ...)
#define Panic(message)
#define Panicf(format, ...)
#define Assert(v)
#define AssertEqual(a, b)
#define AssertNotNull(v)

#else

#include <stdio.h>

#define LogCreate                    \
    {                                \
        FILE *f = fopen("log", "w"); \
        fclose(f);                   \
    }

#define __log_fmt(name) "%s:%d [" name "] "
#define __file_line __FILE__, __LINE__

#define __logf(name, format, ...)                                              \
    {                                                                          \
        FILE *f = fopen("log", "a");                                           \
        if (f != NULL)                                                         \
        {                                                                      \
            fprintf(f, __log_fmt(name) format "\n", __file_line, __VA_ARGS__); \
            fclose(f);                                                         \
        }                                                                      \
    }

#define __log(name, message)                                       \
    {                                                              \
        FILE *f = fopen("log", "a");                               \
        if (f != NULL)                                             \
        {                                                          \
            fprintf(f, __log_fmt(name) message "\n", __file_line); \
            fclose(f);                                             \
        }                                                          \
    }

#define Log(message) __log("LOG", message)
#define Logf(format, ...) __logf("LOG", format, __VA_ARGS__)

#define Error(message) __log("ERROR", message)
#define Errorf(format, ...) __logf("ERROR", format, __VA_ARGS__)

#define Panic(message)                    \
    {                                     \
        UiTextbox(__FILE__ ": " message); \
        __log("PANIC", message);          \
        exit(1);                          \
    }

#define Panicf(format, ...)                   \
    {                                         \
        __logf("PANIC", format, __VA_ARGS__); \
        exit(1);                              \
    }

#define ErrorExit(message)              \
    {                                   \
        printf("ERROR: %s\n", message); \
        Panic(message);                 \
    }

#define ErrorExitf(format, ...)                     \
    {                                               \
        printf("ERROR: " format "\n", __VA_ARGS__); \
        Panicf(format, __VA_ARGS__);                \
    }

#define AssertNotNull(ptr) \
    if ((ptr) == NULL)     \
        Panic("Pointer assertion failed");

#define Assert(v) \
    if (!(v))     \
        Panic("Assertion failed");

#define AssertEqual(a, b) \
    if ((a) != (b))       \
        Panic("Assert equal failed");

#endif