diff: diff.o hash.o packrat.o uforth.o vartable.o nlisp.o pattern.o builder.o
	$(CC) -o $@ $^ -lm

%.o: %.c %.h
	$(CC) -c -g -o $@ $<
