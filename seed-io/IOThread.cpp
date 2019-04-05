#include "stdafx.h"
#include "IOThread.h"
#include "TCPSocketManager.h"
//#include "TCPSocket.h"

namespace seedio
{
	CIOThread::CIOThread()
	{
	}

	CIOThread::~CIOThread()
	{
	}

	unsigned int CIOThread::Run()
	{
		HANDLE iocpHandle = GetInst(CTCPSocketManager).GetIOCPHandle();
		BOOL bResult = FALSE;
		DWORD dwBytes = 0;
		VOID* completionKey = nullptr;
		OVERLAPPED* overlapped = nullptr;
		seedio::CTCPSocket::TCPSOCKET_EVENT* pTCPSocketEvent = nullptr;

		while (TRUE)
		{
			bResult = GetQueuedCompletionStatus(iocpHandle, &dwBytes,
				(PULONG_PTR)&completionKey, &overlapped, INFINITE);

			if (bResult == TRUE && !completionKey && dwBytes == 0)
			{
				//wprintf(L"!!! SERVER OFF !!!\n");
				return 0;
			}

			pTCPSocketEvent = (seedio::CTCPSocket::TCPSOCKET_EVENT*)overlapped;
			if (pTCPSocketEvent == nullptr)
			{
				//wprintf(L"[ERROR]GQCS pTCPSocketEvent is null\n");
				continue;
			}

			auto sptrTCPSocket = pTCPSocketEvent->wptrTCPSocket.lock();
			if (sptrTCPSocket == nullptr)
				continue;

			CTCPSocket* pTCPSocket = sptrTCPSocket.get();
			if (pTCPSocket == nullptr)
			{
				//wprintf(L"[ERROR]GQCS pTCPSocket is null\n");
				continue;
			}

			CTCPSocket* pTCPSocketServer = (CTCPSocket*)completionKey; // 서버 객체
			if (pTCPSocketServer == nullptr)
			{
				//wprintf(L"[ERROR]GQCS CTCPSocketServer is null");
				continue;
			}

			SOCKET ServerSocket = pTCPSocketServer->GetSocket();

			// bResult : false 비정상접속종료
			// bResult : true 정상접속종료
			if (!bResult || (bResult && dwBytes == 0))
			{
				// 새로운 접속
				if (bResult && pTCPSocketEvent->ioType == seedio::CTCPSocket::IO_ACCEPT)
				{
					// AcceptEx 처리부분에서 이미 iocp와 연결시켜놓음
					// 접속했다고 서버에 알리고 WSARecv 수행
					//wprintf(L"GQCS new socket accept %u\n", pTCPSocket->GetSocket());
					printf("GQCS new socket accept %u\n", pTCPSocket->GetSocket());
					pTCPSocket->Accept(ServerSocket);
				}
				else
				{
					switch (pTCPSocketEvent->ioType)
					{
					case seedio::CTCPSocket::IO_CONNECT:
						break;
					case seedio::CTCPSocket::IO_CLOSE:
						// 접속 종료
						pTCPSocket->Close2(false); // 안에서 OnClose 실행
						break;
					case seedio::CTCPSocket::IO_FORCE_CLOSE:
						// 강제 접속 종료
						pTCPSocket->Close2(true); // 안에서 OnClose 실행
						break;
					default:
						// 유저가 접속 종료 
						// 또는
						// 서버에서 접속종료 처리 후 closesocket 함수 수행
						if (pTCPSocket->IsAliveConnection())
						{
							pTCPSocket->Close(ServerSocket); // 안에서 OnClose 실행
							pTCPSocket->InitAccept(pTCPSocketServer); // 재사용
						}
						break;
					}
				}
			}
			else
			{
				switch (pTCPSocketEvent->ioType)
				{
				case seedio::CTCPSocket::IO_RECV:
					// Recv 함수 안에서 패킷 파싱후 로직큐에 push
					//wprintf(L"GQCS socket IO_READ\n");
					pTCPSocket->Recv(dwBytes, ServerSocket);
					break;
					/*
				case seedio::CTCPSocket::IO_SEND:
					// io send
					wprintf(L"GQCS socket IO_WRITE\n");
					break;
					*/
				default:
					break;
				}
			}
		}

		return 0;
	}
}
