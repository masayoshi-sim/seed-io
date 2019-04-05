#include "stdafx.h"
#include "TCPSocket.h"
#include "TCPSocketManager.h"

namespace seedio
{
	CTCPSocket::CTCPSocket(CTCPSocketManager* pTCPSocketManager)
	{
		m_pSocketManager = pTCPSocketManager;
		Initialize();
		printf("CTCPSocket\n");
	}

	CTCPSocket::~CTCPSocket()
	{
		printf("~CTCPSocket\n");
	}

	void CTCPSocket::Initialize()
	{
		m_Socket = INVALID_SOCKET;
		m_bAliveConnection = false;
		m_bServer = false;
		m_iMode = IO_NONBLOCKING | LINGER_OFF;
		m_bForce = false;
		m_bClose = false;

		// IO 이벤트 초기화
		memset(&m_event, 0, sizeof(m_event));
	
		// 버퍼 초기화
		memset(&m_RecvBuffer, 0, sizeof(m_RecvBuffer));
		memset(&m_SendBuffer, 0, sizeof(m_SendBuffer));

		m_RecvBufferHandler.SetBuffer(m_RecvBuffer, TCP_BUFFER_SIZE);
		m_SendBufferHandler.SetBuffer(m_SendBuffer, TCP_BUFFER_SIZE);
	}

	void CTCPSocket::ConvertToSockaddr(const char *szAddress, int iPort, sockaddr* pSockAddr)
	{
		hostent* pHostent;
		memset(&pHostent, 0, sizeof(pHostent));
		sockaddr_in* pSockaddrIn = (sockaddr_in*)pSockAddr;

		if (szAddress)
		{
			pHostent = gethostbyname(szAddress);
			if (!pHostent)
				pSockaddrIn->sin_addr.s_addr = inet_addr(szAddress);
			else
				pSockaddrIn->sin_addr = *((in_addr*)pHostent->h_addr);
		}
		else
		{
			pSockaddrIn->sin_addr.s_addr = htonl(INADDR_ANY);
		}

		pSockaddrIn->sin_family = AF_INET;
		pSockaddrIn->sin_port = htons(iPort);
		memset(pSockaddrIn->sin_zero, 0, sizeof(pSockaddrIn->sin_zero));
	}

	bool CTCPSocket::Connect(char* szAddress, int iPort)
	{
		//std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Socket != INVALID_SOCKET)
			return false;

		// 소켓 생성
		if (CreateSocket() == false)
			return false;

		if (m_Socket == INVALID_SOCKET)
			return false;

		SetMode(m_iMode);

		// 주소 설정
		sockaddr sa;
		memset(&sa, 0, sizeof(sa));
		ConvertToSockaddr(szAddress, iPort, &sa);

		int iResult = ::bind(m_Socket, &sa, sizeof(sa));
		if (iResult)
		{
			Close();
			return false;
		}

		DWORD dwBytes = 0;
		LPFN_CONNECTEX lpConnectEx = nullptr;
		GUID guid = WSAID_CONNECTEX;

		iResult = WSAIoctl(m_Socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, 
			sizeof(guid), &lpConnectEx, sizeof(lpConnectEx), &dwBytes, NULL, NULL);

		if (iResult != 0)
		{
			// ConnectEx 의 함수포인터 얻기 실패
			return false;
		}

		CreateIoCompletionPort((HANDLE)m_Socket, m_pSocketManager->GetIOCPHandle(), (ULONG_PTR)this, 0);

		BOOL bResult = lpConnectEx(m_Socket, (SOCKADDR*)&sa, sizeof(sa), NULL, 0, NULL,
			&m_event[IO_TYPE::IO_CONNECT].overlapped);
		
		int lastError = WSAGetLastError();

		if (iResult == FALSE && lastError != WSA_IO_PENDING)
		{
			Close();
			return false;
		}

		bResult = GetOverlappedResult((HANDLE)m_Socket, &m_event[IO_TYPE::IO_CONNECT].overlapped, &dwBytes, TRUE);
		lastError = WSAGetLastError();
		if (iResult == FALSE)
		{
			Close();
			return false;
		}

		//m_bServer = true;
		m_bAliveConnection = true;

		std::shared_ptr<CTCPSocket> sptrTCPSocket = shared_from_this();
		m_pSocketManager->Add(sptrTCPSocket);

		// 클라이언트 소켓맵 새로 생성
		// 이미 생성했다면 생성하지 않음
		m_pSocketManager->CreateSocketPool(this, 5000);

		Recv();
		return true;
	}

	bool CTCPSocket::Close(SOCKET ServerSocket)
	{
		//std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_bClose.exchange(true))
			return false;

		m_bClose = true;

		int iResult = 0;
		if (m_Socket == INVALID_SOCKET)
			return false;

		// 4-way handshake
		if (m_bAliveConnection && !m_bServer)
		{
			shutdown(m_Socket, SD_BOTH);

			// iocp에 종료이벤트 push
			/*
			auto sptrIOEvent = std::make_shared<CIOEvent>();
			sptrIOEvent->SetSocket(m_Socket);
			sptrIOEvent->SetServerSocket(ServerSocket);

			if (this->m_bForce)
				sptrIOEvent->SetIOEventType(IO_FORCE_CLOSE);
			else
				sptrIOEvent->SetIOEventType(IO_CLOSE);

			//auto sptrIOEvent = std::static_pointer_cast<CIOEvent>(sptrBuffer);
			GetInst(CTCPSocketManager).GetSeedServer()->PushQueue(sptrIOEvent);
			*/

			CBufferHandler buffer;
			buffer.SetSocket(m_Socket);
			buffer.SetServerSocket(ServerSocket);

			if (this->m_bForce)
				buffer.SetIOEventType(IO_FORCE_CLOSE);
			else
				buffer.SetIOEventType(IO_CLOSE);
			GetInst(CTCPSocketManager).GetSeedServer()->PushQueue(buffer);
		}

		closesocket(m_Socket);

		//m_pSocketManager->Remove(this);

		/*
		if (m_pSocketManager->IsServerSocketMapEmpty() && IsServer())
		{
		}
		*/

		Initialize();
		return true;
	}

	bool CTCPSocket::Close2(bool bForce)
	{
		// PostClose 실행 이후

		int iResult = 0;
		if (m_Socket == INVALID_SOCKET)
			return false;

		// 4-way handshake
		if (m_bAliveConnection && !m_bServer)
		{
			shutdown(m_Socket, SD_BOTH);
		}

		m_bForce = bForce;

		closesocket(m_Socket); // IOCP 에서 GQCS 동작
		return true;
	}

	bool CTCPSocket::Listen(WORD wPort, int iBackLog)
	{
		// 소켓 생성
		if (CreateSocket() == false)
			return false;
		
		SetMode(m_iMode | REUSEADDR);

		sockaddr sa;
		memset(&sa, 0, sizeof(sa));
		ConvertToSockaddr(nullptr/*localhost*/, wPort, &sa);

		int iResult = ::bind(m_Socket, &sa, sizeof(sa));
		if (iResult)
		{
			Close();
			return false;
		}

		iResult = ::listen(m_Socket, iBackLog);
		if (iResult)
		{
			Close();
			return false;
		}	

		m_bServer = true;
		m_bAliveConnection = true;

		// iocp 연결
		CreateIoCompletionPort((HANDLE)m_Socket, m_pSocketManager->GetIOCPHandle(), (ULONG_PTR)this, 0);

		m_pSocketManager->CreateSocketPool(this, 5000);

		printf("listen socket success %d\n", m_Socket);
		return true;
	}

	bool CTCPSocket::SetMode(int iMode)
	{
		// 소켓이 생성되었을 때
		if (m_Socket == INVALID_SOCKET)
			return false;
			
		int iResult = 0;

		// 블러킹 모드 설정
		if (iMode & IO_BLOCKING)
		{
			unsigned long arg = 0;
			iResult = ioctlsocket(m_Socket, FIONBIO, &arg);

			if (iResult == SOCKET_ERROR)
				iMode &= ~IO_BLOCKING;
		}

		// 논블러킹 모드로 설정
		if (iMode & IO_NONBLOCKING)
		{
			unsigned long ulArg = 1;
			iResult = ioctlsocket(m_Socket, FIONBIO, &ulArg);

			if (iResult == SOCKET_ERROR)
				iMode &= ~IO_NONBLOCKING;
		}

		// LINGER on
		if (iMode & LINGER_ON)
		{
			linger l;
			memset(&l, 0, sizeof(l));
			int iLength = sizeof(l);
			getsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char*)&l, &iLength);
			l.l_onoff = 1;
			iResult = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char*)&l, iLength);

			if (iResult == SOCKET_ERROR)
				iMode &= ~LINGER_ON;
		}

		// LINGER off
		if (iMode & LINGER_OFF)
		{
			// TIME_WAIT 상태에도 bind 할 수 있도록 함
			linger l;
			memset(&l, 0, sizeof(l));
			int iLength = sizeof(l);
			getsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char*)&l, &iLength);
			l.l_onoff = 0;
			iResult = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (const char*)&l, iLength);

			if (iResult == SOCKET_ERROR)
				iMode &= ~LINGER_OFF;
		}

		// SO_REUSEADDR 설정
		if (iMode & REUSEADDR)
		{
			int on = 1;
			iResult = setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
			if (iResult == SOCKET_ERROR)
				iMode &= ~REUSEADDR;
		}

		// Nagle on
		if (iMode & NAGLE_ON)
		{
			int on = 0;
			iResult = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
			if (iResult == SOCKET_ERROR)
				iMode &= ~NAGLE_ON;
		}

		// Nagle off
		if (iMode & NAGLE_OFF)
		{
			int on = 1;
			iResult = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
			if (iResult == SOCKET_ERROR)
				iMode &= ~NAGLE_OFF;
		}

		// 설정 모드가 모두 실패하면 실패 반환
		if (!iMode)
			return false;

		if (iMode & (IO_BLOCKING | IO_NONBLOCKING))
			m_iMode &= ~(IO_BLOCKING | IO_NONBLOCKING);

		if (iMode & (LINGER_ON | LINGER_OFF))
			m_iMode &= ~(LINGER_ON | LINGER_OFF);

		if (iMode & (NAGLE_ON | NAGLE_OFF))
			m_iMode &= ~(NAGLE_ON | NAGLE_OFF);

		/*
		if (iMode & (ACCEPT_ON | ACCEPT_OFF))
			m_iMode &= ~(ACCEPT_ON | ACCEPT_OFF);
			*/

		m_iMode |= iMode;
		return true;
	}

	int CTCPSocket::GetMode() const
	{
		return m_iMode;
	}

	bool CTCPSocket::IsMode(int iMode) const
	{
		if (iMode != (m_iMode & iMode))
			return false;
		return true;
	}

	bool CTCPSocket::IsServer() const
	{
		return m_bServer;
	}

	bool CTCPSocket::IsAliveConnection() const
	{
		return m_bAliveConnection;
	}

	SOCKET CTCPSocket::GetSocket()
	{
		return m_Socket;
	}

	bool CTCPSocket::CreateSocket()
	{
		// 이미 소켓을 생성했으면 false
		if (m_Socket != INVALID_SOCKET)
			return false;

		m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (m_Socket == INVALID_SOCKET)
		{
			int lastError = WSAGetLastError();
			//wprintf(L"[ERROR]CreateSocket %d\n", lastError);
			return false;
		}

		return true;
	}

	bool CTCPSocket::InitAccept(CTCPSocket* pTCPServerSocket)
	{
		// 서버 켜질때 메인쓰레드에서만 실행되므로 크리티컬섹션 처리를 하지 않음
		if (!pTCPServerSocket)
			return false;

		if (CreateSocket() == false)
			return false;

		for (BYTE i = 0; i < IO_TYPE::IO_TYPE_MAX; i++)
		{
			m_event[i].ioType = IO_TYPE(i);
			m_event[i].wptrTCPSocket = weak_from_this();
		}
		
		char szBuf[192] = { 0 };
		//DWORD recvBytes = 0;

		SetMode(m_iMode);	

		//if (AcceptEx(pTCPServerSocket->GetSocket(), m_Socket, &this->m_acceptEvent.Addr, 0,
			//sizeof(this->m_acceptEvent.Addr.byLocal), sizeof(this->m_acceptEvent.Addr.byRemote), NULL, &this->m_acceptEvent.overlapped) == FALSE)

		if (AcceptEx(pTCPServerSocket->GetSocket(), m_Socket, &szBuf, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, &this->m_event[IO_TYPE::IO_ACCEPT].overlapped) == FALSE)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				//wprintf(L"[ERROR] Accept %d\n", WSAGetLastError());
				Close(pTCPServerSocket->GetSocket());
				return false;
			}
		}

		/*
		sockaddr *pLocal = nullptr, *pRemote = nullptr;
		int iLocal = 0, iRemote = 0;

		GetAcceptExSockaddrs(&this->m_acceptEvent.Addr, 0, sizeof(this->m_acceptEvent.Addr.byLocal),
			sizeof(this->m_acceptEvent.Addr.byRemote), &pLocal, &iLocal, &pRemote, &iRemote);
			*/

		HANDLE handle = CreateIoCompletionPort((HANDLE)m_Socket, m_pSocketManager->GetIOCPHandle(), (ULONG_PTR)pTCPServerSocket, 0);
		if (handle == nullptr)
		{
			//wprintf(L"Accept CreateIoCompletionPort error %u\n", GetLastError());
			Close(pTCPServerSocket->GetSocket());
			return false;
		}

		return true;
	}

	void CTCPSocket::Accept(SOCKET ServerSocket)
	{
		//std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_bAliveConnection == false)
			m_bAliveConnection = true;

		/*
		auto sptrIOEvent = std::make_shared<CIOEvent>();
		sptrIOEvent->SetSocket(m_Socket);
		sptrIOEvent->SetServerSocket(ServerSocket);
		sptrIOEvent->SetIOEventType(IO_ACCEPT);
		GetInst(CTCPSocketManager).GetSeedServer()->PushQueue(sptrIOEvent);
		*/

		CBufferHandler buffer;
		buffer.SetSocket(m_Socket);
		buffer.SetServerSocket(ServerSocket);
		buffer.SetIOEventType(IO_ACCEPT);
		GetInst(CTCPSocketManager).GetSeedServer()->PushQueue(buffer);

		this->Recv();
	}

	void CTCPSocket::Recv(DWORD dwBytes, SOCKET ServerSocket)
	{
		//std::lock_guard<std::mutex> lock(m_Mutex);
		WSABUF wsaBuffer[2] = { 0 };
		DWORD dwReadBytes = 0;
		DWORD dwFlag = 0;
		DWORD dwBufferCount = 0;

		if (dwBytes > 0)
		{
			// 비동기이기때문에 여기서 버퍼에 Write
			this->m_RecvBufferHandler.Write(dwBytes);

			// 패킷 생성 후 큐에 Push
			size_t uiSize = GetInst(CTCPSocketManager).GetSeedServer()->GetCurrentQueueSize();
			//printf("uiSize %d\n", uiSize);

			if (uiSize < 1000)
				ParsePacket(ServerSocket);
		}

		/*
		if (this->m_RecvBufferHandler.GetRemnantSize() <= 0)
		{
			// 남은 버퍼가 없음
			wprintf(L"[ERROR] Recv remnantSize error");
			return false;
		}
		*/

		wsaBuffer[0].buf = this->m_RecvBufferHandler.GetBufferEnd();

		// wsaBuffer.len은 수신 받을 수 있는 패킷크기인데
		// 링버퍼가 앞뒤로 나뉘어져있다면 링버퍼의 앞뒤사이즈에 맞게 버퍼를 지정한다
		this->m_RecvBufferHandler.GetRemnantSize(reinterpret_cast<UINT&>(wsaBuffer[0].len), reinterpret_cast<UINT&>(wsaBuffer[1].len));

		if (wsaBuffer[1].len > 0)
		{
			// 버퍼가 나누어져있다면
			dwBufferCount = 2;
			wsaBuffer[1].buf = m_RecvBufferHandler.GetBuffer();
		}
		else
		{
			dwBufferCount = 1;
		}

		int result = WSARecv(m_Socket, wsaBuffer, dwBufferCount, &dwReadBytes, &dwFlag,
			&m_event[IO_TYPE::IO_RECV].overlapped, NULL);

		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING 
			&& WSAGetLastError() != WSAEWOULDBLOCK)
		{
			//wprintf(L"[ERROR] Recv %d\n", WSAGetLastError());
			//return false;
		}
	}	
	
	CBufferHandler* CTCPSocket::GetRecvBufferHandler()
	{
		return &m_RecvBufferHandler;
	}

	CBufferHandler* CTCPSocket::GetSendBufferHandler()
	{
		return &m_SendBufferHandler;
	}

	PacketHeader CTCPSocket::GetPacketHeader()
	{
		UINT uiFrontBufferUsedSize = 0, uiBehindBufferUsedSize = 0;
		m_RecvBufferHandler.GetUsedSize(uiFrontBufferUsedSize, uiBehindBufferUsedSize);

		PacketHeader packetHeader;
		memset(&packetHeader, 0, sizeof(packetHeader));

		int iHeadSize = sizeof(packetHeader);

		if (uiFrontBufferUsedSize < (UINT)iHeadSize)
		{
			if (uiFrontBufferUsedSize == 0)
			{
				// [/////    ]
				//      E    B
				packetHeader = *((PacketHeader*)m_RecvBufferHandler.GetBuffer());
			}
			else
			{
				// [///   ///]
				//    E   B
				memcpy(&packetHeader, m_RecvBufferHandler.GetBufferBegin(), uiFrontBufferUsedSize);
				memcpy(((char*)&packetHeader) + uiFrontBufferUsedSize, m_RecvBufferHandler.GetBuffer(), iHeadSize - uiFrontBufferUsedSize);
			}
		}
		else
		{
			// [   ////  ]
			//     B  E
			//packetHeader = *((PacketHeader*)m_RecvBufferHandler.GetBufferBegin());
			memcpy(&packetHeader, m_RecvBufferHandler.GetBufferBegin(), sizeof(packetHeader));
		}
		return packetHeader;
	}

	void CTCPSocket::ParsePacket(SOCKET ServerSocket)
	{
		int iUsedSize = m_RecvBufferHandler.GetUsedSize();
		WORD wPacketType = 0;
		WORD wBodySize = 0;
		PacketHeader packetHeader;
		memset(&packetHeader, 0, sizeof(packetHeader));
		int iHeadSize = sizeof(packetHeader);
		
		while (true)
		{
			if (iUsedSize < iHeadSize)
			{
				// 패킷헤더만큼도 도착하지 않았다면 break
				break;
			}

			packetHeader = GetPacketHeader();

			// 패킷헤더 내용 확인
			wPacketType = packetHeader.wType;
			wBodySize = packetHeader.wSize;

			if (wBodySize + iHeadSize > iUsedSize)
			{
				// 패킷바디가 아직 도착하지 않았다면 break
				break;
			}

			// 패킷 데이터를 생성
			//auto sptrBuffer = std::make_shared<CBufferHandler>();
			CBufferHandler buffer;
			buffer.SetSocket(m_Socket);
			buffer.SetServerSocket(ServerSocket); // 서버 소켓
			buffer.SetIOEventType(IO_RECV);
			buffer.SetPacketType(wPacketType);

			// 클라이언트가 보낸 데이터를 패킷 버퍼에 설정
			m_RecvBufferHandler.Read(iHeadSize);
			m_RecvBufferHandler.Read(buffer.GetBuffer(), wBodySize);
			buffer.Write(wBodySize);
			m_RecvBufferHandler.InitReadGuard();

			// Queue에 Push
			//auto sptrIOEvent = std::static_pointer_cast<CIOEvent>(sptrBuffer);
			GetInst(CTCPSocketManager).GetSeedServer()->PushQueue(buffer);

			iUsedSize -= (iHeadSize + wBodySize);
		}
	}

	// 서버에서 해당 클라이언트 소켓을 일방적으로 접속 종료할 때 사용
	void CTCPSocket::PostClose(bool bForce/*강제여부*/)
	{
		LPOVERLAPPED lpoverlapped = { 0 };

		if (bForce == true)
		{
			lpoverlapped = &m_event[IO_TYPE::IO_FORCE_CLOSE].overlapped;
		}
		else
		{
			lpoverlapped = &m_event[IO_TYPE::IO_CLOSE].overlapped;
		}

		PostQueuedCompletionStatus(m_pSocketManager->GetIOCPHandle(), 0, 
			(ULONG_PTR)this, lpoverlapped);
	}
}