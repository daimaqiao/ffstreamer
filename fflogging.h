#ifndef __FFLOGGING_H__
#define __FFLOGGING_H__

#include <stdio.h>

#define __FFLOG(type, format, ...) \
    {printf(" [%s] %s:%d - ", type, __FILE__, __LINE__);printf(format, ##__VA_ARGS__);puts("");}(void*)0

#define _FFDEBUG(format, ...) \
    __FFLOG("DEBUG", format, ##__VA_ARGS__)

#define _FFINFO(format, ...) \
    __FFLOG("INFO", format, ##__VA_ARGS__)

#define _FFWARN(format, ...) \
    __FFLOG("WARN", format, ##__VA_ARGS__)

#define _FFERROR(format, ...) \
    __FFLOG("ERROR", format, ##__VA_ARGS__)


#define LOGD _FFDEBUG
#define LOGI _FFINFO
#define LOGW _FFWARN
#define LOGE _FFERROR


#endif