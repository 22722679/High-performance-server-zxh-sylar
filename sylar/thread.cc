#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

static thread_local Thread* t_thread = nullptr;     //线程局部变量，指向当前线程
static thread_local std::string t_thread_name = "UNKONW";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

//生产消费模型
Semaphore::Semaphore(uint32_t count){
    if(sem_init(&m_semaphore,0,count)) {
        throw std::logic_error("sem_init error");
    }
}
Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}
void Semaphore::wait(){
    if(sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}
const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(name.empty()){
        return ;
    }
    if(t_thread) {
        t_thread->m_name =name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()){
        m_name = "UNKNOW";
    } 
    int rt = pthread_create(&m_thread,nullptr,&Thread::run,this);        //创建线程
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt = " << rt
            << "name = " << name;
            throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}
Thread::~Thread(){
    if(m_thread){
        //设置线程为分离属性
        pthread_detach(m_thread);
    }   
}

void Thread::join(){
    if(m_thread){
        int rt = pthread_join(m_thread,nullptr);
        if(rt){
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt = " << rt
                << "name = " << m_name;
                throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());        //给线程命名

    std::function<void()> cb;       //防止函数里的引用和指针没有被释放掉
    cb.swap(thread->m_cb);

    thread->m_semaphore.notify();

    cb();
    return 0;
}



}