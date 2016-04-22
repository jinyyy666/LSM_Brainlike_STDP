TARGET = NeuromorphicSim
CXX	= g++ 
CXXFLAGS = -pthread  -std=c++11 -O9


HDRS=	channel.h speech.h neuron.h synapse.h network.h parser.h simulator.h readout.h def.h

SRCS=	channel.C speech.C neuron.C synapse.C network.C parser.C simulator.C readout.h main.C

OBJS=	channel.o speech.o neuron.o synapse.o network.o parser.o simulator.o readout.o main.o


MATOBJS = \

$(TARGET):  $(OBJS)  
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

$(OBJS): $(HDRS)

clean:
	/bin/rm -rf $(OBJS) $(TARGET)
	/bin/rm -rf *~

