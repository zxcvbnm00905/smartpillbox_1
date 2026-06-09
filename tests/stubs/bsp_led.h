#ifndef BSP_LED_H
#define BSP_LED_H

void Fake_LedOn(void);
void Fake_LedOff(void);

#define LED_ON()  Fake_LedOn()
#define LED_OFF() Fake_LedOff()

#endif
