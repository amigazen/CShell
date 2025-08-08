/*
 * snprintf.c - Wrapper for Amiga utility.library VSNPrintf
 * 
 * Uses the Amiga's built-in VSNPrintf() function from utility.library
 * which is more efficient and native to the platform.
 */

#include "shell.h"
#include <stdarg.h>

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    int result;
    
    D(fprintf(stderr, "DEBUG: snprintf called with str=%p, size=%lu, format='%s'\n", str, (unsigned long)size, format ? format : "NULL"));
    
    if (size == 0) {
        D(fprintf(stderr, "DEBUG: snprintf - size is 0, returning 0\n"));
        return 0;
    }
    
    va_start(args, format);
    D(fprintf(stderr, "DEBUG: snprintf - va_start completed, calling vsnprintf\n"));
    result = vsnprintf(str, size, format, args);
    va_end(args);
    
    D(fprintf(stderr, "DEBUG: snprintf - vsnprintf returned %d\n", result));
    return result;
}

int vsnprintf(char *str, size_t size, const char *format, va_list args)
{
    LONG result;
    
    D(fprintf(stderr, "DEBUG: vsnprintf called with str=%p, size=%lu, format='%s', args=%p\n", str, (unsigned long)size, format ? format : "NULL", args));
    
    if (!str || size == 0) {
        D(fprintf(stderr, "DEBUG: vsnprintf - invalid parameters, returning -1\n"));
        return -1;
    }
    
    /* Use Amiga's VSNPrintf with proper va_list handling */
    D(fprintf(stderr, "DEBUG: vsnprintf - about to call VSNPrintf\n"));
    result = VSNPrintf((UBYTE *)str, (ULONG)size, (const UBYTE *)format, (APTR)args);
    D(fprintf(stderr, "DEBUG: vsnprintf - VSNPrintf returned %ld\n", result));
    
    /* VSNPrintf returns the required buffer size including NUL,
       but we need to return the number of characters written (excluding NUL) */
    if (result > 0) {
        /* If the result is larger than our buffer, truncation occurred */
        if (result > (LONG)size) {
            /* Return the number of characters that would fit (excluding NUL) */
            return (int)(size - 1);
        } else {
            /* Return the number of characters written (excluding NUL) */
            return (int)(result - 1);
        }
    }
    
    return (int)result;
}
