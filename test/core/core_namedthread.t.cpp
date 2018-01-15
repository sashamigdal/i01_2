#include <gtest/gtest.h>

#include <unistd.h>

#include <iostream>

#include <i01_core/NamedThread.hpp>

TEST(core_namedthread, core_namedthread_test)
{
    using i01::core::NamedThread;
    class MyThread : public NamedThread<MyThread> {
    public:
        char m_buf[17];
        using NamedThread<MyThread>::NamedThread;

        void *process() override final { ::prctl(PR_GET_NAME, m_buf); std::cout << "Thread: " << name() << "/" << m_buf << "/" << tid() << std::endl; ::usleep(20000); return nullptr; }
    };
    MyThread t1(std::string("MyThread"));
    t1.spawn();
    ::usleep(10000);
    ASSERT_EQ(t1.state(), MyThread::State::ACTIVE);
    ASSERT_EQ(std::string(t1.m_buf), t1.name());
    t1.shutdown(/* blocking = */ true);
    ASSERT_EQ(t1.retval(), (void*)0);

    MyThread t2(std::string("MyThread"));
    t2.spawn();
    ::usleep(1000);
    ASSERT_EQ(t2.state(), MyThread::State::ACTIVE);
    t2.cancel(/* blocking = */ true, 1000);
    ASSERT_EQ(t2.state(), MyThread::State::CANCELED);
    ASSERT_EQ(t2.retval(), PTHREAD_CANCELED);
}
