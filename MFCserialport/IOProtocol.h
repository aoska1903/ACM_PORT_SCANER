#pragma once

#include "Serial.h"
#include <functional>

//----------------------------------------------------------------------//
// 사용자 정의
//----------------------------------------------------------------------//

// Desc: 데이터 수신 후 해당 데이터를 처리 할 함수 포인트, 다대선용
typedef std::function<VOID(_In_ BYTE*, _In_ INT, _In_ INT)> T_RECEIVE_EVENT;
typedef std::function<VOID(BYTE*, INT, BYTE)> T_REQUEST_RESEND;

// Desc: 데이터 수신 후 해당 데이터를 처리 할 함수 포인트, ACM용
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
		IO_CMD_PACKET_INIT	   = 0xD0,	// 단말기 초기화
		IO_CMD_PACKET_POLL	   = 0xD1,	// INPUT PORT 상태 요청
		IO_CMD_PACKET_RFCARD   = 0xD2,
		IO_CMD_PACKET_CONTROL  = 0xD3,	// Output port 제어 
		IO_CMD_PACKET_VERSION  = 0xD4,
		IO_CMD_PACKET_DOWNLOAD = 0xD5,		
		IO_CMD_PACKET_OUT_PORT = 0xD6,	// 개별 Output Port

		IO_CMD_PACKET_STATUS		= 0xE1,
		IO_CMD_PACKET_PRINTER_RESET = 0xF0, // 프린터 전원 reset
		IO_CMD_PACKET_BILL_RESET	= 0xF1, // 지폐처리장치 전원 reset
		IO_CMD_PACKET_RF_RESET		= 0xF2, // RF 모듈 전원 reset
		IO_CMD_PACKET_SOL_RESET		= 0xF4, // 솔레노이드 reset
		IO_CMD_PACKET_CM_RESET		= 0xF5, // 동전처리장치 전원 reset
		IO_CMD_PACKET_CD_RESET		= 0xF6  // 카드발매장치 전원 reset
	};

	// 카드발매장치 명령
	static enum CARDDISPENSER_CMD_PACKET {
		CD_CMD_PACKET_CLEAR			 = 0x30, // Error Clear
		CD_CMD_PACKET_STATUS		 = 0x31, // status request
		CD_CMD_PACKET_ISSUEING		 = 0x40, // card out
		CD_CMD_PACKET_BAUDRATE_9600  = 0x50, // baud rate set = 9600
		CD_CMD_PACKET_BAUDRATE_19200 = 0x51, // baud rate set = 19200
		CD_CMD_PACKET_ROMVERSION	 = 0x60  // rom version
	};

	// 포트 번호 설정
	inline void SetCOMPort(_In_ INT _comport){
		m_com_port = _comport; 
	}

	// 포트 번호 얻기
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
	
	// receive event 처리 스레드 생성
	BOOL CreateRecvEvent();

	// 수신 이벤트 핸들 및 스레드 종료
	VOID DeleteRecvEvent();

	BOOL  SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data=NULL, OPTIONAL SIZE_T data_len=0);

	inline VOID  RegisterReceiveFunc(T_RECEIVE_EVENT pFunc) { m_func_receive = std::move(pFunc); }
	inline VOID  RegisterReSendFunc(T_REQUEST_RESEND pFunc) { m_func_resend = std::move(pFunc); }

	//----------------------------------------------------------------------//
	// ACM Operation 

	// 명령 수행 완료 시 호출되는 콜백 함수 등록
	VOID IORegCallbackFunc_RecvComp(T_RECEIVE_DATA _func){	OperationComplete = std::move(_func); }

	BOOL ClearBuffer();

	// 시리얼 통신 연결
	BOOL ConnectACM(
		CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
		CSerial::EDataBits eDataBits = CSerial::EData8,
		CSerial::EParity   eParity   = CSerial::EParEven,
		CSerial::EStopBits eStopBits = CSerial::EStop1
		);

	// IO 명령 전송
	BOOL SendPowerCtrlToIO(_In_ BYTE _cmd, _In_ BYTE _ctrl_cmd);

	// IO 상태 요청
	BOOL SendInputCmdToIO(_In_ const BYTE _cmd);

	// card dispenser 명령 전송
	BOOL SendCmdToCardDispenser(_In_ const BYTE _cmd, _In_ BYTE _devid=0x00);

	// ACM - 시리얼 전송 이벤트 처리
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

	// 다대선 시리얼 전송 이벤트 처리
	VOID  WatchReceiveEvent();

	// 통신 포트 설정
	INT         m_com_port;


	BOOL		m_bConnect;
	
	BYTE		m_bSequnceNum;

	HANDLE		m_hOverlapped;	// overlapped operation 위한 핸들
	HANDLE		m_hStop;		// "STOP" 핸들		

	CWinThread	*m_pThread;

	BYTE		m_recv_buf[MAX_BUFFER];		// 수신 packet buffer	
	SIZE_T		m_recv_buf_len;				// 수신 packet 길이

	BYTE		m_send_buf[MAX_BUFFER];		// 송신 packet buffer
	SIZE_T		m_send_buf_len;				// 송신 packet 길이

	//CFileLog	m_log;

	T_RECEIVE_EVENT	 m_func_receive;		// Receive Evnet Thread에서 호출하는 함수 포인터
	T_REQUEST_RESEND m_func_resend;			// 재전송 요청 함수 포인터

	// 수신 이벤트 완료 처리 함수 포인터
	T_RECEIVE_DATA OperationComplete;
};

