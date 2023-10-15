#pragma once

using namespace std;

#define NICKNAME_MAX_LENGTH 10
#define MAX_PLAYER 10000

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
    WCHAR MyNickName[NICKNAME_MAX_LENGTH];
    WCHAR TargetNickName[NICKNAME_MAX_LENGTH];
};


class AuctionServer
{
    friend class CContentsHandler;

public:

    struct st_Player //������ ���� ������
    {
        BOOL isValid;
        INT64 AccountNo;
        INT64 sessionID;
        WCHAR NickName[NICKNAME_MAX_LENGTH];
        INT32 Point;
        ULONGLONG lastTime;
    };

    struct st_Auction_InitData
    {
        INT8 Time;
        INT32 Price;
    };

    enum en_Auction_Status
    {
        en_AUCTION_SALE = 0,
        en_AUCTION_IMMINENT = 1,
        en_AUCTION_FIN = 2
    };

    enum en_Auction_Register_Status
    {
        en_REGISTER_SUCESS = 0,
        en_REGISTER_FAIL_SOLD = 1,
        en_REGISTER_FAIL_SERVER_SLOT = 2,
        en_REGISTER_FAIL_USER_SLOT = 3,
    };

    enum en_Auction_Change_NickName
    {
        en_CHANGE_NICKNAME_SUCESS = 0,
        en_CHANGE_NICKNAME_FAIL = 1,
    };

    enum en_Auction_Slot
    {
        en_MAX_SERVER_SLOT = 20000,
        en_MAX_USER_SLOT = 5,
    };

    struct st_AuctionData // ��� ������
    {
        enum
        {
            en_AUCTION_ITSMINE_MINE = 0,
            en_AUCTION_ITSMINE_OTHERS = 1,
        };

        enum
        {
            en_AUCTION_BIDDER_ME = 0,
            en_AUCTION_BIDDER_OTHER = 1,
        };

        uint64_t StartTime; //��� ���� �ð�
        uint64_t EndTime; //��� ���� �ð�
        INT8 Status; // 0 : �����, 1 : �����ӹ�, 2 : ����
        INT64 OwnerID; //�Ǹ��� ȸ����ȣ
        INT64 BidderID; //�ְ� ������ ȸ����ȣ
        WCHAR NickName[NICKNAME_MAX_LENGTH]; //�Ǹ� �г���
        INT32 Price; //�ְ� ������
        INT32 Count; //������ ������ ��� ��
    };

    struct st_NickNameList // ���κ� �г��� ���� ����Ʈ
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

    void CS_AUCTION_REQ_SALE(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH], INT8 Time, INT32 Price);
    void CS_AUCTION_RES_SALE(INT64 SessionID, INT8 Flag);
    void CS_AUCTION_REQ_SEARCH(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH]);
    void CS_AUCTION_RES_SEARCH(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH], INT8 ItsMine, INT8 Bidder, INT8 Status, INT32 Price, INT64 EndTime);
    void CS_AUCTION_REQ_BID(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH], INT32 Price);
    void CS_AUCTION_RES_BID(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH], INT8 Bidder, INT32 Price);
    void CS_AUCTION_RES_BID(CSessionSet* SessionSet, WCHAR NickName[NICKNAME_MAX_LENGTH], INT8 ItsMine, INT32 Price);
    void CS_AUCTION_RES_EXPIRE(CSessionSet* SessionSet, WCHAR NickName[NICKNAME_MAX_LENGTH]);
    void CS_AUCTION_REQ_CHANGE_NICKNAME(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH]);
    void CS_AUCTION_RES_CHANGE_NICKNAME(INT64 SessionID, INT8 Status);
    void SS_AUCTION_RES_PLAYER_INFO(INT64 SessionID, WCHAR NickName[NICKNAME_MAX_LENGTH], INT64 AccountNo, INT32 Point);



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
    unordered_map<wstring, st_AuctionData*> CellList;// ������ ��� ����Ʈ (key : �г���, data : �������)
    unordered_map<wstring, st_AuctionData*> ImminentList; //��� �����ӹ� ����Ʈ(key : �г���, data : �������)
    unordered_map<INT64, st_NickNameList> OwnList; //���κ� ��Ž��� ����Ʈ (key : ȸ����ȣ, data : ��Ž�������)
    list<st_AuctionData*> CellList_TimeOrder24; // �ð����� ��� ����Ʈ 24�ð� ����
    list<st_AuctionData*> CellList_TimeOrder48; // �ð����� ��� ����Ʈ 48�ð� ����
    map<int, st_AuctionData*> CellList_PriceOrder; // ���ݼ��� ��� ����Ʈ 
    list<st_AuctionData*> FinList; // ��� ���� ����Ʈ
    unordered_map<INT64, st_Auction_InitData> waitingDataList; // ���� ��ŵ�� �ӽõ�����

    alignas(64) LockFreeQueue<st_JobItem*> JobQueue;
    HANDLE hJobEvent;
    alignas(64) LockFreeQueue<st_JobItem_DBCheck*> JobQueue_DBThread;
    HANDLE hJobEvent_DBThread;
    alignas(64) CMemoryPoolBucket<st_JobItem> JobPool;
    alignas(64) CMemoryPoolBucket<st_JobItem_DBCheck> DBJobPool;
    alignas(64) CMemoryPoolBucket<st_Player> PlayerPool;
    alignas(64) CMemoryPoolBucket<st_AuctionData> AuctionDataPool;
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