#include "tcp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>

#include "tcp_connection.h"

// using io_call_back = std::function<void(EventLoop* el, int fd, void* args)>;
int TcpServer::_max_conns = 0;
std::mutex TcpServer::_mutex;
std::unordered_map<int, TCPConnection *> TcpServer::_conns;

TcpServer::TcpServer(EventLoop *loop, const char *ip, std::uint16_t port)
    : _loop(loop) {
  bzero(&_connection_addr, sizeof(_connection_addr));
  // 忽略一些信号 SIGHUP, SIGPIPE
  // SIGPIPE:如果客户端关闭，服务端再次write就会产生
  // SIGHUP:如果terminal关闭，会给当前进程发送该信号
  if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
    std::cerr << "signal ignore SIGHUP error\n";
  }

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    std::cerr << "signal ignore SIGPIPE error\n";
  }

  // Create a socket
  _sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
  // Check if the socket was created successfully
  if (_sockfd == -1) {
    std::cerr << "tcp::server socket() error\n";
    exit(1);
  }
  // init connection address
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  inet_aton(ip, &server_addr.sin_addr);
  server_addr.sin_port = htons(port);

  // set socket CAN REUSE
  int op = 1;
  if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
    std::cerr << "tcp::server setsockopt() error\n";
    exit(1);
  }

  // bind the port
  if (bind(_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "tcp::server bind() error\n";
    exit(1);
  }
  // listen
  if (listen(_sockfd, 1024) == -1) {
    std::cerr << "tcp::server listen() error\n";
    exit(1);
  }

  // init connection number
  _max_conns = MAX_CONNS;

  // 注册socket读事件accept到eventLoop
  _loop->add_io_event(
      _sockfd, EPOLLIN,
      [](IO_EVENT_ARGUMENT) {
        TcpServer *server = (TcpServer *)args;
        server->handle_accept();
      },
      this);
}

TcpServer::~TcpServer() { close(_sockfd); }

void TcpServer::handle_accept() {
  int connection_fd;
  while (true) {
    // 1 accept
    std::cout << "begin accept\n";
    connection_fd = accept(_sockfd, (struct sockaddr *)&_connection_addr,
                           &_connection_addr_len);
    if (connection_fd == -1) {
      if (errno == EINTR) {
        // 如果是中断信号，继续accept
        continue;
      } else if (errno == EAGAIN) {
        // 如果是非阻塞模式，且没有连接，返回EAGAIN
        std::cerr << "tcp::server accept() error EAGAIN\n";
        break;
      } else if (errno == EMFILE) {
        // 链接过多了
        std::cerr << "tcp::server accept() error EMFILE\n";
        continue;
      } else {
        std::cerr << "tcp::server accept() error\n";
        exit(1);
      }
    }
    // accept success
    if (get_connection_num() >= _max_conns) {
      std::cerr << "tcp::server connection num is max\n";
      close(connection_fd);
    } else {
      new TCPConnection(connection_fd, _loop);
      std::cout << "get new connection succ!\n";
    }
    break;
  }
}

void TcpServer::add_connection(int conn_fd, TCPConnection *conn) {
  std::lock_guard<std::mutex> lock(_mutex);
  std::cout << "add connection: " << conn_fd << "\n";
  _conns[conn_fd] = conn;
}

void TcpServer::remove_connection(int conn_fd) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_conns.find(conn_fd) != _conns.end()) {
    std::cout << "remove connection: " << conn_fd << "\n";
    _conns.erase(conn_fd);
  }
}

int TcpServer::get_connection_num() { return _conns.size(); }
