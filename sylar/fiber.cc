#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};        //记录协程id
static std::atomic<uint64_t> s_fiber_count {0};     //记录协程数量

static thread_local Fiber* t_fiber = nullptr;           //
static thread_local Fiber::ptr t_threadFiber = nullptr;    //main协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =       //协程栈大小
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }
    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator =  MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

//把当前线程的上下文变成自定义可以执行协程操作的线程类
//默认构造函数创建的是主协程，没有属性
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false,"getcontext");
    }
    
    ++s_fiber_count;    //总协程数+1

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

//拷贝构造函数创建子协程,在固定大小的栈上
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb){
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();    
    //m_stacksize表示当前的协程数量,g_fiber_stack_size->getValue()是配置里的大小

    m_stack = StackAllocator::Alloc(m_stacksize);   //生成协程
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;                //当前上下文返回之后指向的上下文
    m_ctx.uc_stack.ss_sp = m_stack;         //栈里的信息赋值
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);      //协程与其关联的方法
    }else{
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);      //协程与其关联的方法
    }
    

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id= " << m_id;
}


//析构函数回收内存
Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack){
        SYLAR_ASSERT(m_state == TERM
                || m_state == EXCEPT 
                || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);    //回收栈
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id << " total = " << s_fiber_count;
}

// 重置协程函数，并重置状态
// INIT， TREM
// 用于当协程使用完后状态、函数重置,重新使用该分配的内存
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM
            ||  m_state == EXCEPT
            ||  m_state == INIT);    //只有在这两种状态时才会执行
    
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false,"getcontext");
    }

    m_ctx.uc_link = nullptr;        // 当前上下文返回之后指向的上下文
    m_ctx.uc_stack.ss_sp = m_stack; // 栈里的信息赋值
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0); // 设置一下他的回调函数
    m_state = INIT;
}

//直接把当前线程切换成目标执行线程
//swapIn和call的差别在SetThis()
void Fiber::call(){
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)) {
        SYLAR_ASSERT2(false,"swapcontext");
    }
}

//和swapOut一样逻辑，但是没有判断，会提高性能
void Fiber::back() {
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;

    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false,"swapcontext");
    }
}
//切换到后台执行
void Fiber::swapOut(){
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}
// 返回当前执行点的协程
Fiber::ptr Fiber::GetThis(){
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}
// 协程切换到后台，，并且设置为Ready状态
void Fiber::YieldToReady() { // 协程让出执行权
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}
// 协程切换到后台，，并且设置为Hold状态
void Fiber::YieldToHold() { // 协程让出执行权
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}
// 总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}
void Fiber::MainFunc(){         //协程的执行函数
    Fiber::ptr cur = GetThis(); //获取智能指针,整个执行结束后它没有释放，还在协程栈上，所以一直大于0
    SYLAR_ASSERT(cur);          //释放
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id = " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch(...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except" 
            << " fiber_id = " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();   //拿出裸指针
    cur.reset();                //释放掉
    raw_ptr->swapOut();         //再用裸指针切换回去

    SYLAR_ASSERT2(false, "never reach fiber_id =" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc(){
    Fiber::ptr cur = GetThis(); // 获取智能指针,整个执行结束后它没有释放，还在协程栈上，所以一直大于0
    SYLAR_ASSERT(cur);          // 释放
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception &ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                  << " fiber_id = " << cur->getId()
                                  << std::endl
                                  << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                  << " fiber_id = " << cur->getId()
                                  << std::endl
                                  << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get(); // 拿出裸指针
    cur.reset();              // 释放掉
    raw_ptr->back();       // 再用裸指针切换回去

    SYLAR_ASSERT2(false, "never reach fiber_id =" + std::to_string(raw_ptr->getId()));
}

}