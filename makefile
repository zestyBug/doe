all: main

TEST_INCLUDE=-Iexternal -Iinclude
GCPP=g++

test1:
	$(GCPP) -I. -I"../external" -I"../include" -g -o bin/ecs.exe test/ecs.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wextra -Wfatal-errors -Wshadow