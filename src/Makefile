
NAME=phnsw
CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

all: lib$(NAME).so install

lib$(NAME).so: $(NAME).cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

install:
	sst-register $(NAME) $(NAME)_LIBDIR=$(CURDIR)
	sst-register SST_ELEMENT_SOURCE $(NAME)=$(CURDIR)
	sst-register SST_ELEMENT_TESTS  $(NAME)=$(CURDIR)/../tests

clean:
	rm -f *.o lib$(NAME).so
