CFLAGS ?= -std=c11 -O3
LDFLAGS ?= -ldl

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
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -fPIC -c -o $@ $<

$(LIBNAME): $(OBJNAME)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) -shared -fPIC -o $@ $<

%.bin: %.o
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $<

%.run: %.bin $(LIBNAME)
	env LD_PRELOAD=./$(LIBNAME) SHELL=./all-is-shell $< | grep "All is shell"

test: $(ALL_TESTS_R)
	@echo
	@echo All tests passed
	@echo "YOU'RE WINNER!"
	@echo
