//
//  RBRLoggerEasyParseEvents.h
//  Wifi Logger 2 Tools
//
//  Created by Jean-Michel Leconte on 2014-06-27.
//  Copyright (c) 2014 RBR Ltd. All rights reserved.
//

#ifndef _RBRLOGGEREASYPARSEEVENTS_H_
#define _RBRLOGGEREASYPARSEEVENTS_H_

#define RBRLOGGER_EASYPARSEEVENT_FOOTPRINT 16

typedef enum
{
    RBRLOGGER_EASYPARSEEVENT_UNKNOWN = 0,                    // 00 catch erroneous values.
    RBRLOGGER_EASYPARSEEVENT_TIME,                            // 01 just a time marker.
    RBRLOGGER_EASYPARSEEVENT_STOP,                            // 02 stop command received.
    RBRLOGGER_EASYPARSEEVENT_RUNTIME,                        // 03 runtime error encountered.
    RBRLOGGER_EASYPARSEEVENT_RESTART,                        // 04 CPU reset detected.
    RBRLOGGER_EASYPARSEEVENT_RECOVER_PARAMS,                // 05 One or more parameters recovered.
    RBRLOGGER_EASYPARSEEVENT_RESTARTFAILED_INVALIDRTC,        // 06 Restart failed : RTC content not valid, logger has no idea of the date/time
    RBRLOGGER_EASYPARSEEVENT_RESTARTFAILED_INVALIDSTATUS,    // 07 Restart failed : RTC ok but logger status not valid, can not tell what to do.
    RBRLOGGER_EASYPARSEEVENT_RESTARTFAILED_INVALIDPARAMS,    // 08 Restart failed : RTC and status ok but primary parameters of schedule could not be recovered.
    RBRLOGGER_EASYPARSEEVENT_UNABLETOLOADALARM,            // 09 Unable to load RTC with alarm time for next sample.
    RBRLOGGER_EASYPARSEEVENT_RESTART_INVALIDRTC,            // 10 SVN-1113,JmL LL-650, rtc was wrong but we continue
    RBRLOGGER_EASYPARSEEVENT_RECOVER_PARAMS_INVALIDRTC,    // 11 SVN-1113,JmL LL-650, rtc was wrong but we continue
    RBRLOGGER_EASYPARSEEVENT_FINISHED,                        // 12 end time reached.
    RBRLOGGER_EASYPARSEEVENT_BURST_START,                    // 13 start of burst storage.
    RBRLOGGER_EASYPARSEEVENT_WAVE_START,                    // 14 start of wave storage.
    RBRLOGGER_EASYPARSEEVENT_unused_15,                    // 15 unused
    RBRLOGGER_EASYPARSEEVENT_STREAM_OFF = 16,                // 16 both streams now off.
    RBRLOGGER_EASYPARSEEVENT_STREAM_USB,                    // 17 usb streaming, serial off.
    RBRLOGGER_EASYPARSEEVENT_STREAM_SERIAL,                // 18 usb off, serial streaming.
    RBRLOGGER_EASYPARSEEVENT_STREAM_BOTH,                    // 19 both streams now on.
    RBRLOGGER_EASYPARSEEVENT_THRESH_START,                    // 20 threshold condition satisfied, start sampling.
    RBRLOGGER_EASYPARSEEVENT_THRESH_PAUSE,                    // 21 threshold condition not met, pause sampling.
    RBRLOGGER_EASYPARSEEVENT_POWERSOURCE_SWITCHTOINTERNALBATTERY,    // 22 switch to internal battery
    RBRLOGGER_EASYPARSEEVENT_POWERSOURCE_SWITCHTOEXTERNALBATTERY,    // 23 switch to external battery
    RBRLOGGER_EASYPARSEEVENT_TWIST_START,                    // 24 twist condition satisfied, start sampling.
    RBRLOGGER_EASYPARSEEVENT_TWIST_PAUSE,                    // 25 twist condition not met, pause sampling.
    RBRLOGGER_EASYPARSEEVENT_WIFI_START,                    // 26 Wifi module powered on.
    RBRLOGGER_EASYPARSEEVENT_WIFI_STOP,                    // 27 Wifi module powered off.
    RBRLOGGER_EASYPARSEEVENT_REGIMES_WAITING,                // 28 Regimes, not yet in a regime
    RBRLOGGER_EASYPARSEEVENT_REGIMES_REGIME1,                // 29 Regimes, in regime 1
    RBRLOGGER_EASYPARSEEVENT_REGIMES_REGIME2,                // 30 Regimes, in regime 2
    RBRLOGGER_EASYPARSEEVENT_REGIMES_REGIME3,                // 31 Regimes, in regime 3
    RBRLOGGER_EASYPARSEEVENT_REGIMES_BIN,                    // 32 Regimes, new bin
    RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_UPCAST,        // 33 Profiles, beginning of an up cast
    RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_DOWNCAST,        // 34 Profiles, beginning of a down cast, see specific paragraph
    RBRLOGGER_EASYPARSEEVENT_PROFILES_END_CAST,            // 35 Profiles, end of cast
    RBRLOGGER_EASYPARSEEVENT_UNKNOWNTYPE
} eEasyParseEventsType;


typedef union
{
    unsigned char _unknownbytes[4];
    unsigned long      profileoffset;
    unsigned long      regimenumberofsamplesinbin;
} unEasyParseEventPayload;

typedef struct _tEasyParseEvent
{
    eEasyParseEventsType type;
    unsigned long long timestamp;
    unEasyParseEventPayload payload;
} tEasyParseEvent;


int rbrLoggerParseEasyParseEvent(const unsigned char bytes[RBRLOGGER_EASYPARSEEVENT_FOOTPRINT],tEasyParseEvent* easyparseevent);

#endif
