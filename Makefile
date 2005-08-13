# Generic Makefile.

NAME	= bot
CC      = gcc
DEBUG   = -ggdb
O_FLAGS = #-O3

# Linux
ifeq ($(shell uname),Linux)
  C_FLAGS  = $(O_FLAGS) $(DEBUG) -Wall
  L_FLAGS  = $(O_FLAGS) $(DEBUG) -ldl -lz
  M_FLAGS  = -shared
  SO_FILES = imperian.so i_mapper.so i_offense.so mmchat.so voter.so
endif

# Macintosh
ifeq ($(shell uname),Darwin)
  C_FLAGS  = $(O_FLAGS) $(DEBUG) -I/sw/include
  L_FLAGS  = $(O_FLAGS) $(DEBUG) -ldl -lz -L/sw/lib
  M_FLAGS  = -dynamiclib -Xlinker -single_module
  SO_FILES = imperian.so i_mapper.so mmchat.so voter.so
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
