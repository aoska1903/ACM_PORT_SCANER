#pragma once

static const unsigned short crc16tab[256]={
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

//----------------------------------------------------------------------//
// ����� ����
//----------------------------------------------------------------------//
static const INT COIN_RETRY_COUNT			= 3;	// ������ Ƚ��
static const INT COIN_MAX_BUFSIZE			= 1024;

//----------------------------------------------------------------------//
// Response Type
//----------------------------------------------------------------------//
#define COIN_ACTION_RESPONSE				0xA2
#define COIN_DATA_READ_RESPONSE				0xB2
#define COIN_CONDITION_RESPONSE				0xC2

// CONLUX Coin Response Type
#define ACK1								0x11
#define ACK2								0x22
#define ACK3								0x33
#define ACK4								0x44
#define ACK5								0x55
#define NAK_EE								0xEE

//----------------------------------------------------------------------//
// Text ������ Text Format ����ü ����
//----------------------------------------------------------------------//
#define COIN_TEXT_TYPE_POLLING				0xF0
#define COIN_TEXT_TYPE_COMMAND				0xF1
#define COIN_TEXT_TYPE_POLLING_RESPONSE		0xE0
#define COIN_TEXT_TYPE_COMMAND_RESPONSE		0xE1

//----------------------------------------------------------------------//
// ��Ŷ Response Event ���
//----------------------------------------------------------------------//
#define DATA_RECEIVE_READ_FAIL  0
#define DATA_RECEIVE_SUCCESS	1
#define DATA_RECEIVE_CONTINUE	2
#define DATA_RECEIVE_TIMEOUT	3

#pragma pack( push, 1 )

typedef struct  
{
	BYTE	start_code;		// 'ENQ' (0x05�� ����)
	BYTE	length;			
	BYTE	text_type;		// polling:0xF0, command:0xF1, polling ����:0xE0, Data ����:0xE1
	BYTE	sequence_num;	// message ���� Ƚ��
	BYTE    f1;				// Fixed : 0x03,  Except Command 0x5a => 0x0B, 0x70 => 0x05 
	BYTE	f2;				// Fixed : 0x02        M->S SendMessage : 0x02
							//                     M<-S RecvMessage : RRES : 0xB2
							//                                        ARES : 0xA2
							//										  CRES : 0xC2
	BYTE	text_data;		// command or data (polling �ÿ��� ����)
}T_COIN_TEXT_FORMAT;

typedef struct  
{
	BYTE	start_code;		// 'ENQ' (0x05�� ����)
	BYTE	length;			
	BYTE	text_type;		// polling:0xF0, command:0xF1, polling ����:0xE0, Data ����:0xE1
	BYTE	sequence_num;	// message ���� Ƚ��
}T_COIN_TEXT_POLLING_FORMAT;

typedef struct  
{
	BYTE command;		// Ŀ���
	BYTE reponse_type;	// ����
	BYTE send_buf[128];	// ���� ������
	BYTE recv_buf[128];	// ���� ������
	DWORD dwReadBytes;	// ���� ������ ����
	DWORD dwRetry;		// ��� �õ� Ƚ��
}ST_CONLUX_TEXT;

#pragma pack( pop )

#include <functional>
#include "Serial.h"

/*
 * Name:
 * IN  : INT(���� ���), BYTE*(���� ������), INT(������ ����) 
 * Out : ����
 * Desc: ������ ���� �� �ش� �����͸� ó�� �� �Լ� ����Ʈ
 */
typedef std::function<VOID(_In_ INT, _In_ BYTE*, _In_ INT)> T_RECEIVE_DATA;

//----------------------------------------------------------------------//
// Class CCoinProtocol Declare
//----------------------------------------------------------------------//
class CCoinProtocol:public CSerial
{
public:
	CCoinProtocol();
	virtual ~CCoinProtocol(void);

	//----------------------------------------------------------------------//
	// ���FA ����ó����ġ �ڸ�� ����
	//----------------------------------------------------------------------//
	enum COIN_CMD_PACKET{
		COIN_CMD_PACKET_NONE			=		0x00,
		COIN_CMD_PACKET_READ_STAUTS		=		0x01,
		COIN_CMD_PACKET_INIT			=		0x10,
		COIN_CMD_PACKET_TOTAL_END		=		0x13,
		COIN_CMD_PACKET_ALARM_DELETE	=		0x14,
		COIN_CMD_PACKET_SHUTTER_OPEN	=		0x16,
		COIN_CMD_PACKET_SHUTTER_CLOSE	=		0x17,
		COIN_CMD_PACKET_READ_UNIT		=		0x25,
		COIN_CMD_PACKET_READ_DATA		=		0x2E,
		COIN_CMD_PACKET_CLEAR_DATA		=		0x3E,
		COIN_CMD_PACKET_DISPENSE_10		=		0x50,
		COIN_CMD_PACKET_DISPENSE_50		=		0x51,
		COIN_CMD_PACKET_DISPENSE_100	=		0x52,
		COIN_CMD_PACKET_DISPENSE_500	=		0x53,
		COIN_CMD_PACKET_DISPENSE_S50	=		0x54,
		COIN_CMD_PACKET_DISPENSE_S100	=		0x55,
		COIN_CMD_PACKET_DISPENSE_ALL	=		0x56,
		COIN_CMD_PACKET_DISPENSE_STOP	=		0x57,
		COIN_CMD_PACKET_COIN_CHANGE		=		0x5A,
		COIN_CMD_PACKET_COIN_REJECT		=		0x5B,
		COIN_CMD_PACKET_BELT_FORWARD	=		0x5C,	// �ݼۺ�Ʈ�� �� 10�ʰ� ����
		COIN_CMD_PACKET_BELT_BACK		=		0x5D,
		COIN_CMD_PACKET_HOPPER_START	=		0x60,	
		COIN_CMD_PACKET_HOPPER_END		=		0x61,
		COIN_CMD_PACKET_WITHDRAW		=		0x70,	// Ư�� ȣ�� ȸ��
		COIN_CMD_PACKET_TEST_START		=		0x80,
		COIN_CMD_PACKET_TEST_END		=		0x81		
	};
	
	//----------------------------------------------------------------------//
	// �ܶ��� ����ó����ġ �ڸ�� ����
	//----------------------------------------------------------------------//
	
	// �ڸ�Ʈ
	enum CONLUX_COIN_COMMAND{
		CONLUX_CMD_STANDBY		= 0x60,	// ���ٹ���
		CONLUX_CMD_ORDER_ALL	= 0x61,	// �ϰ� �䱸 (�� ������ �۽�)
		CONLUX_CMD_ORDER_IN		= 0x62,	// �Ϸ� �䱸 (��ȭ ������ �۽�)	
		CONLUX_CMD_ORDER_OUT	= 0x63,	// ��� ���� (�����Ϳ� �ٰ��Ͽ� ����)
		CONLUX_CMD_REORDER_IN	= 0x64,	// �Ϸ� ��䱸
		CONLUX_CMD_REORDER_OUT	= 0x65	// ��� ������
	};

	// ������ �ڸ��
	enum CONLUX_COIN_DATA_COMMAND{
		CONLUX_DC_CONTROL		= 0x00,	// ����þ�� ��Ʈ��		
		CONLUX_DC_AMT_RETURN	= 0x01,	// �ݱݱݾ� 
		CONLUX_DC_INSERT		= 0x08,	// ���� ��ȭ �ż�
		CONLUX_DC_CHANGE		= 0x09,	// �ܵ� ���� �ż�
		CONLUX_DC_RETURN		= 0x0A,	// ���� �ż� 
		CONLUX_DC_STATUS		= 0x0B,	// ����þ�� ����
		CONLUX_DC_ERROR			= 0x0C,	// ����þ�� �̻�
		CONLUX_DC_FIXED			= 0x0D	// ���� ������ (����, �ڵ�)
	};

	// �������(DC=0) ���, ���� ����
	enum CONLUX_COIN_CTRL_CMD{
		CREM_OFF			= 0x00,
		CREM_ON				= 0x01,
		DC_CLEAR			= 0x02,
		COIN_RETURN			= 0x0A
	};

//----------------------------------------------------------------------//
// Member Operation Declare
//----------------------------------------------------------------------//
public:
	int read_ACK;//���Ű� ACK���� �Ǵ�

	BOOL ClearBuffer();

	// �α� ���� ����
	VOID CreateLog();

	// crc �� ���
	USHORT crc16_ccitt(_In_ const BYTE* databuf, IN INT nlen);

	// receive event ó�� ������ ����
	BOOL CreateRecvEvent();

	// ���� �̺�Ʈ �ڵ� �� ������ ����
	VOID DeleteRecvEvent();

	// ��� ���� ó��
	VOID Disconnect();	

	// ��Ʈ ����
	VOID SetCOMPort(_In_ INT _comport){
		m_port_no = _comport;
	}

	// ��Ʈ �� ��ȯ
	INT GetCOMPort(){
		return m_port_no;
	}

	BYTE GetLRC(_In_ const BYTE* pbData, _In_ INT iCount);

	VOID TestMode(_In_ HWND hwnd, BOOL b_testMode);

	//----------------------------------------------------------------------//
	// ���FA ����ó����ġ Operation

	// ��� ���� �� ����
	BOOL Connect( 
		CSerial::EBaudrate eBaudrate = CSerial::EBaud19200,
		CSerial::EDataBits eDataBits = CSerial::EData8,
		CSerial::EParity   eParity   = CSerial::EParNone,
		CSerial::EStopBits eStopBits = CSerial::EStop1,
		INT com_port_no=2
		);

	// ����ó����ġ ��Ŷ ����
	BOOL SendPacket(IN BYTE command_code, _In_ BYTE *p_data=NULL, IN SIZE_T data_len=0);

	// ����ó����ġ ���� ����
	BOOL SendPolling();

	// ��Ŷ ���� ó��
	BOOL RecvPacket(_Inout_ BYTE *p_byte, _Inout_ SIZE_T *p_len, IN DWORD dw_timeout);

	//----------------------------------------------------------------------//
	// CONLUX ����ó����ġ Operation
	
	// ��� ���� �� ����
	BOOL Connect_Conlux(
		CSerial::EBaudrate eBaudrate = CSerial::EBaud4800,
		CSerial::EDataBits eDataBits = CSerial::EData8,
		CSerial::EParity   eParity   = CSerial::EParEven,
		CSerial::EStopBits eStopBits = CSerial::EStop1
	);

	// ��� ���� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ� ���
	VOID CoinRegCallbackFunc_RecvComp(T_RECEIVE_DATA _func){
		CompleteReceiveData = std::move(_func);
	}

	// �ڸ�� ����
	BOOL SendCmdToConlux(_In_ const BYTE cmd_code);	

	// ������ ����
	BOOL SendDataToConlux(_In_ BYTE* pbyteData, _In_ INT nSize);

	// ���� Ƚ��(���FA ����ó����ġ)
	BYTE GetSequenceNum();

	INT ReadByte(IN DWORD dwTimeOut);

	INT RecvUnit();

	BOOL RecvText(IN DWORD dwTimeOut);

	BOOL SendText(IN BYTE command_code, _In_opt_ BYTE *p_data, IN SIZE_T data_len);

	BOOL SendCtrlChar(IN BYTE b_buf);

	// RTS �帧 ���� High/Low ����
	VOID SetRtsControl(_In_ DWORD _val);

	// ������ �Լ� (Working ȣ��)
	static UINT _threadEntry(_In_ LPVOID);

	// ��ŷ ������
	VOID _threadRecvEvent();		
	
	//----------------------------------------------------------------------//
	// Member Attribute Declare
	//----------------------------------------------------------------------//
private:
	INT         m_port_no;
	BOOL		m_flagConnect;					// ��Ʈ ���� ���� ����
	BYTE		m_bSequnceNum;
	BYTE		m_send_buf[COIN_MAX_BUFSIZE];	// �۽� packet buffer
	BYTE		m_recv_buf[COIN_MAX_BUFSIZE];	// ���� packet buffer
	SIZE_T		m_send_buf_len;					// �۽� packet ����
	SIZE_T		m_recv_buf_len;					// ���� packet ����

	HANDLE		m_hOverlapped;	// overlapped operation ���� �ڵ�
	HANDLE		m_hStop;		// "STOP" �ڵ�		
	
	CWinThread* m_pThread;		// work thread

	// ���� ��� �Ϸ� �ݹ� �Լ�
	T_RECEIVE_DATA CompleteReceiveData;
};


