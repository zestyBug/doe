# Data Oriented Engine
A reimplementation of Unity's entity module (not an exact copy) in C++ plus some improvements as a **hoby project**. It is for **educational purpose** only. Not intended for production use.

### Build
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Why?
The philosophy of this project is to create a minimalist yet powerful framework. Unlike many engines, the main focus is to provide the foundation of an engine.

### Milestone
The current primary goal create a simple, complete, hello world example rendering a 3D model.
Performance optimization is next.
Linux receives OS related updates first.

#### Contributions and questions are welcome.
All issues, bug fixes and improvement will be reviewed.
Comments serve as the documentation as of right now. Report any ambiguity in the project to update the comments.


### TODO
- [ ] Android
- [ ] headless mode
- [x] garbage collctor and resource manager fully implemented
- [ ] replace c runtime libraries with platform dependant functions (printf, strXXX, memXXX, malloc/free, STD containers)
- [ ] batched chunk allocator
- [ ] 32 bit systems compatibility
- [ ] a better README and documentation
- [ ] changing pointers to references as possible (resource owener)
- [ ] better encapsulation
- [ ] write some tests
- [x] assets manager
- [x] vulkan imgui

### What is not implemented? (dont ask for it)
- Enable bits, and enableable components
- Aspect
- Lookup cache
- Baker
- SystemAPI, thread safty
- Journaling
- Serialization
- stable hash, memory order
- Cleanup and meta archetype
- Buffer, cleanup, managed and chunk components
- EntityCommandBuffer