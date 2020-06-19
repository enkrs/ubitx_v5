#ifndef UBITX_KEYER_H_
#define UBITX_KEYER_H_

namespace keyer {

extern unsigned long cw_timeout;
extern char keyer_control;

void CwKeydown();
void CwKeyUp();
void Run();

}

#endif  // UBITX_KEYER_H_
