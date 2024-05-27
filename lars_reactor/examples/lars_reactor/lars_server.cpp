#include <cstring>
#include <filesystem>

#include "net/tcp/tcp_server.h"
#include "utils/config_file.h"

void qps_test(const char *data, std::uint32_t len, int msgid,
              NetConnection *conn, void *user_data) {
  // 得到服务端回执的数据
  printf("===>server callback\n");
  // 发送数据给服务端
  conn->send_message(data, len, msgid);
}

void handle2(const char *data, std::uint32_t len, int msgid,
             NetConnection *conn, void *user_data) {
  // 得到服务端回执的数据
  printf("  server callback handler2\n");
  printf("  recv from client: %s\n", data);
  printf("  msgid: [%d]\n", msgid);
  printf("  len: [%d]\n", len);
  printf("====================================\n");
}

// 新客户端创建后的回调函数
void on_client_build(NetConnection *conn, void *args) {
  int msgid = 200;
  const char *msg = "welcome to server";
  conn->send_message(msg, strlen(msg), msgid);
}

// 客户端断开后的回调函数
void on_client_lost(NetConnection *conn, void *args) {
  printf("=====>client is lost\n");
}

int main(int argc, const char **argv) {
  EventLoop loop;
  std::filesystem::path current_path = std::filesystem::current_path();
  std::cout << "当前目录是: " << current_path << std::endl;
  config_file::setPath("./conf/server.conf");

  std::string ip =
      config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
  short port = config_file::instance()->GetNumber("reactor", "port", 7777);

  TcpServer server(&loop, ip.c_str(), port);
  // 设置hook函数
  server.add_message_router(1, qps_test);
  server.add_message_router(2, handle2);
  // 设置连接hook函数
  server.set_construct_hook(on_client_build);
  server.set_destruct_hook(on_client_lost);
  loop.event_process();
  return 0;
}