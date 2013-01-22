CFLAGS ?= -std=c11 -O3
LDFLAGS ?= -ldl

SRCNAME = onetask.c
OBJNAME = onetask.o
LIBNAME = onetask.so

ALLTESTS_S = $(wildcard tests/*.c)
ALLTESTS_O = $(patsubst %.c,%.o,$(ALLTESTS_S))
ALLTESTS_X = $(patsubst %.o,%.bin,$(ALLTESTS_O))
ALLTESTS_R = $(patsubst %.bin,%.run,$(ALLTESTS_X))

all: $(LIBNAME)

clean:
	$(RM) onetask.o onetask.so $(ALLTESTS_O) $(ALLTESTS_X)

$(OBJNAME): CFLAGS += -fPIC
$(OBJNAME): $(SRCNAME)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

$(LIBNAME): LDFLAGS += -shared -fPIC
$(LIBNAME): $(OBJNAME)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) -o $@ $<

%.bin: %.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) -o $@ $<

%.run: %.bin $(LIBNAME)
	env LD_PRELOAD=./$(LIBNAME) SHELL=./all-is-shell $< | grep "All is shell"

test: $(ALLTESTS_R)
