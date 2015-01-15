TARGET_DIR = ../bin
TARGET = shell
ifdef dbg
DEBUG = -g 
endif
all:
	if [ ! -d "../bin" ]; then mkdir $(TARGET_DIR); fi
	gcc $(DEBUG)-Wall -Wextra -pedantic -o $(TARGET_DIR)/$(TARGET) builtins.c main.c -lreadline
drun:
	gcc -g -Wall -Wextra -pedantic -o $(TARGET_DIR)/$(TARGET) builtins.c main.c -lreadline
	valgrind --tool=memcheck --leak-check=yes $(TARGET_DIR)/$(TARGET)
drun_f:
	gcc -g -Wall -Wextra -pedantic -o $(TARGET_DIR)/$(TARGET) builtins.c main.c -lreadline
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --show-reachable=no --log-file="valgrind_log.log" --track-origins=yes ./$(TARGET)
