#include "simple_log_server.hpp"
#include <string>
#include <algorithm>

using namespace Bn3Monkey;

Bn3Monkey::LogPool::LogPool(size_t max_line_per_files, size_t interval_lines_of_commit) :
	_max_line_per_files(max_line_per_files),
	_interval_lines_of_commit(interval_lines_of_commit),
	_prev_commit_line(0),
	_next_commit_line(_interval_lines_of_commit),
	_current_lines(0)
{
	_current_file = rotate();
}

void Bn3Monkey::LogPool::write(const LogLine& line)
{
	_current_lines = append(_current_file, line, _current_lines);
	synchronize(_current_file, _current_lines);
	if (!hasCapacity()) {
		_current_file = rotate();
	}
}

size_t Bn3Monkey::LogPool::append(MemoryMappedFile& file, const LogLine& line, size_t current_lines)
{
	char* data = file.data();
	char* dest = data + current_lines * sizeof(LogLine);
	memcpy(dest, reinterpret_cast<const char*>(&line), sizeof(LogLine));
	dest[sizeof(LogLine) - 1] = '\n';

	std::replace(dest, dest + sizeof(LogLine), static_cast<unsigned char>('\0'), static_cast<unsigned char>(' '));

	return current_lines + 1;
}

void Bn3Monkey::LogPool::synchronize(MemoryMappedFile& file, size_t current_lines)
{
	if (current_lines == _next_commit_line) {
		file.commit(_prev_commit_line, _next_commit_line);
		_prev_commit_line = _next_commit_line;
		_next_commit_line += _interval_lines_of_commit;
	}
}

bool Bn3Monkey::LogPool::hasCapacity()
{
	return _current_lines < _max_line_per_files;
}

const char* Bn3Monkey::LogPool::createLogFileName()
{
	static char buffer[1024]{ 0 };
	std::time_t now = std::time(nullptr);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &now);
#else
	tm = *std::localtime(&now);
#endif

	// 파일명: log_YYYYMMDD_HHMMSS.txt
	std::snprintf(
		buffer,
		sizeof(buffer),
		"log_%04d%02d%02d_%02d%02d%02d.txt",
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec
	);

	return buffer;
}

MemoryMappedFile Bn3Monkey::LogPool::rotate()
{
	auto* filename = createLogFileName();
	

	_prev_commit_line = 0;
	_next_commit_line = _interval_lines_of_commit;

	_current_lines = 0;
	return MemoryMappedFile(filename, MemoryMappedFile::Access::READWRITE_WITH_CREATE, _max_line_per_files * sizeof(LogLine));

}






SimpleLogServer::SimpleLogServer(uint32_t port) : _port(port), _request_server{
		Bn3Monkey::SocketConfiguration {
			"0.0.0.0",
			port,
			false
		}
	}
{
	Bn3Monkey::initializeSecuritySocket();
	auto res = _request_server.open(&_request_handler, 4);
	_is_initialized = res.code() == Bn3Monkey::SocketCode::SUCCESS;
	if (!_is_initialized) {
		printf("[[SYSTEM]] Error (%s)", res.message());
		return;
	}

}
SimpleLogServer::~SimpleLogServer() {

	if (_is_initialized) {
		_request_server.close();
	}
	Bn3Monkey::releaseSecuritySocket();
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32

#include <random>

inline LogColor randomLogColor(std::mt19937& rng) noexcept
{
	// enum 개수 = 12
	std::uniform_int_distribution<int> dist(0, 11);

	switch (dist(rng)) {
	case 0:  return LogColor::Blue;
	case 1:  return LogColor::DarkBlue;
	case 2:  return LogColor::Green;
	case 3:  return LogColor::DarkGreen;
	case 4:  return LogColor::Purple;
	case 5:  return LogColor::Violet;
	case 6:  return LogColor::Orange;
	case 7:  return LogColor::Yellow;
	case 8:  return LogColor::Red;
	case 9:  return LogColor::DarkRed;
	case 10: return LogColor::Teal;
	case 11: return LogColor::Olive;
	default: return LogColor::Blue; // unreachable, safety
	}
}

void SimpleLogServer::sendTestLog(bool is_hard_test)
{
	std::thread{ [&]() {
#ifdef _WIN32
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);

		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);  // 원하는 서버 포트
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
			return;
		}

		std::mt19937 rng{ std::random_device{}() };

		const int loop =
			is_hard_test ? (1024 * 10 * 10) : 10;

		for (int i = 0; i < loop; ++i) {

			LogColor color = randomLogColor(rng);

			std::string msg =
				"Sample LogLine Protocol Test #" + std::to_string(i);

			LogLine line(
				"SimpleLogServer::sendTestLog",
				"TEST",
				color,
				msg.c_str()
			);

			// 🔥 핵심: LogLine 전체를 그대로 전송
			const char* raw = reinterpret_cast<const char*>(&line);
			size_t remaining = LogLine::SIZE;

			while (remaining > 0) {
				int sent = send(sock, raw, (int)remaining, 0);
				if (sent <= 0)
					break;
				raw += sent;
				remaining -= sent;
			}

			if (!is_hard_test)
				Sleep(200);
		}

		closesocket(sock);
		WSACleanup();

#endif // _WIN32
		}
	}.join();
}
