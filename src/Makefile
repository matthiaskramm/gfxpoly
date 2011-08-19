all: $(OBJS)

CC = gcc -DCHECKS -O2 -g

SRC = active.c convert.c poly.c wind.c render.c xrow.c stroke.c moments.c dict.c
OBJS = active.o convert.o poly.o wind.o render.o xrow.o stroke.o moments.o dict.o

active.o: active.c active.h poly.h Makefile
	$(CC) -c active.c -o active.o

convert.o: convert.c convert.h poly.h Makefile
	$(CC) -c convert.c -o convert.o

poly.o: poly.c poly.h active.h heap.h Makefile
	$(CC) -c poly.c -o poly.o

wind.o: wind.c wind.h poly.h Makefile
	$(CC) -c wind.c -o wind.o

dict.o: dict.c dict.h Makefile
	$(CC) -c dict.c -o dict.o

render.o: render.c wind.h poly.h render.h Makefile
	$(CC) -c render.c -o render.o

xrow.o: xrow.c xrow.h Makefile
	$(CC) -c xrow.c -o xrow.o

stroke.o: stroke.c poly.h convert.h wind.h Makefile
	$(CC) -c stroke.c -o stroke.o

moments.o: moments.c moments.h Makefile
	$(CC) -c moments.c -o moments.o

clean: 
	rm -f *.o
