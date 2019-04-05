#include "stdafx.h"
#include "IOEvent.h"

namespace seedio
{
	CIOEvent::CIOEvent()
	{
	}

	CIOEvent::~CIOEvent()
	{
	}

	SOCKET CIOEvent::GetSocket()
	{
		return m_Socket;
	}

	void CIOEvent::SetSocket(SOCKET Socket)
	{
		m_Socket = Socket;
	}

	SOCKET CIOEvent::GetServerSocket()
	{
		return m_ServerSocket;
	}

	void CIOEvent::SetServerSocket(SOCKET Socket)
	{
		m_ServerSocket = Socket;
	}

	WORD CIOEvent::GetIOEventType()
	{
		return m_wIOEventType;
	}

	void CIOEvent::SetIOEventType(WORD wIOEventType)
	{
		m_wIOEventType = wIOEventType;
	}

	WORD CIOEvent::GetPacketType()
	{
		return m_wPacketType;
	}

	void CIOEvent::SetPacketType(WORD wPacketType)
	{
		m_wPacketType = wPacketType;
	}
}
