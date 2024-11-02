
#include "debug_trace.h"


unsigned int debug_mask = DBG_ALL;

void DBGTRACE(int mask,const char  *fmt, ...)
{
    if( mask & debug_mask ) {
        va_list parms;
        char  buf[256];

        va_start(parms, fmt);
        vsprintf(buf, fmt, parms);
        va_end(parms);

#ifdef _LOG_ANDROID_
	LOGD("%s", buf);
#else
        printf("%s", buf);
#endif
    }
}

void MSG(const char  *fmt, ...)
{
    va_list parms;
    char  buf[256];

    // Try to print in the allocated space.
    va_start(parms, fmt);
    vsprintf(buf, fmt, parms);
    va_end(parms);

#ifdef _LOG_ANDROID_
	LOGD("%s", buf);
#else
        printf("%s", buf);
#endif
}
