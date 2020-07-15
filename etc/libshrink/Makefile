GPERFTOOLSDIR  := ../../gperftools-metalloc
INCLUDES       := -I. -I$(GPERFTOOLSDIR)/src
CFLAGS         := -fpic -Wall -Wextra -g
LDFLAGS        := -shared -Wl,-rpath,$(shell python2 -c 'print 256 * "x"')
LDLIBS         := -ldl
OBJDIR         ?= ./

ADDRSPACE_BITS ?= 32

CFLAGS += -DADDRSPACE_BITS=$(ADDRSPACE_BITS)
ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g3
	LDFLAGS += -O0 -g3
else
	CFLAGS += -O2
	LDFLAGS += -O2
endif
ifeq ($(SHRINKADDRSPACE_DEBUG),1)
	CFLAGS += -DDEBUG
endif

.PHONY: all clean

all: $(OBJDIR)/libshrink-preload.so $(OBJDIR)/libshrink-static.a

$(OBJDIR)/libshrink-preload.so: $(OBJDIR)/libpreload.o $(OBJDIR)/shrink.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/libshrink-static.a: $(OBJDIR)/libstatic.o $(OBJDIR)/shrink.o
	ar rcs $@ $^

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) -c $(INCLUDES) $(CFLAGS) -o $@ $<

$(OBJDIR)/shrink.o: shrink.h

$(OBJDIR):
	mkdir -p $@

clean:
	rm *.o *.so *.a
