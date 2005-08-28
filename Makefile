# Generic Makefile.

NAME	= bot
CC      = gcc
DEBUG   = -ggdb
O_FLAGS = #-O3
C_FLAGS = $(O_FLAGS) $(DEBUG)
L_FLAGS = $(O_FLAGS) $(DEBUG) -ldl -lz

# Linux
ifeq ($(shell uname),Linux)
  C_FLAGS  += -Wall
  L_FLAGS  +=
  M_FLAGS  = -shared
  SO_FILES = imperian.so i_mapper.so i_offense.so mmchat.so voter.so
endif

# Macintosh
ifeq ($(shell uname),Darwin)
  C_FLAGS  += -I/sw/include
  L_FLAGS  += -L/sw/lib
  M_FLAGS  = -dynamiclib -Xlinker -single_module
  SO_FILES = imperian.so i_mapper.so mmchat.so voter.so
endif

# Solaris
ifeq ($(shell uname),SunOS)
  L_FLAGS  += -lsocket
endif

SRC     = *.c *.h

all: $(NAME) $(SO_FILES)

$(NAME): main.c header.h
	@echo -e "\33[1;30mld \33[1;37m'\33[1;36m$(NAME)\33[1;37m'\33[0m"
	@$(CC) $(L_FLAGS) -o $(NAME) main.c $(C_FLAGS)

%.so:	%.c header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) $(M_FLAGS) -o $@ $< $(C_FLAGS)

i_mapper.so: i_mapper.h
imperian.so: imperian.h

i_offense.so : i_offense.c header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) $(M_FLAGS) -o $@ $< $(C_FLAGS) -lpcre

clean:
	@echo -e "\33[1;30mCleaning up.\33[0m"
	@rm -f *.o *.so $(NAME)
