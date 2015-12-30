/*
 * usartROSS.h
 *
 * Created: 12/30/2015 1:47:28 PM
 *  Author: Nick McComb [nickmccomb.net]
 */ 


#ifndef USARTROSS_H_
#define USARTROSS_H_

#include "projectHeader.h"

void configureUSART();
void SendStringPC(char *stufftosend);
void SendNumPC(uint16_t numToSend);

#endif /* USARTROSS_H_ */