#pragma once

#include "TCPSocket.h"
#include "Singleton.h"
#include "IOThread.h"
#include <map>
#include "SeedServer.h"

namespace seedio
{
	//typedef map<SOCKET, CTCPSocket*> SocketMap;
	typedef map<SOCKET, std::shared_ptr<CTCPSocket>> SocketMap;

	class CTCPSocketManager :
		public CSingleton<CTCPSocketManager>
	{
	public:
		CTCPSocketManager();
		virtual ~CTCPSocketManager();
		void Add(std::shared_ptr<CTCPSocket>& sptrTCPSocket);
		void Remove(CTCPSocket* pTCPSocket);
		CTCPSocket* GetServerSocket(SOCKET Socket);
		bool IsServerSocketMapEmpty();
		bool IsSocketMapEmpty();
		CTCPSocket* GetSocket(SOCKET Socket);
		HANDLE GetIOCPHandle();
		void CreateWorkThread();
		void CreateSocketPool(CTCPSocket* pTCPServerSocket, int count);
		void RemoveSocketPool();
		CSeedServer* GetSeedServer();
		void SetSeedServer(CSeedServer* pSeedServer);
		bool Listen(WORD wPort);
		void Tick();

	private:
		bool Initialize();
		void Release();

	private:
		SOCKET m_MaxDesc = 0;  // 현재 Descriptor 중 가장 높은 Descriptor Number
		HANDLE m_IOCPHandle = INVALID_HANDLE_VALUE;
		SocketMap m_ServerSocketMap; // 서버 리슨 소켓 Pool
		SocketMap m_SocketMap;		 // 클라이언트 소켓 Pool
		vector<CIOThread*> m_vecThread;
		CSeedServer* m_pSeedServer = nullptr;
		std::mutex m_Mutex;
	};
}