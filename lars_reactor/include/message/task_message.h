#pragma once

#include <cstdint>
#include <functional>
#include <variant>

#include "eventloop/event_loop.h"

/**
 * @brief thread_queue消息队列能够接受的消息类型
 *
 */
class EventLoop;
using task_func = std::function<void(EventLoop* loop, void* args)>;

struct Task {
  task_func task;
  void* args;
};

struct task_message {
  // 任务类型：
  enum class TaskType : std::uint8_t {
    NEW_CONNECTION,  //  1.新建立的链接
    NEW_TASK,        //  2.正常任务/主thread分发的task
  };
  TaskType type;
  // 任务数据：
  // 1.新建立的链接则是fd
  // 2.正常任务则是回调函数和参数
  std::variant<int, Task> data;
};