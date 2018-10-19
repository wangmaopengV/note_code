#ifndef __M_EVENT_HPP_
#define __M_EVENT_HPP_

#include <chrono>
#include <mutex>
#include <condition_variable>

namespace m_module_space
{
    enum
    {
        EVENT_FAIL = (-1),
        EVENT_SUCCESS = 0,
        EVENT_TIME_OUT = 1
    };

    class Event
    {
    public:
        Event(bool init_state = false) :
                m_signal(init_state),
                m_blocked(0)
        {

        }

        ~Event() {}

    private:
        Event(const Event &) = delete;

        Event(Event &&) = delete;

        Event &operator=(const Event &) = delete;

    private:
        std::mutex m_mtx;
        std::condition_variable m_cv;
        bool m_signal;
        int m_blocked;

    public:
        int wait(const int time_out_ms = (-1))
        {
            std::unique_lock<std::mutex> ul(m_mtx);

            if (m_signal)
            {
                return EVENT_SUCCESS;
            }
            else
            {
                ++m_blocked;
            }

            if (time_out_ms >= 0)
            {
                std::chrono::milliseconds wait_time_ms(time_out_ms);
                auto result = m_cv.wait_for(ul, wait_time_ms, [&] { return m_signal; });
                --m_blocked;

                if (result)
                {
                    return EVENT_SUCCESS;
                }
                else
                {
                    return EVENT_TIME_OUT;
                }
            }
            else
            {
                m_cv.wait(ul, [&] { return m_signal; });
                --m_blocked;
                return EVENT_SUCCESS;
            }
        }

        void set()
        {
            std::lock_guard<std::mutex> lg(m_mtx);

            if (m_signal)
            {
                return;
            }

            m_signal = true;

            if (m_blocked > 0)
            {
                m_cv.notify_all();
            }
        }

        void reset()
        {
            m_mtx.lock();
            m_signal = false;
            m_mtx.unlock();
        }

    };

}

#endif