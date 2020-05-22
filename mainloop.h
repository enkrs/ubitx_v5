#ifndef UBITX_MAIN_H_
#define UBITX_MAIN_H_

namespace mainloop {

const int debounce_count = 500; // TODO reduce, research

struct Buttons {
  bool f_down = false;
  bool ptt_down = false;
  bool f_clicked = false;
  bool f_hold = false;
  bool ptt_engaged = false;
  bool ptt_released = false;
};

extern Buttons buttons;

bool FButtonClicked();
bool FBtnDown();
void Run();

}  // namespace

#endif  // UBITX_CAT_H_
