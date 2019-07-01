#pragma once


#include "ModuleTemplete.h"
#include "TemplateTimer.h"
#include "BillProcessorData.h"
#include "Ini.h"

#define EXECUTE_SUCCESS					1		// 오류 없음
#define BXR_NO_ERROR                    (0) 
#define EXECUTE_FAIL					0		// 실행 오류	
#define XFS_E_ILLEGAL           (1010)  /**< Illegal request. */
#define XFS_C_CDR_LCU_NA                (6019)
#define SPINE_COVER                                       (0x0F0002)  /**< Sensor identifier of Spine Cover. To locate the Spine Cover, see diagram @ref BNR_FUNCTIONAL_ELEMENTS "BNR Modules &amp; Functional Elements". \n\n <b>Decimal value:</b> 983042. */
#define SPINE_BILL_SENSOR_1                           (0x030702)                                          /**< @deprecated See #MIN_SPINE_BILL_SENSOR_ID (2015-10-15). */
#define SPINE_BILL_SENSOR_3                           (0x030703)                                          /**< @deprecated See #MIN_SPINE_BILL_SENSOR_ID (2015-10-15). */
#define SPINE_BILL_SENSOR_5                           (0x030704)                                          /**< @deprecated See #MIN_SPINE_BILL_SENSOR_ID (2015-10-15). */
#define SPINE_BILL_SENSOR_7                           (0x030705)                                          /**< @deprecated See #MIN_SPINE_BILL_SENSOR_ID (2015-10-15). */
#define SPINE_BILL_SENSOR_9                           (0x030706)                                          /**< @deprecated See #MIN_SPINE_BILL_SENSOR_ID (2015-10-15). */
#define SPINEDIVERTERMAX                              (MAX_SPINE_DIVERTER_ID)                             /**< @deprecated Use #MAX_SPINE_DIVERTER_ID (2012-12-04). */
#define SPINEDIVERTERMIN                              (SPINE_DIVERTER_CLASS)                              /**< @deprecated See #MIN_SPINE_DIVERTER_ID or use #SPINE_DIVERTER_CLASS (2012-12-04). */
#define STACKER                                       (STACKER_ID)                                        /**< @deprecated Use #STACKER_ID (2009-01-26). */
#define STACKER_HOME_POSITION_DETECTOR                (STACKER_HOME_POSITION_SENSOR)                      /**< @deprecated Use #STACKER_HOME_POSITION_SENSOR (2012-12-04). */
#define MIN_SPINE_DIVERTER_ID    (SPINE_DIVERTER_CLASS + 0x01)        /**< First possible identifier of a Diverter in the Spine. The last 2 hexadecimal digits are the Diverter index. It matches the position in the Spine. \n\n #MIN_SPINE_DIVERTER_ID corresponds to the Diverter in position 1. \n\n <b>Decimal value:</b> 262145 */ 
#define MAX_SPINE_DIVERTER_ID    (SPINE_DIVERTER_CLASS + NBMAXPCU)    /**< Last possible identifier of a Diverter in the Spine. <b>Decimal value:</b> 262155 */ 


// 모듈에서 공통 사용 요청 수행 코드 정의
enum COMMON_CMD_CODE {
	CMD_CONNECT	= 3001,
	CMD_RECONNECT,			
	CMD_DISCONNECT,         
	CMD_TEST_MODE_ON,
	CMD_TEST_MODE_OFF,			
	CMD_LOG_RESET,
	CMD_OPEN_DOOR,
	CMD_CLOSE_DOOR,
	CMD_CHECK_SHUTTER,

	// LED 제어 명령	
	CMD_LED_ALL_OFF,	
	CMD_LED_INSERT_ON,			// 지폐 투입구	
	CMD_LED_CARD_ON,			// 카드 투입구	
	CMD_LED_RECEIPT_ON,			// 영수증부 	
	CMD_LED_OUTPUT_ON,			// 배출부	
	CMD_LED_TEST_INSERT_ON,		// 지폐 투입구
	CMD_LED_TEST_INSERT_OFF,	
	CMD_LED_TEST_CARD_ON,		// 카드 투입구
	CMD_LED_TEST_CARD_OFF,	
	CMD_LED_TEST_RECEIPT_ON,	// 영수증부 
	CMD_LED_TEST_RECEIPT_OFF,
	CMD_LED_TEST_OUTPUT_ON,		// 배출부
	CMD_LED_TEST_OUTPUT_OFF,

	MSG_ACM_JAM,				// JAM 발생

	CMD_AD_PLAY					// 광고 영상 보여주기
};

// 지폐처리장치 명령 코드와 특별 통지 코드 정의
enum BILL_CMD_CODE {	
	CMD_BILL_NONE						= 400,
	CMD_BILL_THREAD_START               ,
	CMD_BILL_MODULE_INFO				,
	CMD_BILL_ALARM_DELETE				,       // 알람 삭제
	CMD_BILL_RESET						,		// 리셋
	CMD_BILL_CASHIN_START				,		// 투입 모드 시작
	CMD_BILL_CASHIN                     ,       // 투입 모드
	CMD_BILL_CASHINROLLBACK				,		// 반환
	CMD_BILL_CASHIN_END					,		// 투입 모드 종료
	CMD_BILL_CASH_DISPENSE				,		// 방출
	CMD_BILL_CASH_DISPENSE_RETRY		,		// 방출 시도
	CMD_BILL_PRESENT					,		// 지폐 내보내기
	CMD_BILL_EMPTY						,		// 지폐 회수
	CMD_BILL_CASHBOX_EMPTY				,		// 지폐회수함 회수
	CMD_BILL_CHANGES					,		// 거스름 방출 전체 행정
	CMD_BILL_CANCEL_TRANSACTION			,		// 취소 전체 행정
	CMD_BILL_RECEIPT					,		// 지폐처리장치 Receipt(Escorow->Cashbox)
	CMD_BILL_MODULE_LOCK				,		// Recycler, Loader Lock 관련 명령
	CMD_BILL_COLLECT                    ,       // 지폐 회수
	CMD_BILL_LOADER_SUPPLY              ,       // 지폐 보급
	NOTIFY_BILL_INSERT					,		// 지폐 입수 	
	NOTIFY_BILL_DISPENSE				,		// 지폐 방출
	NOTIFY_BILL_STATUS					,		// 상태 정보	
	NOTIFY_BILL_CASHBOX_MISSED          ,       // 회수함 탈착
	NOTIFY_BILL_CASHBOX_INOP            ,       // 회수함 장착
	NOTIFY_BILL_LOADER_MISSED           ,       // 로더 탈착
	NOTIFY_BILL_LOADER_INOP             ,       // 로더 장착
	NOTIFY_BILL_JAM                     ,       // 지폐 걸림
	CMD_BILL_AUDITCLR                   ,       // 회계 초기화
	CMD_BILL_OUTLET_CHK                 ,       // 지폐가 아웃렛 베젤에 있는지 검사
	NOTIFY_BILL_IN_JAM                  ,
	NOTIFY_BILL_MODULE_CASHLOCK                 // 지폐환류기 모듈 Cash Lock 열기->잠금 
};

class CBillProtocol;

class CBillModule : public CModuleTemplete
{
public:
	CBillModule(HWND hParent);           // 동적 만들기에 사용되는 protected 생성자입니다.
	virtual ~CBillModule();

	//----------------------------------------------------------------------//
	// Member Function Declare
public:	
	VOID  TestModeOFF();	
	VOID  TestModeON(HWND _wndParent);

	// timer
	VOID  TimerStart(IN UINT wait_sec, IN INT TimerID);
	VOID  TimerStop(IN INT TimerID);

	VOID  OnTimerEvent_Status();
	VOID  OnTimerEvent_Present();
	VOID  OnTimerEvent_Cashlock();

	//----------------------------------------------------------------------//
	// Override Function
	//----------------------------------------------------------------------//
//	VOID  SetFilelog(_In_ CFileLog *_pLog){m_log = *_pLog;}
	VOID  InitWork(_In_ HANDLE hEvent);
	VOID  ExitWork();
	static UINT _ThreadEntry(_In_ LPVOID);	// Work Thread	
	VOID  Working();		
	VOID  SendCmd(IN INT command);
	VOID  PacketAnalysis();

	//----------------------------------------------------------------------//
	// Membeer operation Function 
	//----------------------------------------------------------------------//
	VOID  BNRStatus();
    VOID  BNRStatusChange(IN UINT32 *n_detail);
	VOID  BNRThreshold();

	VOID  ConfigurationChanged(_In_ LPVOID data);

	VOID  OperationComplete(_In_ LPVOID data, IN DWORD operationId, IN DWORD result);
	VOID  StatusEvent(IN LONG32 status, IN LONG32 result, _In_ LPVOID data);
	VOID  IntermediateOccured(IN LONG32 opID);
	
	VOID  ResponseResultIn();
	VOID  ResponseResultOut(_Inout_ T_BILL_DATA *p_data);		

	VOID  SetAcceptedAmount(_In_ LPVOID data, IN T_BnrXfsResult ocResult);	
	VOID  SetDispenseCount(_In_ T_BILL_OUT_AMOUNT *p_data);
	VOID  SetEmptyModule(_In_ INT *p_module);	
	VOID  SetLockInfo(_In_ T_CU_LOCK *p_data);	
	VOID  SetLoaderSupply(_In_ T_BILL_SUPPLY *_pData, BOOL bClear=FALSE);
	VOID  SetCashCollect(_In_ T_BILL_COLLECT *_pData);
	VOID  SetCashChangeCnt(_In_ T_CASHCNT_CHANGE *_pData){
		ZeroMemory(&m_cash_update, sizeof(T_CASHCNT_CHANGE));
		CopyMemory(&m_cash_update, _pData, sizeof(T_CASHCNT_CHANGE));
	}

	inline T_MODULE_STATUS *GetModulesStatus(){ return &module_stat; }

	inline UINT32 GetCassetteValue(INT module_no){
		return cassette[module_no].value;
	}

private:		
	// audit
	BOOL  CorrectionCancel(LPVOID p_cash);
	BOOL  CorrectionOutputAmount(LPVOID p_cash);
	
	// status
	VOID  ModuleStatusCheck(OPTIONAL INT module_index=0);	
	VOID  SetCommand(IN INT nCmd);
	
	// configration	
	BOOL  GetLCUNum();	
	BOOL  GetModulesId();	
	VOID  CashboxConfiguration(IN INT n_status);
	VOID  LoaderConfiguration(IN INT n_status);

	//----------------------------------------------------------------------//	
	// BNR operation Process 함수 
	//----------------------------------------------------------------------//
	VOID  ProcConnect(IN BOOL bRecovery=FALSE);
	BOOL  ProcCancel();
	BOOL  ProcCashinStart();
	INT   ProcCashIn();
	INT   ProcCashinend();
	BOOL  ProcCashinError();
	BOOL  ProcCashboxEmpty(BOOL bClear=FALSE);
	INT   ProcCashinRollback();	
	BOOL  ProcDisconnect();
	BOOL  ProcDispense(_In_ BOOL bDispense=TRUE);
	BOOL  ProcDispenseError(_In_ LPVOID p_data);
	BOOL  ProcModuleLock();
	BOOL  ProcEmpty(IN BOOL toFloat);
	BOOL  ProcPresent();
	BOOL  ProcRetract();	
	INT   ProcReset(IN BOOL b_getStatus=TRUE);		
	BOOL  ProcSupplyLoder(IN BOOL bAuditClr=FALSE);
	
	//----------------------------------------------------------------------//	
	// alarm code 생성 함수 
	//----------------------------------------------------------------------//
	VOID  AlarmOccured(_In_ TCHAR* p_code, IN BOOL b_remove);
	VOID  XfsError(IN INT32 xfs_ret);
	VOID  MainModuleError(_In_ T_MainModuleStatus *p_status);
	VOID  BundlerError(_In_ T_BundlerStatus *p_status);
	VOID  SpineError(_In_ T_SpineStatus *p_status);
	VOID  LoaderError(_In_ T_LoaderStatus *p_status);
	VOID  Recycler3Error(_In_ T_RecyclerStatus *p_status);
	VOID  Recycler4Error(_In_ T_RecyclerStatus *p_status);
	VOID  Recycler5Error(_In_ T_RecyclerStatus *p_status);
	VOID  Recycler6Error(_In_ T_RecyclerStatus *p_status);
	VOID  CashBoxError(_In_ T_CashboxStatus *p_status);

	//----------------------------------------------------------------------//
	// Member Variable Declare
public:	
	INT                         m_cur_opID;
	HWND                        m_hParent;
	BOOL                        m_bReset;
	BOOL                        m_bGetStatusReset;
	bool                        m_bLoaderStatusChange;
	bool                        m_bCashboxStatusChange;

private:
	// PCU Name 비교
	inline INT PCU_NAME(CHAR *X, CHAR *Y){ return strncmp(X, Y, sizeof(CHAR)*SIZE_OF_PHYSICAL_NAME); }
	// LCU variant 비교
	inline INT LCU_VAR(CHAR *X, CHAR *Y){ return strncmp(X, Y, sizeof(CHAR)*2); }
	
	TCHAR* AUDIT_FILE;	

	static CCriticalSection		m_cs;
	
	CWinThread*					m_pThread;				
	CBillProtocol*				m_protocol;						// BNR API Function Helper Object
	HANDLE						m_hEvent;						// 이벤트 송신 핸들 (to proxy)
	HANDLE						m_hExit;						// 이벤트 송신 핸들 (to proxy)
	
	// data
	T_BILL_DATA*				m_data;							// 공유 데이터 전달용 객체
	T_BILL_ESCROW				escrow[MAX_PCU_NUM];			// 에스크로우 정보 (투입 시 사용)
	T_BILL_CASSETTE				cassette[MAX_LCU_NUM];			// 카세트 정보 (거스름 시 사용)
	DWORD						m_output_amt;					// 방출 총 금액
	T_HOLDING_AMOUNT			m_holding_amt;					// 보유량
	T_CASHCNT_CHANGE            m_cash_update;
	T_ERROR_AMOUNT			    m_error_amt;					// 장비 오류로 인한 금액 정보
																// (고객에게 반환해야 할 돈)
																// UNR, 지폐걸림(JAM)
	// operation
	INT	m_cmd;							// BNR Operation
	INT m_main_cmd;

	INT                         m_empty_no;                     // 현 회수 장비 인덱스
	INT							m_retry_cnt;					// 재시도 횟수
	INT                         m_rollback_tm_cnt;              // 취소 후 금액 회수 대기 수량 
	BOOL                        m_empty_float;

	// configration
	INT							module_size;
	INT							empty[MAX_PCU_NUM];

	// status
	INT32						error_code;						// 행정 당 발생하는 오류코드
	INT							m_result;						// operatoin 수행 결과
	INT                         m_wait_taken_cnt;					
	INT                         m_jam_cnt;
	INT                         m_cashbox_status;               // 지폐함 탈착, 장착 상태
	INT                         m_loader_status;

	BOOL                        m_bInit;                        // 처음 초기화인지 판단
	BOOL						m_bDeviceOpen;					// 장치 접속 Flag
	BOOL						m_bCompleteCmd;					// 현재 명령 수행을 완료 하였는지 판단
	BOOL						m_bTransaction;					// transaction 진행 판단 플래그
	BOOL						m_bCashinMode;					// 투입 모드
	BOOL						m_bDispenseMode;				// 방출 모드
	BOOL						m_bInsertAccept;				// 투입 허용 플래그
	
	BOOL						m_bCashboxMissed;               // 지폐함 탈착 유무
	BOOL                        m_bLoaderMissed;                // 로더 탈착 유무
	BOOL						m_bAutoReset;
	BOOL						m_bUsbNotConnected;				// usb 연결 안되어 있음
	BOOL                        m_bCashAtBezel;

	CMap<CString, LPCTSTR, T_BILL_ALARM, T_BILL_ALARM&> m_mapAlarm;			// 중복 알람 방지용 map
	T_MODULE_STATUS				module_stat;					// BNR 모듈들 상태

	TTimer<CBillModule>			m_status_timer;
	BOOL                        m_bStatusTmStart;

	TTimer<CBillModule>			m_packetanalysis_timer;	
	TTimer<CBillModule>			m_present_timer;	
	TTimer<CBillModule>         m_cashmodule_lock_timer; 

	HANDLE					    m_ocEvent;		

	CIni m_ini_ver;
};
