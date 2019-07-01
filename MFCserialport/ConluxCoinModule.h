#pragma once
#include "moduletemplete.h"
#include "CoinProcessorData.h"
#include "TemplateTimer.h"
#include <functional>
//#include "Ini.h"

typedef std::function<VOID(_In_ const T_COIN_DATA*)> T_OPERATION_COMPLETE;

#ifndef WIN32
#pragma pack(push,1)
#else
#include <pshpack1.h>
#endif

//동전처리장치 상태
typedef struct _COIN_STATUS 
{
	UINT	CREMON:				1;	// 주화수입가능 상태
	UINT	INVENTORY:			1;	// 인벤토리 동작 상태
	UINT	CHANGE:				1;	// 잔돈 불출 가능 상태
	UINT	RETURN_SWON:		1;	// 반환 스위치 ON 
	UINT	CHANGE_COMPLETE:	1;	// 배출 종료 
	UINT	CLEAR_COMPLETE:		1;	// 클리어 완료
	UINT	INVENTORY_STAT:		1;	// 인벤토리 동작 금지 상태
	UINT	U8:					1;	// 항상 '0'
}T_CONLUX_STATUS;

union U_CONLUX_STATUS
{
	BYTE				data;
	T_CONLUX_STATUS		bit_data;
};

//동전처리장치 이상 데이터
typedef struct _COIN_ERROR 
{
	UINT	COINCHANGER:		1;	// 대표 이상
	UINT	WORKABLE:			1;	// 동작가
	UINT	U3:					1;	// 예비
	UINT	ACCEPTOR:			1;	// 악셉터 이상
	UINT	EMPTYSW_10:			1;	// 엠프티 스위치 이상 (10원)
	UINT	EMPTYSW_50:			1;	// 엠프티 스위치 이상 (50원)
	UINT	EMPTYSW_100:		1;	// 엠프티 스위치 이상 (100원)
	UINT	EMPTYSW_500:		1;	// 엠프티 스위치 이상 (500원)
	UINT	U9:					1;	// 예비
	UINT	U10:				1;	// 예비
	UINT	U11:				1;	// 예비
	UINT	U12:				1;	// 예비
	UINT	RETURN_SW:			1;	// 반환 스위치 이상
	UINT	COIN_RETURN_ERROR:	1;	// 주화 배출 불량
	UINT	SAFTY_SW:			1;	// 세프티 스위치 이상
	UINT	U16:				1;	// 예비
}T_CONLUX_ERROR;

union U_CONLUX_ERROR
{
	BYTE				data[2];
	T_CONLUX_ERROR		bit_data;
};

// 통신 데이터 구조체
typedef struct _st_com_info
{
	BYTE coin_command;	// 동전처리장치 코멘드
	BYTE response_type;	// 응답 (ie, ACK11, ACK22, ...) 	
	BYTE data_cmd;		// 데이터 커멘드
	BYTE ctrl_type;		// 제어데이터 타입(0: CREM OFF, 1:CREM ON, 2:CLEAR, ...) 
	BYTE recv_buf[128]; // 응답 데이터
	INT retry_cnt;		// 재시도 횟수
	INT data_size;		// 응답 데이터 크기
	bool is_data_send;	// 데이터 전송 유무(false: 데이터 전송 없음, true:데이터 전송 후 응답 대기)
}ST_CONLUX_COM_INFO;

// restore packing
#ifndef WIN32
#pragma pack(pop)
#else
#include <poppack.h>
#endif

class CCoinProtocol;

class CConluxCoinModule :
	public CModuleTemplete
{
public:
	CConluxCoinModule(void);
	virtual ~CConluxCoinModule(void);

//////////////////////////////////////////////////////////////////////////
// Member Function
public:
	// 명령 수행 완료 플래그 값 가져오기
	BOOL GetOperationCompleteFlag();

	// 초기화 함수
	VOID InitWork();	

	// 수신 결과 후 분석 처리
	VOID ReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	// 메모리, 객체 해제 및 종료 함수
	VOID ReleaseWork();	

	// 명령 수행 완료 시 호출되는 콜백 함수 등록
	VOID RegCallbackFunc_OperComp(T_OPERATION_COMPLETE _func){
		OperationComplete = std::move(_func);
	}

	// 불출 금액 수량 설정
	VOID SetChangesData(_In_ const T_COIN_CHANGES *pChanges);
	VOID SetChangesData(_In_ const INT pChanges);

	// 포트 설정 함수
	VOID SetComPort(_In_ INT _nPort){
		m_nComPortNo = _nPort;
	}
	
	// 로그 파일 포인트 설정
	//VOID SetFileLog(_In_ CFileLog* _pLog);
	
	// 수행 결과 값 설정
	VOID SetResult(_In_ const INT _ret);

	// 타임아웃 측정 타이머 시작/중지
	VOID TimeoutTimerStart(_In_ INT nInterval=1000);
	VOID TimeoutTimerStop();	
	VOID TimeoutEvent();

	// 셧터 닫기 대기 타이머 시작/중지
	VOID ShutterCloseTimerStart(_In_ INT nInterval=500);
	VOID ShutterCloseTimerStop();	
	VOID ShutterCloseEvent();

	// 테스트 로그 모드
	VOID TestModeLog(_In_ HWND _hwnd, _In_opt_ BOOL _bTestMode=TRUE);
	
private:	
	// BCD -> DEC Convert (256 미만)
	static inline int bcd2dec(BYTE hex){
		return (((hex & 0xF0) >> 4) * 10 + (hex & 0x0F));		
	}  

	// DEC -> BCD Convert (100 미만)
	static inline BYTE dec2bcd(INT dec){
		BYTE upper = dec / 10 ;
		BYTE lower = dec % 10 ; 
		return (upper << 4) | lower ; 
	}

	// 알람 발생 및 해제
	VOID AlarmOccurred(_In_ TCHAR* pCode, _In_opt_ BOOL bRelease=FALSE);

	// 회계 저장
	VOID AuditData();

	// 투입, 방출 등 회계 데이터 초기화
	VOID ClearAuditData();

	// 동전처리장치로 명령 송신
	BOOL CmdSendToCoin();

	// 동전처리장치 데이터 명령 송신
	BOOL DataCmdSendToCoin(_In_ const INT _coin_cmd);

	// 동전체인저로 데이터 송신 플래그 리턴
	bool IsDataSend();

	// 패킷 분석 함수
	VOID PacketAnalysis(){}
	INT StatusPacketAnalysis();				

	// 수행 후 수신 결과 데이터 분석
	VOID RecvDataAnalysis(_In_ const BYTE *pData, _In_ const INT nSize);

	// 데이터 통신 재시도
	VOID RetryCOM(_In_opt_ BOOL _bNotConnectError=FALSE);

	// 요청 명령 결과 기록 후 이벤트 호출
	VOID ResponseResultIn();			

	// 공용 메모리 맵에 요청 결과 데이터 복사 
	VOID ResponseResultOut(){};			

	// Retry 카운터 초기화
	VOID ResetRetryCnt();

	// 요청 명령 코드 Get/Set
	INT GetCommand();
	VOID SetCommand(_In_ INT &_cmd);	

	// 동전처리장치 명령 코드 Get/Set
	BYTE GetCoinCmd();
	VOID SetCoinCmd(_In_ BYTE _cmd, _In_opt_ BYTE _data_cmd = 0x00, _In_opt_ BYTE _ctrl_type=0xFF);	

	// 동전 제어데이터 커멘드
	BYTE GetCoinCtrlCmd();

	// 데이터 커멘트 저장
	BYTE GetDataCmd();

	// 요청 명령 코드 분석 후 수행
	VOID SendCmd(_In_ INT _command);

	// 동전 명령 전송 헬퍼 함수
	BOOL SendCoinCmd(_In_ BOOL _bDataCmd, _In_opt_ BYTE _cmd, _In_opt_ BYTE _data_cmd = 0x00, _In_opt_ BYTE _ctrl_cmd=0xFF);

	// 명령 수행 완료 플래그 설정
	VOID SetOperationCompleteFlag(_In_ BOOL bComplete);

	// 스레드 함수 (Working 호출)
	static UINT _threadEntry(_In_ LPVOID);

	// 워킹 스레드
	VOID Working();				 

	// 거래 취소 행정
	INT procCancel();

	// 방출 명령 행정
	INT procChange();

	// 클리어 행정
	INT procClear();

	// 셧터 닫기
	INT procClose();

//////////////////////////////////////////////////////////////////////////
// Member Variable
public:
	static CCriticalSection m_cs;

private:
	// 포트 번호
	INT m_nComPortNo;

	// 시리얼 통신 헬퍼 클래스 객체
	CCoinProtocol* m_protocol;

	// 동전처리장치 공용 데이터 구조체 포인트		
	static T_COIN_DATA *m_pData;	

	// 동전처리장치 통신 정보 구조체
	static ST_CONLUX_COM_INFO *m_pstPack;

	// 수행 명령 완료 콜백 함수
	T_OPERATION_COMPLETE OperationComplete;

	// 시리얼 통신 타임아웃 측정 타이머
	//TTimer<CConluxCoinModule> m_tm_timeout;

	// 셧터 닫기 대기 타임아웃 
	//TTimer<CConluxCoinModule> m_tm_close;

	// 동전처리장치 상태
	U_CONLUX_STATUS m_status;
	U_CONLUX_STATUS  m_bef_stat;

	// 동전처리장치 오류
	U_CONLUX_ERROR  m_error_stat;
	U_CONLUX_ERROR  m_bef_error_stat;

	// 동전처리장치 알람 맵(Map)
	CMap<CString, LPCTSTR, T_COIN_ALARM, T_COIN_ALARM&> map_alarm;

	// 동전 불출 금액 수량 데이터
	T_COIN_CHANGES m_changes;

	// 동전 튜브 탈착 여부 플래그
	BOOL m_bCoinTubeMissed;

	// 셧터 닫기 여부 플래그
	BOOL m_bCloseShutter;

	// 동전 거래 취소 행정 중 금액 반환 여부 판단하는 플래그
	BOOL m_bCointReturn; // TRUE: 금액 반환, FALSE: 금액 반환 없음

	// 동전 금액 방출 진행 중인지 판단하는 플래그
	BOOL m_bChangeComplete;
	
	// 동전 투입 준비 플래그
	bool m_bCoinReady;

	// 시험 동작 플래그
	bool m_bTestMode;

	// 동전 방출 수행 결과
	INT m_nCoinChangeRet;

	// 총 금액 임시 저장
	INT m_nTmpAmt;

	BYTE m_nCurCtrlCmd;

	//CIni m_ini_ver;
};

