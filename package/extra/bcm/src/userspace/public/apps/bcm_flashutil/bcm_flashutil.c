/***********************************************************************
 *
 * <:copyright-BRCM:2019:DUAL/GPL:standard
 * 
 *    Copyright (c) 2019 Broadcom 
 *    All Rights Reserved
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 *
 ************************************************************************/
#include "bcm_flashutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 200

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
#ifndef DESKTOP_LINUX
   char line[SIZE];
   int start, end;

   if (argc < 4)
   {
      printf("usage: %s partition (1 = first : 2 = second : 3 = boot : 4 = non-boot,\n", argv[0]);
      printf("       start block (-1 = beginning), end block (-1 = end of partition),\n");
      printf("       key (optional, leave blank to see all ident values in image\n");
      return(-1);
   }

   start = atoi(argv[2]);
   end = atoi(argv[3]);
   int ret = bcmFlash_getIdent(atoi(argv[1]), &start, &end, argv[4], line, SIZE);

   if (!ret)
      printf("key not found\n");
   else if (ret < 0)
      return(-1);
   else
      printf("\nkey size %d/%d found at block 0x%x:\n%.*s\n", ret, SIZE, start, ret, line);
#endif
   return(0);
}

