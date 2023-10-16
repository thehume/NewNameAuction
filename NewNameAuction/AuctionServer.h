#pragma once

using namespace std;


class AuctionServer
{
    friend class CContentsHandler;

public:

    enum en_Auction_Status
    {
        en_AUCTION_SALE = 0,
        en_AUCTION_IMMINENT = 1,
        en_AUCTION_FIN = 2,
        en_AUCTION_FIN_FAIL = 3,

        en_REGISTER_SUCESS = 0,
        en_REGISTER_FAIL_SOLD = 1,
        en_REGISTER_FAIL_SERVER_SLOT = 2,
        en_REGISTER_FAIL_USER_SLOT = 3,

        en_CHANGE_NICKNAME_SUCESS = 0,
        en_CHANGE_NICKNAME_FAIL = 1,

        en_MAX_SERVER_SLOT = 20000,
        en_MAX_USER_SLOT = 5,

        en_AUCTION_ITSMINE_MINE = 0,
        en_AUCTION_ITSMINE_OTHERS = 1,

        en_AUCTION_BIDDER_ME = 0,
        en_AUCTION_BIDDER_OTHER = 1,

        en_NICKNAME_MAX_LENGTH = 10,
        en_MAX_PLAYER = 10000,
    };

    enum en_JobType
    {
        en_JOB_ON_CLIENT_JOIN,
        en_JOB_ON_RECV,
        en_JOB_ON_CLIENT_LEAVE,

        en_JOB_NICKNAME_CHANGE_CHECK,
        en_JOB_NICKNAME_CHANGE_ABLE,
        en_JOB_NICKNAME_REGISTER,
        en_JOB_NICKNAME_REGISTER_CHECK,
        en_JOB_NICKNAME_REGISTER_ABLE,
        en_JOB_NICKNAME_SOLD,
    };

    struct st_Player //접속한 유저 데이터
    {
        BOOL isValid;
        INT64 AccountNo;
        INT64 sessionID;
        WCHAR NickName[en_NICKNAME_MAX_LENGTH];
        INT32 Point;
        ULONGLONG lastTime;
    };

    struct st_JobItem
    {
        INT64 JobType;
        INT64 SessionID;
        INT64 AccountNo;
        CPacket* pPacket;
    };

    struct st_JobItem_DBCheck
    {
        INT64 JobType;
        INT64 AccountNo;
        INT64 SessionID;
        uint64_t EndTime;
        INT32 Count;
        WCHAR MyNickName[en_NICKNAME_MAX_LENGTH];
        WCHAR TargetNickName[en_NICKNAME_MAX_LENGTH];
    };

    struct st_Auction_InitData // 임시 보관 데이터
    {
        INT8 Time;
        INT32 Price;
    };

    struct st_AuctionData // 경매 데이터
    {
        uint64_t StartTime; //경매 시작 시간
        uint64_t EndTime; //경매 종료 시간
        INT8 Status; // 0 : 경매중, 1 : 마감임박, 2 : 판매완료, 3 : 유찰
        INT64 OwnerID; //판매자 회원번호
        INT64 BidderID; //최고가 입찰자 회원번호
        WCHAR NickName[en_NICKNAME_MAX_LENGTH]; //판매 닉네임
        INT32 Price; //최고 입찰가
        INT32 Count; //입찰에 참여한 사람 수
    };

    struct st_NickNameList // 개인별 닉네임 슬롯 리스트
    {
        int count;
        wstring RequestNickName;
        vector<wstring> NickNames;

        void clear()
        {
            count = 0;
        }
    };
    


    AuctionServer();
    void attachServerInstance(CNetServer* networkServer)
    {
        pNetServer = networkServer;
    }
    static DWORD WINAPI LogicThread(AuctionServer* pChatServer);
    static DWORD WINAPI DBAccessThread(AuctionServer* pAuctionServer);
    bool Start();
    bool Stop();
    void Update();
    void SendInitPackets(st_Player* pPlayer);

    void SendPacket_CS_AUCTION_REQ_SALE(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Time, INT32 Price);
    void SendPacket_CS_AUCTION_RES_SALE(INT64 SessionID, INT8 Flag);
    void SendPacket_CS_AUCTION_REQ_SEARCH(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH]);
    void SendPacket_CS_AUCTION_RES_SEARCH(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 ItsMine, INT8 Bidder, INT8 Status, INT32 Price, INT64 EndTime);
    void SendPacket_CS_AUCTION_REQ_BID(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT32 Price);
    void SendPacket_CS_AUCTION_RES_BID(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Bidder, INT32 Price);
    void SendPacket_CS_AUCTION_RES_BID(CSessionSet* SessionSet, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 ItsMine, INT32 Price);
    void SendPacket_CS_AUCTION_RES_EXPIRE(CSessionSet* SessionSet, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT8 Status);
    void SendPacket_CS_AUCTION_REQ_CHANGE_NICKNAME(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH]);
    void SendPacket_CS_AUCTION_RES_CHANGE_NICKNAME(INT64 SessionID, INT8 Status);
    void SendPacket_SS_AUCTION_RES_PLAYER_INFO(INT64 SessionID, WCHAR NickName[en_NICKNAME_MAX_LENGTH], INT64 AccountNo, INT32 Point);



    bool packetProc_CS_AUCTION_REQ_SALE(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID);
    bool packetProc_CS_AUCTION_REQ_SEARCH(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID);
    bool packetProc_CS_AUCTION_REQ_BID(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID);
    bool packetProc_CS_AUCTION_REQ_CHANGE_NICKNAME(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID);
    bool packetProc_SS_AUCTION_RES_PLAYER_INFO(st_Player* pPlayer, CPacket* pPacket, INT64 SessionID);


    bool PacketProc(st_Player* pPlayer, WORD PacketType, CPacket* pPacket, INT64 SessionID);
    
    void updateJobCount(void);
    size_t getCharacterNum(void); 
    LONG getJobQueueUseSize(void);
    LONG getJobCount(void);
    LONG getNumOfWFSO(void);
    LONG getJobCountperCycle(void);
    LONG getPlayerPoolUseSize(void);
    LONG getJobPoolUseSize(void);
    ULONGLONG Interval = 0;

    mt19937 random_generator;

private:
    HANDLE hLogicThread;
    HANDLE hDBAccessThread;
    volatile bool ShutDownFlag;
    int maxPlayer;
    uint64_t LastUpdateTime_ms;

    int NumOfSales = 0;

    int JobCount=0;
    int NumOfWFSO=0;
    int JobCountperCycle=0;

    int Temp_JobCount=0;
    int Temp_NumOfWFSO=0;
    int Temp_JobCountperCycle=0;

    ULONGLONG lastTime;

    CNetServer* pNetServer;

    unordered_map<INT64, st_Player*> PlayerList;
    unordered_map<wstring, st_AuctionData*> CellList;// 진행중 경매 리스트 (key : 닉네임, data : 경매정보)
    unordered_map<wstring, st_AuctionData*> ImminentList; //경매 종료임박 리스트(key : 닉네임, data : 경매정보)
    unordered_map<INT64, st_NickNameList> OwnList; //개인별 경매슬롯 리스트 (key : 회원번호, data : 경매슬롯정보)
    list<st_AuctionData*> CellList_TimeOrder24; // 시간순서 경매 리스트 24시간 이하
    list<st_AuctionData*> CellList_TimeOrder48; // 시간순서 경매 리스트 48시간 이하
    map<int, st_AuctionData*> CellList_PriceOrder; // 가격순서 경매 리스트 
    list<st_AuctionData*> FinList; // 경매 종료 리스트
    unordered_map<wstring, st_AuctionData*> FinList_UMap; // 경매 종료 리스트

    unordered_map<INT64, st_Auction_InitData> waitingDataList; // 옥션 경매등록 임시데이터

    LockFreeQueue<st_JobItem*> JobQueue;
    HANDLE hJobEvent;
    LockFreeQueue<st_JobItem_DBCheck*> JobQueue_DBThread;
    HANDLE hJobEvent_DBThread;
    CMemoryPoolBucket<st_JobItem> JobPool;
    CMemoryPoolBucket<st_JobItem_DBCheck> DBJobPool;
    CMemoryPoolBucket<st_Player> PlayerPool;
    CMemoryPoolBucket<st_AuctionData> AuctionDataPool;
};

class CContentsHandler : public CNetServerHandler
{
public:
    void attachServerInstance(CNetServer* networkServer, AuctionServer* contentsServer)
    {
        pNetServer = networkServer;
        pAuctionServer = contentsServer;
    }
    virtual bool OnConnectionRequest(WCHAR* IP, int* outParam) { return true; }
    virtual void OnClientJoin(INT64 SessionID, int JoinFlag);
    virtual void OnClientLeave(INT64 SessionID);
    virtual bool OnRecv(INT64 SessionID, CPacket* pPacket);
    virtual void OnError(int errorCode) {};

private:
    CNetServer* pNetServer;
    AuctionServer* pAuctionServer;
};