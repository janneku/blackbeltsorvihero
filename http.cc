/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 *
 *
 * Portable HTTP client
 */
#include "http.h"
#include "version.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <stdexcept>

#if !defined(_WIN32)
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <errno.h>
	#define closesocket	close
#if defined(__ANDROID__)
	#define USER_AGENT	"Android"
#else
	#define USER_AGENT	"Linux"
#endif
#else
	/* windows */
	#include <winsock.h>
	#undef errno
	#undef EAGAIN
	#define errno	WSAGetLastError()
	#define EAGAIN	WSAEWOULDBLOCK
	#define EINPROGRESS WSAEWOULDBLOCK
	#define USER_AGENT	"Windows"
#endif

namespace {

#if defined(_WIN32)

/* fake window proc */
LRESULT CALLBACK hwnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
#endif

int set_nonblock(int fd, bool enabled)
{
#if !defined(_WIN32)
	int flags = fcntl(fd, F_GETFL) & ~O_NONBLOCK;
	if (enabled) {
		flags |= O_NONBLOCK;
	}
	return fcntl(fd, F_SETFL, flags);
#else
	/* windows */
	u_long arg = enabled ? 1 : 0;
	return ioctlsocket(fd, FIONBIO, &arg);
#endif
}

hostent *resolve_name(const std::string &name)
{
#if !defined(_WIN32)
	return gethostbyname(name.c_str());
#else
	/* windows */
	static HWND hwnd = NULL;
	if (hwnd == NULL) {
		/* create dummy window */
		WNDCLASS wc;
		memset(&wc, 0, sizeof wc);
		wc.lpfnWndProc = hwnd_proc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = "DUMMYWINDOW";
		RegisterClass(&wc);

		hwnd = CreateWindow(wc.lpszClassName, "foo", WS_POPUP,
				    0, 0, 1, 1, NULL, NULL, wc.hInstance,
				    NULL);
	}

	const UINT WM_RESOLVE_COMPLETE = WM_USER + 1337;

	/* is returned to the caller */
	static char buffer[MAXGETHOSTSTRUCT];

	HANDLE handle = WSAAsyncGetHostByName(hwnd, WM_RESOLVE_COMPLETE,
		name.c_str(), buffer, sizeof buffer);
	if (handle == 0) {
		return NULL;
	}
	for (int i = 0; i < 50; ++i) {
		MSG msg;
		if (PeekMessage(&msg, hwnd, WM_RESOLVE_COMPLETE,
				WM_RESOLVE_COMPLETE, PM_REMOVE)) {
			assert(msg.message == WM_RESOLVE_COMPLETE);
			printf("resolve took %d ms\n", i * 100);
			if (WSAGETASYNCERROR(msg.lParam) != 0) {
				return NULL;
			}
			return (hostent *) buffer;
		}
		Sleep(100);
	}
	warning("resolve timeout\n");
	WSACancelAsyncRequest(handle);
	return NULL;
#endif
}

}

std::string get_page(const std::string &server, const std::string &path,
			const std::string &payload)
{
	struct hostent *hp = resolve_name(server);
	if (hp == NULL) {
		throw std::runtime_error("Can not to resolve host " + server);
	}

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw std::runtime_error("Can not to create socket");
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr = *(in_addr *) hp->h_addr_list[0];
	sin.sin_port = htons(80);

	set_nonblock(fd, true);

	if (connect(fd, (sockaddr *) &sin, sizeof sin)) {
		if (errno != EINPROGRESS) {
			closesocket(fd);
			throw std::runtime_error("Can not to connect to "
						 + server);
		}
	}

	/* wait for the connection to be completed */
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	timeval tv = {5, 0};
	if (select(fd + 1, NULL, &fds, NULL, &tv) < 1) {
		closesocket(fd);
		throw std::runtime_error("Can not to connect to " + server);
	}

	set_nonblock(fd, false);

	const char *method = "GET";
	if (!payload.empty()) {
		method = "POST";
	}
	std::string req =
		strf("%s /%s HTTP/1.1\r\n"
		     "Host: %s\r\n"
		     "User-Agent: " USER_AGENT " (%s)\r\n"
		     "Connection: close\r\n",
		     method, path.c_str(), server.c_str(), version);
	if (!payload.empty()) {
		req += strf("Content-type: application/x-www-form-urlencoded\r\n"
			    "Content-Length: %d\r\n", int(payload.size()));
	}
	req += "\r\n" + payload;
	printf("request: %s\n", req.c_str());

	size_t pos = 0;
	while (pos < req.size()) {
		int written = send(fd, &req[pos], req.size() - pos, 0);
		if (written < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			closesocket(fd);
			throw std::runtime_error("Can not to write HTTP request");
		}
		pos += written;
	}

	size_t headers_length;
	std::string headers;
	while (true) {
		pos = headers.size();
		headers.resize(pos + 64, 0);
		int got = recv(fd, &headers[pos], 64, 0);
		if (got < 0) {
			headers.resize(pos);
			if (errno == EAGAIN || errno == EINTR)
				continue;
			closesocket(fd);
			throw std::runtime_error("Can not to read HTTP reply");
		}
		headers.resize(pos + got);

		headers_length = headers.find("\r\n\r\n");
		if (headers_length != std::string::npos) {
			headers_length += 4;
			break;
		}
		if (got == 0) {
			closesocket(fd);
			throw std::runtime_error("HTTP connection terminated early");
		}
	}

	printf("headers: %s\n", headers.substr(0, headers_length).c_str());

	size_t i = 0;
	int reply_length = -1;
	while (i < headers_length) {
		size_t j = headers.find("\r\n", i);
		std::string line = headers.substr(i, j - i);

		std::istringstream parser(line);
		std::string type;
		parser >> type;

		if (i == 0) {
			if (type.substr(0, 5) != "HTTP/") {
				closesocket(fd);
				throw std::runtime_error("Invalid HTTP reply");
			}
			int status;
			parser >> status;
			if (status != 200) {
				closesocket(fd);
				throw std::runtime_error(
					strf("HTTP error (%d): ", status)
					+ line.substr(parser.tellg()));
			}

		} else if (type == "Content-length:"
			   || type == "Content-Length:") {
			parser >> reply_length;

		} else if (type == "Transfer-encoding:"
			   || type == "Transfer-Encoding:") {
			std::string encoding;
			parser >> encoding;
			if (encoding == "chunked") {
				closesocket(fd);
				throw std::runtime_error("chunked encoding not supported");
			}
		}
		i = j + 2;
	}

	printf("HTTP reply length %d\n", reply_length);

	std::string reply = headers.substr(headers_length);
	while (true) {
		pos = reply.size();
		size_t toread = 4096;
		if (reply_length >= 0) {
			toread = reply_length - reply.size();
			if (toread == 0)
				break;
		}
		reply.resize(pos + toread, 0);
		int got = recv(fd, &reply[pos], toread, 0);
		if (got < 0) {
			reply.resize(pos);
			if (errno == EAGAIN || errno == EINTR)
				continue;
			closesocket(fd);
			throw std::runtime_error("Can not to read HTTP reply");
		}
		reply.resize(pos + got);

		if (got == 0) {
			if (reply_length < 0)
				break;
			closesocket(fd);
			throw std::runtime_error("HTTP connection terminated early");
		}
	}
	closesocket(fd);

	printf("received %d bytes\n", int(reply.size()));

	return reply;
}
