# Generic Makefile.

NAME	= bot
CC      = gcc
DEBUG   = -ggdb
O_FLAGS = #-O3
C_FLAGS = $(O_FLAGS) $(DEBUG) $(NOCRYPT) -Wall
L_FLAGS = $(O_FLAGS) $(DEBUG) -ldl -lz

SO_FILES = imperian.so i_mapper.so i_offense.so mmchat.so voter.so

SRC     = *.c *.h

all: $(NAME) $(SO_FILES)

$(NAME): main.c header.h
	@echo -e "\33[1;30mld \33[1;37m'\33[1;36m$(NAME)\33[1;37m'\33[0m"
	@$(CC) $(L_FLAGS) -o $(NAME) main.c $(C_FLAGS)

imperian.so : imperian.c imperian.h header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) -shared -o imperian.so imperian.c $(C_FLAGS)

i_mapper.so : i_mapper.c i_mapper.h header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) -shared -o i_mapper.so i_mapper.c $(C_FLAGS)

i_offense.so : i_offense.c header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) -shared -o i_offense.so i_offense.c $(C_FLAGS) -lpcre

mmchat.so : mmchat.c header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) -shared -o mmchat.so mmchat.c $(C_FLAGS)

voter.so : voter.c header.h module.h
	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
	@$(CC) -shared -o voter.so voter.c $(C_FLAGS)

#$(NAME): $(O_FILES)
#	@echo -e "\33[1;30mld \33[1;37m'\33[1;36m$(NAME)\33[1;37m'\33[0m"
#	@rm -f $(NAME)
#	@$(CC) $(L_FLAGS) -o $(NAME) $(O_FILES)

#.c.o: header.h module.h imperian.h i_mapper.h
#	@echo -e "\33[1;30mgcc \33[1;37m'\33[1;36m$<\33[1;37m'\33[0m"
#	@$(CC) -c $(C_FLAGS) $<

clean:
	@echo -e "\33[1;30mCleaning up.\33[0m"
	@rm -f *.o *.so $(NAME) $(R_NAME)
