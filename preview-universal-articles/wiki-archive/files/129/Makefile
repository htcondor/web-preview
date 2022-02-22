all: weak non_weak non_weak1 non_weak2 non_weak3 non_weak4 non_weak5 non_weak6 non_weak7 non_weak8
	for e in $^; do echo -n "$$e: "; ./$$e; done

func.h: Makefile
	echo "void func();" > func.h

main.cpp: func.h
	echo -e '#include "func.h"\nint main(int argc, char ** argv) { func(); return 0; }' > main.cpp

weak.cpp: func.h
	echo -e '#include "func.h"\n#include <iostream>\nvoid __attribute__ ((weak)) func() { std::cout << "Weak func()!" << std::endl; }' > weak.cpp

non_weak.cpp: func.h
	echo -e '#include "func.h"\n#include <iostream>\nvoid func() { std::cout << "Non-weak func()!" << std::endl; }' > non_weak.cpp

clean:
	rm -f $(PROGS)
	rm -f *.o *.a
	rm -f func.h main.cpp weak.cpp non_weak.cpp

non_weak.a: non_weak.o
	ar vrs $@ $^

main.a: weak.o main.o
	ar vrs $@ $^

weak: weak.o main.o
	g++ $^ -o $@

non_weak: non_weak.o main.o
	g++ $^ -o $@

non_weak1: weak.o non_weak.o main.o
	g++ $^ -o $@

non_weak2: non_weak.o weak.o main.o
	g++ $^ -o $@

non_weak3: non_weak.a weak.o main.o
	g++ $^ -o $@

non_weak4: weak.o non_weak.a main.o
	g++ $^ -o $@

non_weak5: non_weak.o main.a
	g++ $^ -o $@

non_weak6: main.a non_weak.o
	g++ $^ -o $@

non_weak7: main.a non_weak.a
	g++ $^ -o $@

non_weak8: non_weak.a main.a
	g++ $^ -o $@
