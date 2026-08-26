CXX ?= g++
CXXFLAGS = -std=c++17 -O2 $(shell pkg-config --cflags taglib)
LDFLAGS = $(shell pkg-config --libs taglib) -lz

TARGET = tag
SRC = tagger.cpp

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
