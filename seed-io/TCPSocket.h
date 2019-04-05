#pragma once

#include <winsock2.h>
#include <MSWSock.h>
#include <ws2tcpip.h>

#pragma comment(lib, "WS2_32.LIB")
#pragma comment(lib,"mswsock.lib")

#include "BufferHandler.h"
#include "MultiThreadSync.h"
#include <mutex>
#include <atomic>

namespace seedio
{
	class CTCPSocketManager;
	class CTCPSocket :
		public std::enable_shared_from_this<CTCPSocket>
		//public CMultiThreadSync<CTCPSocket>,
	{
		friend class CIOThread;
	public:
		CTCPSocket(CTCPSocketManager* pTCPSocketManager);
		virtual ~CTCPSocket();
		void Initialize();
		bool Connect(char* szAddress, int iPort);
		bool Listen(WORD wPort, int iBackLog = 1024);
		bool SetMode(int iMode);
		int GetMode() const;
		bool IsMode(int iMode) const;
		bool IsServer() const;
		bool IsAliveConnection() const;
		SOCKET GetSocket();
		CBufferHandler* GetRecvBufferHandler();
		CBufferHandler* GetSendBufferHandler();
		bool InitAccept(CTCPSocket* pTCPServerSocket);
		void Accept(SOCKET ServerSocket);
		bool Close(SOCKET ServerSocket = INVALID_SOCKET);
		bool Close2(bool bForce = false); // , SOCKET ServerSocket = INVALID_SOCKET);
		void Recv(DWORD dwBytes = 0, SOCKET ServerSocket = INVALID_SOCKET);
		void PostClose(bool bForce/*강제여부*/);

	private:
		void ConvertToSockaddr(const char *szAddress, int iPort, sockaddr* pSockAddr);
		bool CreateSocket();
		PacketHeader GetPacketHeader();
		void ParsePacket(SOCKET ServerSocket);

	public:
		enum SOCKET_MODE : int
		{
			IO_BLOCKING =	 0x00000001, // 블럭 모드
			IO_NONBLOCKING = 0x00000002, // 논블럭 모드(기본)
			LINGER_ON =		 0x00000004, // LINGER on
			LINGER_OFF =	 0x00000008, // LINGER off(기본)
			REUSEADDR =		 0x00000010, // 소켓 재사용(서버만 설정 권장)
			NAGLE_ON =		 0x00000020, // Nagle 알고리즘 On
			NAGLE_OFF =		 0x00000040, // Nagle 알고리즘 Off(기본)
		};

		enum IO_TYPE : BYTE
		{
			IO_CONNECT, // 접속 요청
			IO_ACCEPT,  // 접속 수락
			IO_RECV,    // 패킷 수신
			IO_SEND,    // 패킷 송신
			IO_CLOSE,	// 접속 종료
			IO_FORCE_CLOSE, // 강제 접속 종료
			IO_TYPE_MAX
		};

		typedef struct _TCPSOCKET_EVENT
		{
			OVERLAPPED overlapped = { 0 };
			IO_TYPE ioType = IO_TYPE_MAX;
			std::weak_ptr<CTCPSocket> wptrTCPSocket;
		} TCPSOCKET_EVENT;

		TCPSOCKET_EVENT m_event[IO_TYPE::IO_TYPE_MAX];

	protected:
		SOCKET m_Socket = INVALID_SOCKET;

	private:
		CTCPSocketManager* m_pSocketManager;
		bool m_bAliveConnection = false;
		bool m_bServer = false;
		int m_iMode = 0;
		std::atomic<bool> m_bClose = false;
		bool m_bForce = false;
		std::mutex m_Mutex;
		CBufferHandler m_RecvBufferHandler;// Recv 버퍼 핸들러
		CBufferHandler m_SendBufferHandler;// Send 버퍼 핸들러
		char m_RecvBuffer[TCP_BUFFER_SIZE] = { 0 };
		char m_SendBuffer[TCP_BUFFER_SIZE] = { 0 };
	};
}