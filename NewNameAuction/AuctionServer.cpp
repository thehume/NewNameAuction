#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "libmysql.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <chrono>
#include <list>
#include <random>
#include <locale.h>
#include <process.h>
#include <stdlib.h>
#include <iostream>
#include <strsafe.h>
#include <map>
#include <vector>
#include <unordered_map>
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
#include "CommonProtocol.h"
#include "CNetServer.h"
#include "AuctionServer.h"

using namespace std;
using namespace std::chrono;


AuctionServer::AuctionServer()
{
	ShutDownFlag = false;
	lastTime = 0;
	maxPlayer = en_MAX_PLAYER;
	LastUpdateTime_ms = 0;
	pNetServer = NULL;
	hJobEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hJobEvent == NULL)
	{
		CrashDump::Crash();
	}

	hJobEvent_DBThread = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hJobEvent_DBThread == NULL)
	{
		CrashDump::Crash();
	}

	random_generator = mt19937(random_device{}());
}


void AuctionServer::SendPacket_CS_AUCTION_REQ_SALE(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Time, INT32 Price)
{
	WORD Type = en_PACKET_REQ_SALE;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH *sizeof(WCHAR));
	*pPacket << Time;
	*pPacket << Price;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}


void AuctionServer::SendPacket_CS_AUCTION_RES_SALE(INT64 SessionID, INT8 Flag)
{
	WORD Type = en_PACKET_RES_SALE;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	*pPacket << Flag;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}

}


void AuctionServer::SendPacket_CS_AUCTION_REQ_SEARCH(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH])
{
	WORD Type = en_PACKET_REQ_SEARCH;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}


void AuctionServer::SendPacket_CS_AUCTION_RES_SEARCH(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 ItsMine, INT8 Bidder, INT8 Status, INT32 Price, INT64 EndTime)
{
	WORD Type = en_PACKET_RES_SEARCH;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << ItsMine;
	*pPacket << Bidder;
	*pPacket << Status;
	*pPacket << Price;
	*pPacket << EndTime;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}


void AuctionServer::SendPacket_CS_AUCTION_REQ_BID(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT32 Price)
{
	WORD Type = en_PACKET_REQ_BID;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << Price;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

void AuctionServer::SendPacket_CS_AUCTION_RES_BID(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Bidder, INT32 Price)
{
	WORD Type = en_PACKET_RES_BID;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << Bidder;
	*pPacket << Price;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

void AuctionServer::SendPacket_CS_AUCTION_RES_BID(CSessionSet* SessionSet, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 ItsMine, INT32 Price)
{
	WORD Type = en_PACKET_RES_BID;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << ItsMine;
	*pPacket << Price;

	pNetServer->sendPacket(SessionSet, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}


void AuctionServer::SendPacket_CS_AUCTION_RES_EXPIRE(CSessionSet* SessionSet, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Status)
{
	WORD Type = en_PACKET_RES_EXPIRE;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << Status;

	pNetServer->sendPacket(SessionSet, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

void AuctionServer::SendPacket_CS_AUCTION_REQ_CHANGE_NICKNAME(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH])
{
	WORD Type = en_PACKET_REQ_CHANGE_NICKNAME;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

void AuctionServer::SendPacket_CS_AUCTION_RES_CHANGE_NICKNAME(INT64 SessionID, INT8 Status)
{
	WORD Type = en_PACKET_RES_CHANGE_NICKNAME;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	*pPacket << Status;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

void AuctionServer::SendPacket_SS_AUCTION_RES_PLAYER_INFO(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT64 AccountNo, INT32 Point)
{
	WORD Type = en_PACKET_RES_PLAYER_INFO;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->Clear();
	pPacket->addRef(1);

	*pPacket << Type;
	pPacket->PutData((char*)NickName, en_NICKNAME_MAX_LENGTH * sizeof(WCHAR));
	*pPacket << AccountNo;
	*pPacket << Point;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

bool AuctionServer::packetProc_CS_AUCTION_REQ_SALE(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID)
{
	//------------------------------------------------------------
	// Client → Server 판매 등록 요청
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // 판매 닉네임
	//		INT8 Time // 경매등록시간(h) (24 or 48)
	//		INT32 Price // 최소입찰가	
	//	}
	//
	//------------------------------------------------------------
	WCHAR NickName[en_NICKNAME_MAX_LENGTH];
	INT8 Time;
	INT32 Price;

	pPacket->GetData((char*)NickName, sizeof(WCHAR) * en_NICKNAME_MAX_LENGTH);
	*pPacket >> Time >> Price;

	//접속캐릭터의 닉네임인지 확인
	if (wcscmp(pPlayer->NickName, NickName) != 0)
	{
		systemLog(L"Exception", dfLOG_LEVEL_DEBUG, L"packetProc_CS_AUCTION_REQ_SALE(), NickName1 : %s,  NickName2 : %s", pPlayer->NickName, NickName);
		return false;
	}
	
	st_Auction_InitData tempData;
	tempData.Time = Time;
	tempData.Price = Price;
	waitingDataList.insert(make_pair(pPlayer->AccountNo, tempData));



	//DB스레드로 넘겨서 DB조회후 판매된 닉네임인지 확인
	st_JobItem_DBCheck* DBCheckJob;
	DBJobPool.mAlloc(&DBCheckJob);
	DBCheckJob->JobType = en_JOB_NICKNAME_REGISTER_CHECK;
	DBCheckJob->AccountNo = pPlayer->AccountNo;
	DBCheckJob->SessionID = SessionID;
	wcscpy_s(DBCheckJob->MyNickName, NickName);
	JobQueue_DBThread.Enqueue(DBCheckJob);
	SetEvent(hJobEvent_DBThread);

	return true;
}

bool AuctionServer::packetProc_CS_AUCTION_REQ_SEARCH(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID)
{
	//------------------------------------------------------------
	// Client → Server 닉네임 검색 요청
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // 판매 닉네임
	//	}
	//
	//------------------------------------------------------------
	WCHAR NickName[en_NICKNAME_MAX_LENGTH];

	pPacket->GetData((char*)NickName, sizeof(WCHAR) * en_NICKNAME_MAX_LENGTH);

	//닉네임 wstring으로 바꿔서 검색, 전체순회하면서 부분문자열이 일치하는지 확인
	//일치하는 항목 발견시 send

	wstring wstr(NickName);
	for (auto iter = CellList.begin(); iter != CellList.end(); iter++)
	{
		if ((iter->first).find(wstr) != wstring::npos)
		{
			st_AuctionData* Data = iter->second;
			INT8 ItsMine;
			if (Data->OwnerID == pPlayer->AccountNo)//내 경매면
			{
				ItsMine = en_AUCTION_ITSMINE_MINE;
			}
			else
			{
				ItsMine = en_AUCTION_ITSMINE_OTHERS;
			}

			INT8 Bidder;
			if (Data->BidderID == pPlayer->AccountNo)
			{
				Bidder = en_AUCTION_BIDDER_ME;
			}
			else
			{
				Bidder = en_AUCTION_BIDDER_OTHER;
			}

			SendPacket_CS_AUCTION_RES_SEARCH(SessionID, Data->NickName, ItsMine, Bidder, Data->Status, Data->Price, Data->EndTime);
		}
	}

	for (auto iter = ImminentList.begin(); iter != ImminentList.end(); iter++)
	{
		if ((iter->first).find(wstr) != wstring::npos)
		{
			st_AuctionData* Data = iter->second;
			INT8 ItsMine;
			if (Data->OwnerID == pPlayer->AccountNo)//내 경매면
			{
				ItsMine = en_AUCTION_ITSMINE_MINE;
			}
			else
			{
				ItsMine = en_AUCTION_ITSMINE_OTHERS;
			}

			INT8 Bidder;
			if (Data->BidderID == pPlayer->AccountNo)
			{
				Bidder = en_AUCTION_BIDDER_ME;
			}
			else
			{
				Bidder = en_AUCTION_BIDDER_OTHER;
			}

			SendPacket_CS_AUCTION_RES_SEARCH(SessionID, Data->NickName, ItsMine, Bidder, Data->Status, Data->Price, Data->EndTime);
		}
	}
	return true;
}
bool AuctionServer::packetProc_CS_AUCTION_REQ_BID(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID)
{
	//------------------------------------------------------------
	// Client → Server 입찰 요청
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // 판매 닉네임
	//		INT32 Price // 입찰 금액
	// 
	//	}
	//
	//------------------------------------------------------------
	WCHAR NickName[en_NICKNAME_MAX_LENGTH];
	INT32 Price;

	pPacket->GetData((char*)NickName, sizeof(WCHAR) * en_NICKNAME_MAX_LENGTH);
	*pPacket >> Price;
	//현재 보유 포인트 확인
	if (pPlayer->Point != Price)
	{
		return false;
	}

	//닉네임 ImminentList, CellList에서 검색, 
	wstring wstr(NickName);
	st_AuctionData* pData = NULL;
	auto iter = ImminentList.find(wstr);
	if (iter != ImminentList.end())
	{
		pData = iter->second;
	}
	else
	{
		iter = CellList.find(wstr);
		if (iter != CellList.end())
		{
			pData = iter->second;
		}
	}
	//요청가격이 현재 입찰가보다 높을경우 입찰(경매정보변경). 포인트에서 차감. 
	if (pData == NULL || pData->Price >= Price || pData->OwnerID == pPlayer->AccountNo)
	{
		return true;
	}

	pPlayer->Point -= Price;
	pData->BidderID = pPlayer->AccountNo;
	pData->Price = Price;
	pData->Count++;
	
	SendPacket_CS_AUCTION_RES_BID(pPlayer->sessionID, NickName, en_AUCTION_BIDDER_ME, Price);

	CSessionSet SSet;
	SSet.setClear();
	for (auto iter = PlayerList.begin(); iter != PlayerList.end(); iter++)
	{
		if (iter->second->AccountNo != pPlayer->AccountNo)
		{
			SSet.setSession(iter->second->sessionID);
		}
	}
	SendPacket_CS_AUCTION_RES_BID(&SSet, NickName, en_AUCTION_BIDDER_OTHER, Price);

	return true;
}
bool AuctionServer::packetProc_CS_AUCTION_REQ_CHANGE_NICKNAME(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID)
{
	//------------------------------------------------------------
	// Client → Server 닉네임 변경 요청
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // 변경 요청 닉네임
	//		
	// 
	//	}
	//
	//------------------------------------------------------------

	WCHAR NickName[en_NICKNAME_MAX_LENGTH];
	pPacket->GetData((char*)NickName, sizeof(WCHAR) * en_NICKNAME_MAX_LENGTH);
	
	auto OwnList_iter = OwnList.find(pPlayer->AccountNo);
	st_NickNameList& NickNameList = OwnList_iter->second;
	NickNameList.RequestNickName = NickName;

	//DB스레드로 확인요청 토스 (현재닉네임, 바꿀닉네임)
	st_JobItem_DBCheck* DBCheckJob;
	DBJobPool.mAlloc(&DBCheckJob);
	DBCheckJob->JobType = en_JOB_NICKNAME_CHANGE_CHECK;
	DBCheckJob->AccountNo = pPlayer->AccountNo;
	DBCheckJob->SessionID = SessionID;
	wcscpy_s(DBCheckJob->MyNickName, pPlayer->NickName);
	wcscpy_s(DBCheckJob->TargetNickName, NickName);
	JobQueue_DBThread.Enqueue(DBCheckJob);
	SetEvent(hJobEvent_DBThread);

	return true;
}
bool AuctionServer::packetProc_SS_AUCTION_RES_PLAYER_INFO(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID)
{
	//------------------------------------------------------------
	// Server → Server 접속한 유저 정보 전달
	//
	//	{
	//		SHORT	Type
	//
	//		INT64 AccountNo //회원번호
	//		INT32 Point //닉네임 거래에 사용되는 재화
	//		WCHAR NickName[10] // 현재 접속한 캐릭터 닉네임
	//	}
	//
	//------------------------------------------------------------

	INT64 AccountNo;
	INT32 Point;
	WCHAR NickName[en_NICKNAME_MAX_LENGTH];

	*pPacket >> AccountNo >> Point;
	pPacket->GetData((char*)NickName, sizeof(WCHAR) * en_NICKNAME_MAX_LENGTH);

	auto iter = PlayerList.find(AccountNo);
	if (iter != PlayerList.end()) 
	{ 
		auto* pPlayer = iter->second;
		pPlayer->Point = Point;
		wcscpy_s(pPlayer->NickName, NickName);	
		SendInitPackets(pPlayer);
	}

	return true;
}


bool AuctionServer::PacketProc(st_Player* pPlayer, WORD PacketType, CPacket* pPacket, INT64 SessionID)
{
	switch (PacketType)
	{
	case en_PACKET_REQ_SALE:
		return packetProc_CS_AUCTION_REQ_SALE(pPlayer, pPacket, SessionID);
		break;

	case en_PACKET_REQ_SEARCH:
		return packetProc_CS_AUCTION_REQ_SEARCH(pPlayer, pPacket, SessionID);
		break;

	case en_PACKET_REQ_BID:
		return packetProc_CS_AUCTION_REQ_BID(pPlayer, pPacket, SessionID);
		break;

	case en_PACKET_REQ_CHANGE_NICKNAME:
		return packetProc_CS_AUCTION_REQ_CHANGE_NICKNAME(pPlayer, pPacket, SessionID);
		break;

	case en_PACKET_RES_PLAYER_INFO:
		return packetProc_SS_AUCTION_RES_PLAYER_INFO(pPlayer, pPacket, SessionID);
		break;

	default:
		return false;
	}
}


DWORD WINAPI AuctionServer::LogicThread(AuctionServer* pAuctionServer)
{
	st_JobItem* jobItem;
	WORD packetType;
	while (!pAuctionServer->ShutDownFlag)
	{
		while (pAuctionServer->JobQueue.Dequeue(&jobItem) == true)
		{
			pAuctionServer->Temp_JobCount++;
			pAuctionServer->Temp_JobCountperCycle++;

			INT64 JobType = jobItem->JobType;
			INT64 sessionID = jobItem->SessionID;
			INT64 AccountNo = jobItem->AccountNo;
			CPacket* pPacket = jobItem->pPacket;
			pAuctionServer->JobPool.mFree(jobItem);

			switch (JobType)
			{
			case en_JOB_ON_CLIENT_JOIN:
			{
				st_Player* pNewPlayer;
				pAuctionServer->PlayerPool.mAlloc(&pNewPlayer);
				
				pNewPlayer->AccountNo = 0;
				wcscpy_s(pNewPlayer->NickName, L"NULL");
				pNewPlayer->isValid = true;
				pNewPlayer->Point = 0;
				pNewPlayer->sessionID = sessionID;
				pNewPlayer->lastTime = GetTickCount64();
				pAuctionServer->PlayerList.insert(make_pair(sessionID, pNewPlayer));

				break;
			}

			case en_JOB_ON_CLIENT_LEAVE:
			{
				auto item = pAuctionServer->PlayerList.find(sessionID);
				if (item != pAuctionServer->PlayerList.end())
				{
					st_Player* pPlayer = item->second;
					pPlayer->isValid = false;

					pAuctionServer->waitingDataList.erase(pPlayer->AccountNo);
					pAuctionServer->PlayerList.erase(item);
					pAuctionServer->PlayerPool.mFree(pPlayer);

				}

				break;
			}

			case en_JOB_ON_RECV:
			{
				*pPacket >> packetType;

				auto item = pAuctionServer->PlayerList.find(sessionID);
				if (item == pAuctionServer->PlayerList.end())
				{
					if (pPacket->subRef() == 0)
					{
						CPacket::mFree(pPacket);
					}
					break;
				}

				st_Player& player = *item->second;
				if (player.sessionID != sessionID)
				{
					if (pPacket->subRef() == 0)
					{
						CPacket::mFree(pPacket);
					}
					break;
				}

				if (player.isValid == FALSE)
				{
					if (pPacket->subRef() == 0)
					{
						CPacket::mFree(pPacket);
					}
					break;
				}

				player.lastTime = GetTickCount64();
				bool ret = pAuctionServer->PacketProc(&player, packetType, pPacket, sessionID);
				if (ret == false)
				{
					st_Session* pSession;
					if (pAuctionServer->pNetServer->findSession(player.sessionID, &pSession) == true)
					{
						pAuctionServer->pNetServer->disconnectSession(pSession);
						if (InterlockedDecrement(&pSession->IOcount) == 0)
						{
							pAuctionServer->pNetServer->releaseSession(player.sessionID);
						}
					}
				}

				if (pPacket->subRef() == 0)
				{
					CPacket::mFree(pPacket);
				}
				break;
			}

			case en_JOB_NICKNAME_CHANGE_ABLE:
			{
				//OwnList에서 경매정보삭제
				auto OwnList_iter = pAuctionServer->OwnList.find(AccountNo);
				st_NickNameList& NickNameList = OwnList_iter->second;
				for (int i = 0; i < NickNameList.count; i++)
				{
					vector<wstring>& NickNames = NickNameList.NickNames;
					if (NickNames[i] == NickNameList.RequestNickName)
					{
						NickNames.erase(NickNames.begin()+i);
					}
					NickNameList.count--;
					NickNameList.RequestNickName = L"NULL";
				}
				//이후 원래 유저 DB에서 닉네임 변경해주는 작업 필요하다. 여기서는 생략
				break;
			}

			case en_JOB_NICKNAME_REGISTER_ABLE:
			{
				auto OwnList_iter = pAuctionServer->OwnList.find(AccountNo);
				if (OwnList_iter == pAuctionServer->OwnList.end())
				{
					st_NickNameList temp;
					temp.clear();

					pAuctionServer->OwnList.insert(make_pair(AccountNo, temp));
					OwnList_iter = pAuctionServer->OwnList.find(AccountNo);
				}

				st_NickNameList& NickNameList = OwnList_iter->second;

				//판매슬롯 확인
				if (NickNameList.count >= en_MAX_USER_SLOT)
				{
					pAuctionServer->SendPacket_CS_AUCTION_RES_SALE(sessionID, en_REGISTER_FAIL_USER_SLOT);
					break;
				}
				if (pAuctionServer->NumOfSales >= en_MAX_SERVER_SLOT)
				{
					pAuctionServer->SendPacket_CS_AUCTION_RES_SALE(sessionID, en_REGISTER_FAIL_SERVER_SLOT);
					break;
				}
				
				//문제없을시 등록
				auto waitingDataList_iter = pAuctionServer->waitingDataList.find(AccountNo);
				if (waitingDataList_iter == pAuctionServer->waitingDataList.end()) { break; }
				INT32 Price = waitingDataList_iter->second.Price;
				INT8 Time = waitingDataList_iter->second.Time;
				pAuctionServer->waitingDataList.erase(waitingDataList_iter);

				auto PlayerList_iter = pAuctionServer->PlayerList.find(AccountNo);
				if (PlayerList_iter == pAuctionServer->PlayerList.end()) { break; }

				st_AuctionData* AuctionData;
				pAuctionServer->AuctionDataPool.mAlloc(&AuctionData);
				
				AuctionData->BidderID = 0;
				AuctionData->Count = 0;
				uint64_t curTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
				AuctionData->StartTime = curTime;
				AuctionData->EndTime = curTime + (uint64_t)Time * 60 * 60 * 1000 - 10 * 60 * 1000;
				AuctionData->OwnerID = AccountNo;
				AuctionData->Price = Price;
				AuctionData->Status = en_AUCTION_SALE;
				wcscpy_s(AuctionData->NickName, PlayerList_iter->second->NickName);
				wstring wstr(AuctionData->NickName);
				pAuctionServer->CellList.insert(make_pair(wstr, AuctionData));
				pAuctionServer->CellList_PriceOrder.insert(make_pair(Price, AuctionData));
				if (Time > 24)
				{
					pAuctionServer->CellList_TimeOrder48.push_front(AuctionData);
				}
				else
				{
					pAuctionServer->CellList_TimeOrder24.push_front(AuctionData);
				}
				

				NickNameList.count++;
				NickNameList.NickNames.push_back(wstr);
				pAuctionServer->NumOfSales++;

				pAuctionServer->SendPacket_CS_AUCTION_RES_SALE(sessionID, en_REGISTER_SUCESS);

				//DB스레드로 닉네임 판매등록 요청 토스 (현재닉네임)
				st_JobItem_DBCheck* DBCheckJob;
				pAuctionServer->DBJobPool.mAlloc(&DBCheckJob);
				DBCheckJob->JobType = en_JOB_NICKNAME_CHANGE_CHECK;
				DBCheckJob->AccountNo = AccountNo;
				DBCheckJob->SessionID = sessionID;
				wcscpy_s(DBCheckJob->MyNickName, AuctionData->NickName);
				pAuctionServer->JobQueue_DBThread.Enqueue(DBCheckJob);
				SetEvent(pAuctionServer->hJobEvent_DBThread);

				systemLog(L"Auction", dfLOG_LEVEL_DEBUG, L"Auction Regist id : %d, NickName : %s", AccountNo, AuctionData->NickName);

				break;
			}

			}
		}
		
		pAuctionServer->Update();
		pAuctionServer->JobCountperCycle = pAuctionServer->Temp_JobCountperCycle;
		pAuctionServer->Temp_JobCountperCycle = 0;

		Sleep(100);
	}
	return true;
}

DWORD WINAPI AuctionServer::DBAccessThread(AuctionServer* pAuctionServer)
{
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(version, &data) != 0)
	{
		int ErrorCode = WSAGetLastError();
		systemLog(L"DB Thread Error", dfLOG_LEVEL_ERROR, L"WSAstartup Error, thread id : %d, Error code : %d", GetCurrentThreadId(), ErrorCode);
		return false;
	}

	//DB연결 수립
	CDBConnector DBConnector(L"127.0.0.1", L"root", L"1234", L"AuctionTest", 3306);
	if (DBConnector.Connect() == false)
	{
		WCHAR ErrorMsg[100];
		wcscpy_s(ErrorMsg, DBConnector.GetLastErrorMsg());
		systemLog(L"DB Thread Error", dfLOG_LEVEL_ERROR, L"Connect Error, thread id : %d, error : %s", GetCurrentThreadId(), ErrorMsg);
		CrashDump::Crash();
	}

	st_JobItem_DBCheck* jobItem;
	WORD packetType;
	while (!pAuctionServer->ShutDownFlag)
	{
		while (pAuctionServer->JobQueue_DBThread.Dequeue(&jobItem) == true)
		{
			//JOB Process
			INT64 JobType = jobItem->JobType;
			INT64 sessionID = jobItem->SessionID;
			INT64 AccountNo = jobItem->AccountNo;
			INT32 Count = jobItem->Count;
			uint64_t EndTime = jobItem->EndTime;
			WCHAR MyNickName[en_NICKNAME_MAX_LENGTH];
			WCHAR TargetNickName[en_NICKNAME_MAX_LENGTH];
			wcscpy_s(MyNickName, jobItem->MyNickName);
			wcscpy_s(TargetNickName, jobItem->TargetNickName);

			pAuctionServer->DBJobPool.mFree(jobItem);

			switch (JobType)
			{
			case en_JOB_NICKNAME_CHANGE_CHECK: // 내가 변경 가능한 닉네임인지 확인
			{
				DBConnector.sendQuery_Save(L"SELECT OwnerID FROM soldtable WHERE NickName = '%s'", TargetNickName);
				MYSQL_ROW sql_row;
				sql_row = DBConnector.FetchRow();
				if (sql_row != NULL && atoll(sql_row[0]) == AccountNo)
				{
					st_JobItem* jobItem;
					pAuctionServer->JobPool.mAlloc(&jobItem);
					jobItem->JobType = en_JOB_NICKNAME_CHANGE_ABLE;
					jobItem->SessionID = sessionID;
					jobItem->AccountNo = AccountNo;
					SetEvent(pAuctionServer->hJobEvent);
				}
				else
				{
					pAuctionServer->SendPacket_CS_AUCTION_RES_CHANGE_NICKNAME(sessionID, en_CHANGE_NICKNAME_FAIL);
				}
				DBConnector.FreeResult();
				break;
			}

			case en_JOB_NICKNAME_REGISTER_CHECK: //한번 팔린적 있는 닉네임인지 확인
			{
				DBConnector.sendQuery_Save(L"SELECT EXIST (SELECT * FROM soldtable WHERE NickName = '%s')", TargetNickName);
				MYSQL_ROW sql_row;
				sql_row = DBConnector.FetchRow();
				if (sql_row != NULL)
				{
					pAuctionServer->SendPacket_CS_AUCTION_RES_SALE(sessionID, en_REGISTER_FAIL_SOLD);
				}
				else
				{
					st_JobItem* jobItem;
					pAuctionServer->JobPool.mAlloc(&jobItem);
					jobItem->JobType = en_JOB_NICKNAME_REGISTER_ABLE;
					jobItem->SessionID = sessionID;
					jobItem->AccountNo = AccountNo;
					SetEvent(pAuctionServer->hJobEvent);
				}

				DBConnector.FreeResult();
				break;
			}

			case en_JOB_NICKNAME_REGISTER: //DB에 닉네임 등록
			{
				DBConnector.sendQuery(L"INSERT INTO cellingtable VALUES ('%s', %lld, %lld, %d)", MyNickName, AccountNo, EndTime, Count);
				break;
			}

			case en_JOB_NICKNAME_SOLD: //닉네임 판매완료
			{
				DBConnector.sendQuery(L"DELETE FROM cellingtable WHERE NickName = '%s'", MyNickName);
				if (AccountNo != 0) // 유찰이 아닐시
				{
					DBConnector.sendQuery(L"INSERT INTO soldtable VALUES ('%s', %lld, %lld, %d)", MyNickName, AccountNo, EndTime, Count);
				}
				break;
			}

			}


			WaitForSingleObject(pAuctionServer->hJobEvent_DBThread, INFINITE);
		}
		DBConnector.Disconnect();
		WSACleanup();
		return true;
	}
}

bool AuctionServer::Start()
{
	maxPlayer = pNetServer->getMaxSession();
	hLogicThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&LogicThread, this, 0, 0);
	if (hLogicThread == NULL)
	{
		return false;
	}

	hDBAccessThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DBAccessThread, this, 0, 0);
	if (hDBAccessThread == NULL)
	{
		return false;
	}

	return true;
}

bool AuctionServer::Stop()
{
	ShutDownFlag = true;
	WaitForSingleObject(hLogicThread, INFINITE);
	WaitForSingleObject(hDBAccessThread, INFINITE);
	return true;
}

void AuctionServer::Update()
{
	uint64_t CurTime_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	uniform_int_distribution<int> range(8*60000, 12*60000);
	
	if (CurTime_ms - LastUpdateTime_ms > 60000) //1분 간격
	{
		//CellList 순회, 마감임박 상태로 돌입한 경매들 랜덤으로 마감시간 +8~12분 후 ImminentList로 옮김
		for (auto iter = CellList.begin(); iter != CellList.end(); )
		{
			wstring wstr = iter->first;
			st_AuctionData* Data = iter->second;
			if (Data->EndTime < CurTime_ms)
			{

				iter = CellList.erase(iter);
				Data->Status = en_AUCTION_IMMINENT;
				Data->EndTime += range(random_generator);
				ImminentList.insert(make_pair(wstr, Data));
			}
			else
			{
				iter++;
			}
		}

		while (!CellList_TimeOrder24.empty())
		{   
			//CellList_TimeOrder24 뒤에서부터(시간 오래 지난 순) 순회, 경매 종료된 항목들 삭제
			auto iter = CellList_TimeOrder24.rbegin();
			st_AuctionData* Data = *iter;
			if (Data->Status == en_AUCTION_FIN || Data->Status == en_AUCTION_FIN_FAIL)
			{ 
				CellList_TimeOrder24.erase(iter.base());
			}
			else
			{
				break;
			}
		}

		while (!CellList_TimeOrder48.empty())
		{   
			//CellList_TimeOrder48 뒤에서부터(시간 오래 지난 순) 순회, 24시간 이하로 남은 항목들 TimeOrder24로 이동
			auto iter = CellList_TimeOrder48.rbegin();
			st_AuctionData* Data = *iter;
			if (Data->EndTime < CurTime_ms + (1000 * 60 * 60 * 24))
			{
				CellList_TimeOrder48.erase(iter.base());
				CellList_TimeOrder24.push_front(Data);
			}
			else
			{
				break;
			}

		}


		//CellList_PriceOrder 전체순회, 경매 종료된 항목들 삭제
		for (auto iter = CellList_PriceOrder.begin(); iter != CellList_PriceOrder.end(); )
		{
			st_AuctionData* Data = iter->second;
			if (Data->Status == en_AUCTION_FIN || Data->Status == en_AUCTION_FIN_FAIL)
			{

				iter = CellList_PriceOrder.erase(iter);
			}
			else
			{
				iter++;
			}
		}

	}
	
	if (CurTime_ms - LastUpdateTime_ms > 5000) //5초 간격
	{
		CSessionSet SSet;
		SSet.setClear();
		for (auto iter = PlayerList.begin(); iter != PlayerList.end(); iter++)
		{
			SSet.setSession(iter->second->sessionID);
		}

		//ImminentList 순회, 시간 지나면 전체에게 경매 완료 패킷 뿌림
		//완료된 경매는 FinList로 이동
		for (auto iter = ImminentList.begin(); iter != ImminentList.end(); )
		{
			st_AuctionData* Data = iter->second;
			if (Data->EndTime < CurTime_ms)
			{
				if (Data->BidderID == 0) // 유찰시
				{
					Data->Status = en_AUCTION_FIN_FAIL;
				}
				else
				{
					Data->Status = en_AUCTION_FIN;
					FinList.push_front(Data);
					FinList_UMap.insert(make_pair(iter->first, Data));
				}
				iter = ImminentList.erase(iter);
				SendPacket_CS_AUCTION_RES_EXPIRE(&SSet, Data->NickName, Data->Status);
				//DB스레드로 판매완료 통지
				st_JobItem_DBCheck* DBCheckJob;
				DBJobPool.mAlloc(&DBCheckJob);
				DBCheckJob->JobType = en_JOB_NICKNAME_SOLD;
				DBCheckJob->AccountNo = Data->BidderID;
				DBCheckJob->Count = Data->Count;
				wcscpy_s(DBCheckJob->MyNickName, Data->NickName);
				JobQueue_DBThread.Enqueue(DBCheckJob);
				SetEvent(hJobEvent_DBThread);
			
			}
			else
			{
				iter++;
			}
		}
	}


	LastUpdateTime_ms = CurTime_ms;
}


void AuctionServer::SendInitPackets(st_Player* pPlayer) //옥션 입장시 기본 정보들 전송
{
	INT64 AccountNo = pPlayer->AccountNo;
	INT64 SessionID = pPlayer->sessionID;
	auto iter_OwnList = OwnList.find(AccountNo);
	if (iter_OwnList != OwnList.end())
	{
		st_NickNameList& NickNameList = iter_OwnList->second;
		for (int i = 0; i < NickNameList.count; i++) //OwnList 순회하면서 슬롯에 대한 경매정보 전달
		{
			wstring& wstr = NickNameList.NickNames[i];
			st_AuctionData* pData = NULL;
			auto iter_CellList = CellList.find(wstr);
			if (iter_CellList != CellList.end())
			{
				pData = iter_CellList->second;
			}

			if (pData != NULL)
			{
				auto iter_ImminentList = ImminentList.find(wstr);
				if (iter_ImminentList != ImminentList.end())
				{
					pData = iter_ImminentList->second;
				}
			}

			if (pData != NULL)
			{
				auto iter_FintList = FinList_UMap.find(wstr);
				if (iter_FintList != FinList_UMap.end())
				{
					pData = iter_FintList->second;
				}

			}

			if (pData == NULL) // 유찰
			{
				WCHAR NickName[en_NICKNAME_MAX_LENGTH];
				wcscpy_s(NickName, wstr.c_str());
				SendPacket_CS_AUCTION_RES_SEARCH(pPlayer->sessionID, NickName, en_AUCTION_ITSMINE_MINE, 0, en_AUCTION_FIN_FAIL, 0, 0);
			}

			else
			{
				INT8 ItsMine = en_AUCTION_ITSMINE_OTHERS;
				INT8 Bidder = en_AUCTION_BIDDER_OTHER;
				INT64 EndTime = 0;

				if (pData->OwnerID == pPlayer->AccountNo) { ItsMine = en_AUCTION_ITSMINE_MINE;}
				if (pData->BidderID == pPlayer->AccountNo) { Bidder = en_AUCTION_BIDDER_ME; }
				if (pData->Status == en_AUCTION_SALE) { EndTime = pData->EndTime; }
				SendPacket_CS_AUCTION_RES_SEARCH(pPlayer->sessionID, pData->NickName, ItsMine, Bidder, pData->Status, pData->Price, EndTime);
			}
		}
	}

	int cnt = 0;
	for (auto iter = CellList_TimeOrder48.begin(); iter != CellList_TimeOrder48.end() && cnt<200; iter++) //최근 등록 경매정보 전달
	{
		INT8 ItsMine = en_AUCTION_ITSMINE_OTHERS;
		INT8 Bidder = en_AUCTION_BIDDER_OTHER;

		st_AuctionData* pData = *iter;
		if (pData->OwnerID == pPlayer->AccountNo) { ItsMine = en_AUCTION_ITSMINE_MINE; }
		if (pData->BidderID == pPlayer->AccountNo) { Bidder = en_AUCTION_BIDDER_ME; }
		SendPacket_CS_AUCTION_RES_SEARCH(pPlayer->sessionID, pData->NickName, ItsMine, Bidder, pData->Status, pData->Price, pData->EndTime);
		cnt++;
	}

	cnt = 0;
	for (auto iter = FinList.begin(); iter != FinList.end() && cnt < 200; iter++) //최근 낙찰 경매정보 전달
	{
		INT8 ItsMine = en_AUCTION_ITSMINE_OTHERS;
		INT8 Bidder = en_AUCTION_BIDDER_OTHER;

		st_AuctionData* pData = *iter;
		if (pData->OwnerID == pPlayer->AccountNo) { ItsMine = en_AUCTION_ITSMINE_MINE; }
		if (pData->BidderID == pPlayer->AccountNo) { Bidder = en_AUCTION_BIDDER_ME; }
		SendPacket_CS_AUCTION_RES_SEARCH(pPlayer->sessionID, pData->NickName, ItsMine, Bidder, pData->Status, pData->Price, pData->EndTime);
		cnt++;
	}	
}




void AuctionServer::updateJobCount(void)
{
	this->JobCount = this->Temp_JobCount;
	this->Temp_JobCount = 0;
	this->NumOfWFSO = this->Temp_NumOfWFSO;
	this->Temp_NumOfWFSO = 0;
}

size_t AuctionServer::getCharacterNum(void)
{
	return PlayerList.size();
}

LONG AuctionServer::getJobQueueUseSize(void)
{
	return this->JobQueue.nodeCount;
}

LONG AuctionServer::getJobCount(void)
{
	return this->JobCount;
}

LONG AuctionServer::getNumOfWFSO(void)
{
	return this->NumOfWFSO;
}

LONG AuctionServer::getJobCountperCycle(void)
{
	return this->JobCountperCycle;
}

LONG AuctionServer::getPlayerPoolUseSize(void)
{
	return this->PlayerPool.getUseSize();
}

LONG AuctionServer::getJobPoolUseSize(void)
{
	return this->JobPool.getUseSize();
}

void CContentsHandler::OnClientJoin(INT64 SessionID, int JoinFlag)
{
	AuctionServer::st_JobItem* jobItem;
	pAuctionServer->JobPool.mAlloc(&jobItem);
	jobItem->JobType = AuctionServer::en_JOB_ON_CLIENT_JOIN;
	jobItem->SessionID = SessionID;
	jobItem->pPacket = NULL;


	pAuctionServer->JobQueue.Enqueue(jobItem); // 해당 캐릭터 생성요청
	SetEvent(pAuctionServer->hJobEvent);
}

void CContentsHandler::OnClientLeave(INT64 SessionID)
{
	AuctionServer::st_JobItem* jobItem;
	pAuctionServer->JobPool.mAlloc(&jobItem);
	jobItem->JobType = AuctionServer::en_JOB_ON_CLIENT_LEAVE;
	jobItem->SessionID = SessionID;
	jobItem->pPacket = NULL;

	pAuctionServer->JobQueue.Enqueue(jobItem); //해당 캐릭터 삭제요청
	SetEvent(pAuctionServer->hJobEvent);
}

bool CContentsHandler::OnRecv(INT64 SessionID, CPacket* pPacket)
{
	pPacket->addRef(1);
	AuctionServer::st_JobItem* jobItem;
	pAuctionServer->JobPool.mAlloc(&jobItem);
	jobItem->JobType = AuctionServer::en_JOB_ON_RECV;
	jobItem->SessionID = SessionID;
	jobItem->pPacket = pPacket;

	if (pAuctionServer->JobQueue.Enqueue(jobItem) == false)
	{
		if (pPacket->subRef() == 0)
		{
			CPacket::mFree(pPacket);
		}
		return false;
	}
	SetEvent(pAuctionServer->hJobEvent);


	return true;
}