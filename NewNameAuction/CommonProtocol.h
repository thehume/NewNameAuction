#pragma once

enum en_PROTOCOL_TYPE
{
	
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////
	en_PACKET_REQ_SALE = 1,
	//------------------------------------------------------------
	// Client �� Server �Ǹ� ��� ��û
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // �Ǹ� �г���
	//		INT8 Time // ��ŵ�Ͻð�(h) (24 or 48)
	//		INT32 Price // �ּ�������	
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_RES_SALE = 2,
	//------------------------------------------------------------
	// Server �� Client �Ǹ� ��� ���
	//
	//	{
	//		SHORT	Type
	//
	//		INT8 flag // 0 : ����, 1 : ����(�̹� �Ǹŵ��� �ִ� �г���), 2 : ������ ��� ���� ����,  3 : ���κ� �Ǹ� ���� ����,
	// 
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_REQ_SEARCH = 3,
	//------------------------------------------------------------
	// Client �� Server �г��� �˻� ��û
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // �Ǹ� �г���
	//	}
	//
	//------------------------------------------------------------


	en_PACKET_RES_SEARCH = 4,
	//------------------------------------------------------------
	// Server �� Client �г��� �˻� ���
	//
	//	{
	//		SHORT	Type
	//	
	//		WCHAR NickName[10] // �Ǹ��� �г���
	//		INT8 ItsMine // ��� ���� (0 : �� ���, 1 : Ÿ���� ���)
	//		INT8 Bidder // (0 : ���� ����, 1 : Ÿ���� ����)
	//		INT8 Status // ���� (0 : ���������, 1 : �ǸſϷ�, 2 : �ǸŽ���(����))  
	//		INT32 Price // �ְ� ������
	//		INT64 EndTime // �����ð� (�����ð��� �Ѿ��ٸ� �����ӹ�)
	//	
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_REQ_BID = 5,
	//------------------------------------------------------------
	// Client �� Server ���� ��û
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // �Ǹ� �г���
	//		INT32 Price // ���� �ݾ�
	// 
	//	}
	//
	//------------------------------------------------------------



	en_PACKET_RES_BID = 6,
	//------------------------------------------------------------
	// Server �� Client ���� ����
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // �Ǹ� �г���
	//		INT8 Bidder // �� �������� (0 : �� ����, 1 : Ÿ���� ����)
	//		INT32 Price // ���� �ݾ�
	//	}
	//
	//------------------------------------------------------------


	en_PACKET_RES_EXPIRE = 7,
	//------------------------------------------------------------
	// Server �� Client ��� ����
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // �Ǹ� �г���
	// 
	//	}
	//
	//------------------------------------------------------------



	en_PACKET_REQ_CHANGE_NICKNAME = 8,
	//------------------------------------------------------------
	// Client �� Server �г��� ���� ��û
	//
	//	{
	//		SHORT	Type
	//
	//		WCHAR NickName[10] // ���� ��û �г���
	//		
	// 
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_RES_CHANGE_NICKNAME = 9,
	//------------------------------------------------------------
	// Server �� Client �г��� ���� ����
	//
	//	{
	//		SHORT	Type
	//
	//		INT8 Status // ���� (0 : ����, 1 : ����)
	// 
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_RES_PLAYER_INFO = 10,
	//------------------------------------------------------------
	// Server �� Server ������ ���� ���� ����
	//
	//	{
	//		SHORT	Type
	//
	//		INT64 AccountNo //ȸ����ȣ
	//		INT32 Point //�г��� �ŷ��� ���Ǵ� ��ȭ
	//		WCHAR NickName[10] // ���� ������ ĳ���� �г���
	//	}
	//
	//------------------------------------------------------------
};
