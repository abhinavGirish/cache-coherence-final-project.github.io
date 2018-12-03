PROJECT=sim
CXX=g++
OBJECTS=main.o simulator.o task_manager.o cache.o countdown.o bus.o mesi_memory.o mig_memory.o mesi_manager.o migratory_manager.o request_table.o mesi_consistency_checker.o mig_consistency_checker.o crossbar.o
CXXFLAGS=-I/usr/local/Cellar/boost/1.62.0/include -Wall -Wextra -g -std=c++11 #-O3
LIBS=-L../contech/common/taskLib -lTask -L/usr/local/opt/zlib/lib -lz -L/usr/local/Cellar/boost/1.62.0/lib/ -lboost_program_options


$(PROJECT): $(OBJECTS)
	$(CXX) -o $(PROJECT) $(OBJECTS) $(CXXFLAGS) $(INCLUDE) $(LIBS)


.PHONY: clean
clean:
	rm $(OBJECTS) $(PROJECT)


