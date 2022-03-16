#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H

#include<mutex>
#include<string>

#define MSG_LENGTH  100 /* BYTES */

class TcpSocket {
    private:
        const char* m_ip;       /* Server IP Address */
        const char* m_port;     /* Server Port */  
        int m_listenFd;         /* FD to listen incoming connections */
        fd_set m_masterFdSet;   /* Master FD set for I/O multiplexing */
        std::mutex m_m_masterFdSetMutex;

    public:
        TcpSocket(const char* ip, const char* port);
        virtual ~TcpSocket();

        TcpSocket(const TcpSocket&) = delete;               /* Copy constructor */
        TcpSocket& operator=(const TcpSocket&) = delete;    /* Assignment operator */

        TcpSocket(TcpSocket&&) = delete;                    /* Move constructor */
        TcpSocket& operator=(TcpSocket&&) = delete;         /* Move assignment operator */

        void StartServer();
        void StopServer();

        std::string ReceiveMessage(int fd);
        void SendMessage(int fd, std::string msg);
};
#endif
