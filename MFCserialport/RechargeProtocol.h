#pragma once
#include "Serial.h"
#include <functional>

#define ACM_MAX_PROTOCOL_SIZE (2048)

//----------------------------------------------------------------------//
// 사용자 정의
//----------------------------------------------------------------------//
static const INT CHARGE_MAX_BUFSIZE			= 8192;


/************************* Card Charge Packet Index List ************************/
#define CHARGE_PACKET_INDEX_CMD				1
#define CHARGE_PACKET_INDEX_LENGTH			2
#define CHARGE_PACKET_INDEX_DATA			4

/************************* Card Charge LSAM Card Type List ************************/
#define CHARGE_TYPE_LSAM_MB				0x4D			//마이비 LSAM
#define CHARGE_TYPE_LSAM_HN				0x48			//하나로 LSAM
#define CHARGE_TYPE_LSAM_TM				0x54            // Tmoney LSAM : 'T'
#define CHARGE_TYPE_LSAM_EB				0x45			// EB     LSAM : 'E'

enum ACM_E_STATE
{
	ACM_R_STATE_IDLE = 0,
	ACM_R_STATE_STX,
	ACM_R_STATE_LEN_H,
	ACM_R_STATE_LEN_L,
	ACM_R_STATE_CMD,
	ACM_R_STATE_DATA,  // ( 0 to LEN - 6 ) 
	ACM_R_STATE_ETX,
	ACM_R_STATE_BCC
};

typedef struct acm_packet_check 
{
	int r_state;   // 수신 스테이트 머신 
	unsigned char p_buffer [ ACM_MAX_PROTOCOL_SIZE ]; // 수신 버퍼 
	int p_index;   // 수신 버퍼 인덱스
	int i_sub;     // 수신 데이터 파트 인덱스 
	int d_size;    // 수신 데이터 파트 사이즈 
} ACM_PACKET_CHK;

typedef std::function<VOID(_In_ INT, _In_ BYTE*, _In_ INT)> T_RECEIVE_EVENT_CHARGE;

typedef std::function<VOID(BYTE*, INT, BYTE)> T_REQUEST_RESEND;

class CRechargeProtocol:public CSerial
{
public:
	CRechargeProtocol(void);
	~CRechargeProtocol(void);

	static enum CHARGE_CMD_PACKET {
		CHARGE_CMD_VER                  = 0x00,  // 버젼요청
		CHARGE_CMD_INIT                 = 0x01,  // 장비 초기화 시퀀스 동작
		CHARGE_CMD_POLL                 = 0x02,  // 장비 상태 스캔
		CHARGE_CMD_SEL                  = 0x03,  // SAM CHANNEL    SELECT
		CHARGE_CMD_RESET                = 0x04,  // SAM CHANNEL    RESET
		CHARGE_CMD_EXCHANGE             = 0x05,  // APDU EXCHANGE, CHANNEL SELECT 포함

		/* COMPLEX COMMAND     ( C COMMAND ) */
		CHARGE_CMD_CARD_READ            = 0xC0,  // 카드 읽기
		CHARGE_CMD_CARD_CHARGE          = 0xC1,  // 카드 충전
		CHARGE_CMD_SAM_INFO             = 0xC2,  // SAM 정보 조회
		CHARGE_CMD_CARD_RECHARGE        = 0xC3,  // 카드 재 충전 

		CHARGE_CMD_PACKET_POWER         = 0x53,		
		CHARGE_CMD_PACKET_CARDREAD      = 0x54,
		CHARGE_CMD_PACKET_READERROR     = 0x58,
		CHARGE_CMD_PACKET_CARDCHARGE    = 0x44,
		CHARGE_CMD_PACKET_CHARGEERROR   = 0x5A,
		CHARGE_CMD_PACKET_LSAMINFO      = 0x49,
		CHARGE_CMD_PACKET_LSAMACCESS    = 0x41,
		CHARGE_CMD_PACKET_LSAMSESSION   = 0x4F,
		CHARGE_CMD_PACKET_LSAMSERVER    = 0x4C,
		CHARGE_CMD_PACKET_LSAMINIT      = 0x45,
		CHARGE_CMD_PACKET_LSAMCHARGE    = 0x43, // 보충, 환불
		CHARGE_CMD_PACKET_REPAIRREAD    = 0x42, // 유지보수 카드 읽기
		CHARGE_CMD_PACKET_REPAIRCHARGE  = 0x46, // 유지보수 카드 보충
		CHARGE_CMD_PACKET_TERMINALID    = 0x4D, // 터미널 ID
		
		CHARGE_CMD_PACKET_EJECT         = 0x06, // 디지털카드 제거
		CHARGE_CMD_PACKET_NAK           = 0x15,

		CHARGE_CMD_PACKET_UPDATE_PARAM  = 0x61,
		CHARGE_CMD_PACKET_UPDATE_KEYSET = 0x62,
		CHARGE_CMD_PACKET_ADD_KEYSET    = 0x63,
		CHARGE_CMD_PACKET_FIRMWARE      = 0x7A,

		CHARGE_CMD_PACKET_PRINTER		= 0xE0	// 전표 프린트 명령
	};

	//----------------------------------------------------------------------//
	// Member Operation Declare
public:
	VOID  NewFileLog();

	// 포트 번호 설정
	void SetCOMPort(_In_ INT _comport){
		m_com_port = _comport;
	}

	// 포트 번호 얻기
	INT GetCOMPort(){
		return m_com_port;
	}

	// receive event 처리 스레드 생성
	BOOL CreateRecvEvent();

	// 수신 이벤트 핸들 및 스레드 종료
	VOID DeleteRecvEvent();

	BOOL  Connect(CSerial::EBaudrate eBaudrate = CSerial::EBaud115200,
		          CSerial::EDataBits eDataBits = CSerial::EData8,
		          CSerial::EParity   eParity   = CSerial::EParNone,
		          CSerial::EStopBits eStopBits = CSerial::EStop1);
		          
	VOID  Disconnect();
	BOOL  SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data=NULL, OPTIONAL SIZE_T data_len=0);
	VOID  TestMode(_In_ HWND hwnd, BOOL b_testMode);

	// 수신 이벤트 콜백 함수 등록
	inline VOID  RegisterReceiveFuncCharge(T_RECEIVE_EVENT_CHARGE pFunc) { ProcRecvEvent = std::move(pFunc); }
	//inline VOID  RegisterReSendFunc(T_REQUEST_RESEND pFunc) { m_func_resend = std::move(pFunc); }

private:
	int   chk_packet(unsigned char input);
	int   get_packet(unsigned char * p);

	BYTE  CalculateBCC(_In_ BYTE *p_data, _In_ INT n_len);
	BOOL  SendText(IN BYTE command_code, _In_ BYTE *p_data, IN SIZE_T data_len);
	VOID  RecvText();

	// working thread 
	static UINT _ThreadEntry(_In_ LPVOID);

	// thread에서 호출, 시리얼 수신 이벤트 처리 함수
	VOID        WatchReceiveEvent();	
	VOID _threadRecvEvent();	
	//----------------------------------------------------------------------//
	// Member Attribute Declare
private:
	INT              m_com_port;
	static INT       m_wait_time;
	CWinThread		 *m_pThread;
	BOOL			 m_bConnect;						// 포트 연결 판단 플래그
	BYTE			 m_recv_buf[CHARGE_MAX_BUFSIZE];	// 수신 packet buffer	
	SIZE_T			 m_recv_buf_len;					// 수신 packet 길이
	BYTE			 m_send_buf[CHARGE_MAX_BUFSIZE];	// 송신 packet buffer
	SIZE_T			 m_send_buf_len;					// 송신 packet 길이

	// 수신 완료 함수 포인터
	T_RECEIVE_EVENT_CHARGE	 ProcRecvEvent;

	ACM_PACKET_CHK   chk;

	HANDLE		m_hOverlapped;	// overlapped operation 위한 핸들
	HANDLE		m_hStop;		// "STOP" 핸들		
};

