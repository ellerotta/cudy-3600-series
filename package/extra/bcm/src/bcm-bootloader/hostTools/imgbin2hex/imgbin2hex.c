/***********************************************************************
 *
<:copyright-BRCM:2007:proprietary:standard

   Copyright (c) 2007 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 *  Application running in Linux host to convert software image from
 *  binary to hexadecimal ascii
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define END_OF_LINE "\n"
#define DATA_BUFFER_SIZE    2000   // 2000 bytes for data

int bin2hex(char *inFile, char *outFile)
{
    int i = 0, j = 0;
    int countRd = 0, countWr = 0;
    int exitCode = -1;
    int fdRead = -1, fdWrite = -1;
    unsigned char binaryBuf[DATA_BUFFER_SIZE], hexStr[(DATA_BUFFER_SIZE*2)+1];

    if ((fdRead = open(inFile, O_RDONLY)) == -1)
    {
        printf("Failed to open '%s' for read: %s\n", inFile, strerror(errno));
        goto out;
    }

    if ((fdWrite = open(outFile,
                        O_CREAT | O_WRONLY,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    {
        printf("Failed to open '%s' for write: %s\n", outFile, strerror(errno));
        goto out;
    }

    memset(binaryBuf, 0, DATA_BUFFER_SIZE);
    memset(hexStr, 0, (DATA_BUFFER_SIZE*2) + 1);

    while ((countRd = read(fdRead, (void *)binaryBuf, DATA_BUFFER_SIZE)) > 0)
    {
        for (i = 0, j = 0; i < countRd; i++, j += 2)
        {
            sprintf(&(hexStr[j]), "%02x", binaryBuf[i]);
        }
        countWr = write(fdWrite, (void *)hexStr, countRd * 2);
        if (countWr != (countRd * 2))
        {
            printf("Number of written bytes %d is not double number of read bytes %d\n", countWr, countRd);
            goto out;
        }
        // mark end of line
        write(fdWrite, (void *)END_OF_LINE, strlen(END_OF_LINE));
    }

    // mark end of file
    write(fdWrite, (void *)END_OF_LINE, strlen(END_OF_LINE));

    exitCode = 0;

out:
    if (fdRead != -1)
        close(fdRead);

    if (fdWrite != -1)
        close(fdWrite);

    return exitCode;
}

void usage(char *progName)
{
    printf("usage: %s [-i inFile] [-o outFile]\n", progName);
    printf("       i: binary input file name\n");
    printf("       o: hex output file name\n");
}

int main(int argc, char *argv[])
{
    int  c = 0, exitCode=0;
    char *inFile = NULL, *outFile = NULL;

    /* parse command line args */
    while ((c = getopt(argc, argv, "i:o:")) != -1)
    {
        switch(c)
        {
            case 'i':
                inFile = optarg;
                break;
            case 'o':
                outFile = optarg;
                break;
            default:
                usage(argv[0]);
                exit(-1);
        }
    }

    if (inFile == NULL || outFile == NULL)
    {
        usage(argv[0]);
        exit(-1);
    }

    exitCode = bin2hex(inFile, outFile);

    return exitCode;
}
