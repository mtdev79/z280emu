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
   size_t len, ilen;
   offs_t addr, i;
   char buf[80];
   offs_t base = 0, code = 0;

   printf(";dis280 v1.0\n");

   if (argc<2) {
      printf("Usage: dis280 filename [options]");
      exit(1);
   }
   for (i = 0; i < argc; i++)
   {
      if (strncmp(argv[i],"-base=",6)==0) { /* load address of code seg, eg.0x100 */
         base=strtol(argv[i]+6,NULL,16); 
      } else if (strncmp(argv[i],"-code=",6)==0) { /* phys offset of code seg in input file */
         code=strtol(argv[i]+6,NULL,16); 
      }
   }
   if ((f=fopen(argv[i-1],"rb"))) {
      fseek(f,code,SEEK_SET);
      len = fread(_ram+base,1,131072-base,f);
      printf(";%s: %d code bytes\n",argv[i-1],len);
      fclose(f);
   }
   for (addr=base; addr<len+base; ) {
      ilen = cpu_disassemble_z280(NULL, buf, addr, &_ram[addr], 0)&DASMFLAG_LENGTHMASK;
      printf("%04x:\t",addr);
      for (i=0;i<ilen;i++) printf("%02X",_ram[addr+i]);
      for ( ;i<7;i++) {putchar(' ');putchar(' ');}
      printf("%s\n",buf);
      addr += ilen;
   }
}
