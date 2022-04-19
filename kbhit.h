#ifndef _KBHIT_H
#define _KBHIT_H

#ifdef __cplusplus
extern "C" {
#endif

void init_keyboard(void);
void close_keyboard(void);
int kbhit(void);
int readch(void);


#ifdef __cplusplus
}
#endif

#endif