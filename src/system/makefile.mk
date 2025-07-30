$(OBJ)/$(systemDir)/%.o: $(systemDir)/%.cpp $(headerGLFW) $(headerInclude) $(headerCutil)
	mkdir -p $(@D)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

OBJ_system= $(OBJ)/$(systemDir)/example.o