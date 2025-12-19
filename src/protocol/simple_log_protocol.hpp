#ifndef __BN3MONKEY_SIMPLE_LOG_PROTOCOL__
#define __BN3MONKEY_SIMPLE_LOG_PROTOCOL__

#ifdef _MSC_VER
    // MSVC (Visual Studio)
    #define PACK_START __pragma(pack(push, 1))
    #define PACK_END __pragma(pack(pop))
    #define PACKED
#elif defined(__MINGW32__) || defined(__MINGW64__)
    // MinGW (Windows + GCC)
    #define PACK_START
    #define PACK_END
    #define PACKED __attribute__((packed))
#elif defined(__GNUC__) || defined(__clang__)
    // GCC, Clang (Linux, macOS)
    #define PACK_START
    #define PACK_END
    #define PACKED __attribute__((packed))
#else
    #error "Unsupported compiler"
#endif

#include <cstdint>
#include <cstdio>
#include <chrono>

namespace Bn3Monkey
{
    enum class LogColor : int32_t {
        Blue      = 0x1E90FF,
        DarkBlue  = 0x4682B4,

        Green     = 0x3CB371,
        DarkGreen = 0x2E8B57,

        Purple    = 0x9370DB,
        Violet    = 0x8A2BE2,

        Orange    = 0xFF8C00,
        Yellow    = 0xDAA520,

        Red       = 0xDC143C,
        DarkRed   = 0xCD5C5C,

        Teal      = 0x008080,
        Olive     = 0x6B8E23,

    };
    


    PACK_START
    struct LogHeader
    {
        static constexpr size_t SIZE = 96;
        static constexpr char MAGIC[] {'S', 'L', 'O', 'G'};
        static constexpr char DATE_FORMAT[] = "YYYY-MM-NN HH:MM:DD:mmm ";
        static constexpr char SIGNATURE_FORMAT[] = "12345678901234567890123456789012";
        static constexpr char TAG_FORMAT[] = "1234567890123456";
        

        static constexpr size_t MAGIC_SIZE = sizeof(MAGIC); // 4
        static constexpr size_t DATE_FORMAT_SIZE = sizeof(DATE_FORMAT)-1; // 24
        // for function name or class name
        static constexpr size_t SIGNATURE_SIZE = sizeof(SIGNATURE_FORMAT)-1;// 32
        static constexpr size_t TAG_SIZE = sizeof(TAG_FORMAT) - 1; // 16
        static constexpr size_t COLOR_SIZE = sizeof(LogColor); // 4
        
        static constexpr size_t OFFSET_MAGIC = 0;
        static constexpr size_t OFFSET_DATE_FORMAT = MAGIC_SIZE + sizeof(uint32_t); // 8
        static constexpr size_t OFFSET_SIGNATURE = OFFSET_DATE_FORMAT + DATE_FORMAT_SIZE; // 32
        static constexpr size_t OFFSET_TAG = OFFSET_SIGNATURE + SIGNATURE_SIZE; // 64
        static constexpr size_t OFFSET_COLOR = OFFSET_TAG + TAG_SIZE; // 80

        static constexpr size_t RESERVED_SIZE = SIZE - OFFSET_COLOR - COLOR_SIZE;


        char magic[MAGIC_SIZE] {0};
        uint32_t _padding1 {0};
        char date_format[DATE_FORMAT_SIZE] {0};
        char signature[SIGNATURE_SIZE]{ 0 };
        char tag[TAG_SIZE] {0};
        LogColor color {0};        
        char reserved[RESERVED_SIZE] {0};

        LogHeader() = default;
        LogHeader(const char* signature, const char* tag, LogColor color) : color(color) {
            memcpy(magic, MAGIC, sizeof(magic));
            printLogDate(date_format);
            snprintf(this->signature, SIGNATURE_SIZE, "%s", signature);
            snprintf(this->tag, TAG_SIZE, "%s", tag);
        }


    private:
        inline void printLogDate(char (&date_format)[DATE_FORMAT_SIZE]) {
            auto now = std::chrono::system_clock::now();
            auto since_epoch_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

            const int ms = static_cast<int>(since_epoch_milliseconds.count() % 1000);

            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tmv;

#if defined(_WIN32)
            if (localtime_s(&tmv, &tt) != 0) {
                return;
            }
#else
            if (localtime_r(&tt, &tmv) == nullptr) {
                return;
            }
#endif
            snprintf(date_format, sizeof(date_format), "%04d-%02d-%02d %02d:%02d:%02d:%03d|", tmv.tm_year + 1900,
                tmv.tm_mon + 1,
                tmv.tm_mday,
                tmv.tm_hour,
                tmv.tm_min,
                tmv.tm_sec,
                ms);
        }
    };
    PACK_END

    static_assert(sizeof(LogHeader) == LogHeader::SIZE, "Log Header should be 96");

    PACK_START
    struct LogLine
    {
        static constexpr size_t SIZE = 1024;

        static constexpr size_t HEADER_SIZE = sizeof(LogHeader); // 96
        static constexpr size_t CONTENT_SIZE = SIZE - HEADER_SIZE; // 1024-96

        static constexpr size_t OFFSET_HEADER = 0;
        static constexpr size_t OFFSET_CONTENT = HEADER_SIZE;

        LogHeader header;
        char content[CONTENT_SIZE]{ 0 };

        LogLine() = default;
        LogLine(const char* signature, const char* tag, LogColor color, const char* content) : header(signature, tag, color) {
            memset(this->content, 0, sizeof(this->content));
            
            constexpr size_t MAX_TEXT = CONTENT_SIZE - 1;
            auto size = strlen(content);
            if (size > MAX_TEXT)
                size = MAX_TEXT;

            memcpy(this->content, content, size);
        }

        static bool isValid(const char* input_buffer) {
            auto* ret = reinterpret_cast<const LogHeader*>(input_buffer);
            
            if (memcmp(input_buffer, LogHeader::MAGIC, LogHeader::MAGIC_SIZE) != 0)
                return false;

            return true;
        }
    };
    PACK_END

}

#endif // __BN3MONKEY_SIMPLE_LOG_PROTOCOL__