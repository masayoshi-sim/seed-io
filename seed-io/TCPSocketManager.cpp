#include "stdafx.h"
#include "TCPSocket.h"
#include "TCPSocketManager.h"

namespace seedio
{
	CTCPSocketManager::CTCPSocketManager()
	{
		//printf("%s\n", __FUNCDNAME__);
		Initialize();

		// 워커쓰레드 생성
		CreateWorkThread();
	}

	CTCPSocketManager::~CTCPSocketManager()
	{
		//printf("%s\n", __FUNCDNAME__);
		//printf("SERVER END wait please...\n");
		Release();
		RemoveSocketPool();
		WSACleanup();
	}

	bool CTCPSocketManager::Initialize()
	{
		WSADATA wsaData;
		memset(&wsaData, 0, sizeof(wsaData));

		if (LOBYTE(wsaData.wVersion) != 2)
		{
			int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (iResult)
				return 0;

			if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
			{
				// error
				WSACleanup();
				return 0;
			}
		}

		SYSTEM_INFO si;
		memset(&si, 0, sizeof(si));
		GetSystemInfo(&si);
		int iWorkerThreadCount = si.dwNumberOfProcessors;

		// IOCP 핸들 생성
		m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, iWorkerThreadCount);
		if (m_IOCPHandle == nullptr)
		{
			return false;
		}		

		return true;
	}

	void CTCPSocketManager::Release()
	{
		for(int i = 0; i < m_vecThread.size(); i++)
		{
			PostQueuedCompletionStatus(m_IOCPHandle, 0, 0, NULL);
		}

		// 워커쓰레드 delete
		for(int i = 0; i < m_vecThread.size(); i++)
		{
			CIOThread* pThread = m_vecThread[i];
			if (pThread == nullptr)
				continue;

			pThread->Wait();//2000);
			//thread->EndThread(0);
			SAFE_DELETE(pThread);
		}
		m_vecThread.clear();

		// iocp 핸들 close
		if (m_IOCPHandle)
		{
			::CloseHandle(m_IOCPHandle);
			m_IOCPHandle = INVALID_HANDLE_VALUE;
		}

		//wprintf(L"close server\n");
	}

	// 소켓 리스트에 등록
	// accept 후 실행
	void CTCPSocketManager::Add(std::shared_ptr<CTCPSocket>& sptrTCPSocket)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (!sptrTCPSocket)
			return;

		if (sptrTCPSocket->IsServer())
		{
			// 서버일 경우
			m_ServerSocketMap.insert(SocketMap::value_type(sptrTCPSocket->GetSocket(), sptrTCPSocket));
			//m_ServerSocketMap[pTCPSocket->GetSocket()] = pTCPSocket;
		}
		else
		{
			// 클라이언트인 경우
			m_SocketMap.insert(SocketMap::value_type(sptrTCPSocket->GetSocket(), sptrTCPSocket));
			//m_SocketMap[pTCPSocket->GetSocket()] = pTCPSocket;
		}

		if (sptrTCPSocket->GetSocket() >= m_MaxDesc)
			m_MaxDesc = sptrTCPSocket->GetSocket() + 1;
	}

	void CTCPSocketManager::Remove(CTCPSocket* pTCPSocket)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (!pTCPSocket)
			return;

		if (pTCPSocket->IsServer())
			m_ServerSocketMap.erase(pTCPSocket->GetSocket());
		else
			m_SocketMap.erase(pTCPSocket->GetSocket());
	}

	CTCPSocket* CTCPSocketManager::GetServerSocket(SOCKET Socket)
	{
		return m_ServerSocketMap[Socket].get();
	}

	HANDLE CTCPSocketManager::GetIOCPHandle()
	{
		return m_IOCPHandle;
	}

	bool CTCPSocketManager::IsServerSocketMapEmpty()
	{
		return m_ServerSocketMap.empty();
	}

	bool CTCPSocketManager::IsSocketMapEmpty()
	{
		return m_SocketMap.empty();
	}

	CTCPSocket* CTCPSocketManager::GetSocket(SOCKET Socket)
	{
		if (Socket == INVALID_SOCKET)
		{
			return nullptr;
		}

		SocketMap::iterator iter = m_SocketMap.find(Socket);
		if (iter == m_SocketMap.end())
		{
			return nullptr;
		}

		return (*iter).second.get();
	}

	void CTCPSocketManager::CreateWorkThread()
	{
		printf("CreateWorkThread\n");
		if (m_vecThread.empty())
		{
			for (int i = 0; i < 4; i++)
			{
				CIOThread* pIOThread = new CIOThread();
				pIOThread->BeginThread();
				m_vecThread.push_back(pIOThread);
			}
		}
	}

	void CTCPSocketManager::CreateSocketPool(CTCPSocket* pTCPServerSocket, int count)
	{
		//if (m_SocketMap.empty() == false)
		//	return;

		//Sleep(2000);

		// 유저생성
		std::shared_ptr<CTCPSocket> sptrTCPSocket = nullptr;
		for(int i = 0; i < count; i++)
		{
			sptrTCPSocket = std::make_shared<CTCPSocket>(this);
			if(sptrTCPSocket->InitAccept(pTCPServerSocket))
			{
				Add(sptrTCPSocket);
			}
		}
	}

	void CTCPSocketManager::RemoveSocketPool()
	{
		/*
		SocketMap::iterator iter;
		for (iter = m_SocketMap.begin(); iter != m_SocketMap.end(); iter++)
		{
			if ((*iter).second)
				(*iter).second.reset();
		}
	*/
		m_SocketMap.clear();
	}

	CSeedServer* CTCPSocketManager::GetSeedServer()
	{
		return m_pSeedServer;
	}
	
	void CTCPSocketManager::SetSeedServer(CSeedServer* pSeedServer)
	{
		m_pSeedServer = pSeedServer;
	}

	bool CTCPSocketManager::Listen(WORD wPort)
	{
		auto sptrTCPSocket = std::make_shared<CTCPSocket>(this);
		bool bListen = sptrTCPSocket->Listen(wPort);
		Add(sptrTCPSocket);

		if(GetSeedServer())
			GetSeedServer()->OnListen();

		return bListen;
	}

	void CTCPSocketManager::Tick()
	{
		CBufferHandler buffer;
		size_t uiSize = GetSeedServer()->GetCurrentQueueSize();
		for (size_t i = 0; i < uiSize; i++)
		{
			if (!GetSeedServer()->PopQueue(buffer))
				continue;

			switch (buffer.GetIOEventType())
			{
			case CTCPSocket::IO_TYPE::IO_ACCEPT:
				GetSeedServer()->OnAccept(buffer);
				break;
			case CTCPSocket::IO_TYPE::IO_RECV:
				GetSeedServer()->OnRecv(buffer);
				break;
			case CTCPSocket::IO_TYPE::IO_CLOSE:
				GetSeedServer()->OnClose(buffer, false);
				break;
			case CTCPSocket::IO_TYPE::IO_FORCE_CLOSE:
				GetSeedServer()->OnClose(buffer, true);
				break;
			default:
				break;
			}
		}
	}
}