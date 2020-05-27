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

extern void (*DoActiveApp)();

void DoTuning();

}  // namespace

#endif  // UBITX_CAT_H_
