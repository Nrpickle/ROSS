//
//  RBRLoggerCommands.h
//  Wifi Logger 2 Tools
//
//  Created by Jean-Michel Leconte on 2014-06-27.
//  Copyright (c) 2014 RBR Ltd. All rights reserved.
//

#ifndef _RBRLOGGERCOMMANDS_H_
#define _RBRLOGGERCOMMANDS_H_


typedef enum {
    RBRLOGGER_MEMORY_FORMAT_UNKNOWN,
    RBRLOGGER_MEMORY_FORMAT_EASYPARSE,
    RBRLOGGER_MEMORY_FORMAT_RAWBINARY
    
}eRBRLoggerMemoryFormat;

typedef enum
{
    RBRLOGGER_RESULT_SUCCESS,
    RBRLOGGER_RESULT_FAIL_SYSTEMERROR,
    RBRLOGGER_RESULT_FAIL_TIMEOUT,
    RBRLOGGER_RESULT_FAIL_UNKNOWN
    
} eRBRLoggerErrorCode;

// default error messages

#define RBRLOGGER_DEFAULTERRORMESSAGE_SUCCESS "No error."
#define RBRLOGGER_DEFAULTERRORMESSAGE_FAIL_SYSTEMERROR "System error."
#define RBRLOGGER_DEFAULTERRORMESSAGE_TIMEOUT "Instrument did not reply in time."
#define RBRLOGGER_DEFAULTERRORMESSAGE_UNKNOWN "Unknown error."

// defaults for logger
#define RBRLOGGER_DEFAULT_HOSTNAME "1.2.3.4"
#define RBRLOGGER_DEFAULT_PORT      2000


eRBRLoggerErrorCode rbrLoggerGetMemoryInformation(int sockfd,int timeout, int dataset, int* currentsize);
eRBRLoggerErrorCode rbrLoggerGetMemoryFormat(int sockfd,int timeout, eRBRLoggerMemoryFormat* memformat);
eRBRLoggerErrorCode rbrLoggerDownloadDataFileChunk(int sockfd,int timeout,int dataset, int offset, int sizeToDownload, char* pbBuffer, int* downloadedSize);
eRBRLoggerErrorCode rbrLoggerEnsureWokeUp(int sockfd, int timeout);
eRBRLoggerErrorCode openConnection(int* sock, const char* hostname, uint16_t port);

#endif // _RBRLOGGERCOMMANDS_H_
