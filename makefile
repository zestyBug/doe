all: main

TEST_INCLUDE=-Iexternal -Iinclude
GCPP=g++


test0:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/ecs test/ecs.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow

test2:
	$(GCPP) -I. -I"external" -I"include" -I"src" -o bin/ecs test/ecs.cpp $(TEST_INCLUDE) -s -g0 -O3 -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow	

test1:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/ecs test/ecs.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow -fsanitize=address -static-libasan
	
test3:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/res test/res.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wconversion -Wextra -Wfatal-errors
		
test4:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/queuequeue test/queuequeue.cpp $(TEST_INCLUDE) -g3 -Og -Wall -Wconversion -Wextra -Wfatal-errors

test5:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/bitset test/bitset_test.cpp $(TEST_INCLUDE) -g0 -O3 -s -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
	
test6:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/archetype test/archetype.cpp $(TEST_INCLUDE) -g0 -O3 -s -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow

test7:
	$(GCPP) -I. -I"external" -I"include" -I"src" -g -o bin/resource test/resource.cpp $(TEST_INCLUDE) -g0 -O3 -s -Wall -Wconversion -Wextra -Wfatal-errors -Wshadow
	