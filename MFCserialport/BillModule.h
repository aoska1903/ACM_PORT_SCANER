#pragma once


#include "ModuleTemplete.h"
#include "TemplateTimer.h"
#include "BillProcessorData.h"
#include "Ini.h"

#define EXECUTE_SUCCESS					1		// ���� ����
#define BXR_NO_ERROR                    (0) 
#define EXECUTE_FAIL					0		// ���� ����	
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


// ��⿡�� ���� ��� ��û ���� �ڵ� ����
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

	// LED ���� ���	
	CMD_LED_ALL_OFF,	
	CMD_LED_INSERT_ON,			// ���� ���Ա�	
	CMD_LED_CARD_ON,			// ī�� ���Ա�	
	CMD_LED_RECEIPT_ON,			// �������� 	
	CMD_LED_OUTPUT_ON,			// �����	
	CMD_LED_TEST_INSERT_ON,		// ���� ���Ա�
	CMD_LED_TEST_INSERT_OFF,	
	CMD_LED_TEST_CARD_ON,		// ī�� ���Ա�
	CMD_LED_TEST_CARD_OFF,	
	CMD_LED_TEST_RECEIPT_ON,	// �������� 
	CMD_LED_TEST_RECEIPT_OFF,
	CMD_LED_TEST_OUTPUT_ON,		// �����
	CMD_LED_TEST_OUTPUT_OFF,

	MSG_ACM_JAM,				// JAM �߻�

	CMD_AD_PLAY					// ���� ���� �����ֱ�
};

// ����ó����ġ ��� �ڵ�� Ư�� ���� �ڵ� ����
enum BILL_CMD_CODE {	
	CMD_BILL_NONE						= 400,
	CMD_BILL_THREAD_START               ,
	CMD_BILL_MODULE_INFO				,
	CMD_BILL_ALARM_DELETE				,       // �˶� ����
	CMD_BILL_RESET						,		// ����
	CMD_BILL_CASHIN_START				,		// ���� ��� ����
	CMD_BILL_CASHIN                     ,       // ���� ���
	CMD_BILL_CASHINROLLBACK				,		// ��ȯ
	CMD_BILL_CASHIN_END					,		// ���� ��� ����
	CMD_BILL_CASH_DISPENSE				,		// ����
	CMD_BILL_CASH_DISPENSE_RETRY		,		// ���� �õ�
	CMD_BILL_PRESENT					,		// ���� ��������
	CMD_BILL_EMPTY						,		// ���� ȸ��
	CMD_BILL_CASHBOX_EMPTY				,		// ����ȸ���� ȸ��
	CMD_BILL_CHANGES					,		// �Ž��� ���� ��ü ����
	CMD_BILL_CANCEL_TRANSACTION			,		// ��� ��ü ����
	CMD_BILL_RECEIPT					,		// ����ó����ġ Receipt(Escorow->Cashbox)
	CMD_BILL_MODULE_LOCK				,		// Recycler, Loader Lock ���� ���
	CMD_BILL_COLLECT                    ,       // ���� ȸ��
	CMD_BILL_LOADER_SUPPLY              ,       // ���� ����
	NOTIFY_BILL_INSERT					,		// ���� �Լ� 	
	NOTIFY_BILL_DISPENSE				,		// ���� ����
	NOTIFY_BILL_STATUS					,		// ���� ����	
	NOTIFY_BILL_CASHBOX_MISSED          ,       // ȸ���� Ż��
	NOTIFY_BILL_CASHBOX_INOP            ,       // ȸ���� ����
	NOTIFY_BILL_LOADER_MISSED           ,       // �δ� Ż��
	NOTIFY_BILL_LOADER_INOP             ,       // �δ� ����
	NOTIFY_BILL_JAM                     ,       // ���� �ɸ�
	CMD_BILL_AUDITCLR                   ,       // ȸ�� �ʱ�ȭ
	CMD_BILL_OUTLET_CHK                 ,       // ���� �ƿ��� ������ �ִ��� �˻�
	NOTIFY_BILL_IN_JAM                  ,
	NOTIFY_BILL_MODULE_CASHLOCK                 // ����ȯ���� ��� Cash Lock ����->��� 
};

class CBillProtocol;

class CBillModule : public CModuleTemplete
{
public:
	CBillModule(HWND hParent);           // ���� ����⿡ ���Ǵ� protected �������Դϴ�.
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
	// BNR operation Process �Լ� 
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
	// alarm code ���� �Լ� 
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
	// PCU Name ��
	inline INT PCU_NAME(CHAR *X, CHAR *Y){ return strncmp(X, Y, sizeof(CHAR)*SIZE_OF_PHYSICAL_NAME); }
	// LCU variant ��
	inline INT LCU_VAR(CHAR *X, CHAR *Y){ return strncmp(X, Y, sizeof(CHAR)*2); }
	
	TCHAR* AUDIT_FILE;	

	static CCriticalSection		m_cs;
	
	CWinThread*					m_pThread;				
	CBillProtocol*				m_protocol;						// BNR API Function Helper Object
	HANDLE						m_hEvent;						// �̺�Ʈ �۽� �ڵ� (to proxy)
	HANDLE						m_hExit;						// �̺�Ʈ �۽� �ڵ� (to proxy)
	
	// data
	T_BILL_DATA*				m_data;							// ���� ������ ���޿� ��ü
	T_BILL_ESCROW				escrow[MAX_PCU_NUM];			// ����ũ�ο� ���� (���� �� ���)
	T_BILL_CASSETTE				cassette[MAX_LCU_NUM];			// ī��Ʈ ���� (�Ž��� �� ���)
	DWORD						m_output_amt;					// ���� �� �ݾ�
	T_HOLDING_AMOUNT			m_holding_amt;					// ������
	T_CASHCNT_CHANGE            m_cash_update;
	T_ERROR_AMOUNT			    m_error_amt;					// ��� ������ ���� �ݾ� ����
																// (������ ��ȯ�ؾ� �� ��)
																// UNR, ����ɸ�(JAM)
	// operation
	INT	m_cmd;							// BNR Operation
	INT m_main_cmd;

	INT                         m_empty_no;                     // �� ȸ�� ��� �ε���
	INT							m_retry_cnt;					// ��õ� Ƚ��
	INT                         m_rollback_tm_cnt;              // ��� �� �ݾ� ȸ�� ��� ���� 
	BOOL                        m_empty_float;

	// configration
	INT							module_size;
	INT							empty[MAX_PCU_NUM];

	// status
	INT32						error_code;						// ���� �� �߻��ϴ� �����ڵ�
	INT							m_result;						// operatoin ���� ���
	INT                         m_wait_taken_cnt;					
	INT                         m_jam_cnt;
	INT                         m_cashbox_status;               // ������ Ż��, ���� ����
	INT                         m_loader_status;

	BOOL                        m_bInit;                        // ó�� �ʱ�ȭ���� �Ǵ�
	BOOL						m_bDeviceOpen;					// ��ġ ���� Flag
	BOOL						m_bCompleteCmd;					// ���� ��� ������ �Ϸ� �Ͽ����� �Ǵ�
	BOOL						m_bTransaction;					// transaction ���� �Ǵ� �÷���
	BOOL						m_bCashinMode;					// ���� ���
	BOOL						m_bDispenseMode;				// ���� ���
	BOOL						m_bInsertAccept;				// ���� ��� �÷���
	
	BOOL						m_bCashboxMissed;               // ������ Ż�� ����
	BOOL                        m_bLoaderMissed;                // �δ� Ż�� ����
	BOOL						m_bAutoReset;
	BOOL						m_bUsbNotConnected;				// usb ���� �ȵǾ� ����
	BOOL                        m_bCashAtBezel;

	CMap<CString, LPCTSTR, T_BILL_ALARM, T_BILL_ALARM&> m_mapAlarm;			// �ߺ� �˶� ������ map
	T_MODULE_STATUS				module_stat;					// BNR ���� ����

	TTimer<CBillModule>			m_status_timer;
	BOOL                        m_bStatusTmStart;

	TTimer<CBillModule>			m_packetanalysis_timer;	
	TTimer<CBillModule>			m_present_timer;	
	TTimer<CBillModule>         m_cashmodule_lock_timer; 

	HANDLE					    m_ocEvent;		

	CIni m_ini_ver;
};
