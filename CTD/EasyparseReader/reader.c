//
//  main.c
//  EasyparseReader
//
//  Created by Jean-Michel Leconte on 2014-07-02.
//  Inspired by Graham Jones code
//  Copyright (c) 2014 RBR Ltd. All rights reserved.
//

#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int _CRT_fmode = _O_BINARY;
int _fmode = _O_BINARY;

typedef float float32_t;

void	dtm_date_from_days(char * dtm, uint16_t days);
void	dtm_time_from_secs(char * dtm, uint32_t secs);
float32_t	byte_array_to_float(uint8_t * pbArray);
void	to_ascii(uint8_t * array);
uint64_t	byte_array_to_longlong(uint8_t * pbArray);
void	process_timestamp(uint8_t * pbTimestamp);

int main(int argc, const char * argv[]) //main takes in the argument count and argument vector
{

    int iNumChans=0;
    // extract nb of channels argument
    if (argc < 2) { //if there are less than two arguemtns (ie number of channels not provided) exit the program
        printf("\r\nUsage: %s <numChannels>\n", argv[0]); // numChannels?
        exit(EXIT_FAILURE);
    }
    
    if ((sscanf(argv[1], "%d", &iNumChans) !=1)|| (iNumChans<1) ) { //assigns iNumChans to argv1 ... || redundant?
        printf("Require number of channels as argument\n");
        exit(EXIT_FAILURE);
    }
    
    //allocate buffer for 1 sample
    int buffersize=sizeof(uint64_t)+iNumChans*sizeof(uint32_t);
    uint8_t* buffer=(uint8_t*)malloc(buffersize);
    if (buffer==NULL)
    {
        fprintf(stderr,"Not enough memory.");
        exit(EXIT_FAILURE);
    }
    
    
    // read stdin and output in stdout
    
    while (feof(stdin)==0)
    {
        size_t sizeread=fread(buffer,sizeof(uint8_t),buffersize,stdin);
        if (ferror(stdin))
        {
            fprintf(stderr,"Error reading.");
            exit(EXIT_FAILURE);
        }
        if (sizeread==buffersize)
        {
            process_timestamp(buffer);
            
        
        
        for (int k=0;k<iNumChans;k++)
        {
    
            fprintf(stdout,"\t%.4f", byte_array_to_float(buffer+sizeof(uint64_t)+k*sizeof(uint32_t)));
        }
            fprintf(stdout,"\r\n");
        }
        
    }
    
    free(buffer);
    
    return 0;
}




/*****************************************************************************
 ***
 *** code: doing something with the received data.
 ***
 *****************************************************************************/

/*-----------------------------------------------------------------------*/
/**
  * \brief   Process a time-stamp.
  * \return  None.
  * \param   uint8_t * pbTimestamp, pointer to time-stamp as an array of bytes.
  * \author  gjones
  *
  * \details
  * The conversion is mildly interesting, printing it is trivial.
  *--------------------------------------------------------------------------*/
void	process_timestamp(uint8_t * pbTimestamp)
{
#define	UNIX_EPOCH_OFFSET	(946684800000ULL)
	// EasyParse time-stamps share the Unix time origin, 1970/01/01 00:00:00.
	// RBR's date/time conversion routines start from 2000/01/01 00:00:00, so
	// an offset is needed to use them.  If you are using another date/time
	// convention, different adjustments may be needed.
    
    static const char *	aszMonthNames[12] =
    {
        "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    };
#define	MONTH(x)	(aszMonthNames[10*(((char *)(x))[0]-'0')+(((char *)(x))[1]-'0')-1])
	// Convert month from a 2-digit substring to a name, eg. "04" to "Apr".
    
    uint64_t	uuTimestamp;
    uint16_t	uMilliseconds;
    uint32_t	ulSeconds;
    char		acDateTimeString[32];
    
    // Adjust offset, separate seconds and milliseconds.
    uuTimestamp		= byte_array_to_longlong(pbTimestamp) - UNIX_EPOCH_OFFSET;
    uMilliseconds	= (uint16_t)(uuTimestamp%1000LL);
    ulSeconds		= (uint32_t)(uuTimestamp/1000LL);
    
    // Basic seconds to date/time conversion, the tricky part.
    dtm_date_from_days(&acDateTimeString[0], (uint16_t)(ulSeconds/86400L));
    dtm_time_from_secs(&acDateTimeString[6], ulSeconds%86400L);
    
    // Prettify the output format while writing:
    //	YYMMDDhhmmss -> 20YY-Mon-DD hh:mm:ss.ttt
    printf("20%.2s-%s-%.2s %.2s:%.2s:%.2s.%03d",
           &acDateTimeString[0], MONTH(&acDateTimeString[2]), &acDateTimeString[4],
           &acDateTimeString[6], &acDateTimeString[8], &acDateTimeString[10],
           uMilliseconds);
    
}	// process_timestamp()

/*-----------------------------------------------------------------------*/
/**
 * \brief   Process a single reading from one channel.
 * \return  None.
 * \param   uint8_t * pbReading, pointer to value as an array of bytes.
 * \author  gjones
 *
 * \details
 * Trivial example, just print it.
 *--------------------------------------------------------------------------*/
void	process_reading(uint8_t * pbReading)
{
    printf( "%12.4f", byte_array_to_float(pbReading));
    
}	// process_reading()

/*-----------------------------------------------------------------------*//**
 * \brief   Do any processing once a whole sample set is dealt with.
 * \return  None.
 * \param   None.
 * \author  gjones
 *
 * \details
 * The example is trivial; send a new-line after printing one sample set.
 *--------------------------------------------------------------------------*/
void	process_sample_set()
{
    printf("\r\n");
    
}	// process_sample_set()


/*****************************************************************************
 ***
 *** code: support functions for date/time conversion.
 ***
 *****************************************************************************/

/*-----------------------------------------------------------------------*//**
 *--------------------------------------------------------------------------*/
void	dtm_date_from_days(char * dtm, uint16_t days)
{
    static const uint8_t      dtm_DaysInMonth[2][12] =
    {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    } ;
    
#define	YY	dtm[0]
#define	MM	dtm[1]
#define	DD	dtm[2]
    
    const uint8_t *	mp ;
    
    
    /* get years in complete 4 year cycles	*/
    YY = 4*(days/1461) ;
    
    /* get remaining days; if beyond first	*/
    /* (leap) year, adjust years and get	*/
    /* remaining days in year		*/
    if ((days %= 1461) > 365)
    {
        YY   += (days-1)/365 ;
        days  = (days-1)%365 ;
    }
    
    /* set up days-in-month array pointer,	*/
    /* subtract days in month, incrementing	*/
    /* month, until run out of days		*/
    for (
         mp = &dtm_DaysInMonth[(YY%4)?0:1][0], MM = 1 ;
         days >= *mp ;
         days -= *mp++, ++MM
         )
        ;
    
    /* store complete days  */
    DD = days+1 ;
    
    /* do format conversion	*/
    to_ascii((uint8_t *)(dtm)) ;
    
}	/* end of dtm_date_from_days() */

/*-----------------------------------------------------------------------*//**
 *--------------------------------------------------------------------------*/
void	dtm_time_from_secs(char * dtm, uint32_t secs)
{
#define	hh	dtm[0]
#define	mm	dtm[1]
#define	ss	dtm[2]
    
    uint16_t tmp     = (uint16_t)(secs%3600L) ;		/* fractional hours	*/
    
    hh = (uint8_t)(secs/3600L);
    mm = (uint8_t)(tmp/60);
    ss = (uint8_t)(tmp%60);
    
    /* do format conversion	*/
    to_ascii((uint8_t *)(dtm));
    
}	/* end of dtm_time_from_secs() */


/*****************************************************************************
 ***
 *** code: support functions for date/time output formatting.
 ***
 *****************************************************************************/

/*-----------------------------------------------------------------------*//**
 *--------------------------------------------------------------------------*/
void	to_ascii(uint8_t * array)
{
    uint8_t	i,b;
    
    /* work backwards to use values before trashing them	*/
    for (array[6]=0, i=6 ; i>0 ; )
    {
        b = array[(i>>1)-1]%100;
        array[--i] = (b % 10) + '0';	/* LS-digit in MS-Byte	*/
        array[--i] = (b / 10) + '0';	/* LS-digit in MS-Byte	*/
    }
    
}	/* end of to_ascii() */


/*****************************************************************************
 ***
 *** code: support functions for portable number format conversion.
 ***
 *****************************************************************************/

/*-----------------------------------------------------------------------*//**
 *--------------------------------------------------------------------------*/
uint64_t	byte_array_to_longlong(uint8_t * pbArray)
{
    // use a union to ensure correct alignment.
    union longlong_bytes
    {
        uint64_t	value;
        uint8_t		array[sizeof(uint64_t)];
    } u;
    
#if (!defined(__HOST_IS_BIG_ENDIAN__) || (__HOST_IS_BIG_ENDIAN__ == 0))
	memmove(&u.value, pbArray, sizeof(uint64_t));
#else
	u.array[0] = pbArray[7];
	u.array[1] = pbArray[6];
	u.array[2] = pbArray[5];
	u.array[3] = pbArray[4];
	u.array[4] = pbArray[3];
	u.array[5] = pbArray[2];
	u.array[6] = pbArray[1];
	u.array[7] = pbArray[0];
#endif
    
    return u.value;
    
}	// byte_array_to_longlong()

/*-----------------------------------------------------------------------*//**
 *--------------------------------------------------------------------------*/
float32_t	byte_array_to_float(uint8_t * pbArray)
{
    // use a union to ensure correct alignment.
    union float_bytes
    {
        float32_t	value;
        uint8_t		array[sizeof(float32_t)];
    } u;
    
#if (!defined(__HOST_IS_BIG_ENDIAN__) || (__HOST_IS_BIG_ENDIAN__ == 0))
	memmove(&u.value, pbArray, sizeof(float32_t));
#else
	u.array[0] = pbArray[3];
	u.array[1] = pbArray[2];
	u.array[2] = pbArray[1];
	u.array[3] = pbArray[0];
#endif
    
    return u.value;
    
}	// byte_array_to_float()

