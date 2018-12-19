/*
Func exported from kernel
*/
#ifndef CWBHOOK_H
#define CWBHOOK_H

#include <pspctrl.h>

int cwbCtrlPeekBufferPositive(SceCtrlData *pad_data, int count);
extern int ctrl_kernel;

#endif //CWBHOOK_H

