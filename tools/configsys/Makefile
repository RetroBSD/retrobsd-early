
BIN  = config
SRCS = main.cpp config.cpp mapping.cpp device.cpp cluster.cpp util.cpp core.cpp gstore.cpp
OBJS = main.o config.o mapping.o device.o cluster.o util.o core.o gstore.o
CXXFLAGS = -ggdb3 

all: $(BIN)

$(BIN): makedep $(OBJS)
	g++ -o $@ $(OBJS) $(CXXFLAGS)

clean:
	-rm -f $(OBJS) $(BIN)

.PHONY:makedep
makedep: 
	mkdep $(SRCS)

include .depend
