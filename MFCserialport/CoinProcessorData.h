#pragma once

typedef int                 INT;
typedef unsigned char       BYTE;
typedef unsigned long       DWORD;
typedef unsigned short      WORD;
typedef int                 BOOL;
typedef char TCHAR;
#pragma warning( disable : 4200 )

#define COIN_MMF_NAME		_T("COIN_PROCESSOR_MMF")
#include <WinBase.h>
//#include <MMFile.h>
// 동전 순환 호퍼 수
static const INT HOPPER_NUM = 4; 

// 동전 타입
static const INT COIN_TYPE[HOPPER_NUM] = {
	10, 50, 100, 500
};

//----------------------------------------------------------------------//
// 코인 모듈에서 올라오는 것이 아닌 이 외의 에러코드
//----------------------------------------------------------------------//
#define COIN_ERROR_CHANGE_NOT_ENOUGH			0xD1	// 배출잔돈 부족
#define COIN_ERROR_CHANGE_OVER					0xD2	// 배출잔돈 초과
#define COIN_ERROR_REJECT						0xD3	// 입수금액반환 오류
#define COIN_ERROR_NAK_3_TIMES					0xD4	// 명령시 NAK 3회이상 올라올때
#define COIN_ERROR_COUNTING						0xD5	// 계수이상의 발생시

static const BYTE COIN_NO_ERROR	= 0x00;

#ifndef WIN32
	#pragma pack(push,1)
#else
	#include <pshpack1.h>
#endif

//----------------------------------------------------------------------//
// Structure Declare
//----------------------------------------------------------------------//

// 재고 정보
typedef struct  
{
	WORD	wHopperNum[HOPPER_NUM];		// 순환 호퍼 수량
	DWORD	dwHopperAmt[HOPPER_NUM];	// 순환 호퍼 금액
	WORD	wSpareNum[2];				// 예비 호퍼 수량
	DWORD	dwSpareAmt[2];				// 예비 호퍼 금액
	WORD	wRejectNum[HOPPER_NUM];		// Reject box 동전 수량
	DWORD	dwRejectAmt;				// Reject box 내 총 금액
	WORD	wCoinBox[HOPPER_NUM];		// 동전회수함 수량
	DWORD	dwCoinBoxAmt;				// 동전회수함 금액
}T_COIN_HOLDING;


// 잔동 송출 패킷 정보
typedef struct  
{
	INT     amount;
	BYTE	b_topchanges;				// 상위 잔돈
	BYTE	b_bottomchanges;			// 하위 잔돈
	BYTE	b_changes[4];				// 호퍼 잔돈
	BYTE	b_s_changes[2];				// 예비 호퍼 잔돈	
}T_COIN_CHANGES;


// 잔동 송출 패킷 정보
typedef struct  
{	
	BYTE	b_topchanges;				// 상위 잔돈
	BYTE	b_bottomchanges;			// 하위 잔돈
	BYTE	b_changes[4];				// 호퍼 잔돈
	BYTE	b_s_changes[2];				// 예비 호퍼 잔돈	
}T_COIN_CHANGE_PACKET;


// 투입 정보
typedef struct
{	
	BYTE	b_coin_type[4];				// 동전 타입 별 투입 수량
	DWORD	dw_amount;					// 총 입수 된 금액
}T_COIN_INSERT;


// 투입, 공급, 회수 정보
typedef struct
{
	INT     n_s_coin_type[2]; 
	INT	    n_coin_type[4];				// 동전 타입 별 투입 수량
	INT     n_coin_box[4];
	DWORD	dw_amount;					// 총 입수 된 금액
}T_COIN_SUPPLY, T_COIN_COLLECT, T_COINBOX_COLLECT;


// 호퍼 내 동전 회수 데이터 구조체 
typedef struct  
{
	BYTE b_hopper_no;						// 회수 호퍼 번호
	BYTE b_withdraw_cnt;					// 회수 수량	
	WORD b_hopper_withdraw_cnt[HOPPER_NUM];	// 호퍼 별 회수 카운터	
	WORD b_spare_withdraw_cnt[2];			// 예비 호퍼 별 회수 카운터
}T_COIN_WITHDRAW;


// 동전처리장치 상태 정보 구조체
typedef struct 
{
	BYTE b_error_code;						// H/W 오류 코드

	BYTE bCoinVault;                        // 동전함상태   FULL, NEAR FULL
	BYTE bRejectBox;                        // 회수함 상태  Full, Near FULL
	BYTE bSpareHopper[HOPPER_NUM];			// 예비호퍼상태 NEAR EMPTY, EMPTY
	BYTE bHopper[HOPPER_NUM];				// 호퍼상태     NEAR EMPTY, EMPTY
	BOOL fSpareHopperError;                 // 예비호퍼 이상유무
	BOOL fHopperError;                      // 순환호퍼 이상유무
	BOOL fBeltError;                        // 반송벨트 이상유무
	BOOL fValidatorError;                   // Validator 이상유무

	BOOL fSpareHopperAbnormal[HOPPER_NUM];  // 예비호퍼 이상(각각)
	BOOL fHopperAbnormal[HOPPER_NUM];       // 순환호퍼 이상(각각)   
	BOOL fCoinModuleUnLocking;				// 모듈 장착상태
	BOOL fHopperUnLocking;					// 호퍼탈착상태 
	BOOL fShutterOpen;		
	BOOL device_connect;	
	BOOL cointube_missed;					// 튜브 탈착
} T_COIN_STATUS;


// 동전처리장치 알람
typedef struct  
{
	TCHAR alarm_key[5];
	TCHAR modulecode;
	TCHAR alarmcode[4];	
}T_COIN_ALARM;


// 동전처리장치 데이터
typedef struct  
{
	TCHAR               _ver[11];
	INT					com_port;
	INT					cmd;				// main->device 수행 코드	
	INT                 dev_cmd;            // device->main
	INT					n_result;			// 수행 결과 코드
	
	// audit
	DWORD				dw_unrAmt;
	DWORD				dw_overAmt;
	T_COIN_HOLDING		coin_holding;		// 동전과 회수함 보유량

	// operation audit
	DWORD				change_amount;		// 총 방출 금액 정보
	T_COIN_INSERT		insert_data;		// 투입 정보
	T_COIN_CHANGES		changes_data;		// 방출 예정 정보
	T_COIN_WITHDRAW		withdraw_data;		// 회수 정보 (호퍼 -> 동전함)
	T_COIN_SUPPLY       coin_supply;        // 동전 보충 정보
	T_COINBOX_COLLECT   coin_collect;       // 동전함 회수 정보
	
	// status	
	BOOL				device_open;
	BOOL				operationcomplete;	// 수행 완료 상태(TRUE:완료, FALSE:미완료)
	T_COIN_STATUS		coin_stat;			// 동전 처리 장치 상태
	INT					alarm_cnt;
	T_COIN_ALARM		coin_alarm[150];
}T_COIN_DATA;

#ifndef WIN32
	#pragma pack(pop)
#else
	#include <poppack.h>
#endif

//----------------------------------------------------------------------//
// PassengerUIData class declare
//----------------------------------------------------------------------//
class CCoinProcessorData
{
public:
	
	CCoinProcessorData(_In_ LPCTSTR lpMapName, IN const SIZE_T &nBytes, IN BOOL bOpen=FALSE)
	{
	}

	virtual ~CCoinProcessorData()
	{		
	}
	
	LPVOID GetDataPtr()
	{
	}

	BOOL WriteMMF(_In_ const T_COIN_DATA *DestData)
	{
	}

	BOOL ReadMMF(_Inout_ T_COIN_DATA *SrcData){
	}		
	
private:
	T_COIN_DATA *m_pData;
};

