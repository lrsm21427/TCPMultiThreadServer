#include <bits/stdc++.h>
#include <WINSOCK2.H>
#include <tchar.h>
#include <mutex>

#define BUF_SIZE 64 // ���建������С
std::mutex connectionMutex; // ���ڱ������Ӽ����Ļ�����
int connectionCount = 0; // ��ǰ������

struct ClientInfo {
    SOCKET sclient; // �ͻ����׽���
    sockaddr_in addrClient; // �ͻ��˵�ַ��Ϣ
};

// �̺߳�����������Ӧ�ͻ�������
DWORD WINAPI AnswerThread(LPVOID lparam) {
    auto *clientInfo = (ClientInfo *) lparam; // �Ӳ���ת���ͻ�����Ϣ
    char buf[BUF_SIZE]; // ���ݽ��ջ�����
    int retVal; // ��������ֵ

    // ����������
    connectionMutex.lock();
    connectionCount++;
    printf("Client [%s:%d] connected.\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
           ntohs(clientInfo->addrClient.sin_port), connectionCount);// ��ӡ�ͻ���������Ϣ
    connectionMutex.unlock();

    while (true) {
        ZeroMemory(buf, BUF_SIZE); // ��ջ�����
        retVal = recv(clientInfo->sclient, buf, BUF_SIZE, 0); // ��������
        if (retVal == SOCKET_ERROR) {
            int err = WSAGetLastError(); // ��ȡ������
            if (err != WSAEWOULDBLOCK) { // ���������Ϊ�������������ݿɶ�
                connectionMutex.lock();
                connectionCount--;
                printf("Receive failed with error: %d\n", err);
                printf("Client [%s:%d] disconnected,\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
                       ntohs(clientInfo->addrClient.sin_port), connectionCount);
                connectionMutex.unlock();
                break; // ����ѭ���������߳�
            }
            Sleep(100); // �ȴ����ݵ���
            continue;
        } else if (retVal == 0) { // �ͻ��˹ر�����
            connectionMutex.lock();
            connectionCount--;
            printf("Client disconnected.\n Total connections: %d\n", connectionCount);
            connectionMutex.unlock();
            break; // ����ѭ���������߳�
        }

        buf[retVal] = '\0'; // ȷ���ַ�����null�սᣬ��������
        printf("Received from [%s:%d]: %s\n", inet_ntoa(clientInfo->addrClient.sin_addr),
               ntohs(clientInfo->addrClient.sin_port), buf);; // ��ӡ���յ�������

        if (strcmp(buf, "bye") == 0) { // ����յ�"bye"����
            send(clientInfo->sclient, "bye", 4, 0); // ��ͻ��˷���"bye"
            connectionMutex.lock();
            connectionCount--;
            printf("Client [%s:%d] disconnected,\n Total connections: %d\n", inet_ntoa(clientInfo->addrClient.sin_addr),
                   ntohs(clientInfo->addrClient.sin_port), connectionCount);
            connectionMutex.unlock();
            break; // ����ѭ���������߳�
        } else { // ������Ϣ
            char msg[BUF_SIZE];
            sprintf_s(msg, "Message received: %s", buf); // ׼����Ӧ��Ϣ
            send(clientInfo->sclient, msg, strlen(msg), 0); // ������Ӧ
        }
    }

    closesocket(clientInfo->sclient); // �رտͻ����׽���
    delete clientInfo; // �ͷ��ڴ�
    return 0;
}

// ������
int _tmain(int argc, _TCHAR *argv[]) {
    WSADATA wsd; // WSADATA���ݽṹ�����ڴ洢Windows Sockets��ʼ����Ϣ
    SOCKET sServer; // �������׽���
    int retVal; // ��������ֵ

    // ��ʼ��Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup failed !\n");
        return 1;
    }

    // �����������׽���
    sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sServer == INVALID_SOCKET) {
        printf("Socket failed !\n");
        WSACleanup();
        return -1;
    }

    // ���׽�������Ϊ������ģʽ
    u_long iMode = 1;
    retVal = ioctlsocket(sServer, FIONBIO, &iMode);
    if (retVal == SOCKET_ERROR) {
        printf("Ioctlsocket failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    // ���÷�������ַ��Ϣ����
    sockaddr_in addrServ{};
    addrServ.sin_family = AF_INET;
    int port = 9990;
    addrServ.sin_port = htons(port); // �����˿�
    addrServ.sin_addr.S_un.S_addr = INADDR_ANY; // �����κε�ַ

    retVal = bind(sServer, (sockaddr *) &addrServ, sizeof(addrServ));
    if (retVal == SOCKET_ERROR) {
        printf("Bind failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    // ��ʼ����
    retVal = listen(sServer, SOMAXCONN);
    if (retVal == SOCKET_ERROR) {
        printf("Listen failed !\n");
        closesocket(sServer);
        WSACleanup();
        return -1;
    }

    printf("Server is listening on port %d ...\n", port);

    // ѭ���ȴ��ͻ�������
    while (true) {
        sockaddr_in addrClient{}; // �ͻ��˵�ַ�ṹ
        int addrClientLen = sizeof(addrClient); // �ͻ��˵�ַ����
        SOCKET sClient = accept(sServer, (sockaddr *) &addrClient, &addrClientLen); // ���ܿͻ�������
        if (sClient == INVALID_SOCKET) {
            int err = WSAGetLastError(); // ��ȡ������
            if (err != WSAEWOULDBLOCK) { // ����������Ϊ������ģʽ������������
                printf("Accept failed with error: %d\n", err);
                break; // �������ش���ʱ����ѭ��
            }
            Sleep(100); // ���ݵȴ�������CPU����ʹ��
            continue;
        }
        // Ϊ�¿ͻ��˷����ڴ�
        auto *clientInfo = new ClientInfo; // ʹ��new�����ڴ�
        clientInfo->sclient = sClient; // ����ͻ����׽���
        clientInfo->addrClient = addrClient; // ����ͻ��˵�ַ��Ϣ

        // �����̴߳���ͻ�������
        CreateThread(nullptr, 0, AnswerThread, clientInfo, 0, nullptr);
    }

// ����������������
    closesocket(sServer); // �رշ������׽���
    WSACleanup(); // ����Winsock
    return 0;
}