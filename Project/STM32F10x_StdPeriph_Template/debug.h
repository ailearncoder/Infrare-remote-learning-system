#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "config.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_DEFAULT
#endif

#ifndef DEBUG_NAME
#define DEBUG_NAME DEBUG_NAME_DEFAULT
#endif

#if DEBUG_LEVEL&&DEBUG_LEVEL_DEFAULT

#if DEBUG_USE_RTT
    #include "SEGGER_RTT.h"
    #define log_init() SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL)
#else
    #include "stdio.h"
    #define RTT_CTRL_TEXT_BRIGHT_RED ""
    #define RTT_CTRL_TEXT_BRIGHT_YELLOW ""
    #define RTT_CTRL_TEXT_BRIGHT_GREEN ""
    #define SEGGER_RTT_printf(NUM,...) printf(__VA_ARGS__)
    #define log_init() 
#endif

    #if DEBUG_LEVEL >= 4&&DEBUG_LEVEL_DEFAULT>=4
        #define log_error(...)                                                     \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_RED"%s <error> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_warning(...)                                                        \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_YELLOW"%s <warning> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_info(...)                                                           \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_GREEN"%s <info> ",DEBUG_NAME);  \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_debug(...)                                                        \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_GREEN"%s <debug> ",DEBUG_NAME);  \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_info_raw(...) SEGGER_RTT_printf(0,__VA_ARGS__)

    #elif DEBUG_LEVEL >= 3&&DEBUG_LEVEL_DEFAULT>=3
        #define log_error(...)                                                     \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_RED"%s <error> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_warning(...)                                                        \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_YELLOW"%s <warning> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_info(...)                                                           \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_GREEN"%s <info> ",DEBUG_NAME);  \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_debug(...) 
        #define log_info_raw(...) SEGGER_RTT_printf(0,__VA_ARGS__)

    #elif DEBUG_LEVEL >= 2&&DEBUG_LEVEL_DEFAULT>=2
        #define log_error(...)                                                     \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_RED"%s <error> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_warning(...)                                                        \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_YELLOW"%s <warning> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_info(...) 
        #define log_debug(...)  
        #define log_info_raw(...) 

    #elif DEBUG_LEVEL >= 1&&DEBUG_LEVEL_DEFAULT>=1
        #define log_error(...)                                                     \
            SEGGER_RTT_printf(0,RTT_CTRL_TEXT_BRIGHT_RED"%s <error> ",DEBUG_NAME); \
            SEGGER_RTT_printf(0,__VA_ARGS__)
        #define log_warning(...) 
        #define log_info(...) 
        #define log_debug(...) 
        #define log_info_raw(...) 
    #endif
    
#else

    #define log_init() 
    #define log_error(...) 
    #define log_warning(...) 
    #define log_info(...) 
    #define log_debug(...) 
    #define log_info_raw(...) 

#endif

#endif

