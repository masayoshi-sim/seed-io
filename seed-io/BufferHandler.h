#pragma once

#include "IOEvent.h"

#define TCP_BUFFER_SIZE	102400
#define PACKET_SIZE 32767

// 원형 버퍼
namespace seedio
{
	class CBufferHandler :
		public CIOEvent, std::enable_shared_from_this<CBufferHandler>
	{
	public:
		CBufferHandler();
		CBufferHandler(char* pBuffer, UINT uiSize);
		virtual ~CBufferHandler();
		void SetBuffer(char* pBuffer, UINT uiSize);
		char* GetBufferBegin();
		char* GetBufferEnd();
		char* GetBuffer() const;
		void GetBuffer(char*& pFront, UINT& lpFrontSize, char*& pBehind, UINT& lpBehindSize);
		void Clear();
		UINT GetTotalSize() const;
		UINT GetUsedSize() const;
		void GetUsedSize(UINT& lpuiFrontUsedSize, UINT& lpuiBehindUsedSize) const;
		UINT GetRemnantSize() const;
		void GetRemnantSize(UINT& lpuiFrontRemnantSize, UINT& lpuiBehindRemnantSize) const;
		bool IsEmpty() const;
		bool Write(const char* pBuffer, UINT uiLength);
		bool Write(UINT uiLength);
		bool Read(char* pBuffer, UINT uiLength);
		bool Read(UINT uiLength);
		void InitReadGuard();
		void InitBeginEndPos();
		UINT GetReadSize();

		template<class T>
		bool operator<< (T rhs);
		template<class T>
		bool operator>> (T& rhs);

		
	private:
		void Initialize();

	private:
		UINT m_uiReadSize;	// 읽은 사이즈
		UINT m_uiTotalSize;	// 버퍼 사이즈
		UINT m_uiUsedSize;	// 버퍼에서 사용중인 사이즈
		UINT m_uiBeginPos;	// 버퍼 시작
		UINT m_uiEndPos;	// 버퍼 끝
		char m_szBuffer[PACKET_SIZE] = { 0 };
		char* m_pBuffer;
	};

	template<typename T>
		bool CBufferHandler::operator<<(T rhs)
		{
			return Write((const char*)&rhs, sizeof(T));
		}

		template<typename T>
		bool CBufferHandler::operator>>(T& rhs)
		{
			return Read((char*)&rhs, sizeof(T));
		}
}