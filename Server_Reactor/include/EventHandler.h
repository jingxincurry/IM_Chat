#pragma once

// 事件处理器接口，定义了具体的事件处理方法
typedef int Handle;

class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual Handle get_handle() const = 0;
    virtual void handle_read() {}
    virtual void handle_write() {}
    virtual void handle_error() {}
    virtual void handle_close() {}
};
