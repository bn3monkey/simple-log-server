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

class LogLine {
public:
    static constexpr size_t MAX_LENGTH = 1024;

    explicit LogLine() : _data(nullptr) {}
    explicit LogLine(char* address) : _data(address) {

    }
    inline const char* data() const {
        return _data;
    }
    inline size_t length() const {
        return _length;
    }
    LogLine& set(const char* message, size_t message_size) {
        memset(_data, 0, LogLine::MAX_LENGTH);
        _length = message_size;
        if (message_size > LogLine::MAX_LENGTH - 1)
            _length = LogLine::MAX_LENGTH - 1;
        strncpy(_data, message, _length);
        return *this;
    }

private:
    char* _data;
    size_t _length;
};

class LogPool {
public:
    static constexpr size_t MAX_LINES = 1024 * 10;

    LogPool() : _data(LogLine::MAX_LENGTH * MAX_LINES), _lines(MAX_LINES) {
        for (size_t i = 0; i < MAX_LINES; i++) {
            _lines[i] = LogLine(_data.data() + i * LogLine::MAX_LENGTH);
        }
    }
    inline LogLine getLine() {
        _current_index = (_current_index + 1) % MAX_LINES;
        return _lines[_current_index];        
    }

private:
    std::vector<char> _data;
    std::vector<LogLine> _lines;
    size_t _current_index{ 0 };
};

class LogQueue
{
public:
    inline LogLine createLine(const char* message, size_t message_size) {
        LogLine line = _pool.getLine();
        line.set(message, message_size);
        return line;
    }
    void push(const LogLine& line) {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _queue.push(line);
        }
        _cv.notify_all();
    }
    LogLine pop(std::atomic<bool>& is_running) {
        {
            std::unique_lock<std::mutex> lock(_mtx);
            _cv.wait(lock, [&]() {
                return !is_running || !_queue.empty();
                });
            if (!is_running)
                return LogLine();
            else {
                auto line = _queue.front();
                _queue.pop();
                return line;
            }
        }
    }
    void wake() {
        _cv.notify_all();
    }

private:
    LogPool _pool;
    std::condition_variable _cv;
    std::mutex _mtx;
    std::queue<LogLine> _queue;
};

struct LogHeader {
    static constexpr char MAGIC[] = "SLOG";

    char magic[4];
    uint32_t length;
};

class SimpleLogServerHandler : public Bn3Monkey::SocketRequestHandler {
public:
    SimpleLogServerHandler(LogQueue& queue) : _queue(queue) {}

    // SocketRequestHandler을(를) 통해 상속됨
    size_t getHeaderSize() override
    {
        return sizeof(LogHeader);
    }
    size_t getPayloadSize(const char* header) override
    {
        auto* _header = reinterpret_cast<const LogHeader*>(header);
        return static_cast<size_t>(_header->length);
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
        auto line = _queue.createLine(input_buffer, input_size);
        printf("%s\n", line.data());
        _queue.push(line);
    }
private:
    LogQueue& _queue;
};

class SimpleLogServer {
public:
    explicit SimpleLogServer(uint32_t port);
    virtual ~SimpleLogServer();

    inline operator bool() const {
        return _is_initialized;
    }

    void sendTestLog(bool is_hard_test);

    void createLogFile();
    void closeLogFile();
private:
    bool _is_initialized{ false };

    uint32_t _port;

    std::atomic<bool> _is_running;
    std::thread _log_collector;
    LogQueue _queue;

    std::mutex _file_mtx;
    std::string _filename;
    bool _is_file_open;

    SimpleLogServerHandler _request_handler{ _queue };
    Bn3Monkey::SocketRequestServer _request_server;

    const char* createFileName();

    void appendLogToFile();
};

#endif // __SIMPLE_LOG_SERVER__