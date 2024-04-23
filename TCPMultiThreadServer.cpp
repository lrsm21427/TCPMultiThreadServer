#include <bits/stdc++.h>
#include <WINSOCK2.H>
#include <tchar.h>
#include <mutex>

#define BUF_SIZE 64 // 定义缓冲区大小
std::mutex connectionMutex; // 用于保护连接计数的互斥锁
int connectionCount = 0; // 当前连接数

struct ClientInfo {
    SOCKET sclient; // 客户端套接字
    sockaddr_in addrClient; // 客户端地址信息
};

// 线程函数，用于响应客户端请求
DWORD WINAPI AnswerThread(LPVOID lparam) {
    auto *clientInfo = (ClientInfo *) lparam; // 从参数转换客户端信息
    char buf[BUF_SIZE]; // 数据接收缓冲区
    int retVal; // 函数返回值

    // 增加连接数
    connectionMutex.lock();
    connectionCount++;
    printf("Client [%s:%d] connected.\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
           ntohs(clientInfo->addrClient.sin_port), connectionCount);// 打印客户端连接信息
    connectionMutex.unlock();

    while (true) {
        ZeroMemory(buf, BUF_SIZE); // 清空缓冲区
        retVal = recv(clientInfo->sclient, buf, BUF_SIZE, 0); // 接收数据
        if (retVal == SOCKET_ERROR) {
            int err = WSAGetLastError(); // 获取错误码
            if (err != WSAEWOULDBLOCK) { // 如果不是因为非阻塞而无数据可读
                connectionMutex.lock();
                connectionCount--;
                printf("Receive failed with error: %d\n", err);
                printf("Client [%s:%d] disconnected,\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
                       ntohs(clientInfo->addrClient.sin_port), connectionCount);
                connectionMutex.unlock();
                break; // 跳出循环，结束线程
            }
            Sleep(100); // 等待数据到来
            continue;
        } else if (retVal == 0) { // 客户端关闭连接
            connectionMutex.lock();
            connectionCount--;
            printf("Client disconnected.\n Total connections: %d\n", connectionCount);
            connectionMutex.unlock();
            break; // 跳出循环，结束线程
        }

        buf[retVal] = '\0'; // 确保字符串以null终结，避免乱码
        printf("Received from [%s:%d]: %s\n", inet_ntoa(clientInfo->addrClient.sin_addr),
               ntohs(clientInfo->addrClient.sin_port), buf);; // 打印接收到的数据

        if (strcmp(buf, "bye") == 0) { // 如果收到"bye"命令
            send(clientInfo->sclient, "bye", 4, 0); // 向客户端发送"bye"
            connectionMutex.lock();
            connectionCount--;
            printf("Client [%s:%d] disconnected,\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
                   ntohs(clientInfo->addrClient.sin_port), connectionCount);
            connectionMutex.unlock();
            break; // 跳出循环，结束线程
        } else { // 其他消息
            char msg[BUF_SIZE];
            sprintf_s(msg, "Message received: %s", buf); // 准备响应消息
            send(clientInfo->sclient, msg, strlen(msg), 0); // 发送响应
        }
    }

    closesocket(clientInfo->sclient); // 关闭客户端套接字
    delete clientInfo; // 释放内存
    return 0;
}

// 主函数
int _tmain(int argc, _TCHAR *argv[]) {
    WSADATA wsd; // WSADATA数据结构，用于存储Windows Sockets初始化信息
    SOCKET sServer; // 服务器套接字
    int retVal; // 函数返回值

    // 初始化Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup failed !\n");
        return 1;
    }

    // 创建服务器套接字
    sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sServer == INVALID_SOCKET) {
        printf("Socket failed !\n");
        WSACleanup();
        return -1;
    }

    // 将套接字设置为非阻塞模式
    u_long iMode = 1;
    retVal = ioctlsocket(sServer, FIONBIO, &iMode);
    if (retVal == SOCKET_ERROR) {
        printf("Ioctlsocket failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    // 设置服务器地址信息并绑定
    sockaddr_in addrServ{};
    addrServ.sin_family = AF_INET;
    int port = 9990;
    addrServ.sin_port = htons(port); // 监听端口
    addrServ.sin_addr.S_un.S_addr = INADDR_ANY; // 监听任何地址

    retVal = bind(sServer, (sockaddr *) &addrServ, sizeof(addrServ));
    if (retVal == SOCKET_ERROR) {
        printf("Bind failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    // 开始监听
    retVal = listen(sServer, SOMAXCONN);
    if (retVal == SOCKET_ERROR) {
        printf("Listen failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    printf("Server is listening on port %d ...\n", port);

    // 循环等待客户端连接
    while (true) {
        sockaddr_in addrClient{}; // 客户端地址结构
        int addrClientLen = sizeof(addrClient); // 客户端地址长度
        SOCKET sClient = accept(sServer, (sockaddr *) &addrClient, &addrClientLen); // 接受客户端连接
        if (sClient == INVALID_SOCKET) {
            int err = WSAGetLastError(); // 获取错误码
            if (err != WSAEWOULDBLOCK) { // 检查错误不是因为非阻塞模式下无连接请求
                printf("Accept failed with error: %d\n", err);
                break; // 出现严重错误时跳出循环
            }
            Sleep(100); // 短暂等待，避免CPU过度使用
            continue;
        }
        // 为新客户端分配内存
        auto *clientInfo = new ClientInfo; // 使用new分配内存
        clientInfo->sclient = sClient; // 保存客户端套接字
        clientInfo->addrClient = addrClient; // 保存客户端地址信息

        // 创建线程处理客户端请求
        CreateThread(nullptr, 0, AnswerThread, clientInfo, 0, nullptr);
    }

// 服务结束后的清理工作
    closesocket(sServer); // 关闭服务器套接字
    WSACleanup(); // 清理Winsock
    return 0;
}