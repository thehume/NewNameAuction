#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Pdh.lib")
#pragma comment(lib, "libmysql.lib")
#include <filesystem>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <list>
#include <locale.h>
#include <random>
#include <process.h>
#include <stdlib.h>
#include <iostream>
#include <Pdh.h>
#include <strsafe.h>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <conio.h>
#include <mysql.h>
#include <errmsg.h>
#include "log.h"
#include "ringbuffer.h"
#include "MemoryPoolBucket.h"
#include "Packet.h"
#include "profiler.h"
#include "dumpClass.h"
#include "CDBConnector.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "CNetServer.h"
#include "CommonProtocol.h"
#include "AuctionServer.h"
#include "ProcessMonitor.h"

using namespace std;

CrashDump myDump;


int main()
{
	PRO_INIT();

	WCHAR NetServer_OpenIP[20];
	int NetServer_OpenPort;
	int NetServer_maxThread;
	int NetServer_concurrentThread;
	int NetServer_Nagle;
	int NetServer_maxSession;

	GetPrivateProfileString(L"NetServer", L"openIP", L"0.0.0.0", NetServer_OpenIP, 20, L".\\ServerConfig.ini");
	NetServer_OpenPort = GetPrivateProfileInt(L"NetServer", L"openPort", 0, L".\\ServerConfig.ini");
	NetServer_maxThread = GetPrivateProfileInt(L"NetServer", L"maxThread", 0, L".\\ServerConfig.ini");
	NetServer_concurrentThread = GetPrivateProfileInt(L"NetServer", L"concurrentThread", 0, L".\\ServerConfig.ini");
	NetServer_Nagle = GetPrivateProfileInt(L"NetServer", L"Nagle", 0, L".\\ServerConfig.ini");
	NetServer_maxSession = GetPrivateProfileInt(L"NetServer", L"maxSession", 0, L".\\ServerConfig.ini");

	WCHAR DB_Address[100] = { 0, };
	WCHAR DB_User[50] = { 0, };
	WCHAR DB_Password[50] = { 0, };
	WCHAR DB_Name[50] = { 0, };
	int DB_Port = 0;

	GetPrivateProfileString(L"DataBase", L"DBIP", L"NULL", DB_Address, 100, L".\\ServerConfig.ini");
	GetPrivateProfileString(L"DataBase", L"User", L"NULL", DB_User, 50, L".\\ServerConfig.ini");
	GetPrivateProfileString(L"DataBase", L"Password", L"NULL", DB_Password, 50, L".\\ServerConfig.ini");
	GetPrivateProfileString(L"DataBase", L"DBName", L"NULL", DB_Name, 50, L".\\ServerConfig.ini");
	DB_Port = GetPrivateProfileInt(L"DataBase", L"DBPort", 0, L".\\ServerConfig.ini");

	WCHAR GameServer_OpenIP[20];
	int GameServer_OpenPort;

	GetPrivateProfileString(L"GameServer", L"openIP", L"0.0.0.0", GameServer_OpenIP, 20, L".\\ServerConfig.ini");
	GameServer_OpenPort = GetPrivateProfileInt(L"GameServer", L"openPort", 0, L".\\ServerConfig.ini");

	CInitParam initParam(NetServer_OpenIP, NetServer_OpenPort, NetServer_maxThread, NetServer_concurrentThread, NetServer_Nagle, NetServer_maxSession);
	CNetServer* pNetServer = new CNetServer(&initParam);
	AuctionServer* pAuctionServer = new AuctionServer;

	WCHAR DBIP[16] = { 0, };
	if (CNetServer::DomainToIP(DB_Address, DBIP) == false)
	{
		systemLog(L"Start Error", dfLOG_LEVEL_ERROR, L"DB Domain Error");
		return false;
	}

	//AuctionServer->setDBInfo(DBIP, DB_User, DB_Password, DB_Name, DB_Port);
	pNetServer->setDBInfo(DBIP, DB_User, DB_Password, DB_Name, DB_Port);

	CProcessMonitor Process_Monitor(GetCurrentProcess());

	volatile bool g_ShutDown = false;
	logInit();

	CContentsHandler HandleInstance;
	HandleInstance.attachServerInstance(pNetServer, pAuctionServer);

	pNetServer->attachHandler(&HandleInstance);
	pAuctionServer->attachServerInstance(pNetServer);

	if (pAuctionServer->Start() == false)
	{
		wprintf(L"ChatServer Thread init error");
		systemLog(L"Start Error", dfLOG_LEVEL_ERROR, L"ChatServer Thread init Error");
		return false;
	}

	if (pNetServer->Start() == false)
	{
		systemLog(L"Start Error", dfLOG_LEVEL_ERROR, L"NetServer Init Error, ErrorNo : %u, ErrorCode : %d", pNetServer->InitErrorNum, pNetServer->InitErrorCode);
		return false;
	}

	ULONGLONG startTime = GetTickCount64();
	ULONGLONG lastTime = 0;
	ULONGLONG nowTime = 0;
	ULONGLONG interval = 0;
	while (!g_ShutDown)
	{
		if (_kbhit())
		{
			WCHAR ControlKey = _getwch();
			if (L'q' == ControlKey || L'Q' == ControlKey)
			{
				g_ShutDown = true;
			}
		}
		pAuctionServer->updateJobCount();

		Process_Monitor.Update();

		wprintf(L"======================\n");
		wprintf(L"session number : %d\n", pNetServer->getSessionCount());
		wprintf(L"Character Number : %lld\n", pAuctionServer->getCharacterNum());
		wprintf(L"Accept Sum : %lld\n", pNetServer->getAcceptSum());
		wprintf(L"Accept TPS : %d\n", pNetServer->getAcceptTPS());
		wprintf(L"Disconnect TPS : %d\n", pNetServer->getDisconnectTPS());
		wprintf(L"Send TPS : %d\n", pNetServer->getSendMessageTPS());
		wprintf(L"Recv TPS : %d\n", pNetServer->getRecvMessageTPS());
		wprintf(L"JobQueue UseSize : %d\n", pAuctionServer->getJobQueueUseSize());
		wprintf(L"Job TPS : %d\n", pAuctionServer->getJobCount());
		wprintf(L"PacketPool UseSize : %d\n", CPacket::getPoolUseSize() * POOL_BUCKET_SIZE);
		wprintf(L"PlayerPool UseSize : %d\n", pAuctionServer->getPlayerPoolUseSize());
		wprintf(L"======================\n");
		wprintf(L"Process User Memory : %lld Bytes\n", (INT64)Process_Monitor.getProcessUserMemory());
		wprintf(L"Process Nonpaged Memory : %lld Bytes\n", (INT64)Process_Monitor.getProcessNonpagedMemory());
		wprintf(L"Process : %f %%, ", Process_Monitor.getProcessTotal());
		wprintf(L"ProcessKernel : %f %%, ", Process_Monitor.getProcessKernel());
		wprintf(L"ProcessUser : %f %%\n", Process_Monitor.getProcessUser());
		wprintf(L"======================\n");
		Sleep(1000);
	}

	pAuctionServer->Stop();
	pNetServer->Stop();

	PRO_LOG();
	return 0;
}