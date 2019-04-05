#include "stdafx.h"
#include "BufferHandler.h"

namespace seedio
{
	CBufferHandler::CBufferHandler()
	{
		Initialize();
		SetBuffer(this->m_szBuffer, PACKET_SIZE);
	}

	CBufferHandler::CBufferHandler(char* pBuffer, UINT uiSize)
	{
		Initialize();
		SetBuffer(pBuffer, uiSize);
	}

	CBufferHandler::~CBufferHandler()
	{
	}

	void CBufferHandler::Initialize()
	{
		m_uiReadSize = 0;
		m_uiTotalSize = 0;
		m_uiUsedSize = 0;
		m_uiBeginPos = 0;
		m_uiEndPos = 0;
		memset(&m_pBuffer, 0, sizeof(m_pBuffer));
	}

	void CBufferHandler::SetBuffer(char* pBuffer, UINT uiSize)
	{
		m_pBuffer = pBuffer;
		m_uiTotalSize = uiSize;

		Clear();
	}

	// ���� ���� ��ġ
	char* CBufferHandler::GetBufferBegin()
	{
		return m_pBuffer + m_uiBeginPos;
	}

	// ���� ������ ��ġ
	char* CBufferHandler::GetBufferEnd()
	{
		return m_pBuffer + m_uiEndPos;
	}

	char* CBufferHandler::GetBuffer() const
	{
		return m_pBuffer;
	}

	void CBufferHandler::GetBuffer(char*& pFront, UINT& lpFrontSize, char*& pBehind, UINT& lpBehindSize)
	{
		pFront = m_pBuffer + m_uiBeginPos; // ���� ���� ��ġ

		if (m_uiEndPos <= m_uiBeginPos)
		{
			// [//end////////begin////]
			pBehind = m_pBuffer;
			lpBehindSize = m_uiEndPos;
			lpFrontSize = m_uiTotalSize - m_uiBeginPos;
		}
		else
		{
			pBehind = '\0';
			lpBehindSize = 0;
			lpFrontSize = m_uiUsedSize;
		}
	}

	void CBufferHandler::Clear()
	{
		m_uiUsedSize = 0;
		m_uiBeginPos = 0;
		m_uiEndPos = 0;
	}

	// ���� ������(TCP_RECV_BUFFER_SIZE, TCP_SEND_BUFFER_SIZE)
	UINT CBufferHandler::GetTotalSize() const
	{
		return m_uiTotalSize;
	}

	// ������� ���� ������
	UINT CBufferHandler::GetUsedSize() const
	{
		return m_uiUsedSize;
	}

	void CBufferHandler::GetUsedSize(UINT& lpuiFrontUsedSize, UINT& lpuiBehindUsedSize) const
	{
		if (m_uiUsedSize)
		{
			if (m_uiEndPos <= m_uiBeginPos)
			{
				// [//////    /////]
				lpuiFrontUsedSize = m_uiTotalSize - m_uiBeginPos;
				lpuiBehindUsedSize = m_uiEndPos;
			}
			else
			{
				// [///////////    ]
				lpuiFrontUsedSize = m_uiUsedSize;
				lpuiBehindUsedSize = 0;
			}
		}
		else
		{
			lpuiFrontUsedSize = lpuiBehindUsedSize = 0;
		}
	}

	// ���� ������
	UINT CBufferHandler::GetRemnantSize() const
	{
		return m_uiTotalSize - m_uiUsedSize;
	}

	void CBufferHandler::GetRemnantSize(UINT& lpuiFrontRemnantSize, UINT& lpuiBehindRemnantSize) const
	{
		if (m_uiEndPos < m_uiBeginPos)
		{
			lpuiFrontRemnantSize = 0;
			lpuiBehindRemnantSize = m_uiBeginPos - m_uiEndPos;
		}
		else
		{
			lpuiFrontRemnantSize = m_uiTotalSize - m_uiEndPos;
			lpuiBehindRemnantSize = m_uiBeginPos;
		}
	}

	bool CBufferHandler::IsEmpty() const
	{
		return m_uiUsedSize > 0 ? false : true;
	}

	bool CBufferHandler::Write(const char* pBuffer, UINT uiLength)
	{
		if (pBuffer == nullptr || uiLength <= 0)
			return false;

		UINT uiRemnantSize = GetRemnantSize();

		if (uiRemnantSize < uiLength) // ���� ���۰� ����
			return false;

		UINT uiDummy = uiLength;
		UINT uiRemnantEndLength = m_uiTotalSize - m_uiEndPos; 

		if (uiDummy > uiRemnantEndLength) // ���� ���ۺ��� ����� �����Ͱ� ���� ���
		{
			// �ϴ� ���� EndPos�� ���۳����� ������ write
			memcpy(m_pBuffer + m_uiEndPos, pBuffer, uiRemnantEndLength);
			pBuffer += uiRemnantEndLength;
			uiDummy -= uiRemnantEndLength;
			m_uiEndPos = 0;
		}

		if (uiDummy > 0) // ����� �����Ͱ� �������� ���
		{
			memcpy(m_pBuffer + m_uiEndPos, pBuffer, uiDummy);
			m_uiEndPos += uiDummy;
		}

		// �� �����͸�ŭ ������� ���� ������ ����
		m_uiUsedSize += uiLength;
		return true;
	}

	bool CBufferHandler::Write(UINT uiLength)
	{
		if (uiLength <= 0)
			return false;

		UINT uiRemnantSize = GetRemnantSize();

		if (uiRemnantSize < uiLength) // ���� ���۰� ����
			return false;

		UINT uiDummy = uiLength;
		UINT uiRemnantEndLength = m_uiTotalSize - m_uiEndPos;
		
		if (uiDummy > uiRemnantEndLength)
		{
			uiDummy -= uiRemnantEndLength;
			m_uiEndPos = 0;
		}

		if (uiDummy > 0)
		{
			m_uiEndPos += uiDummy;
		}

		m_uiUsedSize += uiLength;
		return true;
	}

	bool CBufferHandler::Read(char* pBuffer, UINT uiLength)
	{
		if (pBuffer == nullptr || uiLength <= 0)
			return false;

		if (uiLength > m_uiUsedSize)
			return false;

		UINT uiDummy = uiLength;
		UINT uiDiff = m_uiTotalSize - m_uiBeginPos;

		if (uiDiff < uiLength)
		{
			// ���� ���� ����� ���ٸ�
			// �ϴ� ���� ó������ �� ��ġ���� read ���� ���´�
			memcpy(pBuffer, m_pBuffer + m_uiBeginPos, uiDiff);
			pBuffer += uiDiff;
			uiDummy -= uiDiff;
			m_uiBeginPos = 0;
		}

		memcpy(pBuffer, m_pBuffer + m_uiBeginPos, uiDummy);
		m_uiBeginPos += uiDummy;
		m_uiUsedSize -= uiLength;
		m_uiReadSize += uiLength;

		return true;
	}

	bool CBufferHandler::Read(UINT uiLength)
	{
		if (uiLength <= 0)
			return false;

		if (m_uiUsedSize < uiLength)
			return false;

		UINT uiDummy = uiLength;
		UINT uiDiff = m_uiTotalSize - m_uiBeginPos;

		if (uiDiff < uiLength)
		{
			uiDummy -= uiDiff;
			m_uiBeginPos = 0;
		}

		m_uiBeginPos += uiDummy;
		m_uiUsedSize -= uiLength;
		m_uiReadSize += uiLength;

		return true;
	}

	void CBufferHandler::InitReadGuard()
	{
		m_uiReadSize = 0;
	}

	void CBufferHandler::InitBeginEndPos()
	{
		m_uiBeginPos = 0;
		m_uiEndPos = 0;
	}

	UINT CBufferHandler::GetReadSize()
	{
		return m_uiReadSize;
	}

	/*
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
	*/
}