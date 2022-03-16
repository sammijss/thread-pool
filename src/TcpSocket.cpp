#include<tuple>
#include<iostream> 
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include"TcpSocket.hpp"
#include"Exception.hpp"
#include"ThreadPoolManager.hpp"

TcpSocket::TcpSocket(const char* ip,
        const char* port) : 
    m_ip(ip),
    m_port(port),
    m_listenFd(-1)
{
    /* Initialize the FD sets */
    FD_ZERO(&m_masterFdSet);
}

TcpSocket::~TcpSocket()
{
    StopServer();
}

void TcpSocket::StartServer()
{
    try {
        /* Create a socket for the server */
        if ((m_listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            Exception("Error: Failed to open a socket for the TcpSocket server: " + std::string(strerror(errno)));
        }

        /* Set the socket option SO_REUSEADDR to reuse the same port if port is in EBUSY state */
        int option = 1;
        if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            Exception("Error: Failed to set the option SO_REUSEADDR on the socket: " + std::string(strerror(errno)));
        }

        /* Assign the values to the server's socket structure */
        struct sockaddr_in serverSocketAddress;
        memset(&serverSocketAddress, 0, sizeof(serverSocketAddress));

        serverSocketAddress.sin_family = AF_INET;
        //serverSocketAddress.sin_addr.s_addr = INADDR_ANY;   //Let the application choose the ip address

        /* Assign the given ip address to the server */
        if (inet_pton(AF_INET, (const char*)m_ip, (void*)&(serverSocketAddress.sin_addr.s_addr)) <= 0) {
            Exception("Error: Failed to assign ip address to server: " + std::string(strerror(errno)));
        }

        serverSocketAddress.sin_port = htons(atoi(m_port));

        /* Bind the server socket to the specified port number and ip address */
        if (bind(m_listenFd, (struct sockaddr *) &serverSocketAddress, sizeof(serverSocketAddress)) < 0) {
            Exception("Error: Failed to bind the socket provided ip and port: " + std::string(strerror(errno)));
        }

        /* Start the listening of the incoming client connection with specified backlog */
        /* SOMAXCONN is defined in <sys/socket.h> */
        if (listen(m_listenFd, SOMAXCONN) < 0) {
            Exception("Error: Failed to listen on created socket for server: " + std::string(strerror(errno)));
        }
        std::cout <<"TcpSocket Server is stated at [" <<m_ip <<":" <<m_port <<"]" <<std::endl;

        /* This class manage the pool of threads */
        ThreadPoolManager threadPoolManager;

        /* Use this structure to know the client info */
        struct sockaddr clientSocketAddress;
        socklen_t clientSocketAddresslen = sizeof(clientSocketAddress);

        /* Define & initialize the FD sets for I/O multiplexing */
        fd_set readyFdSet;
        FD_ZERO(&readyFdSet);

        FD_SET(m_listenFd, &m_masterFdSet);

        /* The maximum FD allocated by the system, which is assigned to m_listenFd */
        int maxFd = m_listenFd;

        while(true) {
            /* Local scope for synchronization */
            {
                std::unique_lock<std::mutex> lock(m_m_masterFdSetMutex);
                readyFdSet = m_masterFdSet;
            }

            /* Let's do I/O multiplexing */
            int retval = select((maxFd + 1), &readyFdSet, NULL, NULL, NULL);
            if(retval < 0)
            {
                std::cerr <<"Error: select() failed, retval: " <<retval <<", error: " <<strerror(errno) <<std::endl;
                continue;
            }
            if(retval == 0)
            {
                std::cerr <<"Error: select() timeout, retval: " <<retval <<", error: " <<strerror(errno) <<std::endl;
                continue;
            }

            /* TODO: We can improve this by looping only for the actively used FDs for this server */
            for(int fd = 0; fd <= maxFd; fd++)
            {
                if(FD_ISSET(fd, &readyFdSet))
                {
                    if(fd == m_listenFd)
                    {
                        /* Accept a new connection */
                        memset(&clientSocketAddress, 0, sizeof(clientSocketAddress));
                        int connectionFd = accept(m_listenFd, (struct sockaddr*)&clientSocketAddress, (socklen_t*)&clientSocketAddresslen);
                        if(connectionFd < 0)
                        {
                            std::cerr <<"Error: accept() timeout, connectionFd: " <<connectionFd <<", error: " <<strerror(errno) <<std::endl;
                            continue;
                        }
                        else
                        {
                            /* Local scope for synchronization */
                            {
                                std::unique_lock<std::mutex> lock(m_m_masterFdSetMutex);
                                /* Add the new descriptor to the set */
                                FD_SET(connectionFd , &m_masterFdSet);
                            }
                            if(connectionFd > maxFd)
                            {
                                /* Reset the maxFD */
                                maxFd = connectionFd;
                            }
                        }
                    }
                    else
                    {
                        /* Receive the msg from the client */
                        std::string msg = TcpSocket::ReceiveMessage(fd);
                        if (msg.length()) {
                            /* Post the msg to the msg pool */
                            threadPoolManager.addJobToQueue(std::bind(&TcpSocket::SendMessage, this, std::placeholders::_1, std::placeholders::_2), fd, msg);
                        }
                    }
                }
            }
        }
    } catch(const Exception& exception) {
        std::cerr <<exception.what() <<std::endl;
        StopServer();
        exit(EXIT_FAILURE); 
    } catch(...) {
        std::cerr <<"Error: Failed with undefined exception." <<std::endl;
        StopServer();
        exit(EXIT_FAILURE); 
    }
}

void TcpSocket::StopServer()
{
    /* Close the listening FD of the socket */
    if (m_listenFd != -1) {
        close(m_listenFd);
    }
}

std::string TcpSocket::ReceiveMessage(int fd)
{
    /* Remark: We should use recv() to receive byte by byte msg because TCP is a STREAM protocol */
    char buffer[MSG_LENGTH];
    memset(buffer, 0, sizeof(buffer));
    if (read(fd, buffer, sizeof(buffer)) < 0) {
        std::cerr <<"Error: read() failed, fd: " <<fd <<", error: " <<strerror(errno) <<std::endl;
        return(std::string());
    }
    return(std::string(buffer));
}

void TcpSocket::SendMessage(int fd, std::string msg)
{
    /* Send the message to the client */
    int retval = send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    if (retval < 0) {
        std::cerr <<"Error: send() failed, fd: " <<fd <<", error: " <<strerror(errno) <<std::endl;
        return;
    }

    /* Close the connection */
    close(fd);

    /* Local scope for synchronization */
    {
        std::unique_lock<std::mutex> lock(m_m_masterFdSetMutex);
        FD_CLR(fd, &m_masterFdSet);
    }
}
