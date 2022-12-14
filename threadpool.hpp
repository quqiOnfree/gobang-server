#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <stdexcept>
#include <climits>

#define  THREADPOOL_MAX_NUM LLONG_MAX
#define  THREADPOOL_AUTO_GROW

namespace std
{
//线程池最大容量,应尽量设小一点

class threadpool
{
    using Task = function<void()>;    //定义类型
    vector<thread> _pool;     //线程池
    queue<Task> _tasks;            //任务队列
    mutex _lock;                   //同步
    condition_variable _task_cv;   //条件阻塞
    atomic<bool> _run{ true };     //线程池是否执行
    atomic<size_t>  _idlThrNum{ 0LL };  //空闲线程数量
    atomic<bool> _joinable{ true };

public:
    inline threadpool(size_t size = 4) { addThread(size); }
    inline ~threadpool()
    {
        _run=false;
        _task_cv.notify_all(); // 唤醒所有线程执行
        for (thread& thread : _pool) {
            //thread.detach(); // 让线程“自生自灭”
            if(thread.joinable())
                thread.join(); // 等待任务结束， 前提：线程一定会执行完
        }
    }

public:
    // 提交一个任务
    // 调用.get()获取返回值会等待任务执行完,获取返回值
    // 有两种方法可以实现调用类成员，
    // 一种是使用   bind： .commit(std::bind(&Dog::sayHello, &dog));
    // 一种是用   mem_fn： .commit(std::mem_fn(&Dog::sayHello), this)
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>
    {
        if (!_run)    // stoped ??
            throw runtime_error("commit on ThreadPool is stopped.");

        using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
        auto task = make_shared<packaged_task<RetType()>>(
            bind(forward<F>(f), forward<Args>(args)...)
        ); // 把函数入口及参数,打包(绑定)
        future<RetType> future = task->get_future();
        {    // 添加任务到队列
            lock_guard<mutex> lock{ _lock };//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
            _tasks.emplace([task](){ // push(Task{...}) 放到队列后面
                (*task)();
            });
        }
#ifdef THREADPOOL_AUTO_GROW
        if (_idlThrNum < 1 && _pool.size() < THREADPOOL_MAX_NUM)
            addThread(1);
#endif // !THREADPOOL_AUTO_GROW
        _task_cv.notify_one(); // 唤醒一个线程执行

        return future;
    }

    //空闲线程数量
    size_t idlCount() { return _idlThrNum; }
    //线程数量
    size_t thrCount() { return _pool.size(); }

    bool joinable()
    {
        return _joinable;
    }

    void join()
    {
        if (!_joinable)
        {
            throw runtime_error("can't join!");
        }
        _joinable = false;
        for (thread& thread : _pool) {
            //thread.detach(); // 让线程“自生自灭”
            if (thread.joinable())
                thread.join(); // 等待任务结束， 前提：线程一定会执行完
        }
    }

    void detach()
    {
        _joinable = false;
        for (thread& thread : _pool) {
            thread.detach(); // 让线程“自生自灭”
            if (thread.joinable())
                thread.join(); // 等待任务结束， 前提：线程一定会执行完
        }
    }

#ifndef THREADPOOL_AUTO_GROW
private:
#endif // !THREADPOOL_AUTO_GROW
    //添加指定数量的线程
    void addThread(size_t size)
    {
        for (; _pool.size() < THREADPOOL_MAX_NUM && size > 0; --size)
        {   //增加线程数量,但不超过 预定义数量 THREADPOOL_MAX_NUM
            _pool.emplace_back( [this]{ //工作线程函数
                while (_run)
                {
                    Task task; // 获取一个待执行的 task
                    {
                        // unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
                        unique_lock<mutex> lock{ _lock };
                        _task_cv.wait(lock, [this]{
                                return !_run || !_tasks.empty();
                        }); // wait 直到有 task
                        if (!_run && _tasks.empty())
                            return;
                        task = move(_tasks.front()); // 按先进先出从队列取一个 task
                        _tasks.pop();
                    }
                    _idlThrNum--;
                    task();//执行任务
                    _idlThrNum++;
                }
            });
            _idlThrNum++;
        }
    }
};

}

#endif  //https://github.com/lzpong/