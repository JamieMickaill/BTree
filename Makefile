CC=gcc
CFLAGS=-O0 -Werror=vla -std=gnu11 -g -fsanitize=address -pthread -lrt -lm
PERFFLAGS=-O0 -march=native -Werror=vla -std=gnu11 -pthread -lrt -lm
TESTFLAGS=-O0 -Werror=vla -std=gnu11 -g -fprofile-arcs -ftest-coverage -fsanitize=address -pthread -lrt -lm
NAME=btreestore
OBJECT=lib$(NAME).o
LIBRARY=lib$(NAME).a

correctness: btreestore.c
	$(CC) -c $(CFLAGS) $^ -o $(OBJECT) -lpthread
	ar rcs $(LIBRARY) $(OBJECT)

performance: btreestore.c
	$(CC) -c $(PERFFLAGS) $^ -o $(OBJECT)
	ar rcs $(LIBRARY) $(OBJECT)

tests: btreestore.c 
	$(CC) -c $(TESTFLAGS) $^ -o $(OBJECT)
	ar rcs $(LIBRARY) $(OBJECT)
	$(CC) libbtreestore.o -o test tests.c -L "." -lbtreestore $(TESTFLAGS) -lpthread
	bash test.sh
	gcov libbtreestore.o

clean:
	rm -f *.o *.gcno *.gcda *.gcov *.a
	rm test