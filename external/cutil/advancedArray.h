#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H
#include <new>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <exception.hpp>

#define ARRAY_DEBUG 1

#define invalid_index -1
#ifdef ARRAY_DEBUG
#include <stdio.h>
#define ADEBUG(...) printf(__VA_ARGS__)
#else
#define ADEBUG(...) ;
#endif // ARRAY_DEBUG


template<typename T>
class advancedArray final {
public:
    typedef int32_t key;    
    const static uint64_t max_size = INT32_MAX;
    struct blockStructure
    {
        bool * served;
        T * value;
        uint16_t blockSize;
        key indexStart;
        key indexEnd;
    };
    blockStructure * blockArray;
    uint16_t blockCount;
    uint16_t firstBlockSize,extendedBlockSize;

    advancedArray(uint16_t fbs,uint16_t ebs):firstBlockSize(fbs),extendedBlockSize(ebs)
    {
        ADEBUG("initializing new advancedQueue\n");
        blockCount = 1;
        blockArray = (struct blockStructure *) calloc(1, sizeof(blockStructure));
        blockArray[0].served = (bool*) calloc(fbs , sizeof(bool));
        blockArray[0].value = (T*) calloc(fbs , sizeof(T));
        blockArray[0].blockSize = fbs;
        blockArray[0].indexStart = 0;
        blockArray[0].indexEnd = (fbs-1);

    }
    virtual ~advancedArray()
    {
        ADEBUG("destroying advancedQueue\n");
        for(uint16_t x = 0; x < blockCount;x++)
        {
            ADEBUG("destroying block: %d\n",x);
            uint16_t bsize = blockArray[x].blockSize;
            T * bData = blockArray[x].value;
            bool * bIsServed = blockArray[x].served;

            for(uint16_t c = 0;c < bsize;c++)
                if(bIsServed[c])
                    bData[c].~T();

            free(bData);
            free(bIsServed);
        }
        free(blockArray);
        ADEBUG("advancedQueue is Destructed\n");
    }

    T* get(key index)
    {
        ADEBUG("Looking for a data index, inside the blocks : ");
        for(uint16_t x = 0; x < blockCount;x++)
        {
            if(blockArray[x].indexEnd >= index && index >= blockArray[x].indexStart)
            {
                if(blockArray[x].served[ index - blockArray[x].indexStart ] == 1)
                {
                    ADEBUG("value is returned \n");
                    return blockArray[x].value + (index - blockArray[x].indexStart) ;
                }
                else
                {
                    ADEBUG("index is not initialized \n");
                    return NULL;
                }
            }

        }
        ADEBUG("The index is not found\n");
        return NULL;
    }

    template<typename ... Arg>
    key emplace_back(Arg &&... Args)
    {
        ADEBUG("Emplace new data to DynamicArray\n");
        for(uint16_t x = 0; x < blockCount;x++)
            for(uint16_t c = 0;c < blockArray[x].blockSize;c++)
                if(blockArray[x].served[c] == 0)
                {
                    blockArray[x].served[c] = 1;
                    new ((blockArray[x].value + c)) T(Args...);
                    return blockArray[x].indexStart + c;
                }
        ADEBUG("Not enough free space in DynamicArray\n");
        extendBlock();
        ADEBUG("Finding free space in extended space\n");
        uint16_t index = blockCount-1;
        for(uint16_t c = 0;c < blockArray[index].blockSize;c++)
            if(blockArray[index].served[c] == 0)
            {
                blockArray[index].served[c] = 1;
                new ((blockArray[index].value + c)) T(Args...);
                return blockArray[index].indexStart + c;
            }
        throw exception("unexpected error");
    }
protected:

private:
    void extendBlock()
    {
        ADEBUG("extendBlock is called: ");
        void * buffer = realloc(blockArray,(blockCount + 1) * sizeof(blockStructure));
        if(buffer == NULL)
            throw exception("allocation failed",result::realloc_error);
        void * bool_buffer = calloc(extendedBlockSize , sizeof(bool));
        void * data_buffer = calloc(extendedBlockSize , sizeof(T));
        if(bool_buffer == NULL || data_buffer == NULL)
            throw exception("allocation failed",result::calloc_error);

        blockArray = (struct blockStructure *) buffer;
        blockArray[blockCount].served = (bool*) bool_buffer;
        blockArray[blockCount].value = (T*) data_buffer;

        blockArray[blockCount].blockSize = extendedBlockSize;
        int64_t startingIndex= blockArray[blockCount-1].indexEnd;
        blockArray[blockCount].indexStart = startingIndex + 1;
        blockArray[blockCount].indexEnd = (startingIndex + firstBlockSize);

        blockCount++;
    }
};

#endif // DYNAMICARRAY_H
