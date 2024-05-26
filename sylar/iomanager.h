#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"

namespace sylar {

class IOManager : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4
    };

private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {           //在哪一个调度器上执行该事件
            Scheduler* scheduler = nullptr;       //事件执行的scheduler
            Fiber::ptr fiber;                     //事件协程
            std::function<void() > cb;            //事件的回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);   //清理掉Context里边的协程对象，回调事件和协程调度器等任务
        void triggerEvent(Event event);

        
        EventContext read;              //读事件
        EventContext write;             //写事件
        int fd = 0;                     //事件关联的句柄
        Event m_events = NONE;          //已经注册的事件
        MutexType mutex;                
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");     //use_caller表示的是在哪个线程里执行了协程调度器，判断其是否在线程调度器里
    ~IOManager();

    //0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd,Event event);          //直接把事件删除
    bool cancelEvent(int fd, Event event);      //根据条件来触发该事件

    bool cancelAll(int fd);

    static IOManager* GetThis();        //获取当前的IOManager

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);
private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t>     m_pendingEventCount = {0};  //记录现在等待执行的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext* >  m_fdContexts;         //存放句柄上下文的数组

};

}


#endif

