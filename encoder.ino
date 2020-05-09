//Normal encoder state
char prev_enc;
char enc_count;

char EncState (void)
{
  return (digitalRead(ENC_A)?1:0 + digitalRead(ENC_B)?2:0);
}

/*
 * SmittyHalibut's encoder handling, using interrupts. Should be quicker, smoother handling.
 * The Interrupt Service Routine for Pin Change Interrupts on A0-A5.
 */
ISR (PCINT0_vect)
{
  char cur_enc = EncState();

  if ((prev_enc == 0 && cur_enc == 2) ||
      (prev_enc == 2 && cur_enc == 3) ||
      (prev_enc == 3 && cur_enc == 1) ||
      (prev_enc == 1 && cur_enc == 0)) {
    enc_count -= 1;
  } else if ((prev_enc == 0 && cur_enc == 1) ||
      (prev_enc == 1 && cur_enc == 3) ||
      (prev_enc == 3 && cur_enc == 2) ||
      (prev_enc == 2 && cur_enc == 0)) {
    enc_count += 1;
  }
  prev_enc = cur_enc; // Record state for next pulse interpretation
}

/*
 * Setup the encoder interrupts and global variables.
 */
void PciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

void InitEncoder(void) {
  enc_count = 0;
  prev_enc = EncState();

  // Setup Pin Change Interrupts for the encoder inputs
  PciSetup(ENC_A);
  PciSetup(ENC_B);
}

int EncRead() {
  int ret = enc_count;
  enc_count = 0;
  return ret;
}
