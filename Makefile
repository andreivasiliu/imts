# Generic Makefile.

# Export BUILDTYPE as: nothing, or "dist"

# Files to be built.
SO_FILES = imperian.so i_mapper.so i_offense.so mmchat.so voter.so
BIN_FILES = bot

# Target Operating System: Linux, Darwin, or SunOS.
OS = $(shell uname)

# Header and Library directories.
INC = -I./deps/$(OS)/ -I.
LIB = -L./deps/$(OS)/ -L.

# Defaults.
# C_FLAGS go to gcc. B_FLAGS for binaries, M_FLAGS for libraries/modules.
CC      = gcc
C_FLAGS = $(INC) $(LIB)
B_FLAGS = 
M_FLAGS = 

ifeq ($(BUILDTYPE),dist)
  C_FLAGS += -O3
  B_FLAGS += -static
  M_FLAGS += -static
else
  C_FLAGS += -ggdb
endif

# System-specific rules.

# Linux
ifeq ($(OS),Linux)
  C_FLAGS  += -Wall
  M_FLAGS  += -shared
endif

# Macintosh
ifeq ($(OS),Darwin)
  INCS     += -I/sw/include
  LIBS     += -L/sw/lib
  M_FLAGS  += -dynamiclib -Xlinker -single_module
  SO_FILES = imperian.so i_mapper.so mmchat.so voter.so
endif

# Solaris
ifeq ($(OS),SunOS)
  B_FLAGS  += -lsocket -lnsl -lresolv
  C_FLAGS  += 
  M_FLAGS  += -shared
  SO_FILES = imperian.so i_mapper.so i_offense.so mmchat.so voter.so
endif

SRC     = *.c *.h

LIBS_bot = -lz -ldl
LIBS_i_offense.so = -lpcre

all: $(BIN_FILES) $(SO_FILES)

# Generic rules...

$(BIN_FILES): main.c header.h
	@echo -e "\33[1;30mbin \33[1;37m'\33[1;36m$@\33[1;37m'\33[0m"
	$(CC) $(B_FLAGS) $(C_FLAGS) -o $@ $< $(LIBS_$@)

%.so:	%.c header.h module.h
	@echo -e "\33[1;30mmod \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	$(CC) $(M_FLAGS) $(C_FLAGS) -o $@ $< $(LIBS_$@)

clean:
	@echo -e "\33[1;30mCleaning up.\33[0m"
	@rm -f *.o *.so $(BIN_FILES)

# Exceptions to these rules...

i_mapper.so: i_mapper.h
imperian.so: imperian.h

#i_offense.so : i_offense.c header.h module.h
#	@echo -e "\33[1;30mmod \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
#	@$(CC) $(M_FLAGS) -o $@ $< $(C_FLAGS) -lpcre

