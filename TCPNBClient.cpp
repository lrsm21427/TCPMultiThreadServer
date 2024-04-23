#include <bits/stdc++.h>
#include <Winsock2.H>
#include <tchar.h>

#pragma comment(lib, "WS2_32.lib")
#define BUF_SIZE    64// ��������С


int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <Server IP> <Port>" << std::endl;
        return 1;
    }
    WSADATA wsd;// ���ڳ�ʼ��Windows Socket
    SOCKET sHost;// �����������ͨ�ŵ��׽���
    SOCKADDR_IN servAddr;// ��������ַ
    char buf[BUF_SIZE];// ���ڽ������ݻ�����
    int retVal; // ���ø���Socket�����ķ���ֵ
    // ��ʼ��Windows Socket
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup failed !\n");
        return 1;
    }
    sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (INVALID_SOCKET == sHost) {
        printf("socket failed !\n");
        WSACleanup();
        return -1;
    }
    // �����׽���Ϊ������ģʽ
    int iMode = 1;
    retVal = ioctlsocket(sHost, FIONBIO, (u_long FAR *) &iMode);
    if (retVal == SOCKET_ERROR) {
        printf("ioctlsocket failed !\n");
        WSACleanup();
        return -1;
    }
    // ���÷�������ַ
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.S_un.S_addr = inet_addr(argv[1]);  // IP��ַ�������л�ȡ
    servAddr.sin_port = htons(static_cast<u_short>(std::atoi(argv[2])));  // �˿ںŴ������л�ȡ
    int sServerAddlen = sizeof(servAddr);// �����ַ�ĳ���
    // ѭ���ȴ�
    while (true) {
        // ���ӷ�����
        Sleep(200);
        retVal = connect(sHost, (LPSOCKADDR) &servAddr, sizeof(servAddr));
        Sleep(200);
        if (SOCKET_ERROR == retVal) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINVAL) // �޷�������ɷ������׽����ϵĲ���
            {
                continue;
            } else if (err == WSAEISCONN)// �ѽ�������
            {
                break;
            } else {
                continue;
            }
        }
    }

    // ���ӳɹ�
    printf("Connect to server successfully !\n");

    // ѭ��������������ַ���������ʾ������Ϣ��
    // ����bye��ʹ�����������˳���ͬʱ�ͻ��˳�������Ҳ���˳�
    while (true) {
        // ���������������
        printf("Please input a string to send: ");
        // �������������
        std::string str;
        std::getline(std::cin, str);
        // ���û���������ݸ��Ƶ�buf��
        ZeroMemory(buf, BUF_SIZE);
        strcpy_s(buf, str.c_str());
        // ѭ���ȴ�
        while (true) {
            // ���������������
            retVal = send(sHost, buf, strlen(buf), 0);
            if (SOCKET_ERROR == retVal) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK)            // �޷�������ɷ������׽����ϵĲ���
                {
                    Sleep(500);
                    continue;
                } else {
                    printf("send failed !\n");
                    closesocket(sHost);
                    WSACleanup();
                    return -1;
                }
            }
            break;
        }

        while (true) {
            ZeroMemory(buf, BUF_SIZE);// ��ս������ݵĻ�����
            retVal = recv(sHost, buf, sizeof(buf) + 1, 0);// ���շ������ش�������
            if (SOCKET_ERROR == retVal) {
                int err = WSAGetLastError();// ��ȡ�������
                if (err == WSAEWOULDBLOCK)// �������ݻ�������������
                {
                    Sleep(100);
                    printf("waiting back msg......\n");
                    continue;
                } else if (err == WSAETIMEDOUT || err == WSAENETDOWN) {
                    printf("Receive failed !\n");
                    closesocket(sHost);
                    WSACleanup();
                    return -1;
                }
                break;
            }
            break;
        }

        printf("Receive From Server: %s\n", buf);
        // ����յ�bye�����˳�
        if (strcmp(buf, "bye") == 0) {
            printf("bye!\n");
            break;
        }
    }
    // �ͷ���Դ
    closesocket(sHost);
    WSACleanup();


    return 0;
}