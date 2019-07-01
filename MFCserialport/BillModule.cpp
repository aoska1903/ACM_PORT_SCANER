#include "stdafx.h"
#include "BillProtocol.h"
#include "BillModule.h"

#define  BILL_PROG_VER	_T("2019041001")

CCriticalSection CBillModule::m_cs;

//-----------------------------------------------------------------//
// reset 실행 결과 값 정의
static const INT RT_FAIL    = 0;
static const INT RT_SUCCESS = 1;
static const INT RT_AGAIN   = 3;

//-----------------------------------------------------------------//
// timer id 정의
static const INT TID_CHK_TRANS_STAT   = 1;
static const INT TID_CHK_OUTLET_STAT  = 2; 
static const INT TID_CHK_CASHMOD_LOCK = 3;

//@ 생성자 함수 정의
CBillModule::CBillModule(HWND hParent) : CModuleTemplete(),
	m_pThread(NULL),
	m_protocol(NULL),
	m_hEvent(NULL),	
	m_data(NULL),
	m_retry_cnt(0),
	m_bDeviceOpen(TRUE),	
	m_bCompleteCmd(TRUE),
	m_bTransaction(FALSE),
	m_bCashinMode(FALSE),
	m_bDispenseMode(FALSE),
	m_bInsertAccept(FALSE),
	m_bCashboxMissed(FALSE),
	m_bAutoReset(FALSE),
	m_jam_cnt(0),
	m_wait_taken_cnt(0),
	m_hParent(hParent),	
	m_bReset(FALSE),
	m_bGetStatusReset(FALSE),
	m_bInit(FALSE),	
	m_bUsbNotConnected(FALSE),
	m_bStatusTmStart(FALSE),
	m_bCashAtBezel(FALSE),
	m_bCashboxStatusChange(false),
	m_bLoaderStatusChange(false)
{
	
}

CBillModule::~CBillModule()
{		
	ExitWork();
}

//----------------------------------------------------------------------//
// Name : TestModeON
// IN	: 
// OUT	: 
// DESC	: TestMode 시 로그 기록(메인 대화상자 화면) 설정
//----------------------------------------------------------------------//	
VOID CBillModule::TestModeON(HWND _wndParent)
{
}

VOID CBillModule::TestModeOFF()
{
	CommandReset();

	SetCommand(CMD_BILL_NONE);
}


//----------------------------------------------------------------------//
// Name : TimerStart
// IN	: IN UINT wait_sec, 
//		  IN BOOL b_packetanalysis(PacketAnalysis 함수 구동 타이머 여부)
// OUT	: 		  
// DESC	: 
//----------------------------------------------------------------------//
VOID CBillModule::TimerStart(IN UINT wait_sec, IN INT TimerID)
{	
	switch(TimerID)
	{
	case TID_CHK_TRANS_STAT:   // 트랜잭션 상태 검사
		{
			if(m_bStatusTmStart == TRUE) break;
			m_bStatusTmStart = TRUE;

			m_status_timer.SetTimedEvent(this, &CBillModule::OnTimerEvent_Status);
			m_status_timer.Start(wait_sec);
			break;
		}
	case TID_CHK_OUTLET_STAT:  // 아웃렛 지폐 제거 여부
		m_present_timer.SetTimedEvent(this, &CBillModule::OnTimerEvent_Present);
		m_present_timer.Start(wait_sec);
		break;
	case TID_CHK_CASHMOD_LOCK: // cash 모듈 잠금 여부
		{
			m_cashmodule_lock_timer.SetTimedEvent(this, &CBillModule::OnTimerEvent_Cashlock);
			m_cashmodule_lock_timer.Start(wait_sec);
			break;
		}
	}
}

VOID CBillModule::TimerStop(IN INT TimerID/*=1*/)
{	
	switch(TimerID)
	{
	case TID_CHK_TRANS_STAT:   // 트랜잭션 상태 검사		
		{
			m_status_timer.Stop();
			m_bStatusTmStart = FALSE;
			break;
		}
	case TID_CHK_OUTLET_STAT:  // 아웃렛 지폐 제거 여부
		{
			m_present_timer.Stop();
			m_wait_taken_cnt  = 0;
			m_rollback_tm_cnt = 0;
			break;
		}
	case TID_CHK_CASHMOD_LOCK: // cash 모듈 잠금 여부
		{
			m_cashmodule_lock_timer.Stop();
			break;
		}
	}
}

VOID CBillModule::OnTimerEvent_Status()
{
}

//@ 지폐 내보내기 후 손님 지폐 획득 유무 확인
VOID CBillModule::OnTimerEvent_Present()
{	
}

//@ cash module lock open, close 검사 타이머 관련 함수 정의
VOID CBillModule::OnTimerEvent_Cashlock()
{
}

//----------------------------------------------------------------------//
// Name : InitWork
// IN	: _In_ HANDLE hEvent(proxy로 보낼 이벤트 핸들)
// OUT	: 
// DESC	: 초기화
//----------------------------------------------------------------------//	
VOID CBillModule::InitWork(_In_ HANDLE hEvent)
{
	// 공유 메모리 전달용 객체 생성
	m_data = new T_BILL_DATA;
	ZeroMemory(m_data, sizeof(T_BILL_DATA));
	// 버전 정보 기록
	sprintf_s(m_data->_ver, sizeof(m_data->_ver), _T("%s"), BILL_PROG_VER);
	// INI 파일 경로 설정
	TCHAR  curFolderPath[MAX_PATH]={NULL};         
	TCHAR  iniFilePath[MAX_PATH]={NULL};
#ifdef _REMOTE
	_stprintf(curFolderPath, _T("%s"), _T("C:\\AFC"));
#else
	// 복사 대상 폴더 경로 저장 
	GetCurrentDirectory( MAX_PATH, curFolderPath);		
#endif
	// afc_congif.ini
	_stprintf(iniFilePath, _T("%s\\Ini\\ParamVer.ini"), curFolderPath);				
	m_ini_ver.SetPathName(iniFilePath);
	m_ini_ver.WriteString("Version", "Bill_Version", m_data->_ver);

	// create Protocol object
	m_protocol = new CBillProtocol(this);	

	// proxy로 보내는 이벤트 핸들
	m_hEvent = hEvent;

	// thread 종료 이벤트
	m_hExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 지폐환류기 동작 완료 이벤트
	m_ocEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);	

	// data 구조체
	ZeroMemory(escrow, sizeof(T_BILL_ESCROW)*MAX_PCU_NUM);		// 에스크로(투입 시)
	ZeroMemory(cassette, sizeof(T_BILL_CASSETTE)*MAX_LCU_NUM);	// cassette(방출 시)
	ZeroMemory(&m_holding_amt, sizeof(T_HOLDING_AMOUNT));       // 보유 금액
	ZeroMemory(&m_error_amt, sizeof(T_ERROR_AMOUNT));           // UNR, JAM 금액 초기화   

	m_jam_cnt = 0;
}


//----------------------------------------------------------------------//
// Name : ExitWork
// IN	: 
// OUT	: 
// DESC	: 종료
//----------------------------------------------------------------------//
VOID CBillModule::ExitWork()
{
	CloseHandle(m_ocEvent);

	// Timer Stop	
	TimerStop(TID_CHK_TRANS_STAT); // packet analysis timer off
	TimerStop(TID_CHK_OUTLET_STAT);
	TimerStop(TID_CHK_CASHMOD_LOCK);

	ProcDisconnect();

	m_bCompleteCmd = FALSE;

	SetEvent(m_hExit);

	if(m_pThread)
	{
		if(WaitForSingleObject(m_pThread->m_hThread, 10000L) == WAIT_TIMEOUT)
			::TerminateThread(m_pThread->m_hThread, 1);		
	}

	CloseHandle(m_hExit);

	m_mapAlarm.RemoveAll();

	CModuleTemplete::ExitWork();
}


//----------------------------------------------------------------------//
// Name : _ThreadEntry
// IN	: 
// OUT	: 
// DESC	: work thread
//----------------------------------------------------------------------//
UINT CBillModule::_ThreadEntry(_In_ LPVOID pParam)
{	
	CBillModule *pSelf = static_cast<CBillModule*>(pParam);
	pSelf->Working();
	return 0;
}


//----------------------------------------------------------------------//
// Name : Working
// IN	: 
// OUT	: 
// DESC	: Command 분류, thread loop
//----------------------------------------------------------------------//	
VOID CBillModule::Working()
{
	while(TRUE)
	{
		DWORD dwExit = WaitForSingleObject(m_hExit, 100L);
		if(dwExit == WAIT_OBJECT_0) 
			return;

		// 작업 목록에서 수행 작업을 뽑는다.
		INT nCmd = CommandOut(m_bCompleteCmd);
		
		// 작업 목록에 수행 해야할 목록이 존재하는지 확인
		if(nCmd > 0){
			SendCmd(nCmd);
		}

		Sleep(1000);
	}//end while 
}	


//----------------------------------------------------------------------//
// Name : SendCmd
// IN	: IN INT nCmd
// OUT	: 
// DESC	: BNR 명령 처리
//----------------------------------------------------------------------//	
VOID CBillModule::SendCmd(IN INT command)
{
	BOOL bSuccess  = TRUE;
	INT  rt_val    = RT_SUCCESS;
	m_result	   = EXECUTE_SUCCESS;

	SetCommand(command);

	switch(command)
	{
	case CMD_BILL_MODULE_INFO:
		{
			BNRStatus();
			m_result = EXECUTE_SUCCESS;
			ResponseResultIn();
			return;		
		}
	case CMD_RECONNECT:
	case CMD_CONNECT:	
		{
			ProcConnect(m_bUsbNotConnected);
			break;
		}
	case CMD_DISCONNECT:
		{
			bSuccess = ProcDisconnect();

			if(bSuccess == TRUE)
			{			
				m_result = EXECUTE_SUCCESS;

				ResponseResultIn();				
			}

			break;
		}		
	case CMD_BILL_CASHIN_START:
		{
			bSuccess = ProcCashinStart();
			break;
		}
	case CMD_BILL_CASHIN:
		{
			bSuccess = ProcCashIn();
			break;
		}
	case CMD_BILL_CASHIN_END:
		{
			rt_val = ProcCashinend();
			if(rt_val == RT_AGAIN){
				rt_val = ProcCashinend();
			}
			break;
		}
	case CMD_BILL_CASHINROLLBACK:
	case CMD_BILL_CANCEL_TRANSACTION:		
		{
			rt_val = ProcCashinRollback();
			if(rt_val == RT_AGAIN){
				rt_val = ProcCashinRollback();
			}
			break;
		}
	case CMD_BILL_PRESENT:
		{
			bSuccess = ProcPresent();
			break;
		}
	case CMD_BILL_CHANGES:
		{
			bSuccess = ProcDispense(FALSE);
			break;
		}
	case CMD_BILL_CASH_DISPENSE:
		{
			bSuccess = ProcDispense(FALSE);
			break;
		}
	case CMD_BILL_CASHBOX_EMPTY:
		{
			bSuccess = ProcCashboxEmpty();

			m_bCashboxStatusChange = false;

			if(bSuccess == TRUE) 
			{
				m_result = EXECUTE_SUCCESS;
				ResponseResultIn();
			}

			break;
		}
	case CMD_BILL_COLLECT:
	case CMD_BILL_EMPTY:
		{
			m_empty_float = FALSE;
			bSuccess = ProcEmpty(FALSE);
			return;
		}
	case CMD_BILL_ALARM_DELETE:
	case CMD_BILL_RESET:
		{
			rt_val = ProcReset();
			if(rt_val == RT_AGAIN){
				rt_val = ProcReset();
			}
			break;	
		}		
	case CMD_BILL_MODULE_LOCK:
		{
			bSuccess = ProcModuleLock();
			break;
		}
	case CMD_BILL_LOADER_SUPPLY:
		{
			bSuccess = ProcSupplyLoder();

			m_bLoaderStatusChange = false;

			if(bSuccess == TRUE) 
			{
				m_result = EXECUTE_SUCCESS;
				ResponseResultIn();
			}

			break;
		}
	case CMD_BILL_AUDITCLR:
		{
			bSuccess = ProcSupplyLoder(TRUE);

			if(bSuccess == TRUE)
			{
				bSuccess = ProcCashboxEmpty(TRUE);

				if(bSuccess == TRUE)
				{
					bSuccess = ProcReset();
				}
			}
			break;
		}
	case CMD_BILL_OUTLET_CHK:
		{			
			m_cmd = CMD_BILL_OUTLET_CHK;
		}
	}

	if(bSuccess == FALSE || rt_val != RT_SUCCESS) 
	{
		ResponseResultIn();
	}
}

//----------------------------------------------------------------------//
// Name : PacketAnalysis
// IN	: 
// OUT	: 
// DESC	: BNR 모듈 상태 검사
//----------------------------------------------------------------------//	
VOID CBillModule::PacketAnalysis()
{
}

//----------------------------------------------------------------------//
// Name : BNRStatus
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: BNR 상태 점검 
//----------------------------------------------------------------------//	
VOID CBillModule::BNRStatus()
{
	T_BnrXfsResult xfs_ret = BXR_NO_ERROR;
	ModuleStatusCheck();
}

//----------------------------------------------------------------------//
// Name : BNRStatusChange
// IN	: 
// OUT	: 
// DESC	: BNR 장비 체크 변화 시 호출 
//----------------------------------------------------------------------//	
VOID CBillModule::BNRStatusChange(IN UINT32 *n_detail)
{
}

//----------------------------------------------------------------------//
// Name : BNRThreshold
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: The Threshold status has changed
//----------------------------------------------------------------------//	
VOID CBillModule::BNRThreshold()
{	
	if(m_cmd == CMD_BILL_NONE || m_cmd == NOTIFY_BILL_STATUS)
	{
		SetCommand(NOTIFY_BILL_STATUS);
		ResponseResultIn();
	}
}


//----------------------------------------------------------------------//
// Name : ConfigurationChanged
// IN	: 
// OUT	: 
// DESC	: BNR 모듈 장/탈착 감지 처리
//----------------------------------------------------------------------//	
VOID CBillModule::ConfigurationChanged(_In_ LPVOID data)
{
	if( (m_cmd == CMD_CONNECT) || (m_cmd == CMD_RECONNECT) ) return;
}


//----------------------------------------------------------------------//
// Name : OperationComplete
// IN	: _In_ LPVOID data(수행결과 데이터), IN DWORD operationId(BNR operation id)
// OUT	: 
// DESC	: BNR onOperationCompleteOccured 콜백 함수에서 호출
//----------------------------------------------------------------------//	
VOID CBillModule::OperationComplete(_In_ LPVOID data, IN DWORD operationId, IN DWORD result)
{
	CSingleLock cs_lock(&m_cs);
	SetEvent(m_ocEvent);
}

//----------------------------------------------------------------------//
// Name : StatusEvent
// IN	: _In_ LPVOID data(수행결과 데이터), IN LONG32 status(상태)
// OUT	: 
// DESC	: BNR StatusEvent 콜백 함수에서 호출
//----------------------------------------------------------------------//	
VOID CBillModule::StatusEvent(IN LONG32 status, IN LONG32 result, _In_ LPVOID data)
{
}

VOID CBillModule::IntermediateOccured(IN LONG32 opID)
{
	m_cur_opID = opID;
}

//----------------------------------------------------------------------//
// Name : ResponseResultIn
// IN	: 
// OUT	: 
// DESC	: 공유 메모리 전달 객체에 데이터 저장
//		  전달 받은 명령 처리 후 호출
//----------------------------------------------------------------------//	
VOID CBillModule::ResponseResultIn()
{
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();

	sprintf_s(m_data->_ver, _T("%s"), BILL_PROG_VER);

	// 수행한 명령 코드
	m_data->device_cmd = m_cmd;			

	if(m_cmd == CMD_BILL_NONE){
	}

	if(m_cmd == NOTIFY_BILL_MODULE_CASHLOCK)
	{
		m_data->bLoaderStatusChange = m_bLoaderStatusChange;

		if(m_bLoaderStatusChange){		
		}

		m_data->bCashboxStatusChange = m_bCashboxStatusChange;
		
		if(m_bCashboxStatusChange){
		}
	}

	// empty
	CopyMemory(m_data->empty, empty, sizeof(INT)*MAX_PCU_NUM);

	// 장비 Transaction 모드

	// 장비 접속 상태
	m_data->device_open     = m_bDeviceOpen;	
	m_data->status.connect  = m_bDeviceOpen;
	CopyMemory(&m_data->status.modules, &module_stat, sizeof(T_MODULE_STATUS));

	// 수행 결과
	m_data->n_result = m_result;

	// BNR module error
	m_data->alarm_cnt = 0;
	ZeroMemory(m_data->bill_alarm, sizeof(T_BILL_ALARM)*MAX_BNR_ALARM_CNT);

	POSITION pos = m_mapAlarm.GetStartPosition();
	T_BILL_ALARM alarm;
	CString  skey = _T("");
	
	while(pos != NULL)
	{
		skey = _T("");
		ZeroMemory(&alarm, sizeof(T_BILL_ALARM));
		m_mapAlarm.GetNextAssoc(pos, skey, alarm);

		sprintf_s(m_data->bill_alarm[m_data->alarm_cnt].alarm_key, _T("%s"), skey); 
		sprintf_s(m_data->bill_alarm[m_data->alarm_cnt].alarmcode, _T("%s"), alarm.alarmcode);
		m_data->bill_alarm[m_data->alarm_cnt].modulecode = 'B';

		m_data->alarm_cnt++;
	}
	cs_lock.Unlock();
	SetEvent(m_hEvent);
}


//----------------------------------------------------------------------//
// Name : ResponseResultOut
// IN	: _Inout_ T_COIN_DATA *p_data (proxy 공유 맵 데이터 객체)
// OUT	: 
// DESC	: 공유 메모리 전달 객체를 proxy 공유 맵 데이터에 전달
//		  Proxy에서 호출
//----------------------------------------------------------------------//	
VOID CBillModule::ResponseResultOut(_Inout_ T_BILL_DATA *p_data)
{
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	CopyMemory(p_data, m_data, sizeof(T_BILL_DATA));
	m_cmd = CMD_BILL_NONE;
	cs_lock.Unlock();
}

//----------------------------------------------------------------------//	
VOID CBillModule::SetAcceptedAmount(_In_ LPVOID data, IN T_BnrXfsResult ocResult)
{
	INT unit		= 0;
	INT count		= 0;	
	UINT32 amount	= 0;

	CSingleLock cs_lock(&m_cs);
	m_cs.Lock();
	m_cs.Unlock();	
}

//----------------------------------------------------------------------//
// Name : SetDispenseCount
// IN	: _In_ T_BILL_HOLDING *p_data
// OUT	: 
// DESC	: 장비 방출 금액과 수량 설정
//----------------------------------------------------------------------//	
VOID CBillModule::SetDispenseCount(_In_ T_BILL_OUT_AMOUNT *p_data)
{
	m_output_amt = p_data->amount;

	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	CopyMemory(&m_data->amt_output, p_data, sizeof(T_BILL_OUT_AMOUNT));
	cs_lock.Unlock();	

	UINT32 size = 0;

	CString _strReyName;
	for(INT i=LO1; i<MAX_PCU_NUM; i++)
	{
		if( (cassette[i].lock == TRUE) || (p_data->count[i-2] == 0)) 
		{
			cassette[i].b_count = 0;
			continue;
		}

		if(i == LO1){
			_strReyName = _T("Loader");
		}else if(i==RE3){
			_strReyName = _T("Recycle 3");
		}else if(i==RE4){
			_strReyName = _T("Recycle 4");
		}else if(i==RE5){
			_strReyName = _T("Recycle 5");
		}else{
			_strReyName = _T("Recycle 6");
		}
		cassette[i].b_count = p_data->count[i-2];

		size++;
	}//end for	
}

//----------------------------------------------------------------------//
// Name : SetEmptyModule
// IN	: _In_ INT *p_module
// OUT	: 
// DESC	: LO, RE 회수
//----------------------------------------------------------------------//	
VOID CBillModule::SetEmptyModule(_In_ INT *p_module)
{
	m_empty_no = LO1;
	m_empty_float = FALSE;
	ZeroMemory(empty, sizeof(INT)*MAX_PCU_NUM);
	CopyMemory(empty, p_module, sizeof(INT)*MAX_PCU_NUM);
}

//----------------------------------------------------------------------//
// Name : SetLockInfo
// IN	: _In_ T_CU_LOCK *p_data
// OUT	: 
// DESC	: 장비 모듈 잠금 설정(Recycler만 해당)
//----------------------------------------------------------------------//	
VOID CBillModule::SetLockInfo(_In_ T_CU_LOCK *p_data)
{
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	CopyMemory(&m_data->module_lock, p_data, sizeof(T_CU_LOCK));
	cs_lock.Unlock();
}

//----------------------------------------------------------------------//
// Name : SetLockInfo
// IN	: _In_ T_BILL_SUPPLY *_pData
// OUT	: 
// DESC	: 장비 보급 데이터 설정(Loader만 해당)
//----------------------------------------------------------------------//	
VOID CBillModule::SetLoaderSupply(_In_ T_BILL_SUPPLY *_pData, BOOL bClear/*=FALSE*/)
{
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	if(bClear == FALSE)
		CopyMemory(&m_data->bill_supply, _pData, sizeof(T_BILL_SUPPLY));
	else
		m_data->bill_supply.count[LO1] = 0;
	cs_lock.Unlock();
}

//---------------------------------------------------------------------//
// Name : SetCashCollect
// IN	: _In_ T_BILL_COLLECT* _pData
// OUT	: 
// DESC	: 장비 금액 회수 데이터 설정(Recycler만 해당)
//----------------------------------------------------------------------//	
VOID CBillModule::SetCashCollect(_In_ T_BILL_COLLECT* _pData)
{
	m_empty_float = FALSE;
	ZeroMemory(empty, sizeof(INT)*MAX_PCU_NUM);

	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	ZeroMemory(&m_data->bill_collect, sizeof(T_BILL_COLLECT));
	CopyMemory(&m_data->bill_collect, _pData, sizeof(T_BILL_COLLECT));

	m_empty_no = 0;

	for(INT i=LO1; i<MAX_PCU_NUM; i++)
	{
		if(m_data->bill_collect.count[i] > 0){
			if(m_empty_no == 0) m_empty_no = i;
			empty[i] = 1;	
			break;
		}
	}
	cs_lock.Unlock();
}

//----------------------------------------------------------------------//
// Name : CorrectionCancel
// IN	: T_XfsCashOrder *p_cash
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 취소 지폐 수량에 대해 처리
//----------------------------------------------------------------------//	
BOOL CBillModule::CorrectionCancel(_In_ LPVOID p_data)
{
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	cs_lock.Unlock();
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : CorrectionOutputAmount
// IN	: LPVOID p_data, BOOL bDeviceError
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 방출 된 지폐 수량에 대해 장비 내 보유량 보정
//----------------------------------------------------------------------//	
BOOL CBillModule::CorrectionOutputAmount(LPVOID p_data)
{
	DWORD   overAmt=0;
	UINT32  amount= 0;
	for(int i=LO1; i<MAX_PCU_NUM; i++){
		cassette[i].b_count = 0;
	}
	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();
	m_data->amt_output.amount = amount;
	if(overAmt > 0){ 
		m_data->dw_overAmt = overAmt;
	}
	cs_lock.Unlock();
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : ModuleStatusCheck
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 모듈 상태 점검 
//----------------------------------------------------------------------//	
VOID CBillModule::ModuleStatusCheck(OPTIONAL INT module_index/*=0*/)
{
	// 모듈 상태 검사
	UINT32 id = 0;
	INT loop_size = module_size;
	if(module_index != 0){
		loop_size = 1;
	}
	for(INT i=module_index; i<loop_size; i++)
	{	
		Sleep(10);
	}//end for
}

//----------------------------------------------------------------------//
// Name : GetLCUNum
// IN	: 
// OUT	: 
// DESC	: Logical Cash Unit Number를 저장
//----------------------------------------------------------------------//	
BOOL CBillModule::GetLCUNum()
{	
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : GetModulesId
// IN	: 
// OUT	: 
// DESC	: BNR Modules ID
//----------------------------------------------------------------------//	
BOOL CBillModule::GetModulesId()
{
	T_ModuleIdList moduleList;
	T_BnrXfsResult xfs_ret = m_protocol->GetModules(&moduleList);
	{
		XfsError(xfs_ret);
		return FALSE;
	}
		
	module_size = (INT)moduleList.size;
	return TRUE;
}

VOID CBillModule::CashboxConfiguration(IN INT n_status)
{
	m_cashbox_status = n_status;
}

VOID CBillModule::LoaderConfiguration(IN INT n_status)
{
	m_loader_status = n_status;
}

//----------------------------------------------------------------------//
// Name : ProcConnect
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Open Device
//----------------------------------------------------------------------//
VOID CBillModule::ProcConnect(IN BOOL bRecovery/*=FALSE*/)
{
	m_bDeviceOpen = FALSE;

	T_BnrXfsResult xfs_ret = BXR_NO_ERROR;

	if(bRecovery == FALSE)
	{			
	}
	else
	{		
		m_protocol->UsbReset();
	}
	xfs_ret = m_protocol->DeviceOpen(m_hParent);

	if(xfs_ret != BXR_NO_ERROR)
	{
		if(bRecovery == TRUE)
		{		
			xfs_ret = m_protocol->UsbKillNReload();

			if(xfs_ret == BXR_NO_ERROR)
			{
				xfs_ret = m_protocol->DeviceOpen(m_hParent);
			}
		}
		if(xfs_ret != BXR_NO_ERROR)
		{
			XfsError(xfs_ret);
		}
	}
	else
	{
		GetModulesId();
		m_bDeviceOpen = TRUE;
		GetLCUNum();		
		BNRStatus();
		AlarmOccured(_T("UFF"), TRUE);
		if(bRecovery == FALSE){
			m_bUsbNotConnected = TRUE;
		}
		m_result = EXECUTE_SUCCESS;     
		ResponseResultIn();
	}
	TimerStart(BNR_DEFAULT_OPERATION_TIME_OUT_IN_MS, TID_CHK_TRANS_STAT);
}


//----------------------------------------------------------------------//
// Name : ProcCancel
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Asynchronous Command Cancel
//		  "bnr_CancelWaitingCashTaken()" 은 사용하지 않는다.
//		  (해당 경우의 상황은 손님이 지폐를 가져가지 않은 경우 발생하므로
//		   취소가 아닌 "bnr_Retract"를 사용 처리 한다.)
//----------------------------------------------------------------------//
BOOL CBillModule::ProcCancel()
{
	if(m_bInsertAccept == TRUE)
		m_bInsertAccept = FALSE;

	T_BnrXfsResult xfs_ret = m_protocol->OperationCancel();
	if(xfs_ret != BXR_NO_ERROR)
	{
		XfsError(xfs_ret);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------//
// Name : ProcCashinStart
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Cash In Start
//----------------------------------------------------------------------//
BOOL CBillModule::ProcCashinStart()
{
	// UNR, JAM 금액 초기화
	ZeroMemory(&m_error_amt, sizeof(T_ERROR_AMOUNT));           

	m_bCashinMode  = FALSE;	// cashin mode
	m_bTransaction = FALSE;	// transaction 시작

	// 투입 금액 구조체 초기화
	CSingleLock cs_lock(&m_cs);
		
	cs_lock.Lock();
	ZeroMemory(&m_data->amt_input, sizeof(T_BILL_IN_AMOUNT));			
	ZeroMemory(&m_data->amt_output, sizeof(T_BILL_OUT_AMOUNT));
	cs_lock.Unlock();
	T_BnrXfsResult xfs_ret = m_protocol->CashInStart();			

	if(xfs_ret > BXR_NO_ERROR)
	{
		if(WaitForSingleObject(m_ocEvent, BNR_DEFAULT_OPERATION_TIME_OUT_IN_MS) == WAIT_OBJECT_0)
		{
			ResetEvent(m_ocEvent);
			if(m_cmd != CMD_BILL_CASHIN_START)
			{
				ProcCashinend();
				return TRUE;
			}
			ResponseResultIn();
			return TRUE;
		}
		BNRStatus();
	}
	else
	{
		xfs_ret *= -1;
		XfsError(xfs_ret);
	}
	return FALSE;
}


//----------------------------------------------------------------------//
// Name : ProcCashIn
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Cash In 
//----------------------------------------------------------------------//
INT CBillModule::ProcCashIn()
{
	m_bCashinMode  = FALSE;	
	m_bInsertAccept= FALSE;	
	BNRStatus();

	return RT_FAIL;
}


//----------------------------------------------------------------------//
// Name : ProcCashinend
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Cash In End
//----------------------------------------------------------------------//
INT CBillModule::ProcCashinend()
{
	m_bCashinMode	 = FALSE;
	m_bTransaction	 = FALSE;					
	ResetEvent(m_ocEvent);
	T_BnrXfsResult xfs_ret = m_protocol->CashInEnd();		

	if(xfs_ret > BXR_NO_ERROR)
	{
		if(WAIT_OBJECT_0 == WaitForSingleObject(m_ocEvent, BNR_DEFAULT_OPERATION_TIME_OUT_IN_MS))
		{
			if(m_cmd == CMD_BILL_CASHIN_END){
				ResponseResultIn();
			}	
			return RT_SUCCESS;
		}
	}
	else
	{
		xfs_ret *= -1;
		XfsError(xfs_ret);
	}	
	BNRStatus();
	return RT_FAIL;
}

//----------------------------------------------------------------------//
// Name : ProcCashinError
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Cash In 중 오류 발생 처리
//----------------------------------------------------------------------//
BOOL CBillModule::ProcCashinError()
{
	// 오류로 인한 투입, 방출 알람
	if(m_bCashinMode == TRUE)
	{
		m_bInsertAccept	 = FALSE;
		m_bCashinMode	 = FALSE;
		m_bTransaction	 = FALSE;					
		CSingleLock cs_lock(&m_cs);
		cs_lock.Lock();
		m_data->dw_jamAmt = 0;
		if(m_data->amt_input.amount > 0)
			m_data->dw_jamAmt = m_data->amt_input.amount;
		m_cs.Unlock();
		SetCommand(NOTIFY_BILL_INSERT);
		m_result = EXECUTE_FAIL;
		ResponseResultIn();
	}
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : ProcCashboxEmpty
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 지폐 회수함 보유 수량 초기화
//----------------------------------------------------------------------//
BOOL CBillModule::ProcCashboxEmpty(BOOL bClear/*=FALSE*/)
{
	BOOL _bRet = FALSE;
	return _bRet;
}

//----------------------------------------------------------------------//
// Name : ProcCashinRollback
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 거래 취소 
//----------------------------------------------------------------------//
INT CBillModule::ProcCashinRollback()
{
	T_BnrXfsResult	xfs_ret = BXR_NO_ERROR;
	m_rollback_tm_cnt = 0;
	m_bInsertAccept	  = FALSE;	// 투입 허용 금지
	m_bTransaction    = TRUE;
	xfs_ret = m_protocol->CashInRollback();	
	if(xfs_ret < BXR_NO_ERROR)
	{
		xfs_ret *= -1;
		XfsError(xfs_ret);
		BNRStatus();
		return RT_FAIL;
	}
	return RT_SUCCESS;
}

//----------------------------------------------------------------------//
// Name : ProcDisconnect
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Close Device
//----------------------------------------------------------------------//
BOOL CBillModule::ProcDisconnect()
{
	m_bDeviceOpen = FALSE;
	ProcCancel();
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : ProcDispense
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: dispense
//----------------------------------------------------------------------//
BOOL CBillModule::ProcDispense(_In_ BOOL bDispense/*=TRUE*/)
{
	m_wait_taken_cnt= 0;

	// 방출 가능한지 계산
	if(bDispense == FALSE)
	{
		m_bTransaction	= FALSE;
		m_bDispenseMode = FALSE;
		return TRUE;
	}
	else
	{
		// 방출		
		{
			BNRStatus();
			return FALSE;
		}
	}
	m_bTransaction	= TRUE;
	m_bDispenseMode = TRUE;
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : ProcDispenseError
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: 통신 단절, H/W 오류 발생에 대한 처리
//		  통신 단절과 H/W 오류가 동시 발생 될 수 있으므로 전체 UNR 처리
//----------------------------------------------------------------------//
BOOL CBillModule::ProcDispenseError(_In_ LPVOID p_data)
{
	UINT32  amount= 0;
	UINT32  cashbox=0;
	if(p_data != NULL){
	}
	else{
		cashbox = m_data->amt_output.amount;
	}

	m_bDispenseMode	 = FALSE;
	m_bTransaction	 = FALSE;								
	CSingleLock cs_lock(&m_cs);
	m_cs.Lock();					
	m_data->dw_unrAmt         = cashbox;
	m_data->amt_output.amount = 0;
	m_cs.Unlock();		

	return FALSE;
}


//----------------------------------------------------------------------//
// Name : ProcModuleLock
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Set recycler module lock/unlock
//----------------------------------------------------------------------//
BOOL CBillModule::ProcModuleLock()
{
	T_BnrXfsResult xfs_ret = BXR_NO_ERROR;
	error_code = xfs_ret * (-1);
	if(xfs_ret < BXR_NO_ERROR)
	{
		XfsError(xfs_ret);
		return FALSE;
	}

	CSingleLock cs_lock(&m_cs);
	cs_lock.Lock();

	for(INT i=LO1; i<MAX_PCU_NUM; i++)
	{
	}
	cs_lock.Unlock();

	if(xfs_ret < BXR_NO_ERROR)
	{
		xfs_ret *= (-1);
		XfsError(xfs_ret);
		return FALSE;
	}
	return FALSE;
}

//----------------------------------------------------------------------//
// Name : ProcEmpty
// IN	: TRUE(최저 경계까지 회수), FALSE(전체 회수)
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Empty Recycler, Loader
//----------------------------------------------------------------------//
BOOL CBillModule::ProcEmpty(IN BOOL toFloat)
{	
	CSingleLock cs_lock(&m_cs);
	T_BnrXfsResult xfs_ret = BXR_NO_ERROR;
	xfs_ret = m_protocol->BillEmpty(PCUName[m_empty_no], toFloat);	

	if(xfs_ret < BXR_NO_ERROR)
	{
		cs_lock.Lock();
		m_data->bill_collect.count[m_empty_no] = 0;
		cs_lock.Unlock();				
		empty[m_empty_no] = 0;
		XfsError(xfs_ret);
		return FALSE;
	}
	else
	{								
		empty[m_empty_no] = 2;				
	}
	return TRUE;
}


//----------------------------------------------------------------------//
// Name : ProcPresent
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: the presentation of the cash 
//----------------------------------------------------------------------//
BOOL CBillModule::ProcPresent()
{
	m_wait_taken_cnt = 0;
	m_bTransaction = FALSE;
	m_bDispenseMode= FALSE;
	T_BnrXfsResult xfs_ret = m_protocol->PresentAmount();
	if(xfs_ret < BXR_NO_ERROR)
	{
		xfs_ret *= -1;
		XfsError(xfs_ret);
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------//
// Name : ProcRetract
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: he application to force cash that has been presented to be retracted
//----------------------------------------------------------------------//
BOOL CBillModule::ProcRetract()
{
	T_BnrXfsResult ocResult;
	T_BnrXfsResult xfs_ret = m_protocol->Retract(&ocResult);
	if(xfs_ret != BXR_NO_ERROR)
	{
		XfsError(xfs_ret);
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : ProcReset
// IN	: 
// OUT	: TRUE(성공), FALSE(실패)
// DESC	: Open Device
//----------------------------------------------------------------------//
INT CBillModule::ProcReset(IN BOOL b_getStatus/*=TRUE*/)
{
	m_bAutoReset      = FALSE;
	m_bGetStatusReset = b_getStatus;

	BOOL _bRet = TRUE;

	ResetEvent(m_ocEvent);

	T_BnrXfsResult xfs_ret = m_protocol->DeviceReset();

	if(xfs_ret < BXR_NO_ERROR)
	{
		XfsError(xfs_ret);

		INT _nRet = RT_FAIL;

		if(xfs_ret == XFS_E_ILLEGAL)
		{
			ResetEvent(m_ocEvent);

			m_protocol->OperationCancel();
						
			if(WAIT_OBJECT_0 == WaitForSingleObject(m_ocEvent, BNR_DEFAULT_OPERATION_TIME_OUT_IN_MS))
			{
				ProcCashinend();
				return RT_AGAIN;
			}//end if
		}//end if(xfs_ret == XFS_E_ILLEGAL)
		
		{
			_nRet = ProcCashinend();

			if(_nRet == RT_AGAIN)
			{
				_nRet = ProcCashinend();

				if(_nRet == RT_FAIL)
				{
					return RT_FAIL;
				}
				else
					return RT_AGAIN;
			}				
		}
	}//end else
	else
	{
		if(WAIT_OBJECT_0 == WaitForSingleObject(m_ocEvent, BNR_RESET_OPERATION_TIME_OUT_IN_MS))
		{
			{
				if(m_cmd == CMD_BILL_RESET || m_cmd == CMD_BILL_ALARM_DELETE || m_cmd == CMD_BILL_AUDITCLR)
				{
					if(m_cmd == CMD_BILL_RESET)
						SetCommand(NOTIFY_BILL_STATUS);
					m_result = EXECUTE_SUCCESS;
					ResponseResultIn();
				}
				return RT_SUCCESS;
			}
		}
	}
	BNRStatus();

	return RT_FAIL;
}

//----------------------------------------------------------------------//
// Name : ProcSupplyLoder
// IN	: 
// OUT	: 
// DESC	: Loader 재고 추가
//----------------------------------------------------------------------//
BOOL CBillModule::ProcSupplyLoder(IN BOOL bAuditClr/*=FALSE*/)
{
	BOOL _bRet = TRUE;
	return TRUE;
}



//----------------------------------------------------------------------//
// Name : AlarmOccured
// IN	: _In_ TCHAR *p_code, IN INT32 moduleId/*=99*/
// OUT	: 
// DESC	: 알람 발생 확인 
//----------------------------------------------------------------------//
VOID CBillModule::AlarmOccured(_In_ TCHAR* p_code, BOOL b_remove)
{
	CString skey = _T("");
	T_BILL_ALARM alarm;
	ZeroMemory(&alarm, sizeof(T_BILL_ALARM));

	skey.Format(_T("B%s"), p_code);
	if(m_mapAlarm.Lookup(skey, alarm) == FALSE)
	{
		if(b_remove == FALSE){
			alarm.modulecode = 'B';
			sprintf_s(alarm.alarmcode, _T("%s"), p_code);
			sprintf_s(alarm.alarm_key, _T("B%s"), alarm.alarmcode);
			m_mapAlarm.SetAt(skey, alarm);
		}
	}
	else
	{
		if(b_remove == TRUE)
		{
			m_mapAlarm.RemoveKey(skey);
			return;
		}
	}
}


//----------------------------------------------------------------------//
// Name : DescErrorcode
// IN	: IN INT32 error_code(오류 코드)
// OUT	: 
// DESC	: 오류 코드 
//----------------------------------------------------------------------//
VOID CBillModule::XfsError(IN INT32 xfs_ret)
{
	if(xfs_ret < 0) xfs_ret = xfs_ret * (-1);

	//----------------------------------------------------------------------//
	// API Error
	if(xfs_ret > 0 && xfs_ret < 1000)
	{
	}

	//----------------------------------------------------------------------//
	// XFS Error
	if(xfs_ret > 1000 && xfs_ret < 10000)
	{
	}

	//----------------------------------------------------------------------//
	// USB Error
	if(xfs_ret > 10000)	
	{
	}
}


//----------------------------------------------------------------------//
// Name : makeMainModuleError
// IN	: 
// OUT	: 
// DESC	: main module 상태 정보를 가져옴
//----------------------------------------------------------------------//
VOID CBillModule::MainModuleError(_In_ T_MainModuleStatus *p_status)
{
	TCHAR err_code[5] = {0,};
	BOOL  b_remove	  = FALSE;

	// 메인 구성 모듈들 검사 - error code 
	for(INT i=0; i<(INT)p_status->size; i++)
	{		
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		AlarmOccured(err_code, b_remove);		
	}//end for
}


//----------------------------------------------------------------------//
// Name : BundlerError
// IN	: 
// OUT	: 
// DESC	: bundler module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::BundlerError(_In_ T_BundlerStatus *p_status)
{	
	switch(p_status->errorCode)
	{
	case BUEC_CANNOT_FIND_MARK:
		{
			AlarmOccured(_T("BCB"),  FALSE);
			break;
		}
	case BUEC_CANNOT_INIT_WITH_BILLS:
		{
			AlarmOccured(_T("BIM"),  FALSE);		
			break;
		}
	case BUEC_INIT_REQUIRED:
		{
			AlarmOccured(_T("BMM"),  FALSE);
			break;
		}
	case BUEC_NO_ERROR:
		{
			AlarmOccured(_T("BCB"),  TRUE);
			AlarmOccured(_T("BIM"),  TRUE);		
			AlarmOccured(_T("BMM"),  TRUE);			
			break;
		}
	}

	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{		
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
	}//end for
}


//----------------------------------------------------------------------//
// Name : SpineError
// IN	: 
// OUT	: 
// DESC	: Spin module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::SpineError(_In_ T_SpineStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("SCB"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("SIM"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			AlarmOccured(_T("SMM"),  FALSE);
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("SWM"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("SCB"),  TRUE);
			AlarmOccured(_T("SIM"),  TRUE);		
			AlarmOccured(_T("SMM"),  TRUE);
			AlarmOccured(_T("SWM"),  TRUE);
			break;
		}
	}

	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{		
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;

		switch(p_status->items[i].elementStatus.id)
		{			
		case SPINE_COVER:
			{
				sprintf_s(err_code, _T("%s"), _T("SCO"));
				if(p_status->items[i].coverStatus.functionalStatus == CFS_OPENED)
					b_remove = FALSE;
				break;
			}
		case SPINE_BILL_SENSOR_1:
			{
				sprintf_s(err_code, _T("%s"), _T("SS1"));
				if( (p_status->items[i].sensorStatus.errorCode != DEC_NO_ERROR) ||
					(p_status->items[i].sensorStatus.operationalStatus == OS_NOT_OPERATIONAL) )
					b_remove = FALSE;
				break;
			}
		case SPINE_BILL_SENSOR_3:
			{
				sprintf_s(err_code, _T("%s"), _T("SS3"));
				if( (p_status->items[i].sensorStatus.errorCode != DEC_NO_ERROR) ||
					(p_status->items[i].sensorStatus.operationalStatus == OS_NOT_OPERATIONAL) )
					b_remove = FALSE;
				break;
			}
		case SPINE_BILL_SENSOR_5:
			{
				sprintf_s(err_code, _T("%s"), _T("SS5"));
				if( (p_status->items[i].sensorStatus.errorCode != DEC_NO_ERROR) ||
					(p_status->items[i].sensorStatus.operationalStatus == OS_NOT_OPERATIONAL) )
					b_remove = FALSE;
				break;
			}
		}//end switch			
		AlarmOccured(err_code,  b_remove);
	}//end for	
}


//----------------------------------------------------------------------//
// Name : LoaderError
// IN	: 
// OUT	: 
// DESC	: Loader module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::LoaderError(_In_ T_LoaderStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("LCB"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("LIM"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{						
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("LWM"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("LCB"),  TRUE);
			AlarmOccured(_T("LIM"),  TRUE);		
			AlarmOccured(_T("LWM"),  TRUE);
			break;
		}
	}

	// 구성 모듈들 검사 - error code 
	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		switch(p_status->items[i].elementStatus.id)
		{			
			case 0x030750:	// LOADER BILL SENSOR
			{
				sprintf_s(err_code, _T("%s"), _T("LS1"));
				if( (p_status->items[i].sensorStatus.errorCode != DEC_NO_ERROR) ||
					(p_status->items[i].sensorStatus.operationalStatus == OS_NOT_OPERATIONAL) )
					b_remove = FALSE;
				break;
			}
		}//end switch				
		AlarmOccured(err_code,  b_remove);
	}//end for
}


//----------------------------------------------------------------------//
// Name : RecylerError3
// IN	: 
// OUT	: 
// DESC	: recycler module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::Recycler3Error(_In_ T_RecyclerStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("R3C"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("R3I"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			AlarmOccured(_T("R3F"),  FALSE);			
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("R3W"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("R3C"),  TRUE);
			AlarmOccured(_T("R3I"),  TRUE);		
			AlarmOccured(_T("R3F"),  TRUE);
			AlarmOccured(_T("R3W"),  TRUE);

			break;
		}
	}

	// 구성 모듈들 검사 - error code 
	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		AlarmOccured(err_code,  b_remove);
	}//end for
}



//----------------------------------------------------------------------//
// Name : RecylerError4
// IN	: 
// OUT	: 
// DESC	: recycler module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::Recycler4Error(_In_ T_RecyclerStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("R4C"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("R4I"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			AlarmOccured(_T("R4F"),  FALSE);
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("R4W"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("R4C"),  TRUE);
			AlarmOccured(_T("R4I"),  TRUE);		
			AlarmOccured(_T("R4F"),  TRUE);
			AlarmOccured(_T("R4W"),  TRUE);
			break;
		}
	}

	// 구성 모듈들 검사 - error code 
	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		AlarmOccured(err_code,  b_remove);
	}//end for
}


//----------------------------------------------------------------------//
// Name : RecylerError5
// IN	: 
// OUT	: 
// DESC	: recycler module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::Recycler5Error(_In_ T_RecyclerStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("R5C"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("R5I"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			AlarmOccured(_T("R5F"),  FALSE);			
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("R5W"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("R5C"),  TRUE);
			AlarmOccured(_T("R5I"),  TRUE);		
			AlarmOccured(_T("R5F"),  TRUE);
			AlarmOccured(_T("R5W"),  TRUE);
			break;
		}
	}

	// 구성 모듈들 검사 - error code 
	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		AlarmOccured(err_code,  b_remove);
	}//end for
}


//----------------------------------------------------------------------//
// Name : RecylerError6
// IN	: 
// OUT	: 
// DESC	: recycler module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::Recycler6Error(_In_ T_RecyclerStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("R6C"), FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("R6I"), FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			AlarmOccured(_T("R6F"),FALSE);
			break;
		}		
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("R6W"), FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("R6C"),  TRUE);
			AlarmOccured(_T("R6I"),  TRUE);		
			AlarmOccured(_T("R6F"),  TRUE);
			AlarmOccured(_T("R6W"),  TRUE);
			break;
		}
	}
	TCHAR err_code[5];
	BOOL  b_remove;
	for(INT i=0; i<(INT)p_status->size; i++)
	{
		if(m_bDeviceOpen == FALSE) break;
		ZeroMemory(err_code, sizeof(CHAR)*5);
		b_remove = TRUE;
		AlarmOccured(err_code,  b_remove);
	}//end for
}


//----------------------------------------------------------------------//
// Name : makeCashBoxError
// IN	: 
// OUT	: 
// DESC	: cashbox module 오류 코드 생성
//----------------------------------------------------------------------//
VOID CBillModule::CashBoxError(_In_ T_CashboxStatus *p_status)
{
	switch(p_status->errorCode)
	{
	case MODEC_COM_BREAKDOWN:
		{			
			AlarmOccured(_T("CCB"),  FALSE);
			break;
		}
	case MODEC_INCOMPATIBLE_MODULE:
		{
			AlarmOccured(_T("CIM"),  FALSE);
			break;
		}		
	case MODEC_MISSING_MODULE:
		{
			break;
		}
	case MODEC_BOOT_RUNNING:
		{
			break;
		}
	case MODEC_WRONG_MODULE_TYPE:
		{
			AlarmOccured(_T("CWM"),  FALSE);
			break;
		}
	case MODEC_SHUTTER_CLOSED:
		{
			AlarmOccured(_T("CSC"),  FALSE);
			break;
		}
	case MODEC_NO_COM:
		{
			AlarmOccured(_T("CNC"),  FALSE);
			break;
		}
	case MODEC_NO_ERROR:
		{
			AlarmOccured(_T("CCB"),  TRUE);
			AlarmOccured(_T("CIM"),  TRUE);		
			AlarmOccured(_T("CWM"),  TRUE);
			AlarmOccured(_T("CSC"),  TRUE);
			AlarmOccured(_T("CNC"),  TRUE);
			break;
		}
	}
}

VOID CBillModule::SetCommand(IN INT nCmd)
{
	CSingleLock cs(&m_cs);
	cs.Lock();{
		m_cmd = nCmd;	
	}cs.Unlock();
}