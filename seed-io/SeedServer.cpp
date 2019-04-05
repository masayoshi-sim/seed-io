#include "stdafx.h"
#include "SeedServer.h"
#include "TCPSocketManager.h"


#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console") 
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console") 
#endif

namespace seedio
{
	CSeedServer::CSeedServer()
	{
		GetInst(CTCPSocketManager).SetSeedServer(this);
	}

	CSeedServer::~CSeedServer()
	{
	}

	void CSeedServer::Tick()
	{
		SetCurrentQueueSize();
		GetInst(CTCPSocketManager).Tick();
	}

	bool CSeedServer::Listen(WORD wPort)
	{
		return GetInst(CTCPSocketManager).Listen(wPort);
	}

	void CSeedServer::PushQueue(CBufferHandler& buffer)
	{
		m_Queue.enqueue(buffer);
	}

	bool CSeedServer::PopQueue(CBufferHandler& buffer)
	{
		return m_Queue.try_dequeue(buffer);
	}
	/*
	void CSeedServer::PushQueue(std::shared_ptr<CIOEvent>& sptrIOEvent)
	{
		if (!sptrIOEvent)
			return;

		m_Queue.enqueue(sptrIOEvent);
	}
		
	bool CSeedServer::PopQueue(std::shared_ptr<CIOEvent>& sptrIOEvent)
	{
		return m_Queue.try_dequeue(sptrIOEvent);
	}
	*/

	size_t CSeedServer::GetCurrentQueueSize()
	{
		return m_uiCurrentQueueSize;
	}

	void CSeedServer::SetCurrentQueueSize()
	{
		m_uiCurrentQueueSize = m_Queue.size_approx();
	}

	void CSeedServer::Send(CBufferHandler& buffer)
	{
		CTCPSocket* pTCPSocket = GetInst(CTCPSocketManager).GetSocket(buffer.GetSocket());
		if (pTCPSocket == nullptr)
			return;

		WSABUF wsaBuffer[2] = { 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		UINT uiBufferCount = 0;

		PacketHeader packetHeader;
		memset(&packetHeader, 0, sizeof(packetHeader));
		packetHeader.wType = buffer.GetPacketType();
		packetHeader.wSize = buffer.GetUsedSize();

		if (packetHeader.wSize <= 0)
			return;

		pTCPSocket->GetSendBufferHandler()->Write(reinterpret_cast<char*>(&packetHeader), sizeof(packetHeader));
		pTCPSocket->GetSendBufferHandler()->Write(buffer.GetBufferBegin(), packetHeader.wSize);
		pTCPSocket->GetSendBufferHandler()->GetBuffer(wsaBuffer[0].buf, reinterpret_cast<UINT&>(wsaBuffer[0].len), wsaBuffer[1].buf, reinterpret_cast<UINT&>(wsaBuffer[1].len));

		if (wsaBuffer[1].len > 0)
			uiBufferCount = 2;
		else
			uiBufferCount = 1;

		int result = WSASend(pTCPSocket->GetSocket(), wsaBuffer, uiBufferCount, &dwBytes, dwFlag,
			&pTCPSocket->m_event[CTCPSocket::IO_TYPE::IO_SEND].overlapped, NULL);

		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING 
			&& WSAGetLastError() != WSAEWOULDBLOCK)
		{
			//wprintf(L"[ERROR] Send %d\n", WSAGetLastError());
			return;
		}

		if (dwBytes > 0)
		{
			pTCPSocket->GetSendBufferHandler()->Read(dwBytes);

			if (pTCPSocket->GetSendBufferHandler()->GetUsedSize() == 0)
			{
				pTCPSocket->GetSendBufferHandler()->InitBeginEndPos();
			}
		}
	}

	void CSeedServer::Close(SOCKET Socket, bool bForce)
	{
		if (Socket == INVALID_SOCKET)
			return;

		CTCPSocket* pTCPSocket = GetInst(CTCPSocketManager).GetSocket(Socket);
		if (pTCPSocket == nullptr)
			return;

		pTCPSocket->PostClose(bForce);
	}
}
