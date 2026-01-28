# PROGRAM CONFIG
BUILD_DIR    := build/backend
SRC_DIR      := src
INCLUDE_DIRS := include/common include/backend
LOG_DIR      := log/backend
EXECUTABLE   := backend.out

-include $(SRC_DIR)/backend.src
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o, $(SOURCES))
DEPS := $(patsubst %.o,%.d,$(OBJS))

# LIBRARIES
LIBCUTILS_INCLUDE_DIR  := ../cutils/include
LIBCUTILS              := -L../cutils/build/ -lcutils

LIBS := $(LIBCUTILS) 

#INCLUDE
INCLUDE_DIRS_ALL = $(INCLUDE_DIRS) $(LIBCUTILS_INCLUDE_DIR)

# COMPILER CONFIG
CC := g++

CPPFLAGS_DEBUG := -D _DEBUG -ggdb3 -O0 -g

CPPFLAGS_RELEASE := -O2 -march=native

CPPFLAGS_ASAN := -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -pie -fPIE -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

ifeq "$(TARGET)" "Release"
CPPFLAGS_TARGET := $(CPPFLAGS_RELEASE)
else
CPPFLAGS_TARGET := $(CPPFLAGS_DEBUG) $(CPPFLAGS_ASAN)
endif

CPPFLAGS_WARNINGS := -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -Werror=vla -Wstack-usage=8192

CPPFLAGS_DEFINES = -DLOG_DIR='"log"' -DIMG_DIR='"img"'

CPPFLAGS := -MMD -MP -std=c++17 $(addprefix -I,$(INCLUDE_DIRS_ALL)) $(CPPFLAGS_WARNINGS) $(CPPFLAGS_DEFINES) $(CPPFLAGS_TARGET)

# PROGRAM
$(BUILD_DIR)/$(EXECUTABLE): $(OBJS)
	@echo -n Linking $@...
	@$(CC) $(CPPFLAGS) -o $@ $(OBJS) $(LIBS)
	@echo done

$(OBJS): $(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo Building $@...
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) -c -o $@ $< $(LIBS)

.PHONY: run
run: LOG ?= log-backend.html
run: IN ?= build/out.ast
run: OUT ?= build/prog.asm
run: $(BUILD_DIR)/$(EXECUTABLE)
	./$< --log=$(LOG) --in=$(IN) --out=$(OUT)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(LOG_DIR)

-include $(DEPS)
