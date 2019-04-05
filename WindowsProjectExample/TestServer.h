#pragma once

#include "../seed-io/SeedServer.h"

#ifdef _WIN64
#pragma comment(lib, "../x64/Debug/seed-io.lib")
#else
#pragma comment(lib, "../Debug/seed-io.lib")
#endif

using namespace seedio;

class CTestServer :
	public CSeedServer
{
public:
	CTestServer();
	virtual ~CTestServer();
	virtual void Tick() override;
	virtual void OnListen();
	virtual void OnAccept(CBufferHandler& buffer) override;
	virtual void OnClose(CBufferHandler& buffer, bool force) override;
	virtual void OnRecv(CBufferHandler& buffer) override;
};

