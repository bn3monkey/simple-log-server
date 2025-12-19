#ifndef __BN3MONKEY_MEMORY_MAPPED_FILE__
#define __BN3MONKEY_MEMORY_MAPPED_FILE__

#include <cstdint>

namespace Bn3Monkey
{
    class MemoryMappedFile
    {
    public:
        enum class Code : int32_t {
            CLOSED = 0,
            SUCCESS = 1,

            CANNOT_OPEN_FILE = -0x1001,
            FILE_SIZE_IS_ZERO = -0x1002,
            CREATED_FILE_NEED_NON_ZERO_SIZE = -0x1003,
            CANNOT_MAP_FILE = -0x1004,
        };
        enum class Access : uint8_t
        {
            READONLY,
            READWRITE_WITH_CREATE,
            READWRITE_WITH_OPEN
        };

        MemoryMappedFile() noexcept = default;
        MemoryMappedFile(const char* path, Access access, size_t size) noexcept;

        MemoryMappedFile(const MemoryMappedFile&) = delete;
        MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

        MemoryMappedFile(MemoryMappedFile&& other) noexcept;
        MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;

        ~MemoryMappedFile();

        inline operator bool() const { return _code == Code::SUCCESS; }
        inline Code error() const { return _code; }

        void commitAll();
        void commit(size_t offset, size_t length);

        inline char* data() noexcept { return _data; }
        inline const char* data() const noexcept { return _data; }
        inline size_t size() const noexcept { return _size; }

    private:

        void open(const char* path, Access access, size_t size);
        void close();

        Code _code{ Code::CLOSED };
        char* _data{ nullptr };
        size_t _size;

#if defined(_WIN32)
        void* _handle{ nullptr };
        void* _mapping{ nullptr };
#else   
        int32_t _handle{ -1 };
#endif
    };
}

#endif // __BN3MONKEY_MEMORY_MAPPED_FILE__