
LIB=./lib
INCLUDE=./adjacency_list
SRC=./adjacency_list
OBJ=./obj

#Compiladores
CXX=g++
CC=gcc

FLAGS=  -O3 #-Wall

libmaxflow: $(LIB)/libmaxflow.a
	echo "libmaxflow.a built..."


$(LIB)/libmaxflow.a: \
$(OBJ)/graph.o \
$(OBJ)/maxflow.o

	ar csr $(LIB)/libmaxflow.a \
$(OBJ)/graph.o \
$(OBJ)/maxflow.o


$(OBJ)/graph.o: $(SRC)/graph.cpp \
$(INCLUDE)/block.h \
$(INCLUDE)/graph.h
	$(CXX) $(FLAGS) -c $(SRC)/graph.cpp -I$(INCLUDE) \
	-o $(OBJ)/graph.o 

$(OBJ)/maxflow.o: $(SRC)/maxflow.cpp \
$(INCLUDE)/block.h \
$(INCLUDE)/graph.h
	$(CXX) $(FLAGS) -c $(SRC)/maxflow.cpp -I$(INCLUDE) \
	-o $(OBJ)/maxflow.o

clean: 
	rm $(LIB)/lib*.a; rm $(OBJ)/*.o; rm *~


