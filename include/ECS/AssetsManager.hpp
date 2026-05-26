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
    private:
        static constexpr uint32_t RequestPageSize = 2;
        static constexpr uint32_t MaximumOpenFiles = 2;
        struct EntityInfo {
            uint32_t offset;
            uint32_t size;
        };
    public:
        typedef void (CBSignature)(void *,align_ptr<uint8_t[]>, uint32_t size);
        struct RequestInfo {
            CBSignature *cb = nullptr;
            void *ctx = nullptr;
            Hash32 bundle = 0;
            Hash32 entry = 0;
        };
    private:
        struct RequestWork;
        struct Bundle {
            uint32_t fileSize;
            uint32_t cdOffset;
            uint32_t blkSize;
            uint32_t entriesCount;
            uint32_t mapSize;
            Hash32  *hashes;
            EntityInfo *entities;
            char    *path;
            void insertEntity(string_view name, const EntityInfo&);
            int32_t findEntity(Hash32 entry);
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
    public:
        AssetsManager();
        ~AssetsManager();
        void open(const RequestInfo&) noexcept;
        void open(string_view budle, string_view entry, void* ctx,CBSignature *cb) noexcept;
        void indexBundle(string_view filename);
    private:
        void onCleanup(RequestWork *ref);
        void post(RequestWork &,const RequestInfo&);
        static void initial_work(uv__work* w);
        static void cb_failed(uv__work* w, int err);
        static void initial_after_work(uv__work* w, int err);
        static void after_read(uv_fs_s* fs);
        static void close_cb(uv_fs_s* fs);
    };
}

#endif
