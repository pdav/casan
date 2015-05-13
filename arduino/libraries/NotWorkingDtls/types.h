#ifndef __TYPES__
#define __TYPES__

typedef unsigned int u_int32_t;

typedef	unsigned char u_int8_t;
typedef	unsigned short int u_int16_t;
typedef	unsigned int u_int32_t;
typedef	unsigned int time_t;

# if __WORDSIZE == 64
typedef unsigned long int u_int64_t;
# else
typedef unsigned long long int u_int64_t;
# endif

#endif
