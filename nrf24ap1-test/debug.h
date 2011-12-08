#ifndef DEBUG_H_
#define DEBUG_H_

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) printf("DEBUG %s:%d: " M "\n\r", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif
