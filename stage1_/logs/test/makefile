CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I..   # 按实际位置改 -I
TARGET := test
SRC := test_async.cpp
DEPS := ../*.hpp

$(TARGET): $(SRC) $(DEPS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

.PHONY: clean
clean:
	rm -f $(TARGET)
