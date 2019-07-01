#pragma once

#include "Serial.h"
#include <functional>

//----------------------------------------------------------------------//
// ����� ����
//----------------------------------------------------------------------//

// Desc: ������ ���� �� �ش� �����͸� ó�� �� �Լ� ����Ʈ, �ٴ뼱��
typedef std::function<VOID(_In_ BYTE*, _In_ INT, _In_ INT)> T_RECEIVE_EVENT;
typedef std::function<VOID(BYTE*, INT, BYTE)> T_REQUEST_RESEND;

// Desc: ������ ���� �� �ش� �����͸� ó�� �� �Լ� ����Ʈ, ACM��
typedef std::function<VOID(_In_ INT, _In_ BYTE*, _In_ INT)> T_RECEIVE_DATA;


/************************* IO Board Packet Index List ************************/
#define IO_PACKET_INDEX_CMD			1
#define IO_PACKET_INDEX_LENGTH		3
#define IO_PACKET_INDEX_DATA		5


static const INT IO_RETRY_COUNT   = 3;


class CIOProtocol : public CSerial
{
public:
	CIOProtocol(void);
	virtual ~CIOProtocol(void);

	static enum IO_CMD_PACKET {
		IO_CMD_PACKET_INIT	   = 0xD0,	// �ܸ��� �ʱ�ȭ
		IO_CMD_PACKET_POLL	   = 0xD1,	// INPUT PORT ���� ��û
		IO_CMD_PACKET_RFCARD   = 0xD2,
		IO_CMD_PACKET_CONTROL  = 0xD3,	// Output port ���� 
		IO_CMD_PACKET_VERSION  = 0xD4,
		IO_CMD_PACKET_DOWNLOAD = 0xD5,		
		IO_CMD_PACKET_OUT_PORT = 0xD6,	// ���� Output Port

		IO_CMD_PACKET_STATUS		= 0xE1,
		IO_CMD_PACKET_PRINTER_RESET = 0xF0, // ������ ���� reset
		IO_CMD_PACKET_BILL_RESET	= 0xF1, // ����ó����ġ ���� reset
		IO_CMD_PACKET_RF_RESET		= 0xF2, // RF ��� ���� reset
		IO_CMD_PACKET_SOL_RESET		= 0xF4, // �ַ����̵� reset
		IO_CMD_PACKET_CM_RESET		= 0xF5, // ����ó����ġ ���� reset
		IO_CMD_PACKET_CD_RESET		= 0xF6  // ī��߸���ġ ���� reset
	};

	// ī��߸���ġ ���
	static enum CARDDISPENSER_CMD_PACKET {
		CD_CMD_PACKET_CLEAR			 = 0x30, // Error Clear
		CD_CMD_PACKET_STATUS		 = 0x31, // status request
		CD_CMD_PACKET_ISSUEING		 = 0x40, // card out
		CD_CMD_PACKET_BAUDRATE_9600  = 0x50, // baud rate set = 9600
		CD_CMD_PACKET_BAUDRATE_19200 = 0x51, // baud rate set = 19200
		CD_CMD_PACKET_ROMVERSION	 = 0x60  // rom version
	};

	// ��Ʈ ��ȣ ����
	inline void SetCOMPort(_In_ INT _comport){
		m_com_port = _comport; 
	}

	// ��Ʈ ��ȣ ���
	INT GetCOMPort(){
		return m_com_port;
	}

	VOID  NewLog();

	BOOL  Connect(CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
				  CSerial::EDataBits eDataBits = CSerial::EData8,
				  CSerial::EParity   eParity   = CSerial::EParNone,
				  CSerial::EStopBits eStopBits = CSerial::EStop1,
				  INT com_port_no=3);

	VOID  Disconnect();
	
	// receive event ó�� ������ ����
	BOOL CreateRecvEvent();

	// ���� �̺�Ʈ �ڵ� �� ������ ����
	VOID DeleteRecvEvent();

	BOOL  SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data=NULL, OPTIONAL SIZE_T data_len=0);

	inline VOID  RegisterReceiveFunc(T_RECEIVE_EVENT pFunc) { m_func_receive = std::move(pFunc); }
	inline VOID  RegisterReSendFunc(T_REQUEST_RESEND pFunc) { m_func_resend = std::move(pFunc); }

	//----------------------------------------------------------------------//
	// ACM Operation 

	// ��� ���� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ� ���
	VOID IORegCallbackFunc_RecvComp(T_RECEIVE_DATA _func){	OperationComplete = std::move(_func); }

	BOOL ClearBuffer();

	// �ø��� ��� ����
	BOOL ConnectACM(
		CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
		CSerial::EDataBits eDataBits = CSerial::EData8,
		CSerial::EParity   eParity   = CSerial::EParEven,
		CSerial::EStopBits eStopBits = CSerial::EStop1
		);

	// IO ��� ����
	BOOL SendPowerCtrlToIO(_In_ BYTE _cmd, _In_ BYTE _ctrl_cmd);

	// IO ���� ��û
	BOOL SendInputCmdToIO(_In_ const BYTE _cmd);

	// card dispenser ��� ����
	BOOL SendCmdToCardDispenser(_In_ const BYTE _cmd, _In_ BYTE _devid=0x00);

	// ACM - �ø��� ���� �̺�Ʈ ó��
	void _ReceiveEvent();

private:
	BYTE GetSequenceNum();
	BYTE CalculateBCC(_In_ BYTE *p_data, _In_ INT n_len);
	INT  EncodeEscape(_In_ BYTE *p_dst, _In_ BYTE *p_src, IN INT data_len); 
	INT  ConvertEscape(_In_ BYTE *p_dst, _In_ BYTE *p_src, IN INT src_len);

	BOOL SendText(IN BYTE command_code, _In_ BYTE *p_data, IN SIZE_T data_len);
	VOID RecvText();
	BOOL SendCtrlChar(IN BYTE b_char);	

	static UINT _ThreadEntry(_In_ LPVOID);

	// �ٴ뼱 �ø��� ���� �̺�Ʈ ó��
	VOID  WatchReceiveEvent();

	// ��� ��Ʈ ����
	INT         m_com_port;


	BOOL		m_bConnect;
	
	BYTE		m_bSequnceNum;

	HANDLE		m_hOverlapped;	// overlapped operation ���� �ڵ�
	HANDLE		m_hStop;		// "STOP" �ڵ�		

	CWinThread	*m_pThread;

	BYTE		m_recv_buf[MAX_BUFFER];		// ���� packet buffer	
	SIZE_T		m_recv_buf_len;				// ���� packet ����

	BYTE		m_send_buf[MAX_BUFFER];		// �۽� packet buffer
	SIZE_T		m_send_buf_len;				// �۽� packet ����

	//CFileLog	m_log;

	T_RECEIVE_EVENT	 m_func_receive;		// Receive Evnet Thread���� ȣ���ϴ� �Լ� ������
	T_REQUEST_RESEND m_func_resend;			// ������ ��û �Լ� ������

	// ���� �̺�Ʈ �Ϸ� ó�� �Լ� ������
	T_RECEIVE_DATA OperationComplete;
};

