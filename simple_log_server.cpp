#include "simple_log_server.hpp"
#include <string>



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

	_is_running = true;
	_log_collector = std::thread(&SimpleLogServer::appendLogToFile, this);
}
SimpleLogServer::~SimpleLogServer() {

	if (_is_initialized) {
		_is_running = false;
		_queue.wake();
		_log_collector.join();
		_request_server.close();
	}
	Bn3Monkey::releaseSecuritySocket();
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif // _WIN32

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

		// 🔥 Android log 스타일 10개 전송
		if (!is_hard_test) {
			for (int i = 0; i < 10; i++) {


				std::string log =
					"2025-11-21 23:51:0" + std::to_string(i) +
					" D/MyApp: Sample log message #" + std::to_string(i) + "\n";
				uint32_t log_size = static_cast<uint32_t>(log.size());

				send(sock, LogHeader::MAGIC, 4, 0);
				send(sock, reinterpret_cast<const char*>(&log_size), sizeof(log_size), 0);
				send(sock, log.data(), log.size(), 0);

				Sleep(200); // 서버가 보기 좋도록
			}
		}
		else {
			for (int i = 0; i < 1024 * 10 * 10; i++) {
				std::string log =
					"2025-11-21 23:51:0" + std::to_string(i) +
					" D/MyApp: Sample log message #" + std::to_string(i) + "\n";
				uint32_t log_size = static_cast<uint32_t>(log.size());

				send(sock, LogHeader::MAGIC, 4, 0);
				send(sock, reinterpret_cast<const char*>(&log_size), sizeof(log_size), 0);
				send(sock, log.data(), log.size(), 0);
			}
		}

		closesocket(sock);
		WSACleanup();

#endif // _WIN32
		}
	}.join();
}

void SimpleLogServer::createLogFile()
{
	const char* filename = createFileName();
	{
		std::unique_lock<std::mutex> lock(_file_mtx);
		_is_file_open = true;
		_filename = filename;
	}
	printf("[[SYSTEM]] Create File (%s)\n", filename);
}

void SimpleLogServer::closeLogFile()
{
	{
		std::unique_lock<std::mutex> lock(_file_mtx);
		_is_file_open = false;
	}
	printf("[[SYSTEM]] Close File\n");
}

const char* SimpleLogServer::createFileName()
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

void SimpleLogServer::appendLogToFile()
{
	while (_is_running) {
		auto line = _queue.pop(_is_running);
		if (!_is_running)
			break;
		{
			std::unique_lock<std::mutex> lock(_file_mtx);
			std::ofstream ofs(_filename, std::ios::binary | std::ios::app);
			ofs.write(line.data(), line.length());
		}
	}
}
