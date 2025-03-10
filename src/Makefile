
NAME=phnsw

PHNSW_SOURCES := $(wildcard *.cc)
PHNSW_HEADERS := $(wildcard *.h instructions/*.h)
PHNSW_HEADERS += $(wildcard Register/*.h)


PHNSW_OBJS := $(patsubst %.cc,%.o,$(wildcard *.cc))

CXX=$(shell sst-config --CXX)
CXXFLAGS=$(shell sst-config --ELEMENT_CXXFLAGS)
LDFLAGS=$(shell sst-config --ELEMENT_LDFLAGS)

all: lib$(NAME).so install

%.o: %.cc $(PHNSW_SOURCES) $(PHNSW_HEADERS)
	echo $(PHNSW_HEADERS)
	$(CXX) $(CXXFLAGS) -c $<

lib$(NAME).so: $(PHNSW_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(PHNSW_OBJS)

install:
	sst-register $(NAME) $(NAME)_LIBDIR=$(CURDIR)
	sst-register SST_ELEMENT_SOURCE $(NAME)=$(CURDIR)
	sst-register SST_ELEMENT_TESTS  $(NAME)=$(CURDIR)/../tests

clean: 
	rm -f *.o lib$(NAME).so
