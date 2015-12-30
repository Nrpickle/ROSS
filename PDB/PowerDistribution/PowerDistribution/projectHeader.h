/*
 * projectHeader.h
 *
 * Created: 12/30/2015 1:59:06 PM
 *  Author: Nick McComb
 */ 


#ifndef PROJECTHEADER_H_
#define PROJECTHEADER_H_

#ifndef F_CPU
#define F_CPU 32000000  //Define clock speed for _delay
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "Macros.h"
#include "usart_driver.h"
#include "avr_compiler.h"


#define COMP_USART USARTC0

//Global Data



#endif /* PROJECTHEADER_H_ */