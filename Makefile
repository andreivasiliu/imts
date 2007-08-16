# Generic Makefile.

# Export BUILDTYPE as: nothing, or "dist"

# Files to be built.
SO_FILES = imperian.so i_mapper.so i_script.so i_lua.so
BIN_FILES = bot
DIST_FILES = ChangeLog characters COPYING data IMap mhelp options *.is

# Target Operating System: Linux, Darwin, or SunOS.
OS = $(shell uname)

# Header and Library directories.
INC = -I./deps/$(OS)/ -I. -I/usr/include/lua5.1
LIB = -L./deps/$(OS)/ -L.

# Defaults.
# C_FLAGS go to gcc. B_FLAGS for binaries, M_FLAGS for libraries/modules.
CC      = gcc
C_FLAGS = $(INC) $(LIB)
B_FLAGS = 
M_FLAGS = 

ifeq ($(BUILDTYPE),dist)
  C_FLAGS += -O3 -Wall
else
  C_FLAGS += -ggdb -Wall
endif

# System-specific rules.

# Linux
ifeq ($(OS),Linux)
  M_FLAGS  += -shared -fPIC
endif

# Macintosh
ifeq ($(OS),Darwin)
  INCS     += -I/sw/include
  LIBS     += -L/sw/lib
  M_FLAGS  += -dynamiclib -Xlinker -single_module
endif

# Solaris
ifeq ($(OS),SunOS)
  B_FLAGS  += -lsocket -lnsl -lresolv
  C_FLAGS  += 
  M_FLAGS  += -shared
endif

SRC     = *.c *.h

# External library dependences.
ifeq ($(BUILDTYPE),dist)
LIBS_i_script.so = deps/$(OS)/libpcre.a
LIBS_i_lua.so = deps/$(OS)/liblua.a
LIBS_bot = deps/$(OS)/libz.a -ldl
else
LIBS_i_script.so = -lpcre
LIBS_i_lua.so = -llua5.1 -lm
LIBS_bot = -lz -ldl
endif

all: $(BIN_FILES) $(SO_FILES)
ifeq ($(BUILDTYPE),dist)
	@echo -e "\e[1;30mBuildtype was set as 'dist'.\e[0m"
endif

# Generic rules...

$(BIN_FILES): main.c header.h
	@echo -e "\e[1;30mbin \e[1;37m'\e[1;36m$@\e[1;37m'\e[0m"
	@$(CC) $(B_FLAGS) $(C_FLAGS) -o $@ $< $(LIBS_$@)
ifeq ($(BUILDTYPE),dist)
	@strip $@
endif

%.so:	%.c header.h module.h
	@echo -e "\e[1;30mmod \e[1;37m'\e[1;36m$<\e[1;37m'\e[0m"
	@$(CC) $(M_FLAGS) $(C_FLAGS) -o $@ $< $(LIBS_$@)
ifeq ($(BUILDTYPE),dist)
	@strip $@
endif

gz: clean all
	@echo -e "\e[1;30mPreparing package.\e[0m"
	@rm -rf ./imts
	@mkdir imts
	@mv $(BIN_FILES) $(SO_FILES) imts
	@cp $(DIST_FILES) imts
	@tar -zcf imts.tgz imts

clean:
	@echo -e "\e[1;30mCleaning up.\e[0m"
	@rm -f *.o *.so $(BIN_FILES)

# Exceptions to these rules...

i_mapper.so: i_mapper.h
imperian.so: imperian.h

