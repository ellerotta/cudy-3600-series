

BIN = cmplzma
OBJS = cmplzma.o

$(BIN) : $(OBJS)
	g++ -o $(BIN) $(OBJS) -lm

%.o:%.c
	g++ $(CFLAGS) -c $< -o %@

clean:
	rm $(OBJS) $(BIN)

