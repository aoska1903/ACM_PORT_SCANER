#include "stdafx.h"
#include "ConluxCoinModule.h"
#include "CoinProtocol.h"

#define COIN_PROG_VER _T("2019030501")

// 폴링 로그 기록 유무 플래그
BOOL g_bWritePollingLog = FALSE;

// 스레드 루프 종료 
BOOL g_bExit = FALSE;

CCriticalSection CConluxCoinModule::m_cs;
T_COIN_DATA* CConluxCoinModule::m_pData = NULL;
ST_CONLUX_COM_INFO* CConluxCoinModule::m_pstPack = NULL;

CConluxCoinModule::CConluxCoinModule(void) : CModuleTemplete(),
	m_protocol(NULL),
	m_bCoinTubeMissed(FALSE),
	m_bCloseShutter(TRUE)
{
}


CConluxCoinModule::~CConluxCoinModule(void)
{
	ReleaseWork();
}

/*
 * Name: GetOperationCompleteFlag
 * IN  :
 * Out : BOOL (TRUE: 수행 완료, FALSE, 미완료)
 * Desc: 명령 수행 완료 판단 플래그 값 전달
 */
BOOL CConluxCoinModule::GetOperationCompleteFlag()
{
	BOOL _bRet = TRUE;
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		_bRet = m_pData->operationcomplete; 
	}_cs.Unlock();
	return _bRet;
}

/*
 * Name: InitWork
 * IN  : 없음
 * Out : 없음
 * Desc: 진입점, 메모리와 객체 등 초기화 함수
 */
VOID CConluxCoinModule::InitWork()
{	
	////m_log.SetContentsLog(); //m_log.WriteLog(_T("| CConluxCoinModule::InitWork (+) |"));

	m_pData = new T_COIN_DATA;
	ZeroMemory(m_pData, sizeof(T_COIN_DATA));
	m_pData->operationcomplete = TRUE;
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 동전처리장치 프로세스 데이터 구조체 생성"));

	m_pstPack = new ST_CONLUX_COM_INFO;
	::ZeroMemory(m_pstPack, sizeof(ST_CONLUX_COM_INFO));
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 동전처리장치 통신 데이터 구조체 생성"));

	m_hExitThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 스레드 종료를 위한 이벤트 생성"));

	m_protocol = new CCoinProtocol();	
	m_protocol->CreateRecvEvent();	
	m_protocol->RegCallbackFunc_RecvComp(std::bind(&CConluxCoinModule::ReponseAnalysis, this, 
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 시리얼 통신 헬퍼 객체 생성"));

	SetOperationCompleteFlag(TRUE);

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 동전처리장치 보유 정보 폴더 생성"));		
	CreateAuditFolder();

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 동전처리장치 보유 정보"));
	T_COIN_HOLDING _tmpHolding;
	ZeroMemory(&_tmpHolding, sizeof(T_COIN_HOLDING));

	DWORD dwRet = ReadAudit(_T("COIN.audit"), &_tmpHolding, sizeof(T_COIN_HOLDING));
	if(dwRet == ERROR_SUCCESS){
		CopyMemory(&m_pData->coin_holding, &_tmpHolding, sizeof(T_COIN_HOLDING));
	}else{
		ZeroMemory(&m_pData->coin_holding, sizeof(T_COIN_HOLDING));
	}
	
	//m_log.SetContentsLog();
	//m_log.WriteLog(_T("\t- 동전 총 보유 수량 | 10원(%d), 50원(%d), 100원(%d), 500원(%d)"),
		m_pData->coin_holding.wCoinBox[0],
		m_pData->coin_holding.wCoinBox[1],
		m_pData->coin_holding.wCoinBox[2],
		m_pData->coin_holding.wCoinBox[3]);

	ZeroMemory(m_bef_error_stat.data,sizeof(BYTE)*2);

	// 버전 정보
	sprintf_s(m_pData->_ver, _T("%10.10s"), COIN_PROG_VER);

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
	//m_ini_ver.SetPathName(iniFilePath);
	//m_ini_ver.WriteString("Version", "Coin_Version", m_pData->_ver);

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 동전처리장치 버전: %s"), m_pData->_ver);

	m_bCoinReady = false;

	m_pThread = AfxBeginThread(_threadEntry, this, 0, 0, CREATE_SUSPENDED);	
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();	
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- 스레드 동작"));

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("| CConluxCoinModule::InitWork (-) |"));
}

/*
 * Name: RecvDataAnalysis
 * IN  : INT(수신 결과), BYTE *pData(동전처리장치에서 수신 받은 데이터), INT(데이터 길이)
 * Out : 없음
 * Desc: 동전 장치에서 수신 받은 패킷 분석 처리
 */
VOID CConluxCoinModule::ReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength)
{
	CSingleLock _cs(&m_cs);
	BOOL _bConnect = TRUE;

	if(nResult != ERROR_SUCCESS)
	{
		RetryCOM();
		return;
	}

	INT  _recvSize = 0;
	BYTE tmpRecvBuf[128];
	_cs.Lock();
	{
		_bConnect = m_pData->device_open;
		if(_bConnect == FALSE)
		{
			m_pData->device_open = TRUE;
			////m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 통신 상태 : 정상"));
			// 통신 오류 알람 해제
			AlarmOccurred(_T("FF"), TRUE);
		}

		// 수신 데이터 연속 저장
		CopyMemory(&m_pstPack->recv_buf[m_pstPack->data_size], pData, nLength);
		m_pstPack->data_size += nLength;

		// 임시 버퍼 데이터 저장
		memset(&tmpRecvBuf[0], 0x00, sizeof(tmpRecvBuf));
		CopyMemory(&tmpRecvBuf[0], m_pstPack->recv_buf, m_pstPack->data_size);
		_recvSize = m_pstPack->data_size;
	}_cs.Unlock();

	switch(tmpRecvBuf[0])
	{
		case ACK1:
		case ACK2:
		case ACK3:
			{
				RecvDataAnalysis(tmpRecvBuf, _recvSize);
				break;
			}
		case ACK4:	// 동작 중 리셋이 걸린 경우
			{
				// 스탠바이 명령 전송
				////m_log.WriteLog(_T("| RecvDataAnalysis | 응답:ACK4, StandBy Command 송신"));
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_STANDBY);
				_bConnect = CmdSendToCoin();
				if(_bConnect == FALSE)
				{	
					_cs.Lock();{
						if(m_pData){
							m_pData->device_open = FALSE;
						}
						////m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 통신 상태 : 오류"));
					}_cs.Unlock();

					//SetResult(EXECUTE_FAIL);				
				}
				else
				{
					//SetResult(EXECUTE_SUCCESS);
				}

				_cs.Lock();{
					if(m_pData){
						/*
						if(m_pData->cmd == CMD_COIN_POLLING){
							m_pData->cmd = NOTIFY_STATUS;
						}*/
					}					
				}_cs.Unlock();

				ResponseResultIn();

				break;
			}
		case ACK5:
			{
				break;
			}
		default:
			{		
				RetryCOM(TRUE);
				break;
			}
	}//end switch		
}

/*
 * Name: ReleaseWork
 * IN  : 없음
 * Out : 없음
 * Desc: 메모리, 객체 해제 등 종료 함수
 */
VOID CConluxCoinModule::ReleaseWork()
{
	////m_log.WriteLog(_T("| ReleaseWork (+) |"));

	CModuleTemplete::ExitWork();
	
	TimeoutTimerStop();

	g_bExit = TRUE;

	SetEvent(m_hExitThread);
	
	if(m_pThread != NULL)
	{
		DWORD _dwRet = ::WaitForSingleObject(m_pThread->m_hThread, 5000);
		if(_dwRet == WAIT_TIMEOUT){
			::TerminateThread(m_pThread, 1);
		}		
		//SAFE_DELETE(m_pThread);
	}

	ResetEvent(m_hExitThread);

	CloseHandle(m_hExitThread);
	
	////m_log.SetContentsLog(); //m_log.WriteLog(_T("- Thread 종료"));
		
	//SAFE_DELETE(m_protocol);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 통신 헬퍼 객체 해제"));
	
	//SAFE_DELETE(m_pData);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 동전처리장치 프로세스 데이터 구조체 해제"));

	//SAFE_DELETE(m_pstPack);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 동전처리장치 데이터 객체 해제"));

	//m_log.WriteLog(_T("| ReleaseWork (-) |"));
}

/*
 * Name: SetChangesData
 * IN  : const T_COIN_CHANGES *pChanges(동전 방출 수량 데이터 구조체)
 * Out :
 * Desc: 동전 방출 수량 데이터 설정 
 */
VOID CConluxCoinModule::SetChangesData(_In_ const T_COIN_CHANGES *pChanges)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		ZeroMemory(&m_pData->changes_data, sizeof(T_COIN_CHANGES));
		m_pData->changes_data.amount = pChanges->amount;
	}
	_cs.Unlock();
}

VOID CConluxCoinModule::SetChangesData(_In_ const INT pChanges)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		//m_log.SetContentsLog();
		//m_log.WriteLog(_T("| 동전 방출 수량 설정 | 거스름 금액: [%d]"), pChanges);			

		INT _total_amt = pChanges;

		m_changes.b_changes[3] = (INT)(_total_amt / 10000);
		_total_amt = _total_amt - m_changes.b_changes[3] * 10000;

		m_changes.b_changes[2] = (INT)(_total_amt / 1000);
		_total_amt = _total_amt - m_changes.b_changes[2] * 1000;

		m_changes.b_changes[1] = (INT)(_total_amt / 100);
		_total_amt = _total_amt - m_changes.b_changes[1] * 100;

		m_changes.b_changes[0] = (INT)(_total_amt / 10);
		
		//CopyMemory(m_changes.b_changes, pChanges, sizeof(m_changes.b_changes));

		m_bChangeComplete = FALSE;

		//m_log.SetContentsLog();
		//m_log.WriteLog(_T("| 동전 방출 수량 설정 | [0]:%d, [1]:%d, [2]:%d, [3]:%d"),
			m_changes.b_changes[0], m_changes.b_changes[1], m_changes.b_changes[2], m_changes.b_changes[3]);		

	}
	_cs.Unlock();
}

/*
 * Name: SetFileLog
 * IN  :
 * Out :
 * Desc: 파일 로그 설정
 */
/*
VOID CConluxCoinModule::SetFileLog(_In_ CFileLog* _pLog)
{
	if(_pLog == NULL){
		return;
	}

	//m_log = *_pLog;

	if(m_protocol){
		m_protocol->CreateLog();
	}
}
*/
/*
 * Name: SetResult
 * IN  : const INT _ret, 수행 결과 값
 * Out :
 * Desc: 수행 결과 값 설정
 */
VOID CConluxCoinModule::SetResult(_In_ const INT _ret)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock(); m_pData->n_result = _ret; _cs.Unlock();
}

/*
 * Name: TimeoutTimerStart
 * IN  : INT nInterval (이벤트 호출 시간 간격, ms), 기본 1초
 * Out :
 * Desc: 타임아웃 측정을 위한 타이머 가동
 */
VOID CConluxCoinModule::TimeoutTimerStart(_In_ INT nInterval/*=1000*/)
{
	//m_tm_timeout.SetTimedEvent(this, &CConluxCoinModule::TimeoutEvent);
	//m_tm_timeout.Start(nInterval, false, true);
}

/*
 * Name: TimeoutTimerStop
 * IN  :
 * Out :
 * Desc: 타임아웃 측정을 위한 타이머 중지
 */
VOID CConluxCoinModule::TimeoutTimerStop()
{
	//m_tm_timeout.Stop();
}

/*
 * Name: TimeoutEvent
 * IN  :
 * Out :
 * Desc: 타임아웃 발생 이벤트
 */
VOID CConluxCoinModule::TimeoutEvent()
{
	//m_tm_timeout.Stop();

	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		////m_log.SetContentsLog(); //m_log.WriteLog(_T("- 타임 아웃(Time Out) 발생"));
	}_cs.Unlock();

	RetryCOM();
}

/*
 * Name: ShutterCloseTimerStart
 * IN  : INT nInterval (이벤트 호출 시간 간격, ms), 기본 1초
 * Out :
 * Desc: 타임아웃 측정을 위한 타이머 가동
 */
VOID CConluxCoinModule::ShutterCloseTimerStart(_In_ INT nInterval/*=1000*/)
{
	//m_tm_close.SetTimedEvent(this, &CConluxCoinModule::ShutterCloseEvent);
	//m_tm_close.Start(nInterval, false, true);
}

/*
 * Name: ShutterTimerStop
 * IN  :
 * Out :
 * Desc: 타임아웃 측정을 위한 타이머 중지
 */
VOID CConluxCoinModule::ShutterCloseTimerStop()
{
	m_bCloseShutter = FALSE;
	//m_tm_close.Stop();
}

/*
 * Name: ShutterEvent
 * IN  :
 * Out :
 * Desc: 타임아웃 발생 이벤트
 */
VOID CConluxCoinModule::ShutterCloseEvent()
{
	//m_tm_close.Stop();

	////m_log.WriteLog(_T("| 셧터 닫기 완료 |"));

	if(m_bCloseShutter == TRUE)
	{
		//SetResult(EXECUTE_SUCCESS);
		ResponseResultIn();
	}
}

/*
 * Name: TestModeLog
 * IN  : _In_ HWND _hwnd (부모 핸들), _In_opt_ BOOL _bTestMode (테스트 로그 모드 선택)
 * Out :
 * Desc: 테스트 모드 로그 설정 - 대화상자에 로그 출력
 */
VOID CConluxCoinModule::TestModeLog(_In_ HWND _hwnd, _In_opt_ BOOL _bTestMode/* =TRUE */)
{
	if(_bTestMode == TRUE){
		//m_log.TestModeON(_hwnd);
	}else{
		//m_log.TestModeOFF();
	}

	if(m_protocol != NULL){
		m_protocol->TestMode(_hwnd, _bTestMode);
	}
}

//////////////////////////////////////////////////////////////////////////
// private member function

/*
 * Name: AlarmOccurred
 * IN  : TCHAR* pCode(알람 코드), BOOL bRelease(TRUE:해제, FALSE:발생)
 * Out : 
 * Desc: 동전처리장치에 알람 발생 및 해제
 */
VOID CConluxCoinModule::AlarmOccurred(_In_ TCHAR* pCode, _In_opt_ BOOL bRelease/* =FALSE */)
{
	CString skey = _T("");
	T_COIN_ALARM alarm;

	ZeroMemory(&alarm, sizeof(T_COIN_ALARM));
	
	skey.Format(_T("C%s"), pCode);

	if(map_alarm.Lookup(skey, alarm) == FALSE)
	{
		if(bRelease == FALSE)
		{
			//m_log.WriteLog(_T("| 알람 발생 | Code:%s"), skey);
			
			// 알람 키(ex. C01,...)
			sprintf_s(alarm.alarm_key, _T("%s"), skey);
			// 알람 모듈 코드
			alarm.modulecode = 'C';
			// 알람 코드(ex. 01,...)
			sprintf_s(alarm.alarmcode, _T("%s"), pCode);
			
			map_alarm.SetAt(skey, alarm);
		}
	}
	else
	{
		if(bRelease == TRUE)
		{
			//m_log.WriteLog(_T("| 알람 해제 | Code:%s"), skey);
			map_alarm.RemoveKey(skey);
		}
	}
}

/*
 * Name: AuditData
 * IN  : 없음
 * Out : 없음
 * Desc: 회계 데이터 저장, 읽기, 지우기
 */
VOID CConluxCoinModule::AuditData()
{
	if(m_bTestMode){
		return;
	}

	CSingleLock _cs(&m_cs);
	// 회계 기록
	T_COIN_HOLDING _tmpHolding;
	ZeroMemory(&_tmpHolding, sizeof(T_COIN_HOLDING));

	_cs.Lock();
	{
		//m_log.WriteLog(_T("| 회계 데이터 저장 |"));

		CopyMemory(&_tmpHolding, &m_pData->coin_holding, sizeof(T_COIN_HOLDING));						
		
		//m_log.WriteLog(_T("| 동전 총 보유 수량 (+) |"));
		//m_log.SetContentsLog(); 
		//m_log.WriteLog(_T("- 동전 수량 : 10원(%d), 50원(%d), 100원(%d), 500원(%d)"),
			_tmpHolding.wCoinBox[0],
			_tmpHolding.wCoinBox[1],
			_tmpHolding.wCoinBox[2],
			_tmpHolding.wCoinBox[3]);

		//m_log.WriteLog(_T("| 동전 총 보유 수량 (-) |"));
	}
	_cs.Unlock();

	WriteAduit(_T("COIN.audit"), &_tmpHolding, sizeof(T_COIN_HOLDING));
}

/*
 * Name: ClearAuditData
 * IN  : 없음
 * Out : 없음
 * Desc: 공용 데이터 중 투입, 방출 등 데이터 초기화 
 */
VOID CConluxCoinModule::ClearAuditData()
{
	CSingleLock _cs(&m_cs);
		
	_cs.Lock();
	{
		// 투입 수량 데이터 초기화
		m_nTmpAmt = 0;
		ZeroMemory(&m_pData->insert_data, sizeof(T_COIN_INSERT));

		// 방출 수량 데이터 초기화
		m_pData->change_amount = 0;
		ZeroMemory(&m_pData->changes_data, sizeof(T_COIN_CHANGES));

		ZeroMemory(&m_changes, sizeof(T_COIN_CHANGES));

		//m_log.WriteLog(_T("| 투입 및 방출 수량 데이터 초기화 완료 |"));
	}
	_cs.Unlock();
}

/*
 * Name: CmdSendToCoin
 * IN  : 
 * Out : 송신 성공 결과(TRUE: 성공, FALSE: 실패)
 * Desc: 동전처리장치에 명령 송신
 */
BOOL CConluxCoinModule::CmdSendToCoin()
{
	TimeoutTimerStop();

	CSingleLock _cs(&m_cs);

	BOOL _bConnect = FALSE;
	BYTE _cmd = 0x00;
	
	if(m_protocol == NULL)
	{
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 메모리에 프로토콜 담당 객체가 할당되지 않았음(m_protocol = NULL)"));
		return _bConnect;
	}

	_cs.Lock();
	{
		if(m_pstPack)
		{
			memset(&m_pstPack->recv_buf[0], 0x00, sizeof(m_pstPack->recv_buf));
			m_pstPack->data_size = 0;
			m_pstPack->is_data_send = false;
		}
	}
	_cs.Unlock();			

	// 동전처리장치 명령 가져오기
	_cmd = GetCoinCmd();

	if(_cmd != 0x61){
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- Module => Protocol | 코멘드 송신: %02X"), _cmd);
	}
	
	for(INT i=0; i<3; i++)
	{
		_bConnect = m_protocol->SendCmdToConlux(_cmd);
		if(_bConnect == FALSE)
		{
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 코멘드 송신 실패, 시도 카운트: %d"), (i+1));			
		}
		else
		{
			// 타임아웃 측정 타이머 가동
			TimeoutTimerStart();
			break;
		}
		Sleep(500);
	}//end for

	return _bConnect;
}

/*
 * Name: DataCmdSendToCoin
 * IN  : 
 * Out : 송신 성공 결과(TRUE: 성공, FALSE: 실패)
 * Desc: 동전처리장치에 데이터 명령 송신
 */
BOOL CConluxCoinModule::DataCmdSendToCoin(_In_ const INT _coin_cmd)
{
	TimeoutTimerStop();

	CSingleLock _cs(&m_cs);

	BOOL _bConnect = FALSE;
	BYTE _cmd = 0x00;
	
	if(m_protocol == NULL)
	{
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 메모리에 프로토콜 담당 객체가 할당되지 않았음(m_protocol = NULL)"));
		return _bConnect;
	}
	
	// 수신 버퍼 초기화
	BYTE ctrl_cmd = 0xFF;
	BYTE data_cmd = 0xFF;
	_cs.Lock();
	{
		if(m_pstPack)
		{
			memset(&m_pstPack->recv_buf[0], 0x00, sizeof(m_pstPack->recv_buf));
			m_pstPack->data_size = 0;
			m_pstPack->is_data_send = true;
			ctrl_cmd = m_pstPack->ctrl_type;
			data_cmd = m_pstPack->data_cmd;
		}
	}
	_cs.Unlock();			

	CString strLog(_T("")); 
	INT ConluxSendDataSize = 0;
	BYTE _btData[8];
	memset(_btData, 0x00, sizeof(_btData));
	/*
	switch(_coin_cmd)
	{
	case CMD_STAFF_COIN_READY:
	case CMD_TEST_OPEN:
	case CMD_COIN_OPEN_SHUTTER:
		{
			ConluxSendDataSize = 4;
			// BC (DC ~ DATA 까지 크기)
			_btData[0] = 0x02;
			// DC
			_btData[1] = CCoinProtocol::CONLUX_DC_CONTROL; 
			// Data
			_btData[2] = CCoinProtocol::CREM_ON | CCoinProtocol::DC_CLEAR; 
			break;
		}
	case CMD_TEST_CLOSE:
	case CMD_COIN_CLOSE_TOTALEND:
	case CMD_COIN_CLOSE_SHUTTER:
		{
			ConluxSendDataSize = 4;
			// BC (DC ~ DATA 까지 크기)
			_btData[0] = 0x02;
			// DC
			_btData[1] = CCoinProtocol::CONLUX_DC_CONTROL;
			// DATA
			_btData[2] = CCoinProtocol::CREM_OFF; 				

			break;
		}
	case CMD_TEST_CANCEL:
	case CMD_COIN_REJECT:
	case CMD_COIN_CHANGE_MONEY_ALL_STEP:
		{
			if(data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN) // 방출 데이터 생성
			{
				// 투입 금액 방출 데이터 생성
				//ConluxSendDataSize = 7;
				ConluxSendDataSize = 6;
				// BC
				_btData[0] = 0x04;
				// DC
				_btData[1] = CCoinProtocol::CONLUX_DC_AMT_RETURN;
				// DATA			
				_btData[2] = (m_changes.b_changes[0]<<4);					        	 // 10원 반금매수(BCD)
				_btData[3] = (m_changes.b_changes[2]<<4) | (m_changes.b_changes[1]);    // 1000원, 100원 반금매수(BCD)
				_btData[4] = (m_changes.b_changes[3]);						         // 10000원 반금매수(BCD)				
			}
			else
			{	// 셧터 닫기 행정 처리인 경우 또는 동전 투입금, 방출금 데이터 클리어

				ConluxSendDataSize = 4;
				// BC (DC ~ DATA 까지 크기)
				_btData[0] = 0x02;
				// DC
				_btData[1] = CCoinProtocol::CONLUX_DC_CONTROL;
				// DATA
				_btData[2] = ctrl_cmd; 
			}
			break;
		}
	case CMD_TEST_TOTALEND:
	case CMD_COIN_TOTAL_END:
		{
			ConluxSendDataSize = 4;
			// BC (DC ~ DATA 까지 크기)
			_btData[0] = 0x02;
			// DC
			_btData[1] = CCoinProtocol::CONLUX_DC_CONTROL;
			// DATA
			_btData[2] = CCoinProtocol::DC_CLEAR; 
			break;			
		}
		
	}//end switch
	*/
	// FCC
	_btData[ConluxSendDataSize-1] = m_protocol->GetLRC(_btData, ConluxSendDataSize-1);

	strLog = _T("- Module => Protocol | 코멘드 데이터 송신: ");
	for(INT loop=0; loop<ConluxSendDataSize; loop++){
		strLog.AppendFormat(_T("[%02X]"), _btData[loop]);
	}
	//m_log.SetContentsLog(); //m_log.WriteLog(strLog);			

	for(INT i=0; i<3; i++)
	{
		_bConnect = m_protocol->SendDataToConlux(_btData, ConluxSendDataSize);
		if(_bConnect == FALSE)
		{
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 코멘드 송신 실패, 시도 카운트: %d"), (i+1));			
		}
		else
		{
			// 타임아웃 측정 타이머 가동
			TimeoutTimerStart();
			break;
		}
		Sleep(500);
	}//end for

	return _bConnect;
}

/*
 * Name: IsDataSend
 * IN  :
 * Out : true(데이터 코멘트 송신), false(데이터 코멘트 송신 없음)
 * Desc: 동전체인저 데이터 코멘드 송신 유무 리턴
 */
bool CConluxCoinModule::IsDataSend()
{
	bool _IsDataSend = false;
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		if(m_pstPack){
			_IsDataSend = m_pstPack->is_data_send;
		}
	}_cs.Unlock();	

	return _IsDataSend;
}

/*
 * Name: StatusPacketAnalysis
 * IN  :
 * Out :
 * Desc: 동전처리장치 일괄요구 응답 데이터 분석 처리
 */
INT CConluxCoinModule::StatusPacketAnalysis()
{
	static const INT _coin_type[4] = {10, 50, 100, 500};					

	CSingleLock _cs(&m_cs);

	INT _dataSize = 0;
	BYTE _tmpData[128];
	memset(&_tmpData[0], 0x00, sizeof(_tmpData));

	// [11][1E][05] [08][00][00][10][00] [05][09][00][13][35][00] [05][0A][00][00][00][00] [02][0B][00] [04][0C][00][00][00][03][0D][01][41][69]
	_cs.Lock();{		
		CopyMemory(&_tmpData[0], m_pstPack->recv_buf, m_pstPack->data_size);
		_dataSize = m_pstPack->data_size;
		_dataSize -= 3; // 실제 데이터 크기 (ACK, BC, FCC 제외) 
	}_cs.Unlock();

	INT _InCnt[4] = {0}; // 투입 매수		
	INT _OutCnt[4]= {0}; // 방출 매수	
	INT _index = 2;
	INT _result=1;
	CString strLog(_T(""));

	do 
	{		
		INT _BC = static_cast<INT>(_tmpData[_index++]);		
		BYTE _DC = _tmpData[_index++];		
		BYTE _recvData[8];
		memset(_recvData, 0x00, sizeof(_recvData));
		for(INT i=0; i<_BC-1; i++)
			_recvData[i] = _tmpData[_index++];
				
		switch(_DC)
		{
			// 총 투입 개수
		case CCoinProtocol::CONLUX_DC_INSERT:
			{	
				// 투입 금액이 없는 경우 빠져나감
				if(bcd2dec(_recvData[0]) == 0 && bcd2dec(_recvData[1]) == 0 && bcd2dec(_recvData[2]) == 0 && bcd2dec(_recvData[3]) == 0){
					break;
				}

				BYTE _in_cnt[4] = {0};
				_cs.Lock();{					
					CopyMemory(_in_cnt, m_pData->insert_data.b_coin_type, sizeof(_in_cnt));
				}_cs.Unlock();

				if( bcd2dec(_recvData[0]) == _in_cnt[0] && 
					bcd2dec(_recvData[1]) == _in_cnt[1] && 
					bcd2dec(_recvData[2]) == _in_cnt[2] && 
					bcd2dec(_recvData[3]) == _in_cnt[3])
				{
					break;
				}

				_cs.Lock();
				{				

					for(INT _index=0; _index<4; _index++)
					{
						// 권 종별 투입 수량
						_InCnt[_index] = bcd2dec(_recvData[_index]) - m_pData->insert_data.b_coin_type[_index];
						if(_InCnt[_index] > 0){							
							//m_log.WriteLog(_T("| 투입 금액: [%d(원)] |"), (_InCnt[_index]*_coin_type[_index]));
						}						
						// 총 투입 수량
						m_pData->insert_data.b_coin_type[_index] = bcd2dec(_recvData[_index]); 
					}//end for
				}				
				_cs.Unlock();				
				break;
			}
			// 총 잔돈 매수
		case CCoinProtocol::CONLUX_DC_RETURN:
			{
				// 방출 금액이 없는 경우 빠져나감
				if(bcd2dec(_recvData[0]) == 0 && bcd2dec(_recvData[1]) == 0 && bcd2dec(_recvData[2]) == 0 && bcd2dec(_recvData[3]) == 0){
					break;
				}
				_cs.Lock();
				{	
					for(INT i=0; i<4; i++)
					{
						// 각 방출 수량
						_OutCnt[i] = bcd2dec(_recvData[i]) - m_pData->changes_data.b_changes[i];
						if(_OutCnt[i] > 0){
							//m_log.WriteLog(_T("| 방출 금액: %d(원) |"), (_OutCnt[i]*_coin_type[i]));
						}		
						//방출 수량
						m_pData->changes_data.b_changes[i] = bcd2dec(_recvData[i]); 
					}
				}								
				_cs.Unlock();				
				break;
			}
			// 동전처리장치 상태
		case CCoinProtocol::CONLUX_DC_STATUS:
			{
				BOOL _stat_change = FALSE;

				m_status.data = 0x00;
				CopyMemory(&m_status.data, &_recvData[0], sizeof(BYTE));

				if(m_bef_stat.data != m_status.data){
					m_bef_stat.data = m_status.data;					
				}else{
					break;
				}

				//m_log.WriteLog(_T("| 동전처리장치 상태 정보 (+) |"));

				if(m_status.bit_data.CREMON > 0)
				{
					m_bCloseShutter = FALSE;
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- CREM(동전 투입 가능 상태): %u"), m_status.bit_data.CREMON);					
				}
				else
				{
					m_bCloseShutter = TRUE;
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- CREM(동전 투입 불가 상태): %u"), m_status.bit_data.CREMON);					
				}
				if(m_status.bit_data.INVENTORY > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 인벤토리 동작 상태: %u"), m_status.bit_data.INVENTORY);					
				}
				if(m_status.bit_data.CHANGE > 0)
				{					
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 잔돈 불출 가능 상태: %u"), m_status.bit_data.CHANGE);
				}
				if(m_status.bit_data.RETURN_SWON > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 반환 스위치 상태: %u"), m_status.bit_data.RETURN_SWON);
				}
				if(m_status.bit_data.CHANGE_COMPLETE > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 불출 종료 상태: %u"), m_status.bit_data.CHANGE_COMPLETE);
				}
				if(m_status.bit_data.CLEAR_COMPLETE > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 클리어 완료 상태: %u"), m_status.bit_data.CLEAR_COMPLETE);
				}
				if(m_status.bit_data.INVENTORY_STAT > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 인벤토리 동작 금지 상태: %u"), m_status.bit_data.INVENTORY_STAT);
				}				

				//m_log.WriteLog(_T("| 동전처리장치 상태 정보 (-) |"));

				break;
			}
			// 동전처리장치 이상
		case CCoinProtocol::CONLUX_DC_ERROR:
			{
				memset(m_error_stat.data, 0x00, sizeof(m_error_stat.data));
				CopyMemory(m_error_stat.data, _recvData, sizeof(m_error_stat.data));
				
				if(m_bef_error_stat.data[0] == m_error_stat.data[0] && m_bef_error_stat.data[1] == m_error_stat.data[1]){
					break;
				}

				CopyMemory(m_bef_error_stat.data, m_error_stat.data, sizeof(m_bef_error_stat.data));

				// 튜브 탈착
				_cs.Lock();{
					m_pData->coin_stat.cointube_missed = m_error_stat.bit_data.SAFTY_SW;
				}_cs.Unlock();

				INT dev_cmd = GetCommand();
				/*
				if(dev_cmd == CMD_COIN_POLLING)
				{
					dev_cmd = NOTIFY_COIN_STATUS;
					SetCommand(dev_cmd);
				}
				*/
				_result = 0;

				//m_log.WriteLog(_T("| 동전처리장치 이상 상태 (+) |"));
				if(m_error_stat.bit_data.COINCHANGER > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 코인체인터 이상 상태: %u"), m_error_stat.bit_data.COINCHANGER);									
				}

				if(m_error_stat.bit_data.WORKABLE > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 동작가 상태: %u"), m_error_stat.bit_data.WORKABLE);					
				}

				if(m_error_stat.bit_data.ACCEPTOR > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 악셉터 이상: %u"), m_error_stat.bit_data.ACCEPTOR);
					AlarmOccurred(_T("110"));
				}else{
					AlarmOccurred(_T("110"), TRUE);
				}

				if(m_error_stat.bit_data.EMPTYSW_10 > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 엠프티 스위치: %u"), m_error_stat.bit_data.EMPTYSW_10);
					AlarmOccurred(_T("111"));
				}else{
					AlarmOccurred(_T("111"), TRUE);
				}

				if(m_error_stat.bit_data.RETURN_SW > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 반환 스위치 이상: %u"), m_error_stat.bit_data.RETURN_SW);
					AlarmOccurred(_T("112"));
				}else{
					AlarmOccurred(_T("112"), TRUE);
				}

				if(m_error_stat.bit_data.COIN_RETURN_ERROR > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 주화 배출 불량: %u"), m_error_stat.bit_data.COIN_RETURN_ERROR);
					AlarmOccurred(_T("113"));
				}else{
					AlarmOccurred(_T("113"), TRUE);
				}

				if(m_error_stat.bit_data.SAFTY_SW > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 세프티스위치(동전튜브 탈착) 이상: %u"), m_error_stat.bit_data.SAFTY_SW);
					AlarmOccurred(_T("114"));					
				}
				else
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 세프티스위치(동전튜브 장착)"));
					AlarmOccurred(_T("114"), TRUE);
				}

				//m_log.WriteLog(_T("| 동전처리장치 이상 상태 (-) |"));
				break;
			}
		}
	} while (_index < _dataSize);

	// 투입 동전이 있는 경우 
	if( _InCnt[0] > 0 || _InCnt[1] > 0 || _InCnt[2] > 0 || _InCnt[3] > 0 )
	{
		INT _cmd = GetCommand();

		// 동전 준비 상태를 위한 투입 경우
		if(m_bCoinReady)
		{
			m_bCoinReady = false;

			//_cmd = CMD_STAFF_COIN_READY_COMPLETE;			
			SetCommand(_cmd);

			return _result;
		}
		else
		{
			if(m_status.bit_data.CREMON == 1){
				//m_log.WriteLog(_T("| 동전 투입 | 보유 정보 업데이트"));
			}else{
				//m_log.WriteLog(_T("| 동전 투입 | 셧터 닫기 후 투입, 보유 정보 업데이트"));
			}
		}

		_cs.Lock();
		{
			//m_log.WriteLog(_T("| 투입 정보 (+) |"));

			// 투입 금액
			m_pData->insert_data.dw_amount = 
				m_pData->insert_data.b_coin_type[0] * 10  +
				m_pData->insert_data.b_coin_type[1] * 50  +
				m_pData->insert_data.b_coin_type[2] * 100 +
				m_pData->insert_data.b_coin_type[3] * 500;

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- 총 투입 금액: %d(원), 수량: 10원(%d), 50원(%d), 100원(%d), 500원(%d)"),
				m_pData->insert_data.dw_amount,
				m_pData->insert_data.b_coin_type[0],
				m_pData->insert_data.b_coin_type[1],
				m_pData->insert_data.b_coin_type[2],
				m_pData->insert_data.b_coin_type[3]);

			//m_log.WriteLog(_T("| 투입 정보 (-) |"));

			// 보유량 증가
			for(INT i=0; i<4; i++)
			{
				if(m_bTestMode){
					break;
				}

				m_pData->coin_holding.wCoinBox[i] += _InCnt[i];
			}
		}
		_cs.Unlock();

		AuditData();
		/*
		if(_cmd == CMD_COIN_POLLING)
		{
			_cmd = NOTIFY_COIN_INSERT;
			SetCommand(_cmd);
		}
		else if(_cmd == CMD_COIN_REJECT || _cmd == CMD_TEST_CANCEL)
		{
			_result = EXECUTE_ING;
		}*/
	}

	// 동전 방출 경우 
	if( _OutCnt[0] > 0 || _OutCnt[1] > 0 || _OutCnt[2] > 0 || _OutCnt[3] > 0 )
	{
		// 보유량 정보 보정
		_cs.Lock();
		{
			//m_log.WriteLog(_T("| 방출 정보 (+) |"));
			// 방출 금액
			m_pData->change_amount = m_pData->changes_data.amount = 
				m_pData->changes_data.b_changes[0] * 10  +
				m_pData->changes_data.b_changes[1] * 50  +
				m_pData->changes_data.b_changes[2] * 100 +
				m_pData->changes_data.b_changes[3] * 500;

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- 총 방출 금액: %d(원), 수량: 10원(%d), 50원(%d), 100원(%d), 500원(%d)"),
				m_pData->change_amount,
				m_pData->changes_data.b_changes[0],
				m_pData->changes_data.b_changes[1],
				m_pData->changes_data.b_changes[2],
				m_pData->changes_data.b_changes[3]);

			//m_log.WriteLog(_T("| 방출 정보 (-) |"));

			//m_log.WriteLog(_T("| 동전 방출 | 보유 정보 업데이트"));
			for(INT i=0; i<4; i++)
			{
				if(m_bTestMode){
					break;
				}

				// 권 종별 보유량 보정
				if(m_pData->coin_holding.wCoinBox[i] > _OutCnt[i]){
					m_pData->coin_holding.wCoinBox[i] -= _OutCnt[i];
				}else{
					m_pData->coin_holding.wCoinBox[i] = 0;
				}

				// 권 종별 투입 된 수량에서 방출 수량을 뺀다.
				m_nTmpAmt -= (_OutCnt[i] * _coin_type[i]);							
			}
		}
		_cs.Unlock();

		AuditData();
	}

	// 반환 중 오류 발생 경우
	if(m_error_stat.bit_data.COINCHANGER > 0 && m_error_stat.bit_data.COIN_RETURN_ERROR > 0){
		//m_log.WriteLog("| 동전 상태 이상 | 결과:[%d]", EXECUTE_FAIL);
		_result = EXECUTE_FAIL;
	}

/*
	// 불출 정상 종료인 경우
	if( (m_status.bit_data.CHANGE_COMPLETE == 1 && m_bChangeComplete == FALSE ) || 
		(m_status.bit_data.CLEAR_COMPLETE == 1 && m_bCointReturn == FALSE) )
	{		
		m_bChangeComplete = TRUE;
	}

	// 코인체이저 상태 이상 && 주화 배출 불량
	if(m_error_stat.bit_data.COINCHANGER == 1 && m_error_stat.bit_data.COIN_RETURN_ERROR == 1)
	{
		_result = EXECUTE_FAIL;

		if(m_bChangeComplete == FALSE){
			m_bChangeComplete = TRUE;
		}
	}*/

	return _result;
}

/*
 * Name: RecvDataAnalysis
 * IN  :
 * Out : 
 * Desc: 수신 데이터 분석 처리, ReponseAnalysis에서 호출
 */
VOID CConluxCoinModule::RecvDataAnalysis(_In_ const BYTE *pData, _In_ const INT nSize)
{
	CSingleLock _cs(&m_cs);

	CString strLog(_T(""));
	BOOL _bResult = TRUE;	
	INT _result = 1;
	INT _cmd = 0;			// 동전 프로세스 명령
	BYTE _coin_cmd = 0x00;	// 동전 장치 명령
	BYTE _fcc = 0x00;		// 프레임 체크 코드
	BYTE _ctrl_cmd = 0xFF;  // 코인체인저 제어데이터(DC=0) 데이터(ex CREM ON, CREM OFF,...) 
							// CCoinProtocol.h 파일에서 CONLUX_COIN_CTRL_CMD 참조
	BYTE _data_cmd = 0x00;

	// 동전 프로세서 명령 저장
	_cmd = GetCommand();

	// 동전 장치 명령 저장
	_coin_cmd = GetCoinCmd();

	switch(_coin_cmd)
	{
	case CCoinProtocol::CONLUX_CMD_STANDBY:
		{
			ResetRetryCnt();
			_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
			if(_bResult == TRUE){
				return;
			}else{
				_result = 0;
			}
			break;
		}
	case CCoinProtocol::CONLUX_CMD_ORDER_ALL:	// 일괄 요구
		
		{
		/*
			INT _bc = static_cast<INT>(pData[1]);

			if(_bc == 0)
			{					
				return;
			}		
			else if(_bc > (nSize-3))
			{
				return;
			}
			else if(_bc < (nSize-3))
			{
				ResetRetryCnt();
				_bResult = SendCoinCmd(FALSE, 0);
				if(_bResult == TRUE){
					return;
				}else{
					_result = 0;
				}
			}
			else
			{// 데이터 모두 수신 완료				
				TimeoutTimerStop();

				// 로그 출력
				if(_cmd != CMD_COIN_POLLING)
				{
					strLog = (_T("- 수신 데이터: "));
					for(INT i=0; i<nSize; i++){
						strLog.AppendFormat(_T("[%02X]"), pData[i]);
					}
					//m_log.SetContentsLog(); //m_log.WriteLog(strLog);
				}

				// FCC 검사
				_fcc = m_protocol->GetLRC(pData+1, nSize-2);
				if(_fcc != pData[nSize-1])
				{
					//m_log.WriteLog(_T("- FCC 값이 상이함 (송신 된 FCC: %02X, 계산 된 FCC: %02X"), pData[nSize-1], _fcc);
					_result = EXECUTE_FAIL;
				}
				else
				{
					// 수신 패킷 분석
					_result = StatusPacketAnalysis();
					if(_result == EXECUTE_FAIL)
					{
						// 초기화 명령 송신을 한다.
						if( _cmd == CMD_COIN_REJECT					|| _cmd == CMD_TEST_CANCEL			||
							_cmd == CMD_COIN_CHANGE_MONEY_ALL_STEP	|| _cmd == CMD_COIN_CHANGE_MONEY	)
						{
							// 방출 실패 경우 초기화 명령 송신	
							//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 방출 실패, 초기화 명령 송신"));	
							ResetRetryCnt();			
							_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::DC_CLEAR);
							if(_bResult == TRUE){	
								m_nCurCtrlCmd = CCoinProtocol::CONLUX_DC_CONTROL;								
								//return;
							}
						}

						break;
					}

					// 거래 취소 행정 처리
					if(_cmd == CMD_COIN_REJECT || _cmd == CMD_TEST_CANCEL)
					{						
						if(m_nCurCtrlCmd == CCoinProtocol::CREM_OFF){
							// 거래 취소 수행 결과 점검			
							_result = procCancel();
						}else if(m_nCurCtrlCmd == CCoinProtocol::COIN_RETURN){
							// 동전 방출 수행 결과 점검			
							_result = procChange();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// 클리어 수행 결과 점검			
							_result = procClear();						
						}
												
						if(_result == EXECUTE_ING){
							return;
						}
					}//end if
					else if(_cmd == CMD_COIN_CLOSE_SHUTTER || _cmd == CMD_COIN_CLOSE_TOTALEND || _cmd == CMD_COIN_TOTAL_END)
					{
						if(m_nCurCtrlCmd == CCoinProtocol::CREM_OFF){
							// 셧터 닫기 수행 결과 점검			
							_result = procClose();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// 클리어 수행 결과 점검			
							_result = procClear();						
						}

						if(_result == EXECUTE_ING){
							return;
						}
					}
					else if(_cmd == CMD_COIN_CHANGE_MONEY_ALL_STEP || _cmd == CMD_COIN_CHANGE_MONEY)
					{
						if(m_nCurCtrlCmd == CCoinProtocol::COIN_RETURN){
							// 동전 방출 수행 결과 점검			
							_result = procChange();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// 클리어 수행 결과 점검			
							_result = procClear();						
						}

						if(_result == EXECUTE_ING){
							return;
						}
					}
				}
			}
			*/
			break;
		}
	case CCoinProtocol::CONLUX_CMD_ORDER_OUT:	// 출력 지령
	case CCoinProtocol::CONLUX_CMD_REORDER_OUT: // 출력 재 지령
		{
			/*
			// 데이터 커멘드 수신 판단 플래그 저장
			bool _IsDataSend = IsDataSend();
			
			// 데이터 커멘드 전송 결과 후 응답 수신
			if(_IsDataSend)	
			{
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 코인체인저 데이터 커멘드 응답 수신"));

				//////////////////////////////////////////////////////////////////////////
				// 셧터 닫기 요청, 금액 반출 경우 회계 파일에 데이터 저장

				if(_cmd == CMD_COIN_CLOSE_SHUTTER || _cmd == CMD_COIN_CLOSE_TOTALEND || _cmd == CMD_COIN_TOTAL_END)
				{
					m_nCurCtrlCmd = GetCoinCtrlCmd();

					// 상태 정보 읽기
					_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
					if(_bResult == TRUE){
						return;
					}

					_result = EXECUTE_FAIL;					
					break;
				}

				//////////////////////////////////////////////////////////////////////////
				// 거스름 금액 방출
				if(_cmd == CMD_COIN_CHANGE_MONEY_ALL_STEP || _cmd == CMD_COIN_CHANGE_MONEY)
				{
					_data_cmd = GetDataCmd();
					_ctrl_cmd = GetCoinCtrlCmd();

					if(_data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN || _data_cmd == CCoinProtocol::CONLUX_DC_CONTROL)
					{
						if(_data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN){
							m_nCurCtrlCmd = CCoinProtocol::COIN_RETURN;
						}else{
							m_nCurCtrlCmd = _ctrl_cmd;
						}

						// 상태 정보 읽기
						_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
						if(_bResult == TRUE){
							return;
						}

						_result = EXECUTE_FAIL;		
					}
					else
					{
						//m_log.WriteLog(_T("| 이상 | 해당 행정에 필요 없는 데이터 커멘드 [%02X]"), _data_cmd);
						_result = EXECUTE_FAIL;
					}
				}

				//////////////////////////////////////////////////////////////////////////
				// 거래 취소
				if(_cmd == CMD_COIN_REJECT || _cmd == CMD_TEST_CANCEL)
				{		
					_data_cmd = GetDataCmd();
					_ctrl_cmd = GetCoinCtrlCmd();

					if(_data_cmd == CCoinProtocol::CONLUX_DC_CONTROL || _data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN)
					{
						if(_data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN){
							m_nCurCtrlCmd = CCoinProtocol::COIN_RETURN;
						}else{
							m_nCurCtrlCmd = _ctrl_cmd;
						}

						// 상태 정보 읽기
						_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
						if(_bResult == TRUE){
							return;
						}else{
							_result = EXECUTE_FAIL;
						}
					}
					else
					{
						//m_log.WriteLog(_T("| 이상 | 취소 행정에 필요 없는 데이터 커멘드: [%02X]"), _data_cmd);
						_result = EXECUTE_FAIL;
					}
				}
				break;
			}			
			else           
			{// 데이터 커멘드 송신
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 코인체인저 커멘드 결과 수신 완료"));

				BOOL _bResult = SendCoinCmd(TRUE, 0);
				if(_bResult == TRUE){
					return;
				}else{
					_result = EXECUTE_FAIL;
				}
			}
			*/
			break;
		}
	}//end switch

	SetResult(_result);

	ResponseResultIn();
}

/*
 * Name: RetryCOM
 * IN  : BOOL _bNotConnectError (통신 오류가 아닌 데이터 이상으로 재전송인지 판단하는 플래그, TRUE: 통신 접속, FALSE: 이전 통신 상태)
 * Out : 
 * Desc: 통신 데이터 재시도 
 */
VOID CConluxCoinModule::RetryCOM(_In_opt_ BOOL _bNotConnectError/*=FALSE*/)
{
	CSingleLock _cs(&m_cs);

	BOOL _bConnect = TRUE;

	bool _IsDataSend = false;

	_cs.Lock();
	{
		// 코인체인저 데이터 전송 플래그 저장
		_IsDataSend = m_pstPack->is_data_send;

		if(m_pstPack->retry_cnt >= 3)
		{
			m_pstPack->retry_cnt = 0;			
			_bConnect = FALSE;
		}
		else
		{
			m_pstPack->retry_cnt++;
		}
	}
	_cs.Unlock();

	if(_bConnect == TRUE)
	{
		INT _coin_cmd = GetCoinCmd();

		// 코인체인저에 데이터 전송 후 ACK1 응답이 없는 경우
		if(_coin_cmd == CCoinProtocol::CONLUX_CMD_ORDER_OUT && _IsDataSend)
		{
			BYTE _data_cmd, _ctrl_cmd = 0xFF;
			_ctrl_cmd = GetCoinCtrlCmd();
			_data_cmd = GetDataCmd();
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 출력 재지령 코멘드 전송"));
			SetCoinCmd(CCoinProtocol::CONLUX_CMD_REORDER_OUT, _data_cmd, _ctrl_cmd);
		}

		_bConnect = CmdSendToCoin();
	}
	
	if(_bConnect == FALSE)
	{
		// NAK 등 통신 중 수신 데이터 이상이 3번인 경우
		// 통신 연결 오류가 아니므로 결과 값만 실패로 저장 
		if(_bNotConnectError == FALSE)
		{
			_cs.Lock();{
				if(m_pData){
					m_pData->device_open = FALSE;
				}
				AlarmOccurred(_T("FF"));
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 통신 상태 : 오류"));
			}_cs.Unlock();
			/*
			if(GetCommand() == CMD_COIN_POLLING){
				INT cmd = NOTIFY_COIN_STATUS;
				SetCommand(cmd);
			}
			*/
		}

		SetResult(0);				

		ResponseResultIn();
	}
}

/*
 * Name: RespnseResultIn
 * IN  : 없음
 * Out : 없음
 * Desc: 요청 명령 수행 결과 데이터 저장
 */
VOID CConluxCoinModule::ResponseResultIn()
{
	INT cmd = GetCommand();

	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		/*
		if(cmd != CMD_COIN_POLLING)
		{
			//m_log.WriteLog(_T("| ResponseResultIn (+) |"));
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 동전처리장치 통신 연결 상태 [%d] (1:접속, 0:단절)"), m_pData->device_open); 
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 메인 수행 명령 코드 [%d]"), m_pData->cmd);
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 동전처리장치 수행 코드 [%d], 수행 결과 값 [%d] (1:성공, 0:실패)"), 
				m_pData->dev_cmd, m_pData->n_result); 

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- 투입금 정보 | 10원:[%d], 50원:[%d], 100원:[%d], 500원:[%d]"), 
				m_pData->insert_data.b_coin_type[0], m_pData->insert_data.b_coin_type[1],
				m_pData->insert_data.b_coin_type[2], m_pData->insert_data.b_coin_type[3]);

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- 방출금 정보 | 10원:[%d], 50원:[%d], 100원:[%d], 500원:[%d]"),
				m_pData->changes_data.b_changes[0], m_pData->changes_data.b_changes[1],
				m_pData->changes_data.b_changes[2], m_pData->changes_data.b_changes[3]);
		}
		*/
		// 알람 입력
		m_pData->alarm_cnt = 0;
		ZeroMemory(m_pData->coin_alarm, sizeof(m_pData->coin_alarm));

		INT _alarm_cnt = map_alarm.GetCount(); // 알람 카운트
		if(_alarm_cnt > 0)
		{
			CString _strAlarm(_T(""));
			POSITION pos = map_alarm.GetStartPosition();
			while(pos != NULL)
			{
				map_alarm.GetNextAssoc(pos, _strAlarm, m_pData->coin_alarm[m_pData->alarm_cnt]);
				
				if(cmd != CMD_COIN_POLLING)
				{
					//m_log.SetContentsLog();
					//m_log.WriteLog(_T("- Device Alarm[%02d]: %s"), 
						(m_pData->alarm_cnt+1), m_pData->coin_alarm[m_pData->alarm_cnt].alarm_key);
				}
				m_pData->alarm_cnt++;
				_strAlarm = _T("");
			}//end while
		}

		// 명령 수행 완료 콜백 호출
		OperationComplete(m_pData);		

		if(cmd != CMD_COIN_POLLING)
		{
			//m_log.WriteLog(_T("| ResponseResultIn (-) |"));
			//m_log.WriteLog(_T("-- END -------------------------------------------------------------------"));
		}		
	}	
	_cs.Unlock();

	// 수행 완료 플래그 초기화
	SetOperationCompleteFlag(TRUE);
}

/*
 * Name: ResetRetryCnt
 * IN  : 
 * Out :
 * Desc: 통신 재시도 횟수 초기화
 */
VOID CConluxCoinModule::ResetRetryCnt()
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(m_pstPack){
			m_pstPack->retry_cnt = 0;
		}
	}
	_cs.Unlock();
}

/*
 * Name: GetCommand
 * IN  :
 * Out : INT (요청 명령 코드)
 * Desc: 요청 명령 코드 Get
 */
INT CConluxCoinModule::GetCommand()
{
	INT _cmd = 0;
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		if(m_pData)
			_cmd = m_pData->dev_cmd;
	}_cs.Unlock();

	return _cmd;
}

/*
 * Name: SetCommand
 * IN  : INT _cmd
 * Out :
 * Desc: 요청 명령 코드 설정
 */
VOID CConluxCoinModule::SetCommand(_In_ INT &_cmd)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(_cmd > 0)
		{
			if(m_pData){
				m_pData->cmd = m_pData->dev_cmd = _cmd;
			}
		}
		else
		{
			if(m_pData){
				//_cmd = m_pData->cmd = m_pData->dev_cmd = NOTIFY_COIN_STATUS;
				//_cmd = m_pData->cmd = m_pData->dev_cmd = CMD_COIN_POLLING;
			}
		}		
	}
	_cs.Unlock();
}

/*
 * Name: GetCoinCmd
 * IN  :
 * Out : BYTE(동전처리장치 명령)
 * Desc: 동전처리장치 명령 가져오기
 */
BYTE CConluxCoinModule::GetCoinCmd()
{
	BYTE _cmd = 0x00;
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(m_pstPack)
			_cmd = m_pstPack->coin_command;
	}
	_cs.Unlock();
	return _cmd;
}

/*
 * Name: SetCoinCmd
 * IN  : BYTE _cmd(동전처리장치 명령), BYTE _ctrl_type(제어데이터 값, 0:CREM OFF, 1:CREM ON, 2:Clear,...,)
 * Out :
 * Desc: 동전처리장치 명령 설정
 */
VOID CConluxCoinModule::SetCoinCmd(_In_ BYTE _cmd,  _In_opt_ BYTE _data_cmd /*= 0x00*/, _In_opt_ BYTE _ctrl_type/*=0xFF*/)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		if(m_pstPack)
		{
			m_pstPack->coin_command = _cmd;
			m_pstPack->data_cmd = _data_cmd;
			// 제어데이터 값 설정
			m_pstPack->ctrl_type = _ctrl_type;
		}
	}_cs.Unlock();
}

/*
 * Name: GetCoinCtrlCmd
 * IN  :
 * Out :
 * Desc: 코인체인저 제어데이터(DC=0) 데이터 리턴 
 */
BYTE CConluxCoinModule::GetCoinCtrlCmd()
{
	BYTE _ctrl_cmd = 0xFF;

	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(m_pstPack){
			_ctrl_cmd = m_pstPack->ctrl_type;
		}
	}
	_cs.Unlock();

	return _ctrl_cmd;
}

/*
 * Name: GetDataCmd
 * IN  :
 * Out :
 * Desc: 코인체인저 데이터 커멘드 리턴 
 */
BYTE CConluxCoinModule::GetDataCmd()
{
	BYTE _data_cmd = CCoinProtocol::CONLUX_DC_CONTROL;

	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(m_pstPack){
			_data_cmd = m_pstPack->data_cmd;
		}
	}
	_cs.Unlock();

	return _data_cmd;
}

/*
 * Name: SendCmd
 * IN  : INT _command, 수행 명령 코드
 * Out :
 * Desc: 인자 값에 따라 수행 처리를 요청하는 함수, Working에서 호출 됨
 */
VOID CConluxCoinModule::SendCmd(_In_ INT _command)
{
	if(m_protocol == NULL || m_pData == NULL) return;

	BOOL _connect   = FALSE;

	SetCommand(_command);

	CSingleLock _cs(&m_cs);

	// 패킷 데이터 구조체 초기화
	if(m_pstPack){
		memset(m_pstPack, 0x00, sizeof(ST_CONLUX_COM_INFO));
	}
		/*
	if(_command != CMD_COIN_POLLING)
	{
		//m_log.WriteLog(_T("///////////////////////////////////////////////////////////////// Start //"));
		//m_log.WriteLog(_T("| Main => Device | receive command:%d"), _command);
	}
	*/
	switch(_command)
	{
	/*
	case CMD_MAIN_INITILIZE:
		{
			ClearAuditData();
			SetResult(EXECUTE_SUCCESS);
			ResponseResultIn();
			return;
		}
	case CMD_CONNECT:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 통신 연결, cmd:%d"), _command);
			
			m_protocol->SetCOMPort(m_nComPortNo);
			m_protocol->CreateLog();

			_connect = m_protocol->Connect_Conlux();
			if(_connect == TRUE)
			{				
				::Sleep(1000);
				// 스탠바이 코멘드 전송
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_STANDBY);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_DISCONNECT:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 통신 종료, cmd:%d"), _command);
			m_protocol->Disconnect();
			break;
		}
	case CMD_STAFF_COIN_READY:
	case CMD_TEST_OPEN:
	case CMD_COIN_OPEN_SHUTTER:
		{
			m_bCoinReady = false;
			m_bTestMode = false;

			if(_command == CMD_STAFF_COIN_READY){
				m_bCoinReady = true;
			}else if(_command == CMD_TEST_OPEN){
				m_bTestMode = true;
			}else{
				m_bTestMode = false;
			}

			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 셧터 열기, cmd:%d"), _command);	

			ClearAuditData();

			m_bCloseShutter = FALSE;

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_TEST_CLOSE:
	case CMD_COIN_CLOSE_SHUTTER:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 셧터 닫기, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{	
				if(_command == CMD_TEST_CLOSE){
					m_bTestMode = true;
				}else{
					m_bTestMode = false;
				}

				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::CREM_OFF);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_TEST_CANCEL:
	case CMD_COIN_REJECT:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 거래 취소 금액 방출, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				if(_command == CMD_TEST_CANCEL){
					m_bTestMode = true;
				}else{
					m_bTestMode = false;
				}

				// 금액 반환 가능 플래그 저장
				m_bCointReturn = FALSE;
								
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::CREM_OFF);								
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_COIN_CHANGE_MONEY:
	case CMD_COIN_CHANGE_MONEY_ALL_STEP:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 금액 방출, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				// 임시 투입 금액 구조체 저장
				BYTE _change_cnt[4];
				memset(_change_cnt, 0x00, sizeof(_change_cnt));
				_cs.Lock();
				{							
					m_nTmpAmt = m_pData->changes_data.amount;
					m_pData->changes_data.amount = 0;
					CopyMemory(_change_cnt, m_pData->changes_data.b_changes, sizeof(m_pData->insert_data.b_coin_type));					
				}
				_cs.Unlock();

				m_bCointReturn = TRUE;
				
				SetChangesData(m_nTmpAmt);

				// 출력 지령 - 투입 금액 방출 명령
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_AMT_RETURN, 0x00);								
				_connect = CmdSendToCoin();			
			}
			break;
		}
	case CMD_TEST_TOTALEND:
	case CMD_COIN_TOTAL_END:
		{
			if(_command == CMD_TEST_TOTALEND){
				m_bTestMode = true;
			}else{
				m_bTestMode = false;
			}

			// 회계 데이터 저장
			AuditData();

			_cs.Lock();{
				m_nTmpAmt = 0;
				// 투입 금액 정보 초기화			
				memset(m_pData->insert_data.b_coin_type, 0x00, sizeof(m_pData->insert_data.b_coin_type));
				m_pData->insert_data.dw_amount = 0;
			}_cs.Unlock();

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::DC_CLEAR);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_COIN_CLOSE_TOTALEND:
		{
			//m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 셧터 닫기 전체 행정, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, 0x00, 0x00);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_COIN_BOX_EMPTY:
	case CMD_COIN_AUDITCLR:
		{
			// 보유 금액 초기화
			_cs.Lock();
			{
				//m_log.WriteLog(_T("| 동전함 정보 초기화 |"));
				memset(&m_pData->coin_holding, 0x00, sizeof(m_pData->coin_holding));
			}
			_cs.Unlock();

			AuditData();

			SetResult(EXECUTE_SUCCESS);

			ResponseResultIn();

			return;
		}
	case CMD_COIN_POLLING:
		{
			////m_log.WriteLog(_T("| Proxy => Module | 동전처리장치 상태(polling), cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_ALL);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	*/
	}//end switch

	if(_connect == FALSE)
	{	
		_cs.Lock();{
			if(m_pData){
				m_pData->device_open = FALSE;
			}
			AlarmOccurred(_T("FF"));
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 통신 상태 : 오류"));
		}_cs.Unlock();
		/*
		if(GetCommand() == CMD_COIN_POLLING){
			INT cmd = NOTIFY_COIN_STATUS;
			SetCommand(cmd);
		}
		*/
		SetResult(0);

		ResponseResultIn();
	}
}

/*
 * Name: SendCoinCmd
 * IN  : BOOL _bDataCmd(데이터 코멘트인지 판단하는 플래그), BYTE _cmd (동전 명령), BYTE _ctrl_cmd(코인체인저 제어데이터 명령)
 * Out : TRUE(정상 송신), FALSE(송신 실패)
 * Desc: 동전처리장치 패킷 송신 헬퍼 
 */
BOOL CConluxCoinModule::SendCoinCmd(_In_ const BOOL _bDataCmd, _In_opt_ BYTE _cmd, 
									_In_opt_ BYTE _data_cmd/* = 0x00*/, 
									_In_opt_ BYTE _ctrl_cmd/*=0xFF*/)
{
	CSingleLock _cs(&m_cs);
	BOOL _connect = TRUE;

	if(_bDataCmd == TRUE)
	{// 데이터 코멘드 송신
		INT	device_cmd = GetCommand();
		_connect = DataCmdSendToCoin(device_cmd);
	}
	else
	{// 
		if(_cmd > 0){
			SetCoinCmd(_cmd, _data_cmd, _ctrl_cmd);
		}
		_connect = CmdSendToCoin();
	}

	if(_connect == FALSE)
	{	
		_cs.Lock();{
			if(m_pData){
				m_pData->device_open = FALSE;
			}
			AlarmOccurred(_T("FF"));
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 데이터 통신 상태 : 오류"));
		}_cs.Unlock();
		/*
		if(GetCommand() == CMD_COIN_POLLING){
			INT cmd = NOTIFY_COIN_STATUS;
			SetCommand(cmd);
		}*/
	}

	return _connect;
}

/*
 * Name: SetOperationCompleteFlag
 * IN  : _In_ BOOL bComplete (TRUE: 완료, FALSE: 미완료)
 * Out :
 * Desc: 수행 완료 플래그 설정
 */
VOID CConluxCoinModule::SetOperationCompleteFlag(_In_ BOOL bComplete)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		if(m_pData)
			m_pData->operationcomplete = bComplete;
	}_cs.Unlock();
}

/*
 * Name: _threadEntry
 * IN  : LPVOID, 자기 객체 포인트
 * Out : 0, 스레드 종료
 * Desc: 스레드 시작 함수
 */
UINT CConluxCoinModule::_threadEntry(_In_ LPVOID pObj)
{
	CConluxCoinModule *pSelfObj = static_cast<CConluxCoinModule*>(pObj);
	pSelfObj->Working();
	return 0;
}

/*
 * Name: Working
 * IN  : 없음
 * Out : 없음
 * Desc: 워킹 스레드 함수
 */
VOID CConluxCoinModule::Working()
{
	DWORD _dwExit = 0;
	//INT _nCommand = CMD_COIN_NONE;
	BOOL _bOperComplete = TRUE;
	BOOL bConnect = FALSE;
	CSingleLock _cs(&m_cs);

	while(TRUE)
	{
		// 스레드 종료 이벤트 신호 대기
		_dwExit = ::WaitForSingleObject(m_hExitThread, 200L);
		if(_dwExit == WAIT_OBJECT_0){
			return;
		}
		
		// 수행 명령 완료 플래그 값 가져오기
		_bOperComplete = GetOperationCompleteFlag();
		if(_bOperComplete == TRUE && g_bExit == FALSE)
		{
			// 수행 명령 완료되었으면 다음 명령 가져오기
			//_nCommand = CommandOut(TRUE);

			// 수행 명령 및 완료 플래그 설정
			SetOperationCompleteFlag(FALSE);

			//SendCmd(_nCommand);
		}

		Sleep(500);
	}//end while
}

/*
 * Name: procCancel
 * IN  :
 * Out :
 * Desc: 취소 처리 
 */
INT CConluxCoinModule::procCancel()
{
	BOOL _bResult = 1;

	if( m_status.bit_data.CREMON > 0 || m_status.bit_data.CHANGE == 0)
	{
		ResetRetryCnt();
		_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
		if(_bResult == TRUE){
			return 3;
		}
	}

	if(m_status.bit_data.CHANGE > 0)
	{
		//m_log.WriteLog(_T("| 셧터 닫기 완료 | 동전 반출 명령 송신"));

		// 임시 투입 금액 구조체 저장
		BYTE _change_cnt[4] = {0x00};
		
		CSingleLock _cs(&m_cs);
		_cs.Lock();
		{							
			m_nTmpAmt = m_pData->insert_data.dw_amount;
			CopyMemory(_change_cnt, m_pData->insert_data.b_coin_type, sizeof(m_pData->insert_data.b_coin_type));
		}
		_cs.Unlock();
				
		if(m_nTmpAmt == 0)
		{
			// 투입 금액이 없는 경우 초기화 명령 송신
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 투입 금액 없음, 초기화 명령 송신"));
			
			ResetRetryCnt();			
			_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::DC_CLEAR);
			if(_bResult == TRUE)
			{	
				m_nCurCtrlCmd = CCoinProtocol::CONLUX_DC_CONTROL;								
				return 3;
			}
		}
		else
		{
			// 투입 금액이 없는 경우 초기화 명령 송신
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- 투입 금액:[%d(원)], 방출 명령 송신"), m_nTmpAmt);

			// 투입 금액을 방출 금액으로 설정
			SetChangesData(m_nTmpAmt);

			// 출력 지령 - 투입 금액 방출 명령
			ResetRetryCnt();
			_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_AMT_RETURN);
			if(_bResult == TRUE)
			{
				// 잔동 방출 진행 플래그 저장
				//m_bChangeComplete = FALSE;
				return 3;
			}
		}
	}

	return 0;			
}

/*
 * Name: procChange
 * IN  :
 * Out :
 * Desc: 방출 행정
 */
INT CConluxCoinModule::procChange()
{
	BOOL _bResult = 1;

	// 불출 종료 상태가 아닌 경우 상태 요청
	if( m_status.bit_data.CHANGE_COMPLETE == 0 )
	{
		ResetRetryCnt();
		_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
		if(_bResult == TRUE){
			return 3;
		}

		return 0;
	}

	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		if(m_nTmpAmt < 0){
			//m_log.WriteLog(_T("| 동전 과방출 발생(금액: %d(원)) |"), (m_nTmpAmt * -1));	
		}else if(m_nTmpAmt > 0){
			//m_log.WriteLog(_T("| 동전 미방출 발생(금액: %d(원)) |"), m_nTmpAmt);
		}else{									
			//m_log.WriteLog(_T("| 동전 방출 완료 |"));
		}
	}
	_cs.Unlock();				

	// 투입 금액이 없는 경우 초기화 명령 송신
	//m_log.WriteLog(_T("| 방출 완료 | 초기화 명령 송신"));	
	ResetRetryCnt();			
	_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::DC_CLEAR);
	if(_bResult == TRUE)
	{	
		m_nCurCtrlCmd = CCoinProtocol::CONLUX_DC_CONTROL;								
		return 3;
	}

	return 0;
}

/*
 * Name: procClear
 * IN  :
 * Out :
 * Desc: 클리어 행정
 */
INT CConluxCoinModule::procClear()
{
	BOOL _bResult = 1;

	// 불출 종료 상태가 아닌 경우 상태 요청
	if( m_status.bit_data.CLEAR_COMPLETE == 0 )
	{
		ResetRetryCnt();
		_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
		if(_bResult == TRUE){
			return 3;
		}

		_bResult = 0;
	}

	if(m_status.bit_data.CLEAR_COMPLETE == 1)
	{
		INT _cmd = GetCommand();
		/*
		if(_cmd == CMD_COIN_REJECT || _cmd == CMD_TEST_CANCEL)
		{
			// 동전 투입 수량 데이터 초기화
			memset(m_pData->insert_data.b_coin_type, 0x00, sizeof(m_pData->insert_data.b_coin_type));
			// 투입 금액 저장
			m_pData->insert_data.dw_amount = m_nTmpAmt;
		}*/
	}

	return _bResult;
}

/*
 * Name: procClose
 * IN  :
 * Out :
 * Desc: 셧터 닫기 처리 
 */
INT CConluxCoinModule::procClose()
{
	BOOL _bResult = 1;

	// 셧터 닫기고 잔돈 방출 가능 경우
	if(m_status.bit_data.CHANGE > 0 && m_status.bit_data.CREMON == 0)
	{
		//m_log.WriteLog(_T("| 셧터 닫기 완료 |"));
	}
	else
	{
		ResetRetryCnt();
		_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
		if(_bResult == TRUE){
			return 3;
		}

		_bResult = 0;
	}

	return _bResult;			
}

