#ifndef __M_PROCESSOR_HPP_
#define __M_PROCESSOR_HPP_

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <thread>
#include <chrono>
#include <functional>

#include "msemphore.hpp"
#include "mevent.hpp"

namespace m_module_space
{

#define thread_sleep_s(s) std::this_thread::sleep_for(std::chrono::seconds(s))
#define thread_sleep_ms(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
#define thread_sleep_us(us) std::this_thread::sleep_for(std::chrono::microseconds(us))
#define thread_sleep_ns(ns) std::this_thread::sleep_for(std::chrono::nanoseconds(ns))

    class ThreadWrapper
    {
    public:
        ThreadWrapper() : m_sp_thread(nullptr), m_init_flag(0), m_quit_flag(0) {}

        ~ThreadWrapper()
        {
            /// 线程信息类析构前，用户应停止线程，这里的代码是为异常情况准备的
            if (m_sp_thread != nullptr && m_sp_thread->joinable())
            {
                set_quit_flag();
                std::this_thread::yield();
                m_sp_thread->detach(); /// 不要使用 join()
                m_sp_thread.reset();
            }
        }

    private:
        ThreadWrapper(const ThreadWrapper &) = delete;

        ThreadWrapper &operator=(const ThreadWrapper &) = delete;

    public:
        std::shared_ptr<std::thread> m_sp_thread; /// C++11线程类
        volatile int m_init_flag; /// 在创建者与线程之间同步线程信息
        volatile int m_quit_flag; /// 线程退出标志

    public:
        /// 线程退出
        inline void set_quit_flag()
        {
            m_quit_flag = 1;
        }

        /// 判断线程是否退出
        inline bool is_thread_quit() const
        {
            return m_quit_flag != 0;
        }
    };


    using SP_THREAD_WRAPPER = std::shared_ptr<ThreadWrapper>;

    enum
    {
        PROCESSOR_FAIL = -1,
        PROCESSOR_SUCCESS = 0,
        PROCESSOR_TIME_OUT = 1,
        PROCESSOR_QUEUE_FULL = 2,
        PROCESSOR_QUEUE_EMPTY = 3
    };

    template<typename T>
    class Processor
    {
    public:
        Processor() {}

        virtual ~Processor() { end_all_threads(); }

    private:
        Processor(const Processor &) = delete;

        Processor &operator=(const Processor &) = delete;

    protected:
        /// 任务队列
        std::list<std::shared_ptr<T>> m_task_list;
        std::mutex m_task_lock;
        volatile int m_task_size = 0;
        Semphore m_task_semphore;
        volatile int m_task_max_count = 1024;
        volatile int m_task_list_full_flag = 0;

        /// 线程队列
        std::list<SP_THREAD_WRAPPER> m_thread_list;
        std::mutex m_thread_lock;
        volatile int m_thread_max_count = 1024;
        volatile int m_thread_timeout_ms = 10;

        /// 流水线队列
        std::list<Processor<T> *> m_next_processors;
        int m_processor_id = 0;
        std::string m_processor_name;

        /// 获取任务batch
        int m_batch_number = 1;

    protected:
        /// 处理任务
        virtual void handle_task(std::list<std::shared_ptr<T>> &tasks)
        {
            return;
        }

        /// 任务队列溢出报警
        virtual void report_queue_full()
        {
            return;
        }

        /// 任务队列反溢出报警
        virtual void report_queue_changed_to_not_full()
        {
            return;
        }

        /// 读取任务超时报警
        virtual void handle_timeout()
        {
            return;
        }

    public:
        /// 设置单元id
        inline void set_processor_id(int id)
        {
            m_processor_id = id;
        }

        /// 读取单元id
        inline int get_processor_id()
        {
            return m_processor_id;
        }

        /// 设置单元name
        inline void set_processor_name(std::string name)
        {
            m_processor_name = name;
        }

        /// 读取单元name
        inline std::string get_processor_id()
        {
            return m_processor_name;
        }

    protected:
        /// 任务流水线传递
        virtual void fan_out(std::list<std::shared_ptr<T>> &task)
        {
            for (auto &cur_processor : this->m_next_processors)
            {
                cur_processor->push_task(task);
            }
        }

        /// 添加流水线单元
        int add_processor(Processor<T> *processor)
        {
            if (processor == nullptr)
            {
                return PROCESSOR_FAIL;
            }

            for (auto &cur_processor : m_next_processors)
            {
                if (cur_processor == processor)
                {
                    return PROCESSOR_FAIL;
                }
            }

            m_next_processors.push_back(processor);
            return PROCESSOR_SUCCESS;
        }

    public:
        /// 启动线程
        virtual int begin_thread(const int count = 1)
        {
            if (count <= 0)
            {
                return 0;
            }

            std::lock_guard<std::mutex> auto_lock(m_thread_lock);

            int cur_count = static_cast<int>(m_thread_list.size());
            int create_count = 0;

            for (; create_count < count;)
            {
                if (cur_count >= m_thread_max_count)
                {
                    break;
                }

                try
                {
                    auto sp_thread_wrapper(std::make_shared<ThreadWrapper>());

                    sp_thread_wrapper->m_sp_thread = std::make_shared<std::thread>(
                            [this](SP_THREAD_WRAPPER sp_thread_wrapper) -> void {

                                if (sp_thread_wrapper == nullptr)
                                {
                                    return;
                                }

                                /// 等待线程创建成功
                                while (sp_thread_wrapper->m_init_flag == 0)
                                {
                                    std::this_thread::yield();
                                }

                                std::list<std::shared_ptr<T>> tasks;
                                int result = 0;

                                while (true)
                                {
                                    result = this->pop_task(tasks, this->m_thread_timeout_ms);

                                    if (sp_thread_wrapper->is_thread_quit())
                                    {
                                        break;
                                    }

                                    if (result == PROCESSOR_SUCCESS)
                                    {
                                        this->handle_task(tasks);
                                        this->fan_out(tasks);
                                        tasks.clear();
                                    }
                                    else if (result == PROCESSOR_TIME_OUT)
                                    {
                                        this->handle_timeout();
                                    }
                                    else
                                    {
                                        ///异常
                                    }
                                }

                                if (!sp_thread_wrapper->is_thread_quit())
                                {
                                    if (sp_thread_wrapper->m_sp_thread != nullptr)
                                    {
                                        sp_thread_wrapper->m_sp_thread->detach();
                                    }

                                    this->remove_thread_wrapper(sp_thread_wrapper);
                                }

                            }, sp_thread_wrapper);

                    sp_thread_wrapper->m_init_flag = 1024;
                    std::this_thread::yield();

                    m_thread_list.emplace_back(sp_thread_wrapper);

                    ++cur_count;
                    ++create_count;
                }
                catch (...)
                {
                    return create_count;
                }
            }

            return create_count;
        }

    public:
        /// 删除线程
        void remove_thread_wrapper(const SP_THREAD_WRAPPER &sp_thread_wrapper)
        {
            std::lock_guard<std::mutex> auto_lock(m_thread_lock);

            for (auto itr = m_thread_list.begin(); itr != m_thread_list.end(); ++itr)
            {
                if ((*itr) == sp_thread_wrapper)
                {
                    m_thread_list.erase(itr);
                    return;
                }
            }
        }

        /// 暂停一个线程
        void end_one_thread(SP_THREAD_WRAPPER &sp_target, bool sync = true)
        {
            if (sp_target != nullptr && sp_target->m_sp_thread != nullptr && sp_target->m_sp_thread->joinable())
            {
                sp_target->set_quit_flag();
                std::this_thread::yield();

                if (sync)
                {
                    sp_target->m_sp_thread->join();
                }
                else
                {
                    sp_target->m_sp_thread->detach();
                }

                sp_target->m_sp_thread.reset();
            }
        }

        /// 暂停所有线程
        void end_all_threads(bool sync = true)
        {
            std::unique_lock<std::mutex> auto_lock(m_thread_lock);

            if (m_thread_list.empty())
            {
                return;
            }

            auto temp_thread_list(std::move(m_thread_list)); ///取出所有线程
            auto_lock.unlock(); /// 手动解锁，防止join()时死锁

            /// 停止这些线程
            for (auto &sp_target : temp_thread_list)
            {
                end_one_thread(sp_target, sync);
            }
        }

    public:
        virtual int
        push_task(std::list<std::shared_ptr<T>> &task, int *p_new_size = nullptr, int wait_time_on_queue_full = 0)
        {
            auto task_size = task.size();
            if (task_size == 0)
            {
                return PROCESSOR_FAIL;
            }

            std::lock_guard<std::mutex> auto_lock(m_task_lock);

            if (m_task_size + task_size > m_task_max_count)
            {
                if (m_task_list_full_flag == 0)
                {
                    m_task_list_full_flag = 1;
                    report_queue_full();
                }

                if (p_new_size != nullptr)
                {
                    *p_new_size = m_task_size;
                }

                return PROCESSOR_QUEUE_FULL;
            }
            else
            {
                auto task_copy(task);
                m_task_list.splice(m_task_list.end(), task_copy);
                m_task_size += task_size;
                m_task_semphore.signal(task_size);

                if (p_new_size != nullptr)
                {
                    *p_new_size = m_task_size;
                }

                return PROCESSOR_SUCCESS;
            }
        }

        virtual int push_task(std::shared_ptr<T> &sp_task, int *p_new_size = nullptr, int wait_time_on_queue_full = 0)
        {
            std::lock_guard<std::mutex> auto_lock(m_task_lock);

            if (m_task_size >= m_task_max_count)
            {
                if (m_task_list_full_flag == 0)
                {
                    m_task_list_full_flag = 1;
                    report_queue_full();
                }

                if (p_new_size != nullptr)
                {
                    *p_new_size = m_task_size;
                }

                return PROCESSOR_QUEUE_FULL;
            }
            else
            {
                m_task_list.emplace_back(sp_task);
                ++m_task_size;
                m_task_semphore.signal();

                if (p_new_size != nullptr)
                {
                    *p_new_size = m_task_size;
                }

                return PROCESSOR_SUCCESS;
            }
        }


        int pop_task(std::list<std::shared_ptr<T>> &task, const int ms = (-1), int *p_new_size = nullptr)
        {
            if (m_task_semphore.wait(ms, m_batch_number) != SEMPHORE_SUCCESS)
            {
                // return PROCESSOR_TIME_OUT;
            }

            std::lock_guard<std::mutex> auto_lock(m_task_lock);

            if (m_task_list_full_flag != 0)
            {
                m_task_list_full_flag = 0;
                report_queue_changed_to_not_full();
            }

            int i = 0;
            for (; i < m_batch_number; ++i)
            {
                if (m_task_list.empty())
                {
                    if (p_new_size != nullptr)
                    {
                        *p_new_size = 0;
                    }

                    break;
                }

                std::shared_ptr<T> sp_task = std::move(m_task_list.front());
                m_task_list.pop_front();
                task.emplace_back(sp_task);
            }

            m_task_semphore.unsignal(i);
            m_task_size -= i;

            if (p_new_size != nullptr)
            {
                *p_new_size = m_task_size;
            }

            return PROCESSOR_SUCCESS;
        }
    };

}

#endif