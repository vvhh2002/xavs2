/*
 * common.c
 *
 * Description of this file:
 *    misc common functions definition of the xavs2 library
 *
 * --------------------------------------------------------------------------
 *
 *    xavs2 - video encoder of AVS2/IEEE1857.4 video coding standard
 *    Copyright (C) 2018~ VCL, NELVT, Peking University
 *
 *    Authors: Falei LUO <falei.luo@gmail.com>
 *             etc.
 *
 *    Homepage1: http://vcl.idm.pku.edu.cn/xavs2
 *    Homepage2: https://github.com/pkuvcl/xavs2
 *    Homepage3: https://gitee.com/pkuvcl/xavs2
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 *    This program is also available under a commercial proprietary license.
 *    For more information, contact us at sswang @ pku.edu.cn.
 */


#include "common.h"

#include <stdarg.h>
#include <ctype.h>
#if SYS_WINDOWS
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <time.h>

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

/**
 * ===========================================================================
 * global variables
 * ===========================================================================
 */

const float FRAME_RATE[8] = {
    24000.0f / 1001.0f, 24.0f, 25.0f, 30000.0f / 1001.0f, 30.0f, 50.0f, 60000.0f / 1001.0f, 60.0f
};

const char *xavs2_preset_names[] = {
    "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", NULL
};

#if XAVS2_TRACE
FILE *h_trace = NULL;           /* global file handle for trace file */
int g_sym_count = 0;            /* global symbol count for trace */
int g_bit_count = 0;            /* global bit    count for trace */
#endif

#if PTW32_STATIC_LIB
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* this is a global in pthread-win32 to indicate if it has been
 * initialized or not */
extern int ptw32_processInitialized;

#endif

#if XAVS2_TRACE
/**
 * ===========================================================================
 * trace file
 * ===========================================================================
 */

/* ---------------------------------------------------------------------------
 */
int xavs2_trace_init(xavs2_param_t *param)
{
    if (strlen(param->psz_trace_file) > 0) {
        /* create or truncate the trace file */
        h_trace = fopen(param->psz_trace_file, "wt");
        if (h_trace == NULL) {
            xavs2_log(NULL, XAVS2_LOG_ERROR, "trace: can't write to %s\n", param->psz_trace_file);
            return -1;
        }
    }

    return 0;
}

/* ---------------------------------------------------------------------------
 */
void xavs2_trace_destroy(void)
{
    if (h_trace) {
        fclose(h_trace);
    }
}

/* ---------------------------------------------------------------------------
 */
int xavs2_trace(const char *psz_fmt, ...)
{
    int len = 0;

    /* append to the trace file */
    if (h_trace) {
        va_list arg;
        va_start(arg, psz_fmt);

        len = vfprintf(h_trace, psz_fmt, arg);
        fflush(h_trace);
        va_end(arg);
    }

    return len;
}

#endif


/**
 * ===========================================================================
 * xavs2_log
 * ===========================================================================
 */

#ifdef _MSC_VER
/* ---------------------------------------------------------------------------
 */
static ALWAYS_INLINE
void xavs2_set_font_color(int color)
{
    static const WORD colors[] = {
        FOREGROUND_INTENSITY | FOREGROUND_RED,                     // 红色
        FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,  // 黄色
        FOREGROUND_INTENSITY | FOREGROUND_GREEN,                   // 绿色
        FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE, // cyan
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,       // 白色
    };
    color = XAVS2_MIN(4, color);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colors[color]);
}
#endif

/* ---------------------------------------------------------------------------
 */
static void
xavs2_log_default(int i_log_level, const char *psz_fmt)
{
#if !defined(_MSC_VER)
    static const char str_color_clear[] = "\033[0m";  // "\033[0m"
    static const char str_color[][16] = {
    /*        red        yellow        green          cyan        (default)  */
        "\033[1;31m", "\033[1;33m", "\033[1;32m", "\033[1;36m", "\033[0m"
    };
    const char *cur_color = str_color[i_log_level];
#endif
    static const char *null_prefix = "";
    const char *psz_prefix = null_prefix;

    switch (i_log_level) {
    case XAVS2_LOG_ERROR:
        psz_prefix = "[error]: ";
        break;
    case XAVS2_LOG_WARNING:
        psz_prefix = "[warning]: ";
        break;
    case XAVS2_LOG_INFO:
        psz_prefix = "[info]: ";
        break;
    case XAVS2_LOG_DEBUG:
        psz_prefix = "[debug]: ";
        break;
    case XAVS2_LOG_NOPREFIX:
        psz_prefix = null_prefix;
#if !defined(_MSC_VER)
        cur_color = str_color[3];
#endif
        break;
    default:
        psz_prefix = "[unknown]: ";
#if !defined(_MSC_VER)
        cur_color  = str_color[0];
#endif
        break;
    }
#if defined(_MSC_VER)
    xavs2_set_font_color(i_log_level); /* set color */
    fprintf(stdout, "%s%s", psz_prefix, psz_fmt);
    xavs2_set_font_color(4);     /* restore to white color */
#else
    if (i_log_level != XAVS2_LOG_INFO && i_log_level != XAVS2_LOG_NOPREFIX) {
        fprintf(stdout, "%s%s%s%s", cur_color, psz_prefix, psz_fmt, str_color_clear);
    } else {
        fprintf(stdout, "%s%s", psz_prefix, psz_fmt);
    }
#endif
}

/* ---------------------------------------------------------------------------
 */
void xavs2_log(void *p, int i_log_level, const char *psz_fmt, ...)
{
    xavs2_t *h = (xavs2_t *)p;
    if (h == NULL || (i_log_level & 0x0F) <= h->param->i_log_level) {
        va_list arg;
        char str_in[2048];
        va_start(arg, psz_fmt);
        vsprintf(str_in, psz_fmt, arg);
        xavs2_log_default(i_log_level, str_in);
        va_end(arg);
    }
}


/**
 * ===========================================================================
 * utilities
 * ===========================================================================
 */

/* ---------------------------------------------------------------------------
 * get time
 */
int64_t xavs2_mdate(void)
{
#if SYS_WINDOWS
    LARGE_INTEGER nFreq;
    if (QueryPerformanceFrequency(&nFreq)) { // 返回非零表示硬件支持高精度计数器
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        return (int64_t)(1000000 * t1.QuadPart / (double)nFreq.QuadPart);
    } else {  // 硬件不支持情况下，使用毫秒级系统时间
        struct timeb tb;
        ftime(&tb);
        return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
    }
#else
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}


/**
 * ===========================================================================
 * thread
 * ===========================================================================
 */

/* ---------------------------------------------------------------------------
 */
int xavs2_create_thread(xavs2_pthread_t *tid, xavs2_tfunc_t tfunc, void *targ)
{
    return xavs2_pthread_create(tid, NULL, tfunc, targ);
}
