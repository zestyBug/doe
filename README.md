# Data Oriented Engine

The whole idea is to bring the performance of the DOD and the readability of other languages to C++.  
##### Similarities specific to Unity DOTS:
+ using archetype and cache friendly chunks.
+ using component version-ing
+ using hashmap for type to archetype access
+ using entity command buffer

#### DISCLAIMER
This project is an independent implementation that draws inspiration from certain architectural patterns and algorithms found in open-source projects, including the Unity DOTS and the ENTT library.
However, similarities in general functionality may arise due to the common principles of **entity-component-system (ECS)** architectures.
> #### USER RESPONSIBILITY
> I do not possess formal legal knowledge, and I make no warranties or representations regarding any potential patent rights or intellectual property claims that Unity or other third parties may hold. By using, modifying, or distributing this project, you assume all responsibility for ensuring compliance with applicable laws, regulations, and third-party rights, including potential patents.

### TODO
- entity iteration, component query, Version-ing
- system, job and parallel iterators
- EntityCommandBuffer
- Garbage collection and recource managment
- clean code and fix comment/doc doxygen tags: @brief,@tparam, @param, @return, @exception, @deprecated, @note, @attention, and @pre
- rewrite any similarities + more optimization
- write tests