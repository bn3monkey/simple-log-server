#include "simple_log_server.hpp"

int main() {

	SimpleLogServer server{ 13579u };
	if (!server) {
		return -1;
	}

	bool is_running{ true };
	char command[1024]{ 0 };
	while (is_running) {
		memset(command, 0, 1024);
		scanf("%s", command);
		if (!strcmp(command, "open")) {
			server.createLogFile();
		}
		else if (!strcmp(command, "close")) {
			server.closeLogFile();
		}
		else if (!strcmp(command, "test")) {
			server.sendTestLog(false);
		}
		else if (!strcmp(command, "hard")) {
			server.sendTestLog(true);
		}
		else if (!strcmp(command, "exit")) {
			is_running = false;
		}
	}
	return 0;
}