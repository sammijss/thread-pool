#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include"ThreadPoolManager.hpp"

ThreadPoolManager::ThreadPoolManager() : m_shutdownAllThreads(false) {
    //Number of concurrent threads supported. If the value is not well defined or not computable, returns 0
    auto maxthreads = std::thread::hardware_concurrency();
    if (maxthreads == 0) {
        maxthreads = 1;
    }

    std::cout <<"Number of threads created in the pool: " <<maxthreads <<std::endl;
    for (unsigned i = 1; i <= maxthreads; ++i) {
        //std::thread will execute the function `workerThread` as a thread.
        m_workerThreadsQueue.push_back(std::thread(&ThreadPoolManager::workerThread, this));
    }
}

ThreadPoolManager::~ThreadPoolManager() {
    //Shutdown all the threads
    m_shutdownAllThreads = true;

    //Notify all the thread that we have shutdown you, now finish your job
    m_jobQueueConditionVariable.notify_all();
    for (auto& thread : m_workerThreadsQueue) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPoolManager::addJobToQueue(std::function<void(int, std::string)> sendmsgcallback, int fd, std::string msg) {
    //Take a lock on the job queue using mutex
    std::lock_guard<std::mutex> lock(m_jobQueueMutex);

    //Push the job to the queue
    m_jobQueue.push(std::tuple<std::function<void(int, std::string)>, int, std::string>(sendmsgcallback, fd, msg));

    //Notify one of the threads that there is aleast one job to process
    m_jobQueueConditionVariable.notify_one();
}

void ThreadPoolManager::workerThread() {
    //Loop for any assigned job
    while (!m_shutdownAllThreads) {
        std::tuple<std::function<void(int, std::string)>, int, std::string> job;

        //Local scope to release the lock automatically
        {
            std::unique_lock<std::mutex> lock(m_jobQueueMutex);
            m_jobQueueConditionVariable.wait(lock, [&]{
                    //Check for the condition to wake up
                    return (!m_jobQueue.empty() || m_shutdownAllThreads);
                    });
            job = m_jobQueue.front(); m_jobQueue.pop();
        }

        //Process the job now
        std::cout <<"The thread with id: " <<std::this_thread::get_id() <<" is processing the client request." <<std::endl;
        processTheJob(job);
    }
}

void ThreadPoolManager::processTheJob(const std::tuple<std::function<void(int, std::string)>, int, std::string>& job) {
    //Pretend to do somework, otherwise it will take no time to finish the job and in that case thread might not be busy and no job will assign to other threads
    std::cout <<"Pretending to do some work for 5 seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    //Consume the msg received from the client
    std::cout <<"Client Msg: " <<std::get<2>(job) <<std::endl;

    //Send the response to the client
    auto sendmessagecallback = std::get<0>(job);
    sendmessagecallback(std::get<1>(job), "Hello Client...");
}
