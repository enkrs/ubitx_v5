#ifndef UBITX_UI_H_
#define UBITX_UI_H_

char BtnDown();
void BtnWaitUp();
void PrintLine(char linenmbr, const char *c);
void PrintStatus(const char *c);
void PrintStatusValue(const char *c, const char *v);
void UpdateDisplay();
void UpdateVoltage();

#endif
