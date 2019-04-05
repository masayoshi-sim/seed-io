#include "stdafx.h"
#include "TestServer.h"

void OutputDebugString2(LPCTSTR pszStr, ...)
{
	TCHAR szMsg[256] = { 0 };
	va_list args;
	va_start(args, pszStr);
	_vstprintf_s(szMsg, 256, pszStr, args);
	OutputDebugString(szMsg);
}

CTestServer::CTestServer()
{}

CTestServer::~CTestServer()
{}

void CTestServer::Tick()
{
	CSeedServer::Tick();
}

void CTestServer::OnListen()
{
	printf("OnListen\n");
}

void CTestServer::OnAccept(CBufferHandler& buffer)
{
	//OutputDebugString2(TEXT("OnAccept %d"), pBuffer->GetSocket());
	printf("OnAccept %d\n", buffer.GetSocket());

	/*
	char szIP[32] = { 0 };
	sprintf_s(szIP, "%u.%u.%u.%u", (dwIP & 0xff000000) >> 24, (dwIP & 0x00ff0000) >> 16,
		(dwIP & 0x0000ff00) >> 8, (dwIP & 0x000000ff));

	OutputDebugString2(szIP);
	*/
}

void CTestServer::OnClose(CBufferHandler& buffer, bool force)
{
	//OutputDebugString2(TEXT("OnClose %d"), pBuffer->GetSocket());
	printf("OnClose %d\n", buffer.GetSocket());
}

void CTestServer::OnRecv(CBufferHandler& buffer)
{
	wprintf(L"OnRecv %d\n", buffer.GetSocket());
	//OutputDebugString2(TEXT("OnRecv %d, %d, %hs"), buffer.GetSocket(), iSize, szPacketData);

	// ��Ŷ �޴� ��
	/*
	int iSize = 0;
	char szPacketData[512] = { 0 };
	buffer >> iSize;
	buffer.Read(szPacketData, iSize);

	*/

	// ��Ŷ ������ ��
	/*
	CBufferHandler sendBuffer;
	sendBuffer.SetSocket(buffer.GetSocket());
	sendBuffer.Write("1234test", 8);
	sendBuffer.SetIOEventType(CTCPSocket::IO_TYPE::IO_SEND);
	sendBuffer.SetPacketType(1);
	Send(sendBuffer);
	*/

	// �������� �ش� Ŭ���̾�Ʈ�� ���� �����ϴ� ��
	//Close(buffer.GetSocket(), true);

	// �������� �ش� Ŭ���̾�Ʈ�� ���� �����ϴ� ��
	//Close(buffer.GetSocket(), false);
}
