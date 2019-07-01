#pragma once

#include "BillProcessorData.h"
#include "Serial.h"
#include <functional>

// 시리얼 통신 수신 이벤트 결과 함수포인트 정의
typedef std::function<VOID(_In_ INT, _In_ BYTE*, _In_ INT)> T_RECEIVE_DATA;
typedef INT32 T_BnrXfsResult;
#define NBMAXCOMP                     (20)  /**< Maximum of components for the getIdentificationReturn structure */
#define NBMAXREQ                      (20)  /**< Maximum of requirements for the Components structure */
#define MAX_NR_OF_DENOMINATION        (61)  /**< Maximum of denomination in the BNR */
#define MAX_NR_OF_DENOMINATION_ITEMS  (20)  /**< Maximum of Items for the cashInOrder */
#define NBMAXELEMENTS                 (20)  /**< Maximum of elements */
#define MAX_NR_OF_STATUS_EVENTS       (20)  /**< Maximum of Status events in list. */
#define MAX_NR_OF_MODULES (16u)  /**< Maximum number of modules in the BNR. */
typedef UINT32 T_ModuleId;

#define SIZE_OF_SERIAL_NR (12)
#define MAX_NR_OF_SERIAL_NR (256)
typedef char T_BnrSerialNr[SIZE_OF_SERIAL_NR+1];

typedef struct ModuleIdList {
  UINT32     size;
  T_Modules  modules[MAX_NR_OF_MODULES];
} T_ModuleIdList;

typedef struct BnrSerialNrList {
  UINT32         maxSize;                             /**< Maximum number of T_BnrSerialNr items that can be stored in the array. This value must be initialized by the caller and may change with the API version. */
  UINT32         size;
  T_BnrSerialNr  items[MAX_NR_OF_SERIAL_NR];
} T_BnrSerialNrList;

//----------------------------------------------------------------------//
// BNR time out declare
//----------------------------------------------------------------------//

//----------------------------------------------------------------------//
// 토요콤 지폐처리장치(BNK) Command 정의
//----------------------------------------------------------------------//
enum BNK_BILL_COMMAND{
	BNK_CMD_RESET			= 0x30, // Bill validator is initialized
	BNK_CMD_SENSE			= 0x40, // State of the Bill validator is acquired
	BNK_CMD_AUTHORIZED		= 0x50,	// 
	BNK_CMD_INHIBITED		= 0x51,
	BNK_CMD_RECEIPT			= 0x52,
	BNK_CMD_RETURN			= 0x53,
	BNK_CMD_INTAKE			= 0x54,
	BNK_CMD_MEM_TRANSFER	= 0x70,
	BNK_CMD_MEM_ERASE		= 0x71,
	BNK_CMD_DATE_TRANSFER	= 0x72,	// Date data held in the Bill validator are transferred.
	BNK_CMD_DATE_SETTING	= 0x73,	// Date data held in the Bill validator are set.
	BNK_CMD_DATE_ADDITION	= 0x74,	// Date data are added to all responses.
	BNK_CMD_WRITE_MEM		= 0x75,	// BV write the data in the memory of the cash box
	BNK_CMD_CHANGE_STATUS   = 0x80
};

class CBillModule;

class CBillProtocol : public CSerial
{
public:
	CBillProtocol(CBillModule *p_module=NULL);
	virtual ~CBillProtocol(void);

	//----------------------------------------------------------------------//
	// member operation
	inline INT getErrorCount() { return m_alarm_no; }
	VOID getAlarmCode(T_BILL_ALARM *p_alarm){ 
		CopyMemory(p_alarm, m_alarm, sizeof(T_BILL_ALARM)*m_alarm_no); 
	}
	VOID NewLog();
	
	T_BnrXfsResult DeviceOpen(HWND hParent);	
	T_BnrXfsResult DeviceReset();
	T_BnrXfsResult UsbReset();
	T_BnrXfsResult UsbKillNReload();

	//----------------------------------------------------------------------//
	// Cash Operation
	T_BnrXfsResult OperationCancel();
	T_BnrXfsResult CashInStart();
	T_BnrXfsResult CashIn();
	T_BnrXfsResult CashInRollback();
	T_BnrXfsResult CashInEnd();
	T_BnrXfsResult PresentAmount();
	INT32 BillEmpty(_In_ char *pcuName, BOOL toFloat);
	INT32 Eject(_Inout_ T_BnrXfsResult *ocResult);
	INT32 Reject(_Inout_ T_BnrXfsResult *ocResult);
	INT32 Retract(_Inout_ T_BnrXfsResult *ocResult);	

	//----------------------------------------------------------------------//
	// Status
	
	T_BnrXfsResult GetModuleStatus(_In_ UINT32 moduleId, _Inout_ T_ModuleStatus* p_status);
	T_BnrXfsResult GetModules(_Inout_ T_ModuleIdList* p_module_list);
	
//////////////////////////////////////////////////////////////////////////
// 시리얼 통신 관련 멤버 함수 정의
public:
	BOOL  Connect(CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
				  CSerial::EDataBits eDataBits = CSerial::EData8,
				  CSerial::EParity   eParity   = CSerial::EParNone,
				  CSerial::EStopBits eStopBits = CSerial::EStop1
				  );

	// BCC 계산
	BYTE CalcBCC(_In_ BYTE* _pBuffer, _In_ DWORD _dwSize);

	// 통신 연결 및 설정
	BOOL ConnectToDevice(
		CSerial::EBaudrate eBaudrate = CSerial::EBaud9600,
		CSerial::EDataBits eDataBits = CSerial::EData8,
		CSerial::EParity   eParity   = CSerial::EParEven,
		CSerial::EStopBits eStopBits = CSerial::EStop1
	);

	VOID  Disconnect();//연결해제

	// 수신 이벤트 핸들 생성 및 스레드 생성
	BOOL CreateRecvEvent();

	// 수신 이벤트 핸들 생성 및 스레드 해제
	VOID DeleteRecvEvent();

	// 수신 이벤트 완료 시 호출되는 콜백 함수 등록
	VOID BillRegCallbackFunc_RecvComp(T_RECEIVE_DATA _func){
		CompleteReceiveData = std::move(_func);
	}

	// 포트 번호 설정
	VOID SetCOMPort(_In_ const INT _port_com){
		m_nPortCom = _port_com;
	}

	// 포트 번호 얻기
	INT GetCOMPort(){
		return m_nPortCom;
	}

	// 지폐처리장치(BNK-토요콤)로 커멘드 전송
	BOOL SendCmdToBNK(_In_ const BYTE _cmd, _In_opt_ WORD _wEnableBankNoteMask=0);

	// 전송 제어 문자 전송
	BOOL SendTransCtrlChar(_In_ BYTE* _symbol, _In_opt_ INT _length=1);
		
private:
	// 입력 
	BOOL ClearBuffer();

	// 스레드 함수
	static UINT _threadEntry(_In_ LPVOID);

	// 스레드에서 호출하는 실제 작업 함수
	VOID RecvEventWorking(void);

	//----------------------------------------------------------------------//
	// member attribute
public:
	HANDLE					m_ocEvent;	
	
	T_BnrXfsResult			m_ocResult;
private:
	T_BILL_ALARM			m_alarm[MAX_BNR_ALARM_CNT];
	INT						m_alarm_no;

	// 통신 포트 번호
	INT						m_nPortCom;
	// overlapped operation 위한 핸들
	HANDLE					m_hOverlapped;	
	// "STOP" 핸들		
	HANDLE					m_hStop;		
	// 수신 이벤트 작업 스레드
	CWinThread*				m_pThread;		
	// 수신 이벤트 완료 콜백 함수
	T_RECEIVE_DATA CompleteReceiveData;
};

