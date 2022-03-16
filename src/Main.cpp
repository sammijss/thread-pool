#include<iostream>
#include"TcpSocket.hpp"

/*
 * Help: How to use this application?
 */
static void HELP(const char* programname) {
    std::cerr << 
    /* 0 */     "Usage: " <<programname <<" "
    /* 1 */     "<ip> "
    /* 2 */     "<port> "
              <<std::endl;
}

/*
 * Entry point of the program
 */
int main(int argc, const char* argv[])
{
    if ((argc != 3)) {
        HELP(argv[0]); return(EXIT_FAILURE);
    }
    TcpSocket server(argv[1], argv[2]);
    server.StartServer();
    return (0);
}
