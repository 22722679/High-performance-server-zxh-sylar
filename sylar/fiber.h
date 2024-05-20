#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory.h>
#include <functional>       //解决了许多函数指针不适用的场景
#include <ucontext.h>
#include "thread.h"

//ucontext.h
namespace sylar {

//继承该类是由于该类中的shared_from_this可以获取当前类的智能指针
//且继承了enable_shared_from_this<Fiber>之后，就不可以在栈上创建对象，因为其一定要是智能指针的成员
class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,       //初始化状态
        HOLD,       //人工捕获状态
        EXEC,       //正在执行状态
        TERM,
        READY,      //准备执行状态
        EXCEPT
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0,bool use_caller = false);
    ~Fiber();
    
    //重置协程函数，并重置状态
    //INIT， TREM
    //用于当协程使用完后状态、函数重置,重新使用该分配的内存
    void reset(std::function<void()> cb);    
    //切换到当前协程执行
    void swapIn();
    //把当前协程切换到后台
    void swapOut();

    void  call();  //直接把当前的线程切换成目标线程
    void back();
    uint64_t getId() {return m_id;}
    State getState() const  { return m_state;}
public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前执行点的协程
    static Fiber::ptr  GetThis();
    //协程切换到后台，，并且设置为Ready状态
    static void YieldToReady();       //协程让出执行权
    //协程切换到后台，，并且设置为Hold状态
    static void YieldToHold();        //协程让出执行权
    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;
    //协程运行栈大小
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;

};

}



#endif