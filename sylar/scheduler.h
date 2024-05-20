#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"


namespace sylar {
    
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");     //use_caller表示的是在哪个线程里执行了协程调度器，判断其是否在线程调度器里

    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin++ != end) {
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if(need_tickle){
            tickle();
        }
    }
protected:
    virtual void tickle();
    void run();
    virtual bool stopping();  //其他子类有清理任务的机会
    virtual void idle();        //空闲状态执行, 没有任务时，一段时间后清空一下缓存
    
    void SetThis();            //把当前线程的scheduler放进来
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    struct  FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;     //回调
        int thread;                    //线程id

        FiberAndThread(Fiber::ptr f, int thr)
            :fiber(f),thread(thr){
        }

        FiberAndThread(Fiber::ptr* f, int thr)      //第二个构造函数是为将引用计数释放掉 
            :thread(thr) {
                fiber.swap(*f);             
        }

        FiberAndThread(std::function<void()> f, int thr) 
            :cb(f), thread(thr){
        }

        FiberAndThread(std::function<void()>* f, int thr) 
            :thread(thr){
            cb.swap(*f);
        }

        FiberAndThread()    //STL中默认构造函数需要初始化
            :thread(-1) {   //不指定任何线程   
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
    

private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    //std::map<int, std::list<FiberAndThread> > m_thrFibers;
    Fiber::ptr m_rootFiber;         //主协程
    std::string m_name;

protected:
    std::vector<int> m_threadIds;           //存储线程id
    size_t m_threadCount = 0;               //线程数量
    std::atomic<size_t> m_activeThreadCount = {0};         //活跃线程数量
    std::atomic<size_t> m_idleThreadCount = {0};           //空闲线程数量

    bool m_stopping = true;                    //停止状态
    bool m_autoStop = false;                    //是否主动停止
    
    int m_rootThread = 0;                   //主线程id
};

}

#endif
