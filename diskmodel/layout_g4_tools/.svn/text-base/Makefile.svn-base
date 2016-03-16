

include .paths


CFLAGS = -m32 -g $(DISKSIM_CFLAGS) $(DISKMODEL_CFLAGS) $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS) -Wall -Wno-unused

LDFLAGS = -m32 $(DISKSIM_LDFLAGS)  $(MEMSMODEL_LDFLAGS) -L.. -ldiskmodel  $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS) -lm   -L../../ssdmodel/lib/ -lssdmodel

TARGETS = g4_skews
all: $(TARGETS)

%: %.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lpthread

%: %.c


clean:
	rm -f *.o $(TARGETS)

distclean: clean
	rm -f *~
