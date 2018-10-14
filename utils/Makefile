CXX = g++
CXXFLAGS = -g
LIBS = -lexpat -lphysfs

TILEDCONVERT_OBJ = tiledconvert.o expatcpp.o
TILEDCONVERT_EXE = tiledconvert

DIALOGPARSER_OBJ = dialogparser.o
DIALOGPARSER_EXE = dialogparser

all: $(TILEDCONVERT_EXE) $(DIALOGPARSER_EXE)

$(TILEDCONVERT_EXE): $(TILEDCONVERT_OBJ)
	$(CXX) -o $(TILEDCONVERT_EXE) $(TILEDCONVERT_OBJ) $(LIBS)

$(DIALOGPARSER_EXE): $(DIALOGPARSER_OBJ)
	$(CXX) -o $(DIALOGPARSER_EXE) $(DIALOGPARSER_OBJ) $(LIBS)

%.o: %.C
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TILEDCONVERT_EXE) $(DIALOGPARSER_EXE)

# dependencies
