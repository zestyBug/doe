#if !defined(ASSETMANAGER_HPP)
#define ASSETMANAGER_HPP

#include "cutil/basics.hpp"
#include "cutil/set.hpp"
#include "cutil/string_view.hpp"
#include <queue>
#include <atomic>

struct uv_loop_s;
struct uv_fs_s;
struct uv__work;
namespace ECS
{
    struct AssetsManager {
        static constexpr uint32_t RequestPageSize = 2;
        static constexpr uint32_t MaximumOpenFiles = 2;
        using Hash = uint32_t;
        typedef void (*CBSignature)(align_ptr<uint8_t[]>, uint32_t size, uint32_t offset);
        struct EntityInfo {
            uint32_t offset;
            uint32_t size;
        };
        struct RequestInfo {
            CBSignature cb = nullptr;
            Hash bundle = 0;
            Hash entry = 0;
        };
        struct RequestWork;
        struct Bundle {
            uint32_t fileSize;
            uint32_t cdOffset;
            uint32_t blkSize;
            uint32_t entriesCount;
            uint32_t mapSize;
            Hash    *hashes;
            EntityInfo *entities;
            char    *path;
        };
        struct RequestPage {
            uint32_t readIndex = 0;
            uint32_t writeIndex = 0;
            RequestInfo pool[RequestPageSize];
            static_assert(RequestPageSize > 0);
        };
        set<Bundle*>             bundles;
        align_ptr<RequestWork[]> works;
        std::queue<RequestPage>  pageQueue;
        RequestWork             *freeWorks;
        uv_loop_s               *loop;
        AssetsManager();
        ~AssetsManager();
        void open(string_view bundle,string_view file, CBSignature onOpen) noexcept;
        void indexBundle(string_view filename);
    private:
        void onCleanup(RequestWork *ref);
        void post(RequestWork &,RequestInfo);
        static void initial_work(uv__work* w);
        static void cb_failed(uv__work* w, int err);
        static void initial_after_work(uv__work* w, int err);
        static void after_read(uv_fs_s* fs);
        static void close_cb(uv_fs_s* fs);
    };
}

#endif
