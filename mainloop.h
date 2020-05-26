#ifndef UBITX_MAIN_H_
#define UBITX_MAIN_H_

namespace mainloop {

const int debounce_count = 500; // TODO reduce, research

extern struct Buttons {
  bool f_down = false; // 
  bool ptt_down = false; //
  bool f_clicked = false; //
  bool f_held = false;
} buttons;

extern unsigned char active_screen;
const unsigned char SCREEN_TUNING = 0;
const unsigned char SCREEN_MENU = 1;
const unsigned char SCREEN_TX = 2;

void CheckButtons();
bool FButtonClicked();
bool FBtnDown();

unsigned char EnterTuning();
unsigned char DoTuning();

void Run();

}  // namespace

#endif  // UBITX_CAT_H_
