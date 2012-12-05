OBJS=mkrd.o
BIN=mkrd

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $< $(LIBS) $(LDFLAGS)

clean:
	@rm -f $(BIN) $(OBJS)
