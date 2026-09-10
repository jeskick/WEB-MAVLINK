#ifndef UART_LINK_H
#define UART_LINK_H

#include "config.h"

typedef void (*UartScanUiFn)(long baud, int rxPin, int txPin);

void uartLinkSetScanUi(UartScanUiFn fn);
bool uartLinkStart();
void uartLinkTick();
void uartLinkApply(int rxPin, int txPin, long baud);
void uartLinkSave();

#endif
