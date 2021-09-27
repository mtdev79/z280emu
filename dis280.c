/*
 * dis280.c - simple Z280 disassembler
 *
 * Copyright (c) Michal Tomek 2021 <mtdev79b@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "z280/z280.h"

uint8_t _ram[131072];

int main(int argc, char** argv)
{
   FILE* f;
   size_t len, j;
   offs_t i;
   char buf[80];

   printf(";dis280 v1.0\n");

   if (argc<2) {
      printf("Usage: dis280 filename [options]");
      exit(1);
   }
   if ((f=fopen(argv[1],"rb"))) {
      len = fread(_ram,1,131072,f);
      printf(";%s: %d bytes\n",argv[1],len);
      fclose(f);
   }
   for (i=0; i<len; ) {
      j = cpu_disassemble_z280(NULL, buf, i, &_ram[i], 0)&DASMFLAG_LENGTHMASK;
      printf("%04x:\t%s\n",i,buf);
      i += j;
   }
}
