//
//  RBRLoggerEasyParseEvents.c
//  Wifi Logger 2 Tools
//
//  Created by Jean-Michel Leconte on 2014-06-27.
//  Copyright (c) 2014 RBR Ltd. All rights reserved.
//

#include <stdio.h>

#include "RBRLoggerEasyParseEvents.h"

/**
 *  Parse an array of 16 bytes which correspond to an Easyparse event
 *
 *  @param bytes          array of bytes to parse.
 *  @param easyparseevent structure to fill.
 *
 *  @return 0 if success -1 otherwise
 */

int rbrLoggerParseEasyParseEvent(const unsigned char bytes[16],tEasyParseEvent* easyparseevent)
{
    unsigned long u32tmp;
    unsigned long long u64tmp;
    //@todo check CRC first
    
    if (bytes[3]!=0xF4) return -1;
    
    // handle not yet known events (future firmware)
    if (bytes[2]>(unsigned char)RBRLOGGER_EASYPARSEEVENT_UNKNOWNTYPE)
    {
        easyparseevent->type=RBRLOGGER_EASYPARSEEVENT_UNKNOWNTYPE;
    }
    else
    {
        easyparseevent->type=(eEasyParseEventsType)(bytes[2]);
    }
    
    // timestamp
    u64tmp=bytes[11];u64tmp<<=8;
    u64tmp+=bytes[10];u64tmp<<=8;
    u64tmp+=bytes[9];u64tmp<<=8;
    u64tmp+=bytes[8];u64tmp<<=8;
    u64tmp+=bytes[7];u64tmp<<=8;
    u64tmp+=bytes[6];u64tmp<<=8;
    u64tmp+=bytes[5];u64tmp<<=8;
    u64tmp+=bytes[4];
    
    easyparseevent->timestamp=u64tmp;
    // payload
    u32tmp=bytes[15];u32tmp<<=8;
    u32tmp+=bytes[14];u32tmp<<=8;
    u32tmp+=bytes[13];u32tmp<<=8;
    u32tmp+=bytes[12];
    switch (easyparseevent->type)
    {
            // regimes events
        case RBRLOGGER_EASYPARSEEVENT_REGIMES_BIN:
            easyparseevent->payload.regimenumberofsamplesinbin=u32tmp;
            break;
            
            // profiles events
        case RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_UPCAST:
        case RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_DOWNCAST:
        case RBRLOGGER_EASYPARSEEVENT_PROFILES_END_CAST:
            easyparseevent->payload.profileoffset=u32tmp;
            break;
            
            // other events
        default:
            //  store in the _unknownbytes in this case
            easyparseevent->payload._unknownbytes[0]=bytes[12];
            easyparseevent->payload._unknownbytes[1]=bytes[13];
            easyparseevent->payload._unknownbytes[2]=bytes[14];
            easyparseevent->payload._unknownbytes[3]=bytes[15];
            break;
    }
    
    return 0;
}

