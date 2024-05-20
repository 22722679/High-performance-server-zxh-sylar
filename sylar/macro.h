#ifndef __SYLAR_MACRO_H__       //防止头文件被引用
#define __SYLAR_MACRO_H__

#include <string.h>
#define NDEBUG
#include <assert.h>
#include "util.h"
#include "sylar.h"

//调用当前栈信息
#define SYLAR_ASSERT(x) \
    if(!(x)) {  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x   \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    ");    \
        assert(x);  \
    }

#define SYLAR_ASSERT2(x, w) \
    if(!(x)) {  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x   \
            << "\n" << w    \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    ");    \
        assert(x);  \
    }

#endif