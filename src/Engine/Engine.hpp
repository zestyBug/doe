#if !defined(ENGINE_HPP)
#define ENGINE_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include "external/cutil/QueueQueue.hpp"

namespace DOTS
{

class Core{
    std::thread th;
    std::mutex mutex_resource_input;
    std::mutex mutex_resource_output;
    std::condition_variable condition_;
    // QueueQueue
public:
};

} // namespace DOTS


#endif // ENGINE_HPP
