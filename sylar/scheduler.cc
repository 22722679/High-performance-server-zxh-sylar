#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;            //线程局部变量，用来生成协程调度器的指针
static thread_local Fiber* t_fiber = nullptr;                    //主协程函数


Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)  
//use_caller表示的是在哪个线程里执行了协程调度器，判断其是否在线程调度器里
    :m_name(name){
    SYLAR_ASSERT(threads > 0);

    if(use_caller) {
        sylar::Fiber::GetThis();
        --threads;

        SYLAR_ASSERT(GetThis() == nullptr);     //保证一个线程里只有一个协程调度器
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0 ,true));       
        /*
            因为新线程的主协程不会参与到协程调度里去，
            所以要用一个新的线程去做协程调度器，
            这里的Scheduler::run是用来执行协程调度器的运行以及分配线程的方法
        */
        sylar::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();      //主协程创建，保证协程调度器的所在线程与它所执行的方法的主协程不在一个线程里
        m_rootThread = sylar::GetThreadId();        //主线程id
        m_threadIds.push_back(m_rootThread);        //把线程id放入存储线程的数组里
    }else {
        m_rootThread = -1;
    }
    m_threadCount = threads;        //记录线程数
}

Scheduler::~Scheduler(){
    SYLAR_ASSERT(m_stopping);     //断言

    if(GetThis() == this){
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis(){
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber(){
    return t_fiber;
}

//线程池启动和停止
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {       //说明启动了,直接返回
        return ;
    }
    m_stopping = false;     //启动
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i ){      //用数组存储线程信息
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this) //线程跑的方法
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());      //获取协程和存储协程id
    }

    lock.unlock();

    //if(m_rootFiber){
    //    m_rootFiber->call();
    //    SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //}
}

//线程空闲时的循环等待
void Scheduler::stop() {    
    //等待所有任务完成后再退出
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0       //创建这个线程的协程里面run的那个协程
            && (m_rootFiber->getState() == Fiber::TERM        //主协程状态为TERM
            || m_rootFiber->getState() == Fiber::INIT)) {
            SYLAR_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if(stopping()) {        //子类去实现的
                return ;
            }
    }

    //bool exit_on_this_fiber = false;
    if(m_rootThread != -1){
        SYLAR_ASSERT(GetThis() ==  this);  
        /*
            断言的目的是，当它Scheduler(协程调度器)要把创建它的线程去使用时，
            那它的停止就必须在创建它的线程里去执行
        */
    }else {
        /*
            如果该协程调度器不在它的创建线程里的话，则可以在任意线程里结束
        */
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();          //类似与信号量，只要使用这个方法就会唤醒线程,然后他们会自己结束
    }

    if(m_rootFiber) {       //有多少个协程线程运行，就要执行tickle多少次
        tickle();
    }

    if(m_rootFiber){
        // while(stopping()) {
        //     if(m_rootFiber->getState() == Fiber::TERM
        //             || m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run , this), 0, true));
        //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //         t_fiber = m_rootFiber.get();
        //     }
        //     m_rootFiber->call();
        // }
        if(!stopping()){        
            //如果主协程没有停止，就只call一次其他的线程让他自己执行到停止为止
            //就是任务被线程0抢到后，用协程4去跑
            m_rootFiber->call();
        }
    }

    //实现一个优雅的退出，将任务队列里的人物清空了再停止
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    //thread类里面的析构会自动detach，这里应该是主线程先跑完了同时释放了scheduler，子线程还在运行，导致内存泄漏导致


    //if(exit_on_this_fiber) {
    //}
}

void Scheduler::SetThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    SYLAR_LOG_INFO(g_logger) << m_name << "run";
    SetThis();          //把当前线程的scheduler放进来

    if(sylar::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();           //!= 时id就是 run协程所在线程的id
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    
    //无任务时，应该执行idle函数，主要任务是占据CPU或者清理一下缓存
    Fiber::ptr cb_fiber;        //回调函数的协程

    FiberAndThread ft;

    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {   //协程的消息队列里取出一个我们应该要执行的消息， 协程
            MutexType::Lock lock(m_mutex);
            auto    it = m_fibers.begin();     //m_fibers时存放闲置协程的list数组
            while(it != m_fibers.end()){
                //**让空闲协程去执行任务
                if(it->thread != -1 && it->thread != sylar::GetThreadId()){  //如果一个任务已经指定好了要在哪个线程下面执行
                    ++it;   //这个任务虽然不需要去处理，但需要通知别人来处理
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC){
                    ++it;
                    continue;
                }

                ft = *it;
                //tickle_me = true;
                m_fibers.erase(it);     //从消息队列里删除，它已经被线程去执行了
                ++m_activeThreadCount;  //拿出任务时就应该数量加1
                is_active = true;
                break;
            }
        }
        if(tickle_me){
            tickle();
        }


        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)){  
            
            //如果他不是在结束状态的时候呢，
            ft.fiber->swapIn();     //就把他唤醒，然后他就执行
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);         //再次执行
            } else if(ft.fiber->getState() == Fiber::TERM
                    &&  ft.fiber->getState() != Fiber::EXCEPT){  //如果不等于这两个结束的状态时,就把状态设置成HOLD
                ft.fiber->m_state = Fiber::HOLD;    //让出执行状态后就应该是HOLD状态
            }
            ft.reset();      //用完后就reset
        }else if(ft.cb){
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            }else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);     //重新schedule
                cb_fiber.reset();
            }else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                    cb_fiber->reset(nullptr);     
                    //智能指针的reset释放掉，不会引起析构
            } else {  //if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        }else {     
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            //如果这次执行有任务的话就不执行idle了，直到没有任务执行的时候，才去执行idle
            if(idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                tickle();
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle"; 

}
bool Scheduler::stopping(){    // 其他子类有清理任务的机会
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping             //返回值来确定任务是否已经完成了
            && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {     // 空闲状态执行, 没有任务时，一段时间后清空一下缓存
    SYLAR_LOG_INFO(g_logger) << "idle"; 
    while(!stopping()){
        sylar::Fiber::YieldToHold();
    }
}

}
