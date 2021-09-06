ifeq ($(OS),Windows_NT)
	SOCKLIB = -lws2_32
endif

CCOPTS += -O3 -DSOCKETCONSOLE -std=gnu89 -fcommon

all: z280rc makedisk

z280rc: ide.o z280.o z280dasm.o z80daisy.o z280uart.o z280rc.o rtc_z280rc.o ds1202_1302.o
	$(CC) $(CCOPTS) -s -o z280rc $^ $(SOCKLIB)

z280rc.o: z280rc.c sconsole.h z280dbg.h z280/z280.h z280/z80daisy.h z280/z80common.h ds1202_1302/ds1202_1302.h
	$(CC) $(CCOPTS) -c z280rc.c

rtc_z280rc.o: ds1202_1302/rtc.c ds1202_1302/rtc.h
	cd ds1202_1302 ; $(CC) $(CCOPTS) -Dmachine_name=\"z280rc\" -DHAVE_SYS_TIME_H -DHAVE_GETTIMEOFDAY -o ../rtc_z280rc.o -c rtc.c 

ide.o:	ide/ide.c ide/ide.h
	cd ide ; $(CC) $(CCOPTS) -o ../ide.o -c ide.c 

z280.o:	z280/z280.c z280/z280cb.c z280/z280dd.c z280/z280dded.c z280/z280ed.c z280/z280fd.c z280/z280fded.c z280/z280op.c z280/z280xy.c z280/z280.h z280/z280ops.h z280/z280tbl.h z280/z80daisy.h z280/z80common.h
	cd z280 ; $(CC) $(CCOPTS) -o ../z280.o -c z280.c 

z280dasm.o: z280/z280dasm.c z280/z280.h z280/z80common.h
	cd z280 ; $(CC) $(CCOPTS) -o ../z280dasm.o -c z280dasm.c 

z80daisy.o: z280/z80daisy.c z280/z280.h z280/z80daisy.h z280/z80common.h
	cd z280 ; $(CC) $(CCOPTS) -o ../z80daisy.o -c z80daisy.c 

z280uart.o: z280/z280uart.c z280/z280uart.h z280/z280.h z280/z80common.h
	cd z280 ; $(CC) $(CCOPTS) -o ../z280uart.o -c z280uart.c 

ds1202_1302.o: ds1202_1302/ds1202_1302.c ds1202_1302/ds1202_1302.h ds1202_1302/rtc.h
	cd ds1202_1302 ; $(CC) $(CCOPTS) -o ../ds1202_1302.o -c ds1202_1302.c 

#ins8250.o: ins8250/ins8250.c ins8250/ins8250.h
#	cd ins8250 ; $(CC) $(CCOPTS) -o ../ins8250.o -c ins8250.c

makedisk: makedisk.o ide.o
	$(CC) $(CCOPTS) -s -o makedisk $^

makedisk.o: ide/makedisk.c
	cd ide ; $(CC) $(CCOPTS) -o ../makedisk.o -c makedisk.c
