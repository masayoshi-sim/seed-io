#pragma once

#include "TCPSocket.h"
#include "concurrentqueue.h"

namespace seedio
{
	class CSeedServer
	{
	public:
		CSeedServer();
		~CSeedServer();
		bool Listen(WORD wPort);
		void PushQueue(CBufferHandler& buffer);
		bool PopQueue(CBufferHandler& buffer);
		size_t GetCurrentQueueSize();
		void SetCurrentQueueSize();
		void Send(CBufferHandler& buffer);
		void Close(SOCKET Socket, bool bForce = false);
		virtual void Tick();
		virtual void OnConnect() {};
		virtual void OnListen() {};
		virtual void OnAccept(CBufferHandler& buffer) {};
		virtual void OnClose(CBufferHandler& buffer, bool force) {};
		virtual void OnRecv(CBufferHandler& buffer) {};

	protected:
		moodycamel::ConcurrentQueue<CBufferHandler> m_Queue;

	private:
		size_t m_uiCurrentQueueSize = 0;
	};
}
