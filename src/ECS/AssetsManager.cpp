#include "ECS/AssetsManager.hpp"
#include "cutil/HashHelper.hpp"
#include "uv.h"
using namespace ECS;


PACK(struct local_file_header {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
});
PACK(struct central_dir_header {
    uint32_t signature;
    uint16_t version;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression_method;
    uint16_t last_mod_file_time;
    uint16_t last_mod_file_date;
    uint32_t crc_32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t file_name_length;
    uint16_t extra_field_length;
    uint16_t file_comment_length;
    uint16_t disk_number_start;
    uint16_t internal_file_attributes;
    uint32_t external_file_attributes;
    uint32_t local_header_offset;
});
PACK(struct end_of_central_dir_record {
    uint32_t signature;
    uint16_t disk_number;
    uint16_t cdr_disk_number;
    uint16_t disk_num_entries;
    uint16_t num_entries;
    uint32_t cdr_size;
    uint32_t cdr_offset;
    uint16_t ZIP_file_comment_length;
});
struct AssetsManager::RequestWork {
    AssetsManager *thiz;
    CBSignature *cb = nullptr;
    void *ctx = nullptr;
    Hash32 bundleHash = 0;
    Hash32 entryHash = 0;

    uint32_t offset;
    uint32_t bufLen;
    uv_file fd;
    align_ptr<uint8_t[]> content;

    RequestWork *_next;
    union{
        uv_fs_t fs;
        uv_work_t work;
    } ___;
};

AssetsManager::~AssetsManager(){
    auto begin = bundles.begin();
    auto end   = bundles.end();
    while(begin!=end){
        if(begin)
            allocator().deallocate(*begin);
        ++begin;
    }
}
AssetsManager::AssetsManager(){
    static_assert(MaximumOpenFiles > 0);
    this->loop = uv_default_loop();
    uint32_t step = alignCacheLineSize(sizeof(RequestWork));
    uint8_t *ptr = allocator().allocate(step * MaximumOpenFiles);
    this->works.reset((RequestWork*)ptr);
    this->freeWorks = (RequestWork*)ptr;
    const RequestWork *end = (RequestWork*)(ptr + MaximumOpenFiles * step);
    RequestWork *begin = (RequestWork*)(ptr);
    while(begin < end){
        RequestWork *buffer = begin;
        begin = (RequestWork*)((uint8_t*)begin + step);
        buffer->content.release();
        buffer->fd = -1;
        buffer->_next = begin;
    }
    begin = (RequestWork*)((uint8_t*)begin - step);
    begin->_next = nullptr;
}
void AssetsManager::post(RequestWork &req,const RequestInfo &info){    
    if(req.bundleHash != info.bundle){
        if(req.fd >=0)
            uv_fs_close(this->loop, &req.___.fs, req.fd, &close_cb);
        req.fd = -1;
    }
    uv_work_t &work = req.___.work;
    work.work_req.loop = this->loop;
    work.work_req.done = &cb_failed;
    work.work_req.work = &initial_work;
    req.cb = info.cb;
    req.ctx = info.ctx;
    req.bundleHash = info.bundle;
    req.entryHash = info.entry;
    req.thiz = this;
    uv_queue_work_quick(&work);
}
void AssetsManager::onCleanup(RequestWork *req) {    
    req->content.reset();
    if(this->pageQueue.empty()){
        req->_next = this->freeWorks;
        this->freeWorks = req;
    }else{
        RequestPage &page = this->pageQueue.front();
        post(*req,page.pool[page.readIndex++]);
        if(page.readIndex == this->RequestPageSize)
            this->pageQueue.pop();
    }
}
#pragma region open
void AssetsManager::open(string_view bundle, string_view entry, void* ctx,CBSignature *cb) noexcept {
    RequestInfo info {
        .cb = cb,
        .ctx = ctx,
        .bundle = HashHelper::FNV1A32(bundle),
        .entry = HashHelper::FNV1A32(entry),
    };
    this->open(info);
}
void AssetsManager::open(const RequestInfo &info) noexcept {
    //Hash32 bundleHash = HashHelper::FNV1A32(bundleName);
    //Hash32 entryHash = HashHelper::FNV1A32(entryName);
    RequestWork *req = this->freeWorks;
    if(req){
        this->freeWorks = req->_next;
        post(*req,info);
    }else{
        RequestPage *page = &this->pageQueue.back();
        if(page->writeIndex == this->RequestPageSize){
            page = &this->pageQueue.emplace();
        }
        page->pool[page->writeIndex++] = info;
    }
}
#pragma endregion open
#pragma region indexBundle
void AssetsManager::indexBundle(string_view filename){
    Hash32 hash = HashHelper::FNV1A32(filename);
    if(bundles.indexOf(hash) >= 0)
        return;
    uv_file fd;
    uv_buf_t buf;
    align_ptr<Bundle> bundle;
    std::unique_ptr<uint8_t[]> bufmem;
    // read central directory
    {
        uv_fs_t req;
        size_t fileSize;
        size_t blkSize;
        end_of_central_dir_record eocdr;

        fd = uv_fs_open(loop, &req, filename.data(), UV_FS_O_RDONLY, 0, NULL);
        if(req.result < 0)
            throw std::runtime_error("indexBundle(): unable to open the file");
        uv_fs_req_cleanup(&req);


        uv_fs_fstat(loop, &req, fd, NULL);
        if(req.result < 0 || !req.ptr)
            throw std::runtime_error("indexBundle(): unable to open the file");
        {
            uv_stat_t &statbuf = *(uv_stat_t*)req.ptr;
            fileSize = statbuf.st_size;
            blkSize = statbuf.st_blksize;
        }
        uv_fs_req_cleanup(&req);
        if(blkSize & (blkSize-1))
            throw std::runtime_error("indexBundle(): unable to open the file");

        if((sizeof(local_file_header) + sizeof(central_dir_header) + sizeof(end_of_central_dir_record)) > fileSize)
            throw std::runtime_error("indexBundle(): unable to open the file");
        if(fileSize > INT32_MAX || blkSize > INT16_MAX)
            throw std::runtime_error("indexBundle(): unable to open the file");
        // atleast sizeof(end_of_central_dir_record) bytes
        if(blkSize < 64)
            blkSize = 64;

        buf.base = (char*)&eocdr;
        buf.len = sizeof(eocdr);
        uv_fs_read(loop,&req,fd,&buf,1,fileSize-sizeof(end_of_central_dir_record),NULL);
        if(req.result != buf.len)
            throw std::runtime_error("indexBundle(): unable to open the file");
        uv_fs_req_cleanup(&req);
        if (eocdr.num_entries == UINT16_MAX ||
            eocdr.num_entries <  1          ||
            eocdr.cdr_offset  == UINT32_MAX ||
            eocdr.cdr_offset  >= fileSize   ||
            eocdr.cdr_size    == UINT32_MAX ||
            eocdr.cdr_size    <  sizeof(central_dir_header) ||
            eocdr.signature   != 0x06054B50 ||
            eocdr.disk_number != 0          ||
            eocdr.cdr_disk_number != 0      ||
            eocdr.disk_num_entries != eocdr.num_entries)
            throw std::runtime_error("indexBundle(): unable to open the file");

        {
            uint32_t size[4];
            uint32_t mapSize = 2;
            while(mapSize < eocdr.num_entries)
                mapSize *= 2;
            size[0] =           alignCacheLineSize(sizeof(Bundle));
            size[1] = size[0] + alignPointerSize  (sizeof(Hash32)       * mapSize);
            size[2] = size[1] + alignPointerSize  (sizeof(EntityInfo) * mapSize);
            size[3] = size[2] + alignPointerSize  ((uint32_t)filename.size()+1);
            bundle.reset((Bundle*)allocator().allocate(size[3]));
            bundle->fileSize  = (uint32_t)fileSize;
            bundle->blkSize  = (uint32_t)blkSize;
            bundle->cdOffset     = eocdr.cdr_offset;
            bundle->entriesCount = eocdr.num_entries;
            bundle->mapSize  = mapSize;
            bundle->hashes   =       (Hash32*)((uint8_t*)bundle.get() + size[0]);
            bundle->entities = (EntityInfo*)((uint8_t*)bundle.get() + size[1]);
            bundle->path     =       (char*)((uint8_t*)bundle.get() + size[2]);
            memset(bundle->hashes, 0, sizeof(Hash32)*mapSize);
            memcpy(bundle->path, filename.data(), filename.size());
            bundle->path[filename.size()] = '\0';
        }

        bufmem = std::make_unique<uint8_t[]>(eocdr.cdr_size);
        buf.base = (char*)bufmem.get();
        buf.len = eocdr.cdr_size;
        uv_fs_read(loop, &req, fd, &buf, 1, eocdr.cdr_offset, NULL);
        if(req.result != buf.len)
            throw std::runtime_error("indexBundle(): unable to open the file");
        uv_fs_req_cleanup(&req);
    }
    // iterate records
    for(uint32_t i=0;true;)
    {
        uint32_t iBuffer = i;
        i += sizeof(central_dir_header);
        if(i > (uint32_t)buf.len)
            break;
        const central_dir_header &header = *(central_dir_header *)(buf.base + iBuffer);
        if (header.signature != 0x02014B50 ||
            header.version_needed > 10 ||
            // header.compressed_size != header.uncompressed_size ||
            header.uncompressed_size == UINT32_MAX ||
            header.compressed_size == UINT32_MAX ||
            header.disk_number_start != 0 ||
            // header.compression_method != 0 ||
            header.file_name_length < 1 ||
            // header.internal_file_attributes != 0 ||
            //(header.external_file_attributes != 16 && header.external_file_attributes != 128) ||
            header.local_header_offset == UINT32_MAX)
            throw std::runtime_error("indexBundle(): unable to open the file");
        i+= header.file_name_length + 
            header.extra_field_length + 
            header.file_comment_length;
        if(i > (uint32_t)buf.len)
            break;
        // insert
        if(header.compressed_size)
        {
            uv_fs_t req;
            uv_buf_t buf2;
            local_file_header buffer;
            buf2.base = (char*)&buffer;
            buf2.len = sizeof(buffer);
            uv_fs_read(loop,&req,fd,&buf2,1,header.local_header_offset,NULL);
            uv_fs_req_cleanup(&req);
            if(req.result != sizeof(buffer))
                throw std::runtime_error("indexBundle(): unable to locate the entity");
            if(buffer.compressed_size != header.compressed_size ||
                header.signature != 0x02014B50)
                throw std::runtime_error("indexBundle(): unable to locate the entity");
            // TODO: validate local_header_offset and compressed file lenght being in range
            bundle->insertEntity(
                string_view {(char*)(&header+1), header.file_name_length},
                EntityInfo {
                    .offset = header.local_header_offset + (uint32_t)sizeof(buffer) + buffer.extra_field_length + buffer.file_name_length ,
                    .size = header.compressed_size
                }
            );
        }
    }
    {
        uv_fs_t req;
        uv_fs_close(loop,&req,fd,NULL);
        uv_fs_req_cleanup(&req);
    }
    bundles.insert(hash,bundle.release());
}
void AssetsManager::Bundle::insertEntity(string_view name, const EntityInfo& info)
{
    Hash32 hashValue = HashHelper::FNV1A32(name);
    if(hashValue == 0)
        hashValue = 1;
    uint32_t hashMask = this->mapSize-1;
    uint32_t offset = (int)(hashValue & hashMask);
    while (true)
    {
        uint32_t hashBuffer = this->hashes[offset];
        if(hashBuffer == hashValue)
            throw std::invalid_argument("indexBundle(): hash already exists");
        if (hashBuffer == 0) {
            this->hashes[offset] = hashValue;
            this->entities[offset] = info;
            break;
            return;
        }
        offset = (offset + 1) & hashMask;
    }
}
int AssetsManager::Bundle::findEntity(Hash32 entryHash)
{
    if(entryHash == 0)
        entryHash = 1;
    uint32_t hashMask = this->mapSize - 1;
    uint32_t capacity = this->mapSize; 
    uint32_t attempts = 0;
    uint32_t hash;
    uint32_t offset = (entryHash & hashMask);
    while (true)
    {
        hash = this->hashes[offset];
        if (hash == 0)
            return -1;
        if (unlikely(hash == entryHash))
            return (int32_t)offset;
        offset = (offset + 1) & hashMask;
        ++attempts;
        if (attempts == capacity)
            return -1;
    }
}
#pragma endregion indexBundle
#pragma region libuv callbacks

void AssetsManager::cb_failed(struct uv__work* w, int err){
    RequestWork* req = (RequestWork*)((char*)w - offsetof(RequestWork, ___.work.work_req));
    unsigned int *count = &w->loop->active_reqs.count;
    if(*count <= 0) throw std::runtime_error("");
    (*count)--;
    req->cb(req->ctx,nullptr,0);
    req->thiz->onCleanup(req);
}
void AssetsManager::close_cb(uv_fs_t* fs) {
    uv_fs_req_cleanup(fs);
}
void AssetsManager::after_read(uv_fs_t* fs) {
    RequestWork* req = (RequestWork*)((char*)fs - offsetof(RequestWork, ___.fs));
    uv_fs_req_cleanup(fs);
    if(req->bufLen > fs->result)
        goto error;
    req->cb(req->ctx,std::move(req->content),req->bufLen);
    goto cleanup;
error:
    req->content.reset();
    req->cb(req->ctx,nullptr,0);
cleanup:
    req->thiz->onCleanup(req);
}
void AssetsManager::initial_after_work(struct uv__work* w, int err) {
    RequestWork* req = (RequestWork*)((char*)w - offsetof(RequestWork, ___.work.work_req));
    uv_loop_t *loop = w->loop;
    {
        unsigned int *count = &loop->active_reqs.count;
        if(*count <= 0) throw std::runtime_error("");
        (*count)--;
    }
    req->content = make_align<uint8_t[]>(req->bufLen);
    uv_buf_t bf{req->bufLen,(char*)req->content.get()};
    uv_fs_read(loop, &req->___.fs, req->fd, &bf, 1, req->offset, after_read);
}
void AssetsManager::initial_work(uv__work* w){
    RequestWork* req = (RequestWork*)((char*)w - offsetof(RequestWork, ___.work.work_req));
    Bundle *bundle;
    int32_t offset;
    {
        {
            AssetsManager *thiz = req->thiz;
            auto &bundles = thiz->bundles;
            int index = bundles.indexOf(req->bundleHash);
            if(index < 0)
                return;        
            bundle = bundles.getValue(index);
        }
        offset = bundle->findEntity(req->entryHash);
        if(offset < 0)
            return;
    }
    //const uint32_t blkSize = bundle->blkSize;
    const EntityInfo info = bundle->entities[offset];
    req->offset = info.offset;
    req->bufLen = info.size;
    uv_fs_t reqTemp;
    if(req->fd < 0)
    {
        req->fd = uv_fs_open(w->loop,&reqTemp,bundle->path,UV_FS_O_RDONLY, 0, NULL);
        uv_fs_req_cleanup(&reqTemp);
        if(req->fd < 0)
            return;
    }
    w->done = &initial_after_work;
}

#pragma endregion