//
//  main.c
//  Wifi Logger 2 Downloader
//
//  Created by Jean-Michel Leconte on 2014-06-17.
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
#include <getopt.h>

#include <stdbool.h>
#include <limits.h>

#include "RBRLoggerCommands.h"
#include "RBRLoggerEasyParseEvents.h"

#define RBRWIFILOGGER_DOWNLOADER_VERSION 0.001

#define RBRWIFILOGGER_DEFAULT_TIMEOUT 5




int createSocket(unsigned short*port, int type);




#define LOGGER2_MAXCHUNK_SIZE 1024+2
/* Flag set by `--verbose'. */
static int verbose_flag;

static const char downloaderversion[]="wifilogger2downloader version 0.001\n";
static const char downloaderusage[]="Usage : wifilogger2downloader\n"
"\t[output-filename=<file>]\n"
"\t[profile[=first|last|<profile number>]]\n";

typedef enum _eProfileSelectionKind
{
    PROFILE_SELECTION_FIRST_KIND,
    PROFILE_SELECTION_LAST_KIND
} eProfileSelectionKind;

typedef enum _eCastIncludedDefinition
{
    PROFILE_UPANDDOWN_INCLUDED,
    PROFILE_UPCAST_ONLY,
    PROFILE_DOWNCAST_ONLY
} eCastIncludedDefinition;

typedef struct _tProfileOptions
{
    eProfileSelectionKind eProfileselectionkind;
    eCastIncludedDefinition eProfileDefinition;
    int numberofprofiles;
} tProfileOptions;


typedef struct _tDownloadOptions
{
    bool boPerProfile;
    bool boWakeup;
    const char* pcOutputFilename;
    const char* pcEventFilename;
    tProfileOptions tProfilesoptions;
    unsigned int offset;
    int sizeToDownload;
    
} tDownloadOptions;


typedef unsigned long tDatasetAddress;


void extractProfiles(tEasyParseEvent* events,int nbEvents,  tDatasetAddress* profilesStarts,tDatasetAddress* profilesEnds,int* nbProfiles, eCastIncludedDefinition profileDef);


int main(int argc, const char * argv[])
{
    
    
    // process options first
    int c;
    bool boVersionRequired=false;
    bool boHelpRequired=false;
    
    
    
    /**
     *  default options
     */
    tDownloadOptions downloadoptions=
    {
        true, // per profile
        true, // wake up
        NULL, // outputfilename
        NULL, // events filename
        {PROFILE_SELECTION_LAST_KIND,PROFILE_UPANDDOWN_INCLUDED,1}, // profile options
        0, //offset
        INT_MAX //size to download
        
    };
    
    
    FILE* outputdescriptor=stdout;
    FILE* outputevents=NULL;

    
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", 0, &verbose_flag, 1},
            {"brief", 0, &verbose_flag, 0},
            {"version", no_argument, NULL, 'v'},
            {"help", no_argument, NULL, 'h'},
            {"datafile",required_argument, 0, 'd'},
            {"eventsfile",required_argument, 0, 'e'},
            {"profile", optional_argument, 0, 'p'},
            {"numberofprofiles", required_argument, 0, 'n'},
            {"typeofprofiles", required_argument, 0, 't'},
            {"offset", optional_argument, 0, 'o'},
            {"length", optional_argument, 0, 'l'},
            {"wakeup", required_argument, 0, 'w'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long_only (argc, (char* const*)argv, "",
                              long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option UU %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
                
            case 'd':
                if (optarg)
                {
                    downloadoptions.pcOutputFilename=optarg;
                    // try to open the file
                    outputdescriptor=fopen(downloadoptions.pcOutputFilename,"a+");
                    if (outputdescriptor==NULL)
                    {
                        perror("Data file can't be opened :");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'o':
                // default to offset download
                downloadoptions.boPerProfile=false;
                if (optarg)
                {
                    
                    if (sscanf(optarg,"%d",&(downloadoptions.offset))!=1)
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'l':
                // default to offset download
                downloadoptions.boPerProfile=false;
                if (optarg)
                {
                    
                    if (strcmp(optarg,"all")==0)
                        downloadoptions.sizeToDownload=INT_MAX;
                        break;
                    if (sscanf(optarg,"%d",&(downloadoptions.sizeToDownload))!=1)
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'e':
                if (optarg)
                {
                    downloadoptions.pcEventFilename=optarg;
                    // try to open the file
                    outputevents=fopen(downloadoptions.pcEventFilename,"a+");
                    if (outputevents==NULL)
                    {
                        perror("Events file can't be opened :");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'w':
                if (optarg)
                {
                    if (strcmp(optarg,"yes")==0)
                    {
                        downloadoptions.boWakeup=true;
                    }
                    else if (strcmp(optarg,"no")==0)
                    {
                        downloadoptions.boWakeup=false;
                    }
                    else
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                    
                }
                break;
            case 'p':
                if (optarg)
                {
                    if (strcmp(optarg,"last")==0)
                    {
                        downloadoptions.tProfilesoptions.eProfileselectionkind=PROFILE_SELECTION_LAST_KIND;
                    }
                    else if (strcmp(optarg,"first")==0)
                    {
                        downloadoptions.tProfilesoptions.eProfileselectionkind=PROFILE_SELECTION_FIRST_KIND;
                    }
                    else
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 't':
                if (optarg)
                {
                    if (strcmp(optarg,"down")==0)
                    {
                        downloadoptions.tProfilesoptions.eProfileDefinition=PROFILE_DOWNCAST_ONLY;
                    }
                    else if (strcmp(optarg,"up")==0)
                    {
                        downloadoptions.tProfilesoptions.eProfileDefinition=PROFILE_UPCAST_ONLY;
                    }
                    else if (strcmp(optarg,"downandup")==0)
                    {
                        downloadoptions.tProfilesoptions.eProfileDefinition=PROFILE_UPANDDOWN_INCLUDED;
                    }
                    else
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'n':
                if (optarg)
                {
                    if (sscanf(optarg,"%d",&(downloadoptions.tProfilesoptions.numberofprofiles))!=1)
                    {
                        //@ todo error message
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'v':
                boVersionRequired=true;
                break;
                
            case 'h':
                boHelpRequired=true;
                break;
                
            case '?':
                /* getopt_long already printed an error message. */
                break;
                
            default:
                abort ();
        }
    }
    
    
    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        fprintf (stderr,"Unrecognized arguments: ");
        while (optind < argc)
            fprintf (stderr,"%s ", argv[optind++]);
        fprintf(stderr,"\n");
        fprintf(stderr,"%s",downloaderusage);
        exit(EXIT_FAILURE);
    }
    
    
    if (boVersionRequired==true)
    {
        fprintf(stderr,"%s",downloaderversion);
    }
    
    if (boHelpRequired == true)
    {
        fprintf(stderr,"%s",downloaderusage);
    }
    
    
    
    char downloadbuffer[LOGGER2_MAXCHUNK_SIZE];
    
    
    
    
    
    
    
    
   
    int filesize=0, fileeventsize=0;
    int sock=0;
    
    // error codes and message
    eRBRLoggerErrorCode errorcode=RBRLOGGER_RESULT_SUCCESS;
    const char* specificerrormessage=NULL;
  
    
    // arrays which may be allocated
    tEasyParseEvent* events=NULL;
    tDatasetAddress* profilesend=NULL;
    tDatasetAddress* profilesstart=NULL;
    tDatasetAddress* todownloadstart=NULL;
    tDatasetAddress* todownloadend=NULL;
    int nbofdownloadblocks=0;
    
    
    if ((errorcode=openConnection(&sock,RBRLOGGER_DEFAULT_HOSTNAME, RBRLOGGER_DEFAULT_PORT))!=RBRLOGGER_RESULT_SUCCESS) goto errorwhiledownloading;
    
    fprintf(stderr,"Connected\r\n");
    
    
   

    
  
    
    // wake up call
    if (downloadoptions.boWakeup==true)
    {
        fprintf(stderr,"Wake up\r\n");
        if ((errorcode=rbrLoggerEnsureWokeUp(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT) )!= RBRLOGGER_RESULT_SUCCESS) goto errorwhiledownloading;
    }
    
    // get memory format
    eRBRLoggerMemoryFormat memformat=RBRLOGGER_MEMORY_FORMAT_UNKNOWN;
    if ((errorcode=rbrLoggerGetMemoryFormat(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,&memformat) )!= RBRLOGGER_RESULT_SUCCESS) goto errorwhiledownloading;
    
    
    if (memformat==RBRLOGGER_MEMORY_FORMAT_RAWBINARY)
    {
        fprintf(stderr,"Meminfo raw binary\r\n");
        rbrLoggerGetMemoryInformation(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,1,&filesize);
        fileeventsize=0;
    }
    else if(memformat==RBRLOGGER_MEMORY_FORMAT_EASYPARSE)
    {
        fprintf(stderr,"Meminfo easyParse\r\n");
        rbrLoggerGetMemoryInformation(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,0,&fileeventsize);
        rbrLoggerGetMemoryInformation(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,1,&filesize);
    }
    
    // if download per profile is selected
    // we should download the entire events file and parse it
    if (downloadoptions.boPerProfile==true)
    {
        if (memformat==RBRLOGGER_MEMORY_FORMAT_RAWBINARY)
        {
            specificerrormessage="Can't download per profile with a logger configured in rawbin00.";
            goto errorwhiledownloading;
        }
        // download the entire event file
        
        // first make some place in the memory to download the event file
        int nbofevents=fileeventsize/RBRLOGGER_EASYPARSEEVENT_FOOTPRINT;
        if ((nbofevents>1000000)||(nbofevents<=0)||(fileeventsize%16!=0))
        {
            specificerrormessage="Problem with the event file.";
            goto errorwhiledownloading;
        }
        
        // alloc memory to store the events
        tEasyParseEvent* events=(tEasyParseEvent*)malloc(sizeof(tEasyParseEvent)*nbofevents);
        if (events==NULL)
        {
            specificerrormessage="Not enough memory.";
            goto errorwhiledownloading;
        }
        
        // download & parse the events file
      
        int bytesleft=fileeventsize;
        int offset=0;
        int currentevent=0;
        int downloaded;
        
        while (bytesleft>0)
        {
            if ((errorcode=rbrLoggerDownloadDataFileChunk(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,0, offset,LOGGER2_MAXCHUNK_SIZE/RBRLOGGER_EASYPARSEEVENT_FOOTPRINT, downloadbuffer,&downloaded))!=RBRLOGGER_RESULT_SUCCESS) goto errorwhiledownloading;
            if (outputevents!=NULL)
            {
                fwrite(downloadbuffer,sizeof(uint8_t),downloaded,outputevents);
            }

            int k;
            for (k=0;k<downloaded/RBRLOGGER_EASYPARSEEVENT_FOOTPRINT;k++)
            {
                if (rbrLoggerParseEasyParseEvent((const unsigned char *)(downloadbuffer+k*RBRLOGGER_EASYPARSEEVENT_FOOTPRINT),&(events[k]))!=0)
                {
                    specificerrormessage="Problem with the event file, event not parsable.";
                    goto errorwhiledownloading;
                }
            }
            
            currentevent+=k;
            offset+=downloaded;
            bytesleft-=downloaded;
            
            
        }
        if (currentevent==0){;} // @todo !!
        
        // detect profiles start and stop according to the definition of a profile
        int nbprofiles=0;
      
        // alloc maximum number of profiles
        profilesstart=(tDatasetAddress*)malloc(currentevent*sizeof(currentevent));
        profilesend=(tDatasetAddress*)malloc(currentevent*sizeof(currentevent));
        if ((profilesend==NULL)||(profilesstart==NULL))
        {
            specificerrormessage="Not enough memory.";
            goto errorwhiledownloading;
        }
        
        // extract profiles
        extractProfiles(events, currentevent,profilesstart,profilesend,&nbprofiles,downloadoptions.tProfilesoptions.eProfileDefinition);
        fprintf(stderr,"Number of availables profiles :%d\n",nbprofiles);
        
        // build download blocks info
        nbofdownloadblocks=downloadoptions.tProfilesoptions.numberofprofiles;
        if (nbofdownloadblocks>nbprofiles)nbofdownloadblocks=nbprofiles;

        // allocate memory
        todownloadstart=malloc(nbofdownloadblocks*sizeof(tDatasetAddress));
        todownloadend=malloc(nbofdownloadblocks*sizeof(tDatasetAddress));
        if ((todownloadstart==NULL)||(todownloadend==NULL))
        {
            specificerrormessage="Not enough memory.";
            goto errorwhiledownloading;
        }
        switch (downloadoptions.tProfilesoptions.eProfileselectionkind)
        {
            case PROFILE_SELECTION_FIRST_KIND:
                for (int k=0,k2=0;k<nbofdownloadblocks;k++,k2++)
                {
                    todownloadstart[k2]=profilesstart[k];
                    todownloadend[k2]=profilesend[k];
                }
                break;
            case PROFILE_SELECTION_LAST_KIND:
                for (int k=nbprofiles-nbofdownloadblocks,k2=0;k<nbprofiles;k++,k2++)
                {
                    todownloadstart[k2]=profilesstart[k];
                    todownloadend[k2]=profilesend[k];
                }
                break;
        }
    }
    else
    {
        // download per offset, build download blocks
        nbofdownloadblocks=1;
        todownloadstart=malloc(nbofdownloadblocks*sizeof(tDatasetAddress));
        todownloadend=malloc(nbofdownloadblocks*sizeof(tDatasetAddress));
        if ((todownloadstart==NULL)||(todownloadend==NULL))
        {
            specificerrormessage="Not enough memory.";
            goto errorwhiledownloading;
        }
        // make sure we are not downloading non existing data
        todownloadstart[0]=downloadoptions.offset;
        todownloadend[0]=downloadoptions.offset+downloadoptions.sizeToDownload;
        
        if (todownloadstart[0]>=filesize)
        {
            nbofdownloadblocks=0;
        }
        else if (todownloadend[0]>=filesize)
        {
            todownloadend[0]=filesize;
        }
        
    }
    
    
    
    
    // download all blocks requested
    
    
    for (int k=0;k<nbofdownloadblocks;k++)
    {
        //download data file
        fprintf(stderr,"Downloading data block : %ld %ld\n",todownloadstart[k],todownloadend[k]);
        int bytesleft=(int)(todownloadend[k]-todownloadstart[k]);
        int offset=(int)todownloadstart[k];
        int downloaded=0;
        
        while (bytesleft>0)
        {
            if ((errorcode=rbrLoggerDownloadDataFileChunk(sock,RBRWIFILOGGER_DEFAULT_TIMEOUT,1, offset,((bytesleft>LOGGER2_MAXCHUNK_SIZE)?LOGGER2_MAXCHUNK_SIZE:bytesleft), downloadbuffer,&downloaded))!=RBRLOGGER_RESULT_SUCCESS) goto errorwhiledownloading;

            fwrite(downloadbuffer,1,downloaded,outputdescriptor);
            offset+=downloaded;
            bytesleft-=downloaded;
        }
    }
    
    
    // end of the program a
errorwhiledownloading:
    // free memory allocated first
    if (events!=NULL) free(events);
    if (profilesend!=NULL) free(profilesend);
    if (profilesstart!=NULL) free(profilesstart);
    if (todownloadend!=NULL) free(todownloadend);
    if (todownloadstart!=NULL) free(todownloadstart);
    
    
    // because we may screw the errno, display now any system error
    if (errorcode==RBRLOGGER_RESULT_FAIL_SYSTEMERROR) perror("Error while downloading :");
    // close the socket in any case
    while ((close(sock)==-1)&&(errno==EINTR));

    
    // close output file in any case
    fflush(outputdescriptor);
    fclose(outputdescriptor);
    
    // close event file if in use
    if (outputevents!=NULL){
        fflush(outputevents);
        fclose(outputevents);
    }
    // process errors
    switch (errorcode)
    {
        case RBRLOGGER_RESULT_FAIL_SYSTEMERROR:
            // alredy done
            break;
        case RBRLOGGER_RESULT_FAIL_UNKNOWN:
            fprintf(stderr,"%s\r\n",RBRLOGGER_DEFAULTERRORMESSAGE_UNKNOWN);
            break;
        case RBRLOGGER_RESULT_FAIL_TIMEOUT:
            fprintf(stderr,"%s\r\n",RBRLOGGER_DEFAULTERRORMESSAGE_TIMEOUT);
            break;
        case RBRLOGGER_RESULT_SUCCESS:
            break;
            
    }
    
    if (errorcode!=RBRLOGGER_RESULT_SUCCESS) exit(EXIT_FAILURE);
    if (specificerrormessage!=NULL)
    {
        fprintf(stderr,"%s\r\n",specificerrormessage);
        exit(EXIT_FAILURE);
    }
    
    
    
    
    fprintf(stderr,"Disconnected\r\n");
    
    
    exit(EXIT_SUCCESS);
    return 0;
}







void extractProfiles(tEasyParseEvent* events,int nbEvents,  tDatasetAddress* profilesStarts,tDatasetAddress* profilesEnds,int* nbProfiles, eCastIncludedDefinition profileDef)
{
    
    // inspect events
    enum CurrentCastKind
    {
        CURRENTCASTKIND_OUTOFCAST,
        CURRENTCASTKIND_UPCAST,
        CURRENTCASTKIND_DOWNCAST
    };
    enum CurrentCastKind currentcastkind=CURRENTCASTKIND_OUTOFCAST;
  
    
    
    tDatasetAddress currentProfileStart=0;
   // tDatasetAddress currentProfileEnd=0;
    
    for (int i=0;i<nbEvents;i++)
    {
        switch (events[i].type)
        {
            case RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_UPCAST:
                
                //@todo check we are not currently in another cast (normally not, the logger should take care of it)
                //
                currentcastkind=CURRENTCASTKIND_UPCAST;
                switch (profileDef)
            {
                case PROFILE_UPCAST_ONLY:
                    currentProfileStart=events[i].payload.profileoffset;
                    break;
                case PROFILE_DOWNCAST_ONLY:
                    // nothing to do
                    break;
                case PROFILE_UPANDDOWN_INCLUDED:
                    // do nothing
                    break;
            }
                break;
                
                
            case RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_DOWNCAST:
                //@todo check we are not currently in another cast (normally not, the logger should take care of it)
                //
                currentcastkind=CURRENTCASTKIND_DOWNCAST;
                switch (profileDef)
            {
                case PROFILE_UPCAST_ONLY:
                    // nothing to do
                    break;
                case PROFILE_DOWNCAST_ONLY:
                    currentProfileStart=events[i].payload.profileoffset;
                    break;
                case PROFILE_UPANDDOWN_INCLUDED:
                    currentProfileStart=events[i].payload.profileoffset;
                    break;
            }
                break;
            case RBRLOGGER_EASYPARSEEVENT_PROFILES_END_CAST:
                switch (currentcastkind)
            {
                case CURRENTCASTKIND_DOWNCAST:
                    if (profileDef==PROFILE_DOWNCAST_ONLY)
                    {
                        profilesStarts[*nbProfiles]=currentProfileStart;
                        profilesEnds[*nbProfiles]=events[i].payload.profileoffset;
                        (*nbProfiles)++;
                    }
                    else if (profileDef==PROFILE_UPANDDOWN_INCLUDED)
                    {
                        // in this case consume next event and check it is the beginning of an upcast
                        // if not just close this profile with what we have
                        i++;
                        if ((i>=nbEvents)|| (events[i].type!=RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_UPCAST))
                        {
                            profilesStarts[*nbProfiles]=currentProfileStart;
                            profilesEnds[*nbProfiles]=events[i].payload.profileoffset;
                            nbProfiles++;
                            currentcastkind=CURRENTCASTKIND_OUTOFCAST;
                            if (events[i].type==RBRLOGGER_EASYPARSEEVENT_PROFILES_BEGIN_DOWNCAST)
                            {
                                currentcastkind=CURRENTCASTKIND_DOWNCAST;
                                currentProfileStart=events[i].payload.profileoffset;
                            }
                            continue; // continue to process events
                        }
                        currentcastkind=CURRENTCASTKIND_UPCAST;
                        continue;
                    }
                    break;
                case CURRENTCASTKIND_UPCAST:
                    if ((profileDef==PROFILE_UPCAST_ONLY)||(profileDef==PROFILE_UPANDDOWN_INCLUDED))
                    {
                        profilesStarts[*nbProfiles]=currentProfileStart;
                        profilesEnds[*nbProfiles]=events[i].payload.profileoffset;
                        (*nbProfiles)++;
                    }
                    break;
                case CURRENTCASTKIND_OUTOFCAST:
                    //@todo verbose weird event
                    break;
                    
            }
                currentcastkind=CURRENTCASTKIND_OUTOFCAST;
                break;
            default:
                break;
        }
    }
    
}



