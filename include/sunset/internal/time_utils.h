#include <stdint.h>
#include <time.h>

typedef struct timespec Time;

Time get_time(void);

uint64_t get_time_ms(void);

uint64_t get_time_us(void);

uint64_t time_since_ms(Time start);

uint64_t time_since_us(Time start);

float time_since_s(Time start);
