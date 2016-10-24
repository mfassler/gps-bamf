#ifndef __PRESSTEMP_H
#define __PRESSTEMP_H

extern int presstemp_init(void);
extern int presstemp_get_UT(void);
extern int presstemp_get_UP(void);
extern void presstemp_calcPressureAndTemp(int32_t*, int32_t*);

#endif // __PRESSTEMP_H
