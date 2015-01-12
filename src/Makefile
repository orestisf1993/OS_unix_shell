TARGET = shell
all:
	gcc -Wall -Wextra -pedantic -o $(TARGET) builtins.c main.c -lreadline
debug:
	gcc -g -Wall -Wextra -pedantic -o $(TARGET) builtins.c main.c -lreadline
drun:
	valgrind --tool=memcheck --leak-check=yes ./$(TARGET)
drun_f:
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --show-reachable=no --log-file="valgrind_log.log" --track-origins=yes ./$(TARGET)
