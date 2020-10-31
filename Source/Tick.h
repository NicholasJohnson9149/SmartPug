#ifndef _TICK_H_

#define _TICK_H_

#define TICK    uint64_t

#define MS(a)               (a)
#define SEC(a)              (a*1000)
#define TickGetDiff(a,b)    (a-b)


uint64_t TickGet(void);
void TickInit(void);

#endif