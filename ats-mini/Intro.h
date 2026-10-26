#ifndef INTRO_H
#define INTRO_H

// Personalized startup for PU2QTF: CQ in CW typed out on an animated screen.
//
// Silent by design: the ESP32 has no audio path to the speaker (all audio comes
// from the SI4732). Coupling an RF carrier from a GPIO into the receiver was
// tried and lands only 1-3 dB above the noise floor, so it is not audible.

#define INTRO_CALLSIGN   "PU2QTF"
#define INTRO_CW_TEXT    "CQ CQ CQ DE PU2QTF K"
#define INTRO_MARQUEE    "Shack de escuta PU2QTF  *  Fernando  *  Sao Bernardo do Campo/SP  *  Grid GG66rh  *  " \
                         "ATS-Mini SI4732 - LW/MW/SW/FM/SSB  *  QRV ouvindo 80m 40m 20m 15m 10m  *  73 & DX!  *  "
#define INTRO_WPM        25

// Play the intro. Returns false if the user skipped it with an encoder click.
bool introPlay();

#endif
