#ifndef _THREAD_POOL_MANAGER_H_
#define _THREAD_POOL_MANAGER_H_

#include<queue>
#include<vector>
#include<mutex>
#include<thread>
#include<functional>
#include<condition_variable>

//This class manages a thread pool that will process requests
class ThreadPoolManager {
    private:
        /*
         * JOB MANAGEMENT
         */

        //We are going to add the job in the queue for the next process
        //std::queue<std::pair<int, const std::string>> m_jobQueue;
        std::queue<std::tuple<std::function<void(int, std::string)>, int, std::string>> m_jobQueue;

        //Mutex to protect the job queue by locking machanism
        std::mutex m_jobQueueMutex;

        //The threads will use this condition variable to wait until there is job to do so
        std::condition_variable_any m_jobQueueConditionVariable;

        /*
         * THREAD MANAGEMENT
         */
        
        //Maintain a vector (instead of queue, we can use for loop easily) for all created threads, so that later on we can stop them gracefully
        std::vector<std::thread> m_workerThreadsQueue;

        //We will use this to stop each thread of the thread pool. This will flag indicate to thread that need to stop job processing
        bool m_shutdownAllThreads;

        //This is the actual thread, which is going to pick the job from the job queue
        void workerThread();

        //Function to process the job 
        void processTheJob(const std::tuple<std::function<void(int, std::string)>, int, std::string>& job);

    public:
        //Default constructor
        ThreadPoolManager();

        //Destructor
        ~ThreadPoolManager();

        //Copy constructor
        ThreadPoolManager(const ThreadPoolManager&) = delete;

        //Assignment operator
        ThreadPoolManager& operator=(const ThreadPoolManager&) = delete;

        //Move constructor
        ThreadPoolManager(ThreadPoolManager&&) = delete;

        //Move assignment operator
        ThreadPoolManager& operator=(ThreadPoolManager&&) = delete;

        //To add a job into the job queue
        void addJobToQueue(std::function<void(int, std::string)> sendmsgcallback, int fd, std::string msg);
};
#endif
