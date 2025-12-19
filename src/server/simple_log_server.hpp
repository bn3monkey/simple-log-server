#if !defined(__SIMPLE_LOG_SERVER__)
#define __SIMPLE_LOG_SERVER__

#include <SecuritySocket.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <array>

#include <simple_log_protocol.hpp>
#include "memory_mapped_file/memory_mapped_file.hpp"

namespace Bn3Monkey {

    class LogPool {
    public:
        LogPool(size_t max_line_per_files = 4096, size_t interval_lines_of_commit = 64);

        inline operator bool() const { return _is_initialized; }
        void write(const LogLine& line);

    private:
        bool _is_initialized{ false };
        size_t _max_line_per_files;
        size_t _interval_lines_of_commit;

        size_t _prev_commit_line;
        size_t _next_commit_line;

        size_t _current_lines;
        MemoryMappedFile _current_file;

        const char* createLogFileName();

        MemoryMappedFile rotate();
        size_t append(MemoryMappedFile& file, const LogLine& line, size_t current_lines);
        void synchronize(MemoryMappedFile& file, size_t current_lines);
        bool hasCapacity();

    };

    class LogPrinter
    {
        struct LogColorSetter {
            LogColorSetter(LogColor color) {
                const uint32_t v = static_cast<uint32_t>(color);
                int32_t r = (v >> 16) & 0xFF;
                int32_t g = (v >> 8) & 0xFF;
                int32_t b = v & 0xFF;
                printf("\x1b[38;2;%d;%d;%dm", r, g, b);
            }
            ~LogColorSetter() {
                printf("\x1b[0m");
            }
        };
    public:
        static void print(const LogLine& logline) {
            {
                LogColorSetter color_setter{ logline.header.color };
                printf("%s %s %s | %s\n", logline.header.date_format, logline.header.signature, logline.header.tag, logline.content);
            }
        }
    };


    class SimpleLogServerHandler : public Bn3Monkey::SocketRequestHandler {
    public:
        SimpleLogServerHandler(LogPool& pool) : _pool(pool) {}

        // SocketRequestHandler��(��) ���� ��ӵ�
        size_t getHeaderSize() override
        {
            return sizeof(LogHeader);
        }
        size_t getPayloadSize(const char* header) override
        {
            (void*)header;
            return LogLine::CONTENT_SIZE;
        }
        Bn3Monkey::SocketRequestMode onModeClassified(const char* header) override
        {
            return Bn3Monkey::SocketRequestMode::WRITE_STREAM;
        }
        void onClientConnected(const char* ip, int port) override
        {
            printf("[[SYSTEM]] Log Client is connected : %s %d\n", ip, port);
        }
        void onClientDisconnected(const char* ip, int port) override
        {
            printf("[[SYSTEM]] Log Client is disconnected : %s %d\n", ip, port);
        }
        void onProcessed(const char* header, const char* input_buffer, size_t input_size, char* output_buffer, size_t* output_size) override
        {
            return;
        }
        void onProcessedWithoutResponse(const char* header, const char* input_buffer, size_t input_size) override
        {
            if (LogLine::isValid(header))
            {
                LogLine line{};
                memcpy(&line.header, header, sizeof(line.header));
                memcpy(&line.content, input_buffer, input_size);

                LogPrinter::print(line);
                _pool.write(line);
            }
        }

    private:
        LogPool& _pool;
    };

    class SimpleLogServer {
    public:
        explicit SimpleLogServer(uint32_t port);
        virtual ~SimpleLogServer();

        inline operator bool() const {
            return _is_initialized;
        }

        void sendTestLog(bool is_hard_test);

    private:
        bool _is_initialized{ false };

        uint32_t _port;

        LogPool _pool;

        SimpleLogServerHandler _request_handler{ _pool };
        Bn3Monkey::SocketRequestServer _request_server;

    };
}

#endif // __SIMPLE_LOG_SERVER__