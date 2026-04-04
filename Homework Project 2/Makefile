CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2
TARGET = simplify

SRCS = main.cpp geometry.cpp polygon.cpp io.cpp simplifier.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	bash tests/run_tests.sh

clean:
	rm -f $(OBJS) $(TARGET)


# =========================
# DEMO CONFIG
# =========================

TEST_DIR1 = demo
TEST_DIR2 = demo2
TEST_DIR3 = demo3
TARGET_VERTICES ?= 3

# Find all csv test files
TEST_FILES := $(wildcard $(TEST_DIR)/*.csv)

# =========================
# RUN ALL TESTS
# =========================

demo: $(TARGET)
	@cd $(TEST_DIR) && for file in *.csv; do \
		echo "-----------------------------------"; \
		echo "Test: $$file"; \
		../$(TARGET) $$file $(TARGET_VERTICES); \
		echo ""; \
	done
demo2: $(TARGET)
	@cd $(TEST_DIR) && for file in *.csv; do \
		echo "-----------------------------------"; \
		echo "Test: $$file"; \
		../$(TARGET) $$file $(TARGET_VERTICES); \
		echo ""; \
	done
demo3: $(TARGET)
	@cd $(TEST_DIR) && for file in *.csv; do \
		echo "-----------------------------------"; \
		echo "Test: $$file"; \
		../$(TARGET) $$file $(TARGET_VERTICES); \
		echo ""; \
	done