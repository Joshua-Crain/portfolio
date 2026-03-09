myshell: myshell.o argparse.o builtin.o
	gcc -Wall -Werror -o myshell myshell.o builtin.o argparse.o

myshell.o: myshell.c builtin.h argparse.h
	gcc -c myshell.c 

builtin.o: builtin.c argparse.h
	gcc -c builtin.c

argparse.o: argparse.c 
	gcc -c argparse.c

clean:
	rm -f *.o
