CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2

TARGET = simplify
LAKE_TARGET = makelake

SRCS = main.cpp geometry.cpp polygon.cpp io.cpp simplifier.cpp
OBJS = $(SRCS:.cpp=.o)

LAKE_SRC = generate_lake.cpp
LAKE_OBJ = $(LAKE_SRC:.cpp=.o)

.PHONY: all clean test lake

all: $(TARGET)

# --- main program ---
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# --- lake generator ---
$(LAKE_TARGET): $(LAKE_OBJ)
	$(CXX) $(CXXFLAGS) -o $(LAKE_TARGET) $(LAKE_OBJ)

# --- generic rule ---
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- cleanup ---
clean:
	rm -f $(OBJS) $(LAKE_OBJ) $(TARGET) $(LAKE_TARGET)