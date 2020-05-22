#ifndef UBITX_MAIN_H_
#define UBITX_MAIN_H_

namespace mainloop {

const int debounce_count = 500;

struct Buttons {
  unsigned char f_down;
  unsigned char ptt_down;
  unsigned char f_clicked;
  unsigned char f_hold;
  unsigned char ptt_engaged;
  unsigned char ptt_released;
};

extern Buttons buttons;

char FButtonClicked();
char FBtnDown();
void Run();

}  // namespace

#endif  // UBITX_CAT_H_
