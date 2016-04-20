//
//  RBRLoggerCommands.c
//  Wifi Logger 2 Tools
//
//  Created by Jean-Michel Leconte on 2014-06-27.
//  Copyright (c) 2014 RBR Ltd. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#include <stdbool.h>
#include "RBRLoggerCommands.h"



#define LOGGER2_PROMPT "Ready:"
#define MAX_NUM_TOKENS 32
#define LOGGER2_MAXANSWER_SIZE 1024
#define LOGGER2_MAXCOMMAND_SIZE 256
char pbRepliesBuffer[LOGGER2_MAXANSWER_SIZE];


ssize_t discardBytes(int sockfd);

ssize_t sendLoggerCommandLine(int sockfd,int timeout,const char* bCommandToSend,char** pcaTokens/*,int* nbTokens*/,char* pbReplyBuffer,size_t stReplyBufferMaxSize);
ssize_t readBytes(int sockfd,char *  bBytesToRead, size_t stNbBytesToRead);
ssize_t writeBytes(int sockfd,const char *  bBytesToSend, size_t stNbBytesToWrite);
ssize_t readLineFromLogger(int sockfd, char* bBytesLine,size_t stNbBytesMax);

int createSocket(unsigned short*port, int type);

/**
 *  Sends characters wake up sequence to the datalogger
 *
 *  @param sockfd  socket descriptor
 *  @param timeout timeout in ms (@todo implementation)
 *
 *  @return eRBRLoggerErrorCode error code
 */
eRBRLoggerErrorCode rbrLoggerEnsureWokeUp(int sockfd, int timeout)
{
    // just send 3 times '\r\n'
    for (int k=0;k<3;k++)
    {
        if (writeBytes(sockfd,"\r\n",sizeof("\r\n")-1)==-1)
        {
            return RBRLOGGER_RESULT_FAIL_SYSTEMERROR;
        }
    }
    
    
    // wait a bit
    sleep(1);
    
    // discard everything
    discardBytes(sockfd); //@todo check error code here
    
    return RBRLOGGER_RESULT_SUCCESS;
    
}

/**
 *  Retrieve the current file format in the logger's memory
 *
 *  @param sockfd    socket descriptor
 *  @param timeout   timeout in ms (@todo implementation)
 *  @param memformat retrieved current file format
 *
 *  @return eRBRLoggerErrorCode error code
 */
eRBRLoggerErrorCode rbrLoggerGetMemoryFormat(int sockfd,int timeout, eRBRLoggerMemoryFormat* memformat)
{
    
    
    char* answertokens[MAX_NUM_TOKENS];
    int k;
    
    sendLoggerCommandLine(sockfd,timeout,"memformat\r\n",answertokens,pbRepliesBuffer,LOGGER2_MAXANSWER_SIZE);
    
    if (answertokens[0]==NULL) return RBRLOGGER_RESULT_FAIL_UNKNOWN;
    // inspect parameters
    for (k=1;answertokens[k]!=NULL;k++)
    {
        if (strcmp("type",answertokens[k])==0)
        {
            k++;
            if (answertokens[k]==NULL) break;
            if (strcmp("rawbin00",answertokens[k])==0)
            {
                *memformat= RBRLOGGER_MEMORY_FORMAT_RAWBINARY;
                return RBRLOGGER_RESULT_SUCCESS;
            }
            if (strcmp("calbin00",answertokens[k])==0)
            {
                *memformat= RBRLOGGER_MEMORY_FORMAT_EASYPARSE;
                return RBRLOGGER_RESULT_SUCCESS;
            }
        }
    }
    return RBRLOGGER_RESULT_FAIL_UNKNOWN;
}


/**
 *  Get the current size of one datalogger's file
 *
 *  @param sockfd    socket descriptor
 *  @param timeout   timeout in ms (@todo implementation)
 *  @param dataset     file descriptor in the logger (depends on the file format)
 *  @param currentsize size of the file
 *
 *  @return eRBRLoggerErrorCode error code
 */
eRBRLoggerErrorCode rbrLoggerGetMemoryInformation(int sockfd,int timeout, int dataset, int* currentsize)
{
    char* answertokens[MAX_NUM_TOKENS];
    char command[LOGGER2_MAXCOMMAND_SIZE];
    int k;
    *currentsize=-1;
    sprintf(command,"meminfo dataset=%d\r\n",dataset);
    sendLoggerCommandLine(sockfd,timeout,command,answertokens,pbRepliesBuffer,LOGGER2_MAXANSWER_SIZE);
    if (answertokens[0]==NULL) return RBRLOGGER_RESULT_FAIL_UNKNOWN;
    // inspect parameters
    for (k=1;answertokens[k]!=NULL;k++)
    {
        if (strcmp("used",answertokens[k])==0)
        {
            k++;
            if (answertokens[k]==NULL) break;
            *currentsize=(int)atol(answertokens[k]);
            return RBRLOGGER_RESULT_SUCCESS;
        }
    }
    return RBRLOGGER_RESULT_FAIL_UNKNOWN;
}

/**
 *  Download one chunk of data from a logger file
 *
 *  @param sockfd    socket descriptor
 *  @param timeout   timeout in ms (@todo implementation)
 *  @param dataset        file descriptor in the logger
 *  @param offset         offset in the file
 *  @param sizeToDownload requested number of bytes
 *  @param pbBuffer       destination buffer
 *  @param downloadedSize number of bytes read
 *
 *  @return eRBRLoggerErrorCode error code
 */
eRBRLoggerErrorCode rbrLoggerDownloadDataFileChunk(int sockfd, int timeout, int dataset, int offset, int sizeToDownload, char* pbBuffer, int* downloadedSize)
{
    static char pbDownloadCommandAnswerBuffer[LOGGER2_MAXANSWER_SIZE];
    static char pbDownloadCommandToSend[LOGGER2_MAXCOMMAND_SIZE];
    
    char* pbAnswer;
    
    int adataset,aoffset,asize;
    bool bHasBeenRetried=false;
    
    
    // create command
    sprintf(pbDownloadCommandToSend,"read d %d %d %d\r\n",dataset,sizeToDownload,offset);
    //fprintf(stderr,"%s",pbDownloadCommandToSend);
    
    // send command
retrysenddownloadcommand:
    if (writeBytes(sockfd,pbDownloadCommandToSend,strlen(pbDownloadCommandToSend))==-1)
    {
        return RBRLOGGER_RESULT_FAIL_SYSTEMERROR;
    }
    
    // wait for line with data X XX XXX
    ssize_t sstReturnValue=0;
    while ( (sstReturnValue=readLineFromLogger(sockfd,pbDownloadCommandAnswerBuffer, LOGGER2_MAXANSWER_SIZE-1))!= -1 )
    {
        pbDownloadCommandAnswerBuffer[sstReturnValue]='\0';
        pbAnswer=pbDownloadCommandAnswerBuffer;
        // read line until we find 'data' keyword (allows to ignore the Ready:)
        while (*pbAnswer!='\0')
        {
            if (strncmp("data",(const char*)pbAnswer,4)==0)
            {
                // !! found it
                pbAnswer+=4;
                break;
            }
            else pbAnswer++;
        }
        if (*pbAnswer=='\0') continue;
        
        // read parameters along 'data' keyword
        if (sscanf((const char*)pbAnswer,"%d %d %d",&adataset,&asize,&aoffset)!=3) continue;
        if ((adataset!=dataset)||(aoffset!=offset)||(asize>sizeToDownload)||(asize==0)) // misinterpretation of the previous command or end of file
        {
            // send abort
            if (writeBytes(sockfd,"abort\r\n",strlen("abort\r\n"))==-1)
            {
                return RBRLOGGER_RESULT_FAIL_SYSTEMERROR;
            }
            if (bHasBeenRetried==false)
            {
                bHasBeenRetried=true;
                goto retrysenddownloadcommand;
            }
            else  return RBRLOGGER_RESULT_FAIL_UNKNOWN;
        }
        
        // download chunk
        ssize_t downloaded=readBytes(sockfd,pbBuffer, asize);
        if (downloaded==-1) return RBRLOGGER_RESULT_FAIL_UNKNOWN;
        char chunkcrc[2];
        if (readBytes(sockfd,chunkcrc,2)==1) return RBRLOGGER_RESULT_FAIL_UNKNOWN;
        *downloadedSize=(int)downloaded;
        return RBRLOGGER_RESULT_SUCCESS;
        
        
    }
    return RBRLOGGER_RESULT_FAIL_UNKNOWN;
    
}


/**
 *  Send one command and wait for the answer to the same command.
 *  Populates the buffer with the answer
 *
 *  @param sockfd    socket descriptor
 *  @param timeout   timeout in ms (@todo implementation)
 *  @param bCommandToSend       command
 *  @param pcaTokens            <#pcaTokens description#>
 *  @param nbTokens             <#nbTokens description#>
 *  @param pbReplyBuffer        <#pbReplyBuffer description#>
 *  @param stReplyBufferMaxSize <#stReplyBufferMaxSize description#>
 *
 *  @return <#return value description#>
 */
ssize_t sendLoggerCommandLine(int sockfd,int timeout,const char* bCommandToSend,char** pcaTokens,char* pbReplyBuffer,size_t stReplyBufferMaxSize)
{
    
    char* strtok_save_ptr;
    
    unsigned int uIx=0;
    unsigned long len=0;
    unsigned int nbcharsCommand;
    unsigned int uNbTokens;
    
    // no answers tokens yet
    pcaTokens[0]=NULL;
    
    // check length of the command to send
    len=strlen(bCommandToSend);
    if (len==0) return 0;
    
    
    // send command
    if (writeBytes(sockfd,bCommandToSend,strlen(bCommandToSend))==-1)
    {
        perror("Connection : ");
        exit(EXIT_FAILURE);
    }
    
    // extract command  from what was sent
    assert(bCommandToSend[0]!=' ');
    assert(bCommandToSend[0]!='\r');
    assert(bCommandToSend[0]!='\n');
    for (uIx=0;uIx<len;uIx++)
    {
        if ((bCommandToSend[uIx]==' ')||(bCommandToSend[uIx]=='\r')||(bCommandToSend[uIx]=='\n'))
        {
            break;
        }
    }
    
    nbcharsCommand=uIx;
    
    // wait for answer
    ssize_t sstReturnValue=0;
    while ( (sstReturnValue=readLineFromLogger(sockfd,pbReplyBuffer, stReplyBufferMaxSize-1))!= -1 )
    {
        pbReplyBuffer[sstReturnValue]='\0';
        
        for (uIx=0;uIx<MAX_NUM_TOKENS-1;uIx++)
        {
            pcaTokens[uIx]=strtok_r(uIx==0 ? pbReplyBuffer:NULL," =,",&strtok_save_ptr);
            if (pcaTokens[uIx]==NULL) break;
        }
        uNbTokens=uIx;
        
        if (uNbTokens<1) continue; // nothing to process get next line
        
        // check if we have the prompt in front, remove it
        if (strcmp(pcaTokens[0],LOGGER2_PROMPT)==0)
        {
            for (uIx=1;uIx<uNbTokens;uIx++)
            {
                pcaTokens[uIx-1]=pcaTokens[uIx];
            }
            uNbTokens-=1;
        }
        if (uNbTokens<1) continue; // nothing to process, get next line
        if (strlen(pcaTokens[0])!=nbcharsCommand) continue;
        if (strncmp(bCommandToSend,pcaTokens[0],nbcharsCommand)==0)
        {
            // we got the answer requested ! yahoo !
            //*nbTokens=uNbTokens;
            pcaTokens[uNbTokens]=NULL;
            return sstReturnValue;
        }
    }
    pcaTokens[0]=NULL;
    return sstReturnValue;
}


/**
 *  Read the next line sent by the logger
 *s
 *  @param sockfd       socket descriptor
 *  @param bBytesLine   buffer to store the line characters without the end of line
 *  @param stNbBytesMax max number of bytes to store
 *
 *  @return number of bytes retrieved, -1 if error, check errno in this case
 */
ssize_t readLineFromLogger(int sockfd, char* bBytesLine,size_t stNbBytesMax)
{
    // read char per char the socket
    ssize_t sstReturnValue=0;
    size_t stNbBytesRead = 0;
    char cTmp;
    if (stNbBytesMax==0) return 0;
    
    do
    {
        sstReturnValue=recv(sockfd,(void*)(&cTmp),1,0);
        if (sstReturnValue>=0)
        {
            if (cTmp=='\r')
            {
                // just ignore this character
            }
            else if (cTmp=='\n')
            {
                return stNbBytesRead;
            }
            else
            {
                *(bBytesLine++)=cTmp;
                stNbBytesRead+=sstReturnValue;
            }
        }
    }while (((errno==EINTR)||(sstReturnValue>=0)) && (stNbBytesRead<stNbBytesMax));
    
    if (sstReturnValue<0) return sstReturnValue;
    return stNbBytesRead;
    
}

/**
 *  <#Description#>
 *
 *  @param sockfd <#sockfd description#>
 *
 *  @return <#return value description#>
 */
ssize_t discardBytes(int sockfd)
{
    struct timeval timeout;
    struct timeval oldtimeout;
    
    socklen_t tmp;
    char tmpbuffer;
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    
    
    if(getsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&oldtimeout,
                   &tmp)==-1) return -1;
    
    if(setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout,
                   sizeof(timeout))==-1) return -1;
    while (readBytes(sockfd,&tmpbuffer, 1) == 1);
    
    
    if(setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&oldtimeout,
                   sizeof(timeout))==-1) return -1;
    
    return 0;
}

/**
 *  Reads bytes on the socket
 *
 *  @param sockfd          <#sockfd description#>
 *  @param bBytesToRead    <#bBytesToRead description#>
 *  @param stNbBytesToRead <#stNbBytesToRead description#>
 *
 *  @return <#return value description#>
 */
ssize_t readBytes(int sockfd,char *  bBytesToRead, size_t stNbBytesToRead)
{
    ssize_t sstReturnValue=0;
    size_t stNbBytesRead = 0;
    do
    {
        sstReturnValue=recv(sockfd,(void*)(bBytesToRead+stNbBytesRead),stNbBytesToRead-stNbBytesRead,0);
        if (sstReturnValue>=0)
        {
            stNbBytesRead+=sstReturnValue;
        }
    }while (((errno==EINTR)||(sstReturnValue>=0)) && (stNbBytesRead!=stNbBytesToRead));
    
    if (sstReturnValue<0) return sstReturnValue;
    return stNbBytesRead;
}


/**
 *  Writes bytes onto the socket
 *
 *  @param sockfd           socket descriptor
 *  @param bBytesToSend     buffer of bytes to write
 *  @param stNbBytesToWrite nb of bytes to write
 *
 *  @return number of bytes written
 */
ssize_t writeBytes(int sockfd,const char *  bBytesToSend, size_t stNbBytesToWrite)
{
    ssize_t sstReturnValue=0;
    size_t stNbBytesSent = 0;
    do
    {
        sstReturnValue=send(sockfd,(void*)(bBytesToSend+stNbBytesSent),stNbBytesToWrite-stNbBytesSent,0);
        if (sstReturnValue>=0)
        {
            stNbBytesSent+=sstReturnValue;
        }
    }while (((errno==EINTR)||(sstReturnValue>=0)) && (stNbBytesToWrite!=stNbBytesSent));
    if (sstReturnValue<0) return sstReturnValue;
    return stNbBytesSent;
}


eRBRLoggerErrorCode openConnection(int* sock, const char* hostname, uint16_t port)
{
    
    struct hostent* hp;
    struct sockaddr_in addr;
    socklen_t   lgradr;
    
    //long l,lrep;
    
    unsigned short localport=htons(0);
    
    *sock=createSocket(&localport,SOCK_STREAM);
    hp=gethostbyname(hostname);
    
    if (hp==NULL) return RBRLOGGER_RESULT_FAIL_SYSTEMERROR;
    lgradr=sizeof(struct sockaddr_in);
    memset(&addr,0,lgradr);
    
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr = *((struct in_addr*)(hp->h_addr_list[0]));
    
    // connect
    int returnValue;
    
    do
    {
        returnValue=connect(*sock, (const struct sockaddr*)&addr, lgradr);
    }while ((returnValue==-1)&&(errno==EINTR));
    
    if (returnValue==-1){
        return RBRLOGGER_RESULT_FAIL_SYSTEMERROR;
    }
    return RBRLOGGER_RESULT_SUCCESS;
    
}


int createSocket(unsigned short*port, int type)
{
    int sSocket, sAuthorization,sOk;
    struct sockaddr_in stAddress;
    socklen_t   socklenLgr;
    
    sSocket=socket(PF_INET,type,0);
    if (sSocket==-1){perror("socket");exit(EXIT_FAILURE);}
    
    sAuthorization=1;
    sOk=setsockopt(sSocket,SOL_SOCKET,SO_REUSEADDR,&sAuthorization,sizeof(int));
    
    
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    sOk=setsockopt (sSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                    sizeof(timeout));
    if (sOk==-1) {perror("setsockopt");exit(EXIT_FAILURE);}
    
    sOk=setsockopt (sSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                    sizeof(timeout));
    if (sOk==-1) {perror("setsockopt");exit(EXIT_FAILURE);}
    
    
    socklenLgr=sizeof(struct sockaddr_in);
    memset(&stAddress,0,socklenLgr);
    stAddress.sin_family=AF_INET;
    stAddress.sin_port=htons(*port);
    stAddress.sin_addr.s_addr=htonl(INADDR_ANY);
    
    sOk=bind(sSocket,(struct sockaddr*)&stAddress,socklenLgr);
    if (sOk==-1) {perror("bind");exit(EXIT_FAILURE);}
    sOk=getsockname(sSocket,(struct sockaddr*)&stAddress,&socklenLgr);
    if (sOk==-1){perror("getsockname");exit(EXIT_FAILURE);}
    
    *port = ntohs(stAddress.sin_port);
    return (sSocket);
}

