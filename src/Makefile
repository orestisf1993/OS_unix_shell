TARGET = shell
#~ ifneq ($(dbg),$(UNUSED_VARIABLE))
#~ 	DEBUG = -g
#~ endif
ifdef dbg
DEBUG = -g 
endif
all:
	gcc $(DEBUG)-Wall -Wextra -pedantic -o $(TARGET) builtins.c main.c -lreadline
drun:
	gcc -g -Wall -Wextra -pedantic -o $(TARGET) builtins.c main.c -lreadline
	valgrind --tool=memcheck --leak-check=yes ./$(TARGET)
drun_f:
	gcc -g -Wall -Wextra -pedantic -o $(TARGET) builtins.c main.c -lreadline
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --show-reachable=no --log-file="valgrind_log.log" --track-origins=yes ./$(TARGET)
