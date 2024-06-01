#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h> 

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event){    //根据事件获取事件对应的类
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false,"getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx){      //清理掉Context里边的协程对象，回调事件和协程调度器等任务
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    SYLAR_ASSERT(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return ;
}



IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) 
    :Scheduler(threads, use_caller,name){
        m_epfd = epoll_create(5000);      //fd数量
        SYLAR_ASSERT(m_epfd > 0);

        int rt = pipe(m_tickleFds);
        SYLAR_ASSERT(rt);

        epoll_event event;            //epoll的事件结构体
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;   //EPOLLET指的是一次事件触发并处理一次，之后不会再去处理
        event.data.fd = m_tickleFds[0];

        //异步
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);    //fcntl用于修改句柄属性
        SYLAR_ASSERT(rt);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SYLAR_ASSERT(rt);

        contextResize(32);

        start();
}

//将里边默认的句柄关掉或处理掉
IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);   //句柄关掉
    close(m_tickleFds[1]);   //句柄关掉

    for(size_t i=0; i < m_fdContexts.size(); ++i) {    //将fd的context值释放掉,因为存放的是指针
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {  //分配空间大一点，从而减少读写锁的消耗
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

// 0 success, -1 error
//添加事件的方法
int IOManager::addEvent(int fd, Event event, std::function<void()> cb = nullptr) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    
    if(m_fdContexts.size() > fd) {   //说明空间已经分配好了
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(m_fdContexts.size() * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::Mutex::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event) {       //防止多次执行同一个任务
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                    <<  " event = " << event
                    <<  " fd_event = " << fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    //判断新加还是修改
    int op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;

    epevent.events = EPOLLET | fd_ctx->events | event;  //新的事件
    epevent.data.ptr = fd_ctx;        //时间数据赋值

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);    //事件加载进epoll里
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << " epoll_ctl (" << m_epfd << ", "
                    <<  op << "," << fd << "," << epevent.events << "):" //op操作名称,fd操作对象的fd, epevent.events事件
                    <<  rt << " (" << errno << ") (" << strerror(errno) << ")";//rt为ctl的返回值,errno为系统错误
        return -1;
    }

    ++m_pendingEventCount;    //记录事件数量
    //修改IO数据的events
    fd_ctx->m_events = (Event)(fd_ctx->m_events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        context.cb.swap(cb);
    }else {
        context.fiber = Fiber::GetThis();
        SYLAR_ASSERT(context.fiber->getState() == Fiber::EXEC);    //赋值协程时，协程的状态应该是正在执行中
    }
    return 0;
}



bool IOManager::delEvent(int fd, Event event) {   // 直接把事件删除
    MutexType::ReadLock lock(m_mutex);
    if(m_fdContexts.size() <= fd){   //判断要删除的数据任务的大小是否满足和自己context的大小
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->m_events & event)) {       //没有该事件
        return false;
    }
    Event new_events = (Event)(fd_ctx->m_events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;     //事件结构体
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << " epoll_ctl (" << m_epfd << ", "
                    <<  op << "," << fd << "," << epevent.events << "):" //op操作名称,fd操作对象的fd, epevent.events事件
                    <<  rt << " (" << errno << ") (" << strerror(errno) << ")";//rt为ctl的返回值,errno为系统错误
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->m_events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);

    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {      // 根据条件来触发该事件
    MutexType::ReadLock lock(m_mutex);
    if(m_fdContexts.size() <= fd){   //判断要删除的数据任务的大小是否满足和自己context的大小
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->m_events & event)) {       //没有该事件
        return false;
    }
    Event new_events = (Event)(fd_ctx->m_events & ~event);  //找到fdContext
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;     //事件结构体
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << " epoll_ctl (" << m_epfd << ", "
                    <<  op << "," << fd << "," << epevent.events << "):" //op操作名称,fd操作对象的fd, epevent.events事件
                    <<  rt << " (" << errno << ") (" << strerror(errno) << ")";//rt为ctl的返回值,errno为系统错误
        return false;
    }

    //FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

//把该句柄下的所有任务，所有事件全部取消
//event 事件必须是自己找到的的对应event
bool IOManager::cancelAll(int fd) {
    MutexType::ReadLock lock(m_mutex);
    if(m_fdContexts.size() <= fd){   //判断要删除的数据任务的大小是否满足和自己context的大小
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->m_events) {       //没有该事件
        return false;
    }
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;     //事件结构体
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << " epoll_ctl (" << m_epfd << ", "
                    <<  op << "," << fd << "," << epevent.events << "):" //op操作名称,fd操作对象的fd, epevent.events事件
                    <<  rt << " (" << errno << ") (" << strerror(errno) << ")";//rt为ctl的返回值,errno为系统错误
        return false;
    }
    if(fd_ctx->m_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->m_events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    SYLAR_ASSERT(fd_ctx->m_events == 0)
    return true;
}

IOManager IOManager::*GetThis(){   // 获取当前的IOManager
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

//协程调度器框架的核心
void IOManager::tickle() {
    if(hasIdleThreads()) {      //空闲线程说明他已经陷入的idle里了，这个消息来唤醒它回来
        return ;               //如果没有空闲的线程，则跳过
    }
    int rt = write(m_tickleFds[1],"T", 1);   //写入IO数据,返回值为长度
    SYLAR_ASSERT(rt == 1);

}

bool IOManager::stopping() {
    return Scheduler::stopping()            //调度器正在stopping
        && m_pendingEventCount == 0;         //未处理的消息为0
}

void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event>   shared_events(events,[](epoll_event* ptr){   //C++11不支持智能指针传递数组，但C++14支持,这里是用析构的方法实现存储
        delete[] events;
    });

    while(true) {
        if(stopping()){
            SYLAR_LOG_INFO(g_logger) << "name = " << m_name << " idle stoppint exit";
            break;
        }

        //epoll_wait
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 5000;                //毫秒级的精度
            rt = epoll_wait(m_epfd, events, 64, MAX_TIMEOUT);  //如果没有事件回来，它也会被唤醒

            if(rt < 0 && errno == EINTR) {

            } else {
                break;
            }
        } while(true);

        for(int i=0; i < rt ;++i){
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                while(read(m_tickleFds[0],&dummy,1));    //必须读干净，不然不会通知了
                continue;
            }

            FdContext* fd_ctx = event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->m_mutex);
            if(event.events && (EPOLLERR | EPOLLHUP)) {      //如果是错误或者是中断的话，就需要修改events
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);      //剩余事件
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                SYLAR_LOG_ERROR(g_logger) << " epoll_ctl (" << m_epfd << ", "
                        << op << "," << fd_ctx->fd << "," << event.events << "):"        // op操作名称,fd操作对象的fd, epevent.events事件
                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")"; // rt为ctl的返回值,errno为系统错误
                    continue;
            }

            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;         //记录事件数量
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        //idle处理完一个回合后，应该把控制权
        //让出来，让出到协程调度框架里,因为有的事件需要到外部处理
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();        
        //在每次while循环处理事件结束后，让出执行时间，再回到协程调度器里面
    }
}

}