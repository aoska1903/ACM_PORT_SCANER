#include "stdafx.h"
#include "ConluxCoinModule.h"
#include "CoinProtocol.h"

#define COIN_PROG_VER _T("2019030501")

// ���� �α� ��� ���� �÷���
BOOL g_bWritePollingLog = FALSE;

// ������ ���� ���� 
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
 * Out : BOOL (TRUE: ���� �Ϸ�, FALSE, �̿Ϸ�)
 * Desc: ��� ���� �Ϸ� �Ǵ� �÷��� �� ����
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
 * IN  : ����
 * Out : ����
 * Desc: ������, �޸𸮿� ��ü �� �ʱ�ȭ �Լ�
 */
VOID CConluxCoinModule::InitWork()
{	
	////m_log.SetContentsLog(); //m_log.WriteLog(_T("| CConluxCoinModule::InitWork (+) |"));

	m_pData = new T_COIN_DATA;
	ZeroMemory(m_pData, sizeof(T_COIN_DATA));
	m_pData->operationcomplete = TRUE;
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ����ó����ġ ���μ��� ������ ����ü ����"));

	m_pstPack = new ST_CONLUX_COM_INFO;
	::ZeroMemory(m_pstPack, sizeof(ST_CONLUX_COM_INFO));
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ����ó����ġ ��� ������ ����ü ����"));

	m_hExitThread = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ������ ���Ḧ ���� �̺�Ʈ ����"));

	m_protocol = new CCoinProtocol();	
	m_protocol->CreateRecvEvent();	
	m_protocol->RegCallbackFunc_RecvComp(std::bind(&CConluxCoinModule::ReponseAnalysis, this, 
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- �ø��� ��� ���� ��ü ����"));

	SetOperationCompleteFlag(TRUE);

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ����ó����ġ ���� ���� ���� ����"));		
	CreateAuditFolder();

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ����ó����ġ ���� ����"));
	T_COIN_HOLDING _tmpHolding;
	ZeroMemory(&_tmpHolding, sizeof(T_COIN_HOLDING));

	DWORD dwRet = ReadAudit(_T("COIN.audit"), &_tmpHolding, sizeof(T_COIN_HOLDING));
	if(dwRet == ERROR_SUCCESS){
		CopyMemory(&m_pData->coin_holding, &_tmpHolding, sizeof(T_COIN_HOLDING));
	}else{
		ZeroMemory(&m_pData->coin_holding, sizeof(T_COIN_HOLDING));
	}
	
	//m_log.SetContentsLog();
	//m_log.WriteLog(_T("\t- ���� �� ���� ���� | 10��(%d), 50��(%d), 100��(%d), 500��(%d)"),
		m_pData->coin_holding.wCoinBox[0],
		m_pData->coin_holding.wCoinBox[1],
		m_pData->coin_holding.wCoinBox[2],
		m_pData->coin_holding.wCoinBox[3]);

	ZeroMemory(m_bef_error_stat.data,sizeof(BYTE)*2);

	// ���� ����
	sprintf_s(m_pData->_ver, _T("%10.10s"), COIN_PROG_VER);

	// INI ���� ��� ����
	TCHAR  curFolderPath[MAX_PATH]={NULL};         
	TCHAR  iniFilePath[MAX_PATH]={NULL};
#ifdef _REMOTE
	_stprintf(curFolderPath, _T("%s"), _T("C:\\AFC"));
#else
	// ���� ��� ���� ��� ���� 
	GetCurrentDirectory( MAX_PATH, curFolderPath);		
#endif
	// afc_congif.ini
	_stprintf(iniFilePath, _T("%s\\Ini\\ParamVer.ini"), curFolderPath);				
	//m_ini_ver.SetPathName(iniFilePath);
	//m_ini_ver.WriteString("Version", "Coin_Version", m_pData->_ver);

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ����ó����ġ ����: %s"), m_pData->_ver);

	m_bCoinReady = false;

	m_pThread = AfxBeginThread(_threadEntry, this, 0, 0, CREATE_SUSPENDED);	
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();	
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("\t- ������ ����"));

	//m_log.SetContentsLog(); //m_log.WriteLog(_T("| CConluxCoinModule::InitWork (-) |"));
}

/*
 * Name: RecvDataAnalysis
 * IN  : INT(���� ���), BYTE *pData(����ó����ġ���� ���� ���� ������), INT(������ ����)
 * Out : ����
 * Desc: ���� ��ġ���� ���� ���� ��Ŷ �м� ó��
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
			////m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ ��� ���� : ����"));
			// ��� ���� �˶� ����
			AlarmOccurred(_T("FF"), TRUE);
		}

		// ���� ������ ���� ����
		CopyMemory(&m_pstPack->recv_buf[m_pstPack->data_size], pData, nLength);
		m_pstPack->data_size += nLength;

		// �ӽ� ���� ������ ����
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
		case ACK4:	// ���� �� ������ �ɸ� ���
			{
				// ���Ĺ��� ��� ����
				////m_log.WriteLog(_T("| RecvDataAnalysis | ����:ACK4, StandBy Command �۽�"));
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_STANDBY);
				_bConnect = CmdSendToCoin();
				if(_bConnect == FALSE)
				{	
					_cs.Lock();{
						if(m_pData){
							m_pData->device_open = FALSE;
						}
						////m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ ��� ���� : ����"));
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
 * IN  : ����
 * Out : ����
 * Desc: �޸�, ��ü ���� �� ���� �Լ�
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
	
	////m_log.SetContentsLog(); //m_log.WriteLog(_T("- Thread ����"));
		
	//SAFE_DELETE(m_protocol);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ��� ���� ��ü ����"));
	
	//SAFE_DELETE(m_pData);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ó����ġ ���μ��� ������ ����ü ����"));

	//SAFE_DELETE(m_pstPack);
	//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ó����ġ ������ ��ü ����"));

	//m_log.WriteLog(_T("| ReleaseWork (-) |"));
}

/*
 * Name: SetChangesData
 * IN  : const T_COIN_CHANGES *pChanges(���� ���� ���� ������ ����ü)
 * Out :
 * Desc: ���� ���� ���� ������ ���� 
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
		//m_log.WriteLog(_T("| ���� ���� ���� ���� | �Ž��� �ݾ�: [%d]"), pChanges);			

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
		//m_log.WriteLog(_T("| ���� ���� ���� ���� | [0]:%d, [1]:%d, [2]:%d, [3]:%d"),
			m_changes.b_changes[0], m_changes.b_changes[1], m_changes.b_changes[2], m_changes.b_changes[3]);		

	}
	_cs.Unlock();
}

/*
 * Name: SetFileLog
 * IN  :
 * Out :
 * Desc: ���� �α� ����
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
 * IN  : const INT _ret, ���� ��� ��
 * Out :
 * Desc: ���� ��� �� ����
 */
VOID CConluxCoinModule::SetResult(_In_ const INT _ret)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock(); m_pData->n_result = _ret; _cs.Unlock();
}

/*
 * Name: TimeoutTimerStart
 * IN  : INT nInterval (�̺�Ʈ ȣ�� �ð� ����, ms), �⺻ 1��
 * Out :
 * Desc: Ÿ�Ӿƿ� ������ ���� Ÿ�̸� ����
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
 * Desc: Ÿ�Ӿƿ� ������ ���� Ÿ�̸� ����
 */
VOID CConluxCoinModule::TimeoutTimerStop()
{
	//m_tm_timeout.Stop();
}

/*
 * Name: TimeoutEvent
 * IN  :
 * Out :
 * Desc: Ÿ�Ӿƿ� �߻� �̺�Ʈ
 */
VOID CConluxCoinModule::TimeoutEvent()
{
	//m_tm_timeout.Stop();

	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		////m_log.SetContentsLog(); //m_log.WriteLog(_T("- Ÿ�� �ƿ�(Time Out) �߻�"));
	}_cs.Unlock();

	RetryCOM();
}

/*
 * Name: ShutterCloseTimerStart
 * IN  : INT nInterval (�̺�Ʈ ȣ�� �ð� ����, ms), �⺻ 1��
 * Out :
 * Desc: Ÿ�Ӿƿ� ������ ���� Ÿ�̸� ����
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
 * Desc: Ÿ�Ӿƿ� ������ ���� Ÿ�̸� ����
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
 * Desc: Ÿ�Ӿƿ� �߻� �̺�Ʈ
 */
VOID CConluxCoinModule::ShutterCloseEvent()
{
	//m_tm_close.Stop();

	////m_log.WriteLog(_T("| ���� �ݱ� �Ϸ� |"));

	if(m_bCloseShutter == TRUE)
	{
		//SetResult(EXECUTE_SUCCESS);
		ResponseResultIn();
	}
}

/*
 * Name: TestModeLog
 * IN  : _In_ HWND _hwnd (�θ� �ڵ�), _In_opt_ BOOL _bTestMode (�׽�Ʈ �α� ��� ����)
 * Out :
 * Desc: �׽�Ʈ ��� �α� ���� - ��ȭ���ڿ� �α� ���
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
 * IN  : TCHAR* pCode(�˶� �ڵ�), BOOL bRelease(TRUE:����, FALSE:�߻�)
 * Out : 
 * Desc: ����ó����ġ�� �˶� �߻� �� ����
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
			//m_log.WriteLog(_T("| �˶� �߻� | Code:%s"), skey);
			
			// �˶� Ű(ex. C01,...)
			sprintf_s(alarm.alarm_key, _T("%s"), skey);
			// �˶� ��� �ڵ�
			alarm.modulecode = 'C';
			// �˶� �ڵ�(ex. 01,...)
			sprintf_s(alarm.alarmcode, _T("%s"), pCode);
			
			map_alarm.SetAt(skey, alarm);
		}
	}
	else
	{
		if(bRelease == TRUE)
		{
			//m_log.WriteLog(_T("| �˶� ���� | Code:%s"), skey);
			map_alarm.RemoveKey(skey);
		}
	}
}

/*
 * Name: AuditData
 * IN  : ����
 * Out : ����
 * Desc: ȸ�� ������ ����, �б�, �����
 */
VOID CConluxCoinModule::AuditData()
{
	if(m_bTestMode){
		return;
	}

	CSingleLock _cs(&m_cs);
	// ȸ�� ���
	T_COIN_HOLDING _tmpHolding;
	ZeroMemory(&_tmpHolding, sizeof(T_COIN_HOLDING));

	_cs.Lock();
	{
		//m_log.WriteLog(_T("| ȸ�� ������ ���� |"));

		CopyMemory(&_tmpHolding, &m_pData->coin_holding, sizeof(T_COIN_HOLDING));						
		
		//m_log.WriteLog(_T("| ���� �� ���� ���� (+) |"));
		//m_log.SetContentsLog(); 
		//m_log.WriteLog(_T("- ���� ���� : 10��(%d), 50��(%d), 100��(%d), 500��(%d)"),
			_tmpHolding.wCoinBox[0],
			_tmpHolding.wCoinBox[1],
			_tmpHolding.wCoinBox[2],
			_tmpHolding.wCoinBox[3]);

		//m_log.WriteLog(_T("| ���� �� ���� ���� (-) |"));
	}
	_cs.Unlock();

	WriteAduit(_T("COIN.audit"), &_tmpHolding, sizeof(T_COIN_HOLDING));
}

/*
 * Name: ClearAuditData
 * IN  : ����
 * Out : ����
 * Desc: ���� ������ �� ����, ���� �� ������ �ʱ�ȭ 
 */
VOID CConluxCoinModule::ClearAuditData()
{
	CSingleLock _cs(&m_cs);
		
	_cs.Lock();
	{
		// ���� ���� ������ �ʱ�ȭ
		m_nTmpAmt = 0;
		ZeroMemory(&m_pData->insert_data, sizeof(T_COIN_INSERT));

		// ���� ���� ������ �ʱ�ȭ
		m_pData->change_amount = 0;
		ZeroMemory(&m_pData->changes_data, sizeof(T_COIN_CHANGES));

		ZeroMemory(&m_changes, sizeof(T_COIN_CHANGES));

		//m_log.WriteLog(_T("| ���� �� ���� ���� ������ �ʱ�ȭ �Ϸ� |"));
	}
	_cs.Unlock();
}

/*
 * Name: CmdSendToCoin
 * IN  : 
 * Out : �۽� ���� ���(TRUE: ����, FALSE: ����)
 * Desc: ����ó����ġ�� ��� �۽�
 */
BOOL CConluxCoinModule::CmdSendToCoin()
{
	TimeoutTimerStop();

	CSingleLock _cs(&m_cs);

	BOOL _bConnect = FALSE;
	BYTE _cmd = 0x00;
	
	if(m_protocol == NULL)
	{
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �޸𸮿� �������� ��� ��ü�� �Ҵ���� �ʾ���(m_protocol = NULL)"));
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

	// ����ó����ġ ��� ��������
	_cmd = GetCoinCmd();

	if(_cmd != 0x61){
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- Module => Protocol | �ڸ�� �۽�: %02X"), _cmd);
	}
	
	for(INT i=0; i<3; i++)
	{
		_bConnect = m_protocol->SendCmdToConlux(_cmd);
		if(_bConnect == FALSE)
		{
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �ڸ�� �۽� ����, �õ� ī��Ʈ: %d"), (i+1));			
		}
		else
		{
			// Ÿ�Ӿƿ� ���� Ÿ�̸� ����
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
 * Out : �۽� ���� ���(TRUE: ����, FALSE: ����)
 * Desc: ����ó����ġ�� ������ ��� �۽�
 */
BOOL CConluxCoinModule::DataCmdSendToCoin(_In_ const INT _coin_cmd)
{
	TimeoutTimerStop();

	CSingleLock _cs(&m_cs);

	BOOL _bConnect = FALSE;
	BYTE _cmd = 0x00;
	
	if(m_protocol == NULL)
	{
		//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �޸𸮿� �������� ��� ��ü�� �Ҵ���� �ʾ���(m_protocol = NULL)"));
		return _bConnect;
	}
	
	// ���� ���� �ʱ�ȭ
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
			// BC (DC ~ DATA ���� ũ��)
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
			// BC (DC ~ DATA ���� ũ��)
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
			if(data_cmd == CCoinProtocol::CONLUX_DC_AMT_RETURN) // ���� ������ ����
			{
				// ���� �ݾ� ���� ������ ����
				//ConluxSendDataSize = 7;
				ConluxSendDataSize = 6;
				// BC
				_btData[0] = 0x04;
				// DC
				_btData[1] = CCoinProtocol::CONLUX_DC_AMT_RETURN;
				// DATA			
				_btData[2] = (m_changes.b_changes[0]<<4);					        	 // 10�� �ݱݸż�(BCD)
				_btData[3] = (m_changes.b_changes[2]<<4) | (m_changes.b_changes[1]);    // 1000��, 100�� �ݱݸż�(BCD)
				_btData[4] = (m_changes.b_changes[3]);						         // 10000�� �ݱݸż�(BCD)				
			}
			else
			{	// ���� �ݱ� ���� ó���� ��� �Ǵ� ���� ���Ա�, ����� ������ Ŭ����

				ConluxSendDataSize = 4;
				// BC (DC ~ DATA ���� ũ��)
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
			// BC (DC ~ DATA ���� ũ��)
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

	strLog = _T("- Module => Protocol | �ڸ�� ������ �۽�: ");
	for(INT loop=0; loop<ConluxSendDataSize; loop++){
		strLog.AppendFormat(_T("[%02X]"), _btData[loop]);
	}
	//m_log.SetContentsLog(); //m_log.WriteLog(strLog);			

	for(INT i=0; i<3; i++)
	{
		_bConnect = m_protocol->SendDataToConlux(_btData, ConluxSendDataSize);
		if(_bConnect == FALSE)
		{
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ �ڸ�� �۽� ����, �õ� ī��Ʈ: %d"), (i+1));			
		}
		else
		{
			// Ÿ�Ӿƿ� ���� Ÿ�̸� ����
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
 * Out : true(������ �ڸ�Ʈ �۽�), false(������ �ڸ�Ʈ �۽� ����)
 * Desc: ����ü���� ������ �ڸ�� �۽� ���� ����
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
 * Desc: ����ó����ġ �ϰ��䱸 ���� ������ �м� ó��
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
		_dataSize -= 3; // ���� ������ ũ�� (ACK, BC, FCC ����) 
	}_cs.Unlock();

	INT _InCnt[4] = {0}; // ���� �ż�		
	INT _OutCnt[4]= {0}; // ���� �ż�	
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
			// �� ���� ����
		case CCoinProtocol::CONLUX_DC_INSERT:
			{	
				// ���� �ݾ��� ���� ��� ��������
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
						// �� ���� ���� ����
						_InCnt[_index] = bcd2dec(_recvData[_index]) - m_pData->insert_data.b_coin_type[_index];
						if(_InCnt[_index] > 0){							
							//m_log.WriteLog(_T("| ���� �ݾ�: [%d(��)] |"), (_InCnt[_index]*_coin_type[_index]));
						}						
						// �� ���� ����
						m_pData->insert_data.b_coin_type[_index] = bcd2dec(_recvData[_index]); 
					}//end for
				}				
				_cs.Unlock();				
				break;
			}
			// �� �ܵ� �ż�
		case CCoinProtocol::CONLUX_DC_RETURN:
			{
				// ���� �ݾ��� ���� ��� ��������
				if(bcd2dec(_recvData[0]) == 0 && bcd2dec(_recvData[1]) == 0 && bcd2dec(_recvData[2]) == 0 && bcd2dec(_recvData[3]) == 0){
					break;
				}
				_cs.Lock();
				{	
					for(INT i=0; i<4; i++)
					{
						// �� ���� ����
						_OutCnt[i] = bcd2dec(_recvData[i]) - m_pData->changes_data.b_changes[i];
						if(_OutCnt[i] > 0){
							//m_log.WriteLog(_T("| ���� �ݾ�: %d(��) |"), (_OutCnt[i]*_coin_type[i]));
						}		
						//���� ����
						m_pData->changes_data.b_changes[i] = bcd2dec(_recvData[i]); 
					}
				}								
				_cs.Unlock();				
				break;
			}
			// ����ó����ġ ����
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

				//m_log.WriteLog(_T("| ����ó����ġ ���� ���� (+) |"));

				if(m_status.bit_data.CREMON > 0)
				{
					m_bCloseShutter = FALSE;
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- CREM(���� ���� ���� ����): %u"), m_status.bit_data.CREMON);					
				}
				else
				{
					m_bCloseShutter = TRUE;
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- CREM(���� ���� �Ұ� ����): %u"), m_status.bit_data.CREMON);					
				}
				if(m_status.bit_data.INVENTORY > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �κ��丮 ���� ����: %u"), m_status.bit_data.INVENTORY);					
				}
				if(m_status.bit_data.CHANGE > 0)
				{					
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �ܵ� ���� ���� ����: %u"), m_status.bit_data.CHANGE);
				}
				if(m_status.bit_data.RETURN_SWON > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ��ȯ ����ġ ����: %u"), m_status.bit_data.RETURN_SWON);
				}
				if(m_status.bit_data.CHANGE_COMPLETE > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���� ���� ����: %u"), m_status.bit_data.CHANGE_COMPLETE);
				}
				if(m_status.bit_data.CLEAR_COMPLETE > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- Ŭ���� �Ϸ� ����: %u"), m_status.bit_data.CLEAR_COMPLETE);
				}
				if(m_status.bit_data.INVENTORY_STAT > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �κ��丮 ���� ���� ����: %u"), m_status.bit_data.INVENTORY_STAT);
				}				

				//m_log.WriteLog(_T("| ����ó����ġ ���� ���� (-) |"));

				break;
			}
			// ����ó����ġ �̻�
		case CCoinProtocol::CONLUX_DC_ERROR:
			{
				memset(m_error_stat.data, 0x00, sizeof(m_error_stat.data));
				CopyMemory(m_error_stat.data, _recvData, sizeof(m_error_stat.data));
				
				if(m_bef_error_stat.data[0] == m_error_stat.data[0] && m_bef_error_stat.data[1] == m_error_stat.data[1]){
					break;
				}

				CopyMemory(m_bef_error_stat.data, m_error_stat.data, sizeof(m_bef_error_stat.data));

				// Ʃ�� Ż��
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

				//m_log.WriteLog(_T("| ����ó����ġ �̻� ���� (+) |"));
				if(m_error_stat.bit_data.COINCHANGER > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ü���� �̻� ����: %u"), m_error_stat.bit_data.COINCHANGER);									
				}

				if(m_error_stat.bit_data.WORKABLE > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���۰� ����: %u"), m_error_stat.bit_data.WORKABLE);					
				}

				if(m_error_stat.bit_data.ACCEPTOR > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- �Ǽ��� �̻�: %u"), m_error_stat.bit_data.ACCEPTOR);
					AlarmOccurred(_T("110"));
				}else{
					AlarmOccurred(_T("110"), TRUE);
				}

				if(m_error_stat.bit_data.EMPTYSW_10 > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����Ƽ ����ġ: %u"), m_error_stat.bit_data.EMPTYSW_10);
					AlarmOccurred(_T("111"));
				}else{
					AlarmOccurred(_T("111"), TRUE);
				}

				if(m_error_stat.bit_data.RETURN_SW > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ��ȯ ����ġ �̻�: %u"), m_error_stat.bit_data.RETURN_SW);
					AlarmOccurred(_T("112"));
				}else{
					AlarmOccurred(_T("112"), TRUE);
				}

				if(m_error_stat.bit_data.COIN_RETURN_ERROR > 0){
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ��ȭ ���� �ҷ�: %u"), m_error_stat.bit_data.COIN_RETURN_ERROR);
					AlarmOccurred(_T("113"));
				}else{
					AlarmOccurred(_T("113"), TRUE);
				}

				if(m_error_stat.bit_data.SAFTY_SW > 0)
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����Ƽ����ġ(����Ʃ�� Ż��) �̻�: %u"), m_error_stat.bit_data.SAFTY_SW);
					AlarmOccurred(_T("114"));					
				}
				else
				{
					//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����Ƽ����ġ(����Ʃ�� ����)"));
					AlarmOccurred(_T("114"), TRUE);
				}

				//m_log.WriteLog(_T("| ����ó����ġ �̻� ���� (-) |"));
				break;
			}
		}
	} while (_index < _dataSize);

	// ���� ������ �ִ� ��� 
	if( _InCnt[0] > 0 || _InCnt[1] > 0 || _InCnt[2] > 0 || _InCnt[3] > 0 )
	{
		INT _cmd = GetCommand();

		// ���� �غ� ���¸� ���� ���� ���
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
				//m_log.WriteLog(_T("| ���� ���� | ���� ���� ������Ʈ"));
			}else{
				//m_log.WriteLog(_T("| ���� ���� | ���� �ݱ� �� ����, ���� ���� ������Ʈ"));
			}
		}

		_cs.Lock();
		{
			//m_log.WriteLog(_T("| ���� ���� (+) |"));

			// ���� �ݾ�
			m_pData->insert_data.dw_amount = 
				m_pData->insert_data.b_coin_type[0] * 10  +
				m_pData->insert_data.b_coin_type[1] * 50  +
				m_pData->insert_data.b_coin_type[2] * 100 +
				m_pData->insert_data.b_coin_type[3] * 500;

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- �� ���� �ݾ�: %d(��), ����: 10��(%d), 50��(%d), 100��(%d), 500��(%d)"),
				m_pData->insert_data.dw_amount,
				m_pData->insert_data.b_coin_type[0],
				m_pData->insert_data.b_coin_type[1],
				m_pData->insert_data.b_coin_type[2],
				m_pData->insert_data.b_coin_type[3]);

			//m_log.WriteLog(_T("| ���� ���� (-) |"));

			// ������ ����
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

	// ���� ���� ��� 
	if( _OutCnt[0] > 0 || _OutCnt[1] > 0 || _OutCnt[2] > 0 || _OutCnt[3] > 0 )
	{
		// ������ ���� ����
		_cs.Lock();
		{
			//m_log.WriteLog(_T("| ���� ���� (+) |"));
			// ���� �ݾ�
			m_pData->change_amount = m_pData->changes_data.amount = 
				m_pData->changes_data.b_changes[0] * 10  +
				m_pData->changes_data.b_changes[1] * 50  +
				m_pData->changes_data.b_changes[2] * 100 +
				m_pData->changes_data.b_changes[3] * 500;

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- �� ���� �ݾ�: %d(��), ����: 10��(%d), 50��(%d), 100��(%d), 500��(%d)"),
				m_pData->change_amount,
				m_pData->changes_data.b_changes[0],
				m_pData->changes_data.b_changes[1],
				m_pData->changes_data.b_changes[2],
				m_pData->changes_data.b_changes[3]);

			//m_log.WriteLog(_T("| ���� ���� (-) |"));

			//m_log.WriteLog(_T("| ���� ���� | ���� ���� ������Ʈ"));
			for(INT i=0; i<4; i++)
			{
				if(m_bTestMode){
					break;
				}

				// �� ���� ������ ����
				if(m_pData->coin_holding.wCoinBox[i] > _OutCnt[i]){
					m_pData->coin_holding.wCoinBox[i] -= _OutCnt[i];
				}else{
					m_pData->coin_holding.wCoinBox[i] = 0;
				}

				// �� ���� ���� �� �������� ���� ������ ����.
				m_nTmpAmt -= (_OutCnt[i] * _coin_type[i]);							
			}
		}
		_cs.Unlock();

		AuditData();
	}

	// ��ȯ �� ���� �߻� ���
	if(m_error_stat.bit_data.COINCHANGER > 0 && m_error_stat.bit_data.COIN_RETURN_ERROR > 0){
		//m_log.WriteLog("| ���� ���� �̻� | ���:[%d]", EXECUTE_FAIL);
		_result = EXECUTE_FAIL;
	}

/*
	// ���� ���� ������ ���
	if( (m_status.bit_data.CHANGE_COMPLETE == 1 && m_bChangeComplete == FALSE ) || 
		(m_status.bit_data.CLEAR_COMPLETE == 1 && m_bCointReturn == FALSE) )
	{		
		m_bChangeComplete = TRUE;
	}

	// ����ü���� ���� �̻� && ��ȭ ���� �ҷ�
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
 * Desc: ���� ������ �м� ó��, ReponseAnalysis���� ȣ��
 */
VOID CConluxCoinModule::RecvDataAnalysis(_In_ const BYTE *pData, _In_ const INT nSize)
{
	CSingleLock _cs(&m_cs);

	CString strLog(_T(""));
	BOOL _bResult = TRUE;	
	INT _result = 1;
	INT _cmd = 0;			// ���� ���μ��� ���
	BYTE _coin_cmd = 0x00;	// ���� ��ġ ���
	BYTE _fcc = 0x00;		// ������ üũ �ڵ�
	BYTE _ctrl_cmd = 0xFF;  // ����ü���� �������(DC=0) ������(ex CREM ON, CREM OFF,...) 
							// CCoinProtocol.h ���Ͽ��� CONLUX_COIN_CTRL_CMD ����
	BYTE _data_cmd = 0x00;

	// ���� ���μ��� ��� ����
	_cmd = GetCommand();

	// ���� ��ġ ��� ����
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
	case CCoinProtocol::CONLUX_CMD_ORDER_ALL:	// �ϰ� �䱸
		
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
			{// ������ ��� ���� �Ϸ�				
				TimeoutTimerStop();

				// �α� ���
				if(_cmd != CMD_COIN_POLLING)
				{
					strLog = (_T("- ���� ������: "));
					for(INT i=0; i<nSize; i++){
						strLog.AppendFormat(_T("[%02X]"), pData[i]);
					}
					//m_log.SetContentsLog(); //m_log.WriteLog(strLog);
				}

				// FCC �˻�
				_fcc = m_protocol->GetLRC(pData+1, nSize-2);
				if(_fcc != pData[nSize-1])
				{
					//m_log.WriteLog(_T("- FCC ���� ������ (�۽� �� FCC: %02X, ��� �� FCC: %02X"), pData[nSize-1], _fcc);
					_result = EXECUTE_FAIL;
				}
				else
				{
					// ���� ��Ŷ �м�
					_result = StatusPacketAnalysis();
					if(_result == EXECUTE_FAIL)
					{
						// �ʱ�ȭ ��� �۽��� �Ѵ�.
						if( _cmd == CMD_COIN_REJECT					|| _cmd == CMD_TEST_CANCEL			||
							_cmd == CMD_COIN_CHANGE_MONEY_ALL_STEP	|| _cmd == CMD_COIN_CHANGE_MONEY	)
						{
							// ���� ���� ��� �ʱ�ȭ ��� �۽�	
							//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���� ����, �ʱ�ȭ ��� �۽�"));	
							ResetRetryCnt();			
							_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::DC_CLEAR);
							if(_bResult == TRUE){	
								m_nCurCtrlCmd = CCoinProtocol::CONLUX_DC_CONTROL;								
								//return;
							}
						}

						break;
					}

					// �ŷ� ��� ���� ó��
					if(_cmd == CMD_COIN_REJECT || _cmd == CMD_TEST_CANCEL)
					{						
						if(m_nCurCtrlCmd == CCoinProtocol::CREM_OFF){
							// �ŷ� ��� ���� ��� ����			
							_result = procCancel();
						}else if(m_nCurCtrlCmd == CCoinProtocol::COIN_RETURN){
							// ���� ���� ���� ��� ����			
							_result = procChange();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// Ŭ���� ���� ��� ����			
							_result = procClear();						
						}
												
						if(_result == EXECUTE_ING){
							return;
						}
					}//end if
					else if(_cmd == CMD_COIN_CLOSE_SHUTTER || _cmd == CMD_COIN_CLOSE_TOTALEND || _cmd == CMD_COIN_TOTAL_END)
					{
						if(m_nCurCtrlCmd == CCoinProtocol::CREM_OFF){
							// ���� �ݱ� ���� ��� ����			
							_result = procClose();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// Ŭ���� ���� ��� ����			
							_result = procClear();						
						}

						if(_result == EXECUTE_ING){
							return;
						}
					}
					else if(_cmd == CMD_COIN_CHANGE_MONEY_ALL_STEP || _cmd == CMD_COIN_CHANGE_MONEY)
					{
						if(m_nCurCtrlCmd == CCoinProtocol::COIN_RETURN){
							// ���� ���� ���� ��� ����			
							_result = procChange();
						}else if(m_nCurCtrlCmd == CCoinProtocol::DC_CLEAR){
							// Ŭ���� ���� ��� ����			
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
	case CCoinProtocol::CONLUX_CMD_ORDER_OUT:	// ��� ����
	case CCoinProtocol::CONLUX_CMD_REORDER_OUT: // ��� �� ����
		{
			/*
			// ������ Ŀ��� ���� �Ǵ� �÷��� ����
			bool _IsDataSend = IsDataSend();
			
			// ������ Ŀ��� ���� ��� �� ���� ����
			if(_IsDataSend)	
			{
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ü���� ������ Ŀ��� ���� ����"));

				//////////////////////////////////////////////////////////////////////////
				// ���� �ݱ� ��û, �ݾ� ���� ��� ȸ�� ���Ͽ� ������ ����

				if(_cmd == CMD_COIN_CLOSE_SHUTTER || _cmd == CMD_COIN_CLOSE_TOTALEND || _cmd == CMD_COIN_TOTAL_END)
				{
					m_nCurCtrlCmd = GetCoinCtrlCmd();

					// ���� ���� �б�
					_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
					if(_bResult == TRUE){
						return;
					}

					_result = EXECUTE_FAIL;					
					break;
				}

				//////////////////////////////////////////////////////////////////////////
				// �Ž��� �ݾ� ����
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

						// ���� ���� �б�
						_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
						if(_bResult == TRUE){
							return;
						}

						_result = EXECUTE_FAIL;		
					}
					else
					{
						//m_log.WriteLog(_T("| �̻� | �ش� ������ �ʿ� ���� ������ Ŀ��� [%02X]"), _data_cmd);
						_result = EXECUTE_FAIL;
					}
				}

				//////////////////////////////////////////////////////////////////////////
				// �ŷ� ���
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

						// ���� ���� �б�
						_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_ALL);
						if(_bResult == TRUE){
							return;
						}else{
							_result = EXECUTE_FAIL;
						}
					}
					else
					{
						//m_log.WriteLog(_T("| �̻� | ��� ������ �ʿ� ���� ������ Ŀ���: [%02X]"), _data_cmd);
						_result = EXECUTE_FAIL;
					}
				}
				break;
			}			
			else           
			{// ������ Ŀ��� �۽�
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ü���� Ŀ��� ��� ���� �Ϸ�"));

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
 * IN  : BOOL _bNotConnectError (��� ������ �ƴ� ������ �̻����� ���������� �Ǵ��ϴ� �÷���, TRUE: ��� ����, FALSE: ���� ��� ����)
 * Out : 
 * Desc: ��� ������ ��õ� 
 */
VOID CConluxCoinModule::RetryCOM(_In_opt_ BOOL _bNotConnectError/*=FALSE*/)
{
	CSingleLock _cs(&m_cs);

	BOOL _bConnect = TRUE;

	bool _IsDataSend = false;

	_cs.Lock();
	{
		// ����ü���� ������ ���� �÷��� ����
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

		// ����ü������ ������ ���� �� ACK1 ������ ���� ���
		if(_coin_cmd == CCoinProtocol::CONLUX_CMD_ORDER_OUT && _IsDataSend)
		{
			BYTE _data_cmd, _ctrl_cmd = 0xFF;
			_ctrl_cmd = GetCoinCtrlCmd();
			_data_cmd = GetDataCmd();
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ��� ������ �ڸ�� ����"));
			SetCoinCmd(CCoinProtocol::CONLUX_CMD_REORDER_OUT, _data_cmd, _ctrl_cmd);
		}

		_bConnect = CmdSendToCoin();
	}
	
	if(_bConnect == FALSE)
	{
		// NAK �� ��� �� ���� ������ �̻��� 3���� ���
		// ��� ���� ������ �ƴϹǷ� ��� ���� ���з� ���� 
		if(_bNotConnectError == FALSE)
		{
			_cs.Lock();{
				if(m_pData){
					m_pData->device_open = FALSE;
				}
				AlarmOccurred(_T("FF"));
				//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ ��� ���� : ����"));
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
 * IN  : ����
 * Out : ����
 * Desc: ��û ��� ���� ��� ������ ����
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
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ó����ġ ��� ���� ���� [%d] (1:����, 0:����)"), m_pData->device_open); 
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���� ���� ��� �ڵ� [%d]"), m_pData->cmd);
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ����ó����ġ ���� �ڵ� [%d], ���� ��� �� [%d] (1:����, 0:����)"), 
				m_pData->dev_cmd, m_pData->n_result); 

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- ���Ա� ���� | 10��:[%d], 50��:[%d], 100��:[%d], 500��:[%d]"), 
				m_pData->insert_data.b_coin_type[0], m_pData->insert_data.b_coin_type[1],
				m_pData->insert_data.b_coin_type[2], m_pData->insert_data.b_coin_type[3]);

			//m_log.SetContentsLog();
			//m_log.WriteLog(_T("- ����� ���� | 10��:[%d], 50��:[%d], 100��:[%d], 500��:[%d]"),
				m_pData->changes_data.b_changes[0], m_pData->changes_data.b_changes[1],
				m_pData->changes_data.b_changes[2], m_pData->changes_data.b_changes[3]);
		}
		*/
		// �˶� �Է�
		m_pData->alarm_cnt = 0;
		ZeroMemory(m_pData->coin_alarm, sizeof(m_pData->coin_alarm));

		INT _alarm_cnt = map_alarm.GetCount(); // �˶� ī��Ʈ
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

		// ��� ���� �Ϸ� �ݹ� ȣ��
		OperationComplete(m_pData);		

		if(cmd != CMD_COIN_POLLING)
		{
			//m_log.WriteLog(_T("| ResponseResultIn (-) |"));
			//m_log.WriteLog(_T("-- END -------------------------------------------------------------------"));
		}		
	}	
	_cs.Unlock();

	// ���� �Ϸ� �÷��� �ʱ�ȭ
	SetOperationCompleteFlag(TRUE);
}

/*
 * Name: ResetRetryCnt
 * IN  : 
 * Out :
 * Desc: ��� ��õ� Ƚ�� �ʱ�ȭ
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
 * Out : INT (��û ��� �ڵ�)
 * Desc: ��û ��� �ڵ� Get
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
 * Desc: ��û ��� �ڵ� ����
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
 * Out : BYTE(����ó����ġ ���)
 * Desc: ����ó����ġ ��� ��������
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
 * IN  : BYTE _cmd(����ó����ġ ���), BYTE _ctrl_type(������� ��, 0:CREM OFF, 1:CREM ON, 2:Clear,...,)
 * Out :
 * Desc: ����ó����ġ ��� ����
 */
VOID CConluxCoinModule::SetCoinCmd(_In_ BYTE _cmd,  _In_opt_ BYTE _data_cmd /*= 0x00*/, _In_opt_ BYTE _ctrl_type/*=0xFF*/)
{
	CSingleLock _cs(&m_cs);
	_cs.Lock();{
		if(m_pstPack)
		{
			m_pstPack->coin_command = _cmd;
			m_pstPack->data_cmd = _data_cmd;
			// ������� �� ����
			m_pstPack->ctrl_type = _ctrl_type;
		}
	}_cs.Unlock();
}

/*
 * Name: GetCoinCtrlCmd
 * IN  :
 * Out :
 * Desc: ����ü���� �������(DC=0) ������ ���� 
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
 * Desc: ����ü���� ������ Ŀ��� ���� 
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
 * IN  : INT _command, ���� ��� �ڵ�
 * Out :
 * Desc: ���� ���� ���� ���� ó���� ��û�ϴ� �Լ�, Working���� ȣ�� ��
 */
VOID CConluxCoinModule::SendCmd(_In_ INT _command)
{
	if(m_protocol == NULL || m_pData == NULL) return;

	BOOL _connect   = FALSE;

	SetCommand(_command);

	CSingleLock _cs(&m_cs);

	// ��Ŷ ������ ����ü �ʱ�ȭ
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
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ��� ����, cmd:%d"), _command);
			
			m_protocol->SetCOMPort(m_nComPortNo);
			m_protocol->CreateLog();

			_connect = m_protocol->Connect_Conlux();
			if(_connect == TRUE)
			{				
				::Sleep(1000);
				// ���Ĺ��� �ڸ�� ����
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_STANDBY);				
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_DISCONNECT:
		{
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ��� ����, cmd:%d"), _command);
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

			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ���� ����, cmd:%d"), _command);	

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
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ���� �ݱ�, cmd:%d"), _command);	

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
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ �ŷ� ��� �ݾ� ����, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				if(_command == CMD_TEST_CANCEL){
					m_bTestMode = true;
				}else{
					m_bTestMode = false;
				}

				// �ݾ� ��ȯ ���� �÷��� ����
				m_bCointReturn = FALSE;
								
				SetCoinCmd(CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_CONTROL, CCoinProtocol::CREM_OFF);								
				_connect = CmdSendToCoin();				
			}
			break;
		}
	case CMD_COIN_CHANGE_MONEY:
	case CMD_COIN_CHANGE_MONEY_ALL_STEP:
		{
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ �ݾ� ����, cmd:%d"), _command);	

			_connect = m_protocol->CheckDeviceOpen();
			if(_connect == TRUE)
			{				
				// �ӽ� ���� �ݾ� ����ü ����
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

				// ��� ���� - ���� �ݾ� ���� ���
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

			// ȸ�� ������ ����
			AuditData();

			_cs.Lock();{
				m_nTmpAmt = 0;
				// ���� �ݾ� ���� �ʱ�ȭ			
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
			//m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ���� �ݱ� ��ü ����, cmd:%d"), _command);	

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
			// ���� �ݾ� �ʱ�ȭ
			_cs.Lock();
			{
				//m_log.WriteLog(_T("| ������ ���� �ʱ�ȭ |"));
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
			////m_log.WriteLog(_T("| Proxy => Module | ����ó����ġ ����(polling), cmd:%d"), _command);	

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
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ ��� ���� : ����"));
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
 * IN  : BOOL _bDataCmd(������ �ڸ�Ʈ���� �Ǵ��ϴ� �÷���), BYTE _cmd (���� ���), BYTE _ctrl_cmd(����ü���� ������� ���)
 * Out : TRUE(���� �۽�), FALSE(�۽� ����)
 * Desc: ����ó����ġ ��Ŷ �۽� ���� 
 */
BOOL CConluxCoinModule::SendCoinCmd(_In_ const BOOL _bDataCmd, _In_opt_ BYTE _cmd, 
									_In_opt_ BYTE _data_cmd/* = 0x00*/, 
									_In_opt_ BYTE _ctrl_cmd/*=0xFF*/)
{
	CSingleLock _cs(&m_cs);
	BOOL _connect = TRUE;

	if(_bDataCmd == TRUE)
	{// ������ �ڸ�� �۽�
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
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ������ ��� ���� : ����"));
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
 * IN  : _In_ BOOL bComplete (TRUE: �Ϸ�, FALSE: �̿Ϸ�)
 * Out :
 * Desc: ���� �Ϸ� �÷��� ����
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
 * IN  : LPVOID, �ڱ� ��ü ����Ʈ
 * Out : 0, ������ ����
 * Desc: ������ ���� �Լ�
 */
UINT CConluxCoinModule::_threadEntry(_In_ LPVOID pObj)
{
	CConluxCoinModule *pSelfObj = static_cast<CConluxCoinModule*>(pObj);
	pSelfObj->Working();
	return 0;
}

/*
 * Name: Working
 * IN  : ����
 * Out : ����
 * Desc: ��ŷ ������ �Լ�
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
		// ������ ���� �̺�Ʈ ��ȣ ���
		_dwExit = ::WaitForSingleObject(m_hExitThread, 200L);
		if(_dwExit == WAIT_OBJECT_0){
			return;
		}
		
		// ���� ��� �Ϸ� �÷��� �� ��������
		_bOperComplete = GetOperationCompleteFlag();
		if(_bOperComplete == TRUE && g_bExit == FALSE)
		{
			// ���� ��� �Ϸ�Ǿ����� ���� ��� ��������
			//_nCommand = CommandOut(TRUE);

			// ���� ��� �� �Ϸ� �÷��� ����
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
 * Desc: ��� ó�� 
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
		//m_log.WriteLog(_T("| ���� �ݱ� �Ϸ� | ���� ���� ��� �۽�"));

		// �ӽ� ���� �ݾ� ����ü ����
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
			// ���� �ݾ��� ���� ��� �ʱ�ȭ ��� �۽�
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���� �ݾ� ����, �ʱ�ȭ ��� �۽�"));
			
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
			// ���� �ݾ��� ���� ��� �ʱ�ȭ ��� �۽�
			//m_log.SetContentsLog(); //m_log.WriteLog(_T("- ���� �ݾ�:[%d(��)], ���� ��� �۽�"), m_nTmpAmt);

			// ���� �ݾ��� ���� �ݾ����� ����
			SetChangesData(m_nTmpAmt);

			// ��� ���� - ���� �ݾ� ���� ���
			ResetRetryCnt();
			_bResult = SendCoinCmd(FALSE, CCoinProtocol::CONLUX_CMD_ORDER_OUT, CCoinProtocol::CONLUX_DC_AMT_RETURN);
			if(_bResult == TRUE)
			{
				// �ܵ� ���� ���� �÷��� ����
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
 * Desc: ���� ����
 */
INT CConluxCoinModule::procChange()
{
	BOOL _bResult = 1;

	// ���� ���� ���°� �ƴ� ��� ���� ��û
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
			//m_log.WriteLog(_T("| ���� ������ �߻�(�ݾ�: %d(��)) |"), (m_nTmpAmt * -1));	
		}else if(m_nTmpAmt > 0){
			//m_log.WriteLog(_T("| ���� �̹��� �߻�(�ݾ�: %d(��)) |"), m_nTmpAmt);
		}else{									
			//m_log.WriteLog(_T("| ���� ���� �Ϸ� |"));
		}
	}
	_cs.Unlock();				

	// ���� �ݾ��� ���� ��� �ʱ�ȭ ��� �۽�
	//m_log.WriteLog(_T("| ���� �Ϸ� | �ʱ�ȭ ��� �۽�"));	
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
 * Desc: Ŭ���� ����
 */
INT CConluxCoinModule::procClear()
{
	BOOL _bResult = 1;

	// ���� ���� ���°� �ƴ� ��� ���� ��û
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
			// ���� ���� ���� ������ �ʱ�ȭ
			memset(m_pData->insert_data.b_coin_type, 0x00, sizeof(m_pData->insert_data.b_coin_type));
			// ���� �ݾ� ����
			m_pData->insert_data.dw_amount = m_nTmpAmt;
		}*/
	}

	return _bResult;
}

/*
 * Name: procClose
 * IN  :
 * Out :
 * Desc: ���� �ݱ� ó�� 
 */
INT CConluxCoinModule::procClose()
{
	BOOL _bResult = 1;

	// ���� �ݱ�� �ܵ� ���� ���� ���
	if(m_status.bit_data.CHANGE > 0 && m_status.bit_data.CREMON == 0)
	{
		//m_log.WriteLog(_T("| ���� �ݱ� �Ϸ� |"));
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

