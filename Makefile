CFLAGS ?= -std=c11 -O3
LDFLAGS ?=

CFLAGS_LIB ?= -fPIC
LDFLAGS_LIB ?= -shared -fPIC -ldl
LIBS_LIB ?= -ldl

SRCNAME = onetask.c
OBJNAME = onetask.o
LIBNAME = onetask.so

ALL_C_TESTS_S = $(wildcard tests/*.c)
ALL_C_TESTS_O = $(patsubst %.c,%.o,$(ALL_C_TESTS_S))
ALL_C_TESTS_X = $(patsubst %.o,%.bin,$(ALL_C_TESTS_O))
ALL_C_TESTS_R = $(patsubst %.bin,%.run,$(ALL_C_TESTS_X))
ALL_CXX_TESTS_S = $(wildcard tests/*.cpp)
ALL_CXX_TESTS_O = $(patsubst %.cpp,%.o,$(ALL_CXX_TESTS_S))
ALL_CXX_TESTS_X = $(patsubst %.o,%.bin,$(ALL_CXX_TESTS_O))
ALL_CXX_TESTS_R = $(patsubst %.bin,%.run,$(ALL_CXX_TESTS_X))

ALL_TESTS_R = $(ALL_C_TESTS_R) $(ALL_CXX_TESTS_R)

all: $(LIBNAME)

clean:
	$(RM) onetask.o onetask.so $(ALL_C_TESTS_O) $(ALL_C_TESTS_X)

$(OBJNAME): $(SRCNAME)
	$(CC) $(CFLAGS) $(CFLAGS_LIB) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

$(LIBNAME): $(OBJNAME)
	$(CC) $(LDFLAGS) $(LDFLAGS_LIB) $(TARGET_ARCH) -o $@ $< $(LIBS_LIB)

%.bin: %.o
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $<

%.run: %.bin $(LIBNAME)
	env LD_PRELOAD=./$(LIBNAME) SHELL=./all-is-shell $< | grep "All is shell"

test: $(ALL_TESTS_R)
	@echo
	@echo All tests passed
	@echo "YOU'RE WINNER!"
	@echo
