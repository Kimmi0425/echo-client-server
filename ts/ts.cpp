#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // __linux
#ifdef WIN32
#include <winsock2.h>
#include "../mingw_net.h"
#endif // WIN32
#include <iostream>
#include <thread>
#include <set>
#include <pthread.h> // used for thread synchronization

using namespace std;

#ifdef WIN32
void perror(const char* msg) { fprintf(stderr, "%s %ld\n", msg, GetLastError()); }
#endif // WIN32

void usage() {
	cout << "syntax: ts [-e] <port>\n";
	cout << "  -e : echo\n";
	cout << "sample: ts 1234\n";
}

struct Param {
	bool echo{false};
	bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				continue;
			}
			port = stoi(argv[i++]);
		}
		return port != 0;
	}
} param;

set<int> cli_ac; // declare associate container
pthread_mutex_t ini_mut = PTHREAD_MUTEX_INITIALIZER; // pthread_mutex_t : declare variables in the thread, PTHREAD_MUTEX_INITIALIZER: initialize mutex to default value

void recvThread(int sd) {
	cout << "connected\n";
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return " << res;
			perror(" ");
			break;
		}
		buf[res] = '\0';
		cout << buf;
		cout.flush();
		if (param.echo) {
			res = send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				cerr << "send return " << res;
				perror(" ");
				break;
			}
		}
		else if (param.broadcast){
			pthread_mutex_lock(&ini_mut); // approach critical session
			set<int> ::iterator iter; // set <data type> variable name, declare iterator
			for(iter = cli_ac.begin();iter!=cli_ac.end();iter++){ // .begin(): return first element, .end(): return last element
				res = send(*iter, buf, res, 0);
				if(res == 0 || res == -1){
					cerr << "send return " << res;
					perror(" ");
					break;
				}
			}
			pthread_mutex_unlock(&ini_mut); // come out in critical session
		}
	}
	cout << "disconnected\n";
	pthread_mutex_lock(&ini_mut);
	cli_ac.erase(sd); // remove element
	pthread_mutex_unlock(&ini_mut);
	close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

#ifdef WIN32
	WSAData wsaData;
	WSAStartup(0x0202, &wsaData);
#endif // WIN32

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
#ifdef __linux__
	int optval = 1;
	res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
#endif // __linux

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5); // queue can store max 5
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		
		cli_ac.insert(cli_sd); // insert element cli_sd
		
		thread* t = new thread(recvThread, cli_sd);
		t->detach();
	}
	close(sd);
}
