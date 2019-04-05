#pragma once

#include <memory>
#include "value.h"

namespace seedio
{
	typedef struct _PACKET_HEADER
	{
		WORD wType = 0;
		WORD wSize = 0;
	}PacketHeader;

	class CIOEvent :
		public std::enable_shared_from_this<CIOEvent>
	{
	public:
		CIOEvent();
		virtual ~CIOEvent();
		SOCKET GetSocket();
		void SetSocket(SOCKET Socket);
		SOCKET GetServerSocket();
		void SetServerSocket(SOCKET Socket);
		WORD GetIOEventType();
		void SetIOEventType(WORD wIOEventType);
		WORD GetPacketType();
		void SetPacketType(WORD wPacketType);

	protected:
		SOCKET m_Socket = INVALID_SOCKET;
		SOCKET m_ServerSocket = INVALID_SOCKET;
		WORD m_wIOEventType = 0;
		WORD m_wPacketType = 0;
	};
}
