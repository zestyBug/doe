all: main

TEST_INCLUDE=-Iexternal -Iinclude
GCPP=g++

test1:
	$(GCPP) -I. -I"../external" -I"../include" -g -o bin/ecs.exe test/ecs.cpp $(TEST_INCLUDE) -g0 -O3 -s -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
	
test2:
	$(GCPP) -I. -I"../external" -I"../include" -g -o bin/res.exe test/res.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wconversion -Wextra -Wfatal-errors