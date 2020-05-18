#ifndef UBITX_SI5351_H_
#define UBITX_SI5351_H_

namespace si5351 {

void SetFreq(unsigned char clknum, unsigned long fout);
void SetCalibration(long cal);
void Init();

}

#endif  // UBITX_SI5351_H_
