# Data Oriented Engine
A rewriting of unity's entity module in C++ plus some improvements as a hoby project. It is for **educational purpose** only. Not to be used in any production.

#### diffrents:
Its not an exact copy, there are some differents. ChunkIndex is not used, for performance reason. Chunks header has different application. threadpool is implemented with libuv, and other small details.


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


### TODO
- 32 bit systems compatibility
- a better README and documentation
- changing pointers to references as possible (resource owener)
- better encapsulation
- write some tests
- assets manager
- glfw + vulkan + imgui
- remove unnecesary aligned alloctions