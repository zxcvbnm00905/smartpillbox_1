#ifndef BSP_BEEP_H
#define BSP_BEEP_H

void Fake_BeepOn(void);
void Fake_BeepOff(void);

#define BEEP_ON()  Fake_BeepOn()
#define BEEP_OFF() Fake_BeepOff()

#endif
