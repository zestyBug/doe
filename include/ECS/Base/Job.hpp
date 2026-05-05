#if !defined(JOB_HPP)
#define JOB_HPP

#include "cutil/basics.hpp"

namespace ECS
{
    struct Chunk;
    struct JobHandle {
        friend struct JobsUtility;
        friend struct JobDataChunk;
        // default value is the invalid value
        JobHandle() = default;
        inline operator bool   () const {return this->id>=0;}
        inline bool  operator !() const {return this->id<0;}
        inline bool operator == (const JobHandle& o) const {return this->id == o.id;}
        inline int32_t index  () const {return this->id;}
    private:
        static const uint32_t MaximumCount = INT32_MAX;
        JobHandle(int32_t v):id{v}{}
        inline bool operator <  (const JobHandle& o) const {return this->id <  o.id;}
        inline bool operator >  (const JobHandle& o) const {return this->id >  o.id;}
        inline bool operator <= (const JobHandle& o) const {return this->id <= o.id;}
        inline bool operator >= (const JobHandle& o) const {return this->id >= o.id;}
        int32_t id = -1;
    };
    typedef void(*JobFunctionSignature)(void*,uint32_t,uint32_t);
    struct JobParameter {
        JobFunctionSignature function;
        void *context = nullptr;
        uint32_t batchCount = 1;
        uint32_t batchStepSize = 1;
        JobHandle dependsOn = JobHandle();
    };
}

#endif // JOB_HPP
