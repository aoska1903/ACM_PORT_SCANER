#include "stdafx.h"
#include "BillProtocol.h"
#include "ASCIICode.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)              if (p) { delete (p); (p) = NULL; }
#endif
#define BXR_NO_ERROR                    (0)  /**< Returned by synchronious method when there is no error */
#define BXR_USB_ERROR_OFFSET        (10000)  /**< Group of code returned by the USB driver */
#define BXR_BNR_EXCEPTION_OFFSET     (1000)  /**< Group of XFS error and exception codes returned by the BNR */

static bool g_bBillThreadExit = false;

//----------------------------------------------------------------------//
// BNR API static function & variable declare
//----------------------------------------------------------------------//

// CBillProtocol ����
static CBillModule			*pModule = NULL;
static CBillProtocol		*pProtocol = NULL;

// BNR callback function
/******************************************************************************
 * onStatusOccured
 ***************************************************************************//**
 * Called when an Status event is received
 *
 * @param status XFS status code
 * @param result J/XFS result code
 * @param extendedResult extended result code send by the BNR
 * @param details event data. Contents depend upon status.
 *
 ******************************************************************************/


/******************************************************************************
 * onIntermediateOccured
 ***************************************************************************//**
 * Called when an Intermediate event is received
 *
 * @param identificationID job identification number
 * @param operationID J/XFS operation code
 * @param reason J/XFS reason code
 * @param data event data. Contents depend upon reason
 *
 ******************************************************************************/

/******************************************************************************
 * onOperationCompleteOccured
 **************************************************************************//**
 * Called when an Operation Complete event is received
 *
 * @param identificationID job identification number
 * @param operationID J/XFS operation code
 * @param result J/XFS result code
 * @param extendedResult extended result code send by the BNR
 * @param data event data. Contents depend upon operationID.
 *
 *****************************************************************************/

CBillProtocol::CBillProtocol(CBillModule *p_module/*=NULL*/) :
	m_nPortCom(4)
{
	if(p_module != NULL )
	{
		// BNR �ݹ� �Լ� �� �ش� ��ü�� �̿��ϱ� ���� ��ü ����
		pModule   = p_module;
	}
	g_bBillThreadExit = false;
}

CBillProtocol::~CBillProtocol(void)
{
	g_bBillThreadExit = true;

	DeleteRecvEvent();

	CSerial::Close();
}


VOID CBillProtocol::NewLog()
{
}


//----------------------------------------------------------------------//
// Name : DeviceOpen
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: ��ġ ����
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::DeviceOpen(HWND hParent)
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;

	// ��ǰ �ø��� ��ȣ ȹ��
	return xfs_ret;
}

//----------------------------------------------------------------------//
// Name : Reset
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: ����
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::DeviceReset()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}

//@  This function is used to reset the Device port.
T_BnrXfsResult CBillProtocol::UsbReset()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}

//@ This function is used to kill and reload device driver
T_BnrXfsResult CBillProtocol::UsbKillNReload()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}

//----------------------------------------------------------------------//
// Name : OperationCancel
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: ���� ���� ���� �񵿱� ��� ���
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::OperationCancel()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : CashInStart
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: ��ġ ����
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::CashInStart()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : CashIn
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: �Լ� �� ���� ����ũ�η� �̵�
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::CashIn()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : CashInRollback
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	:  roll back the cash in transaction
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::CashInRollback()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : CashInEnd
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: ��ġ ����
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::CashInEnd()
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}

//----------------------------------------------------------------------//
// Name : PresentAmount
// IN	: 
// OUT	: 
// DESC	: �°����� �ݾ� ��������
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::PresentAmount()
{
	T_BnrXfsResult xfs_ret = BXR_NO_ERROR;
	return xfs_ret;
}

//----------------------------------------------------------------------//
// Name : BillEmpty
// IN	: _In_ char *pcuName(Physical Cash Unit name), BOOL toFloat (empties up to Low threshold)
// OUT	: 
// DESC	: recycler or loader�� ���� cash box�� ȸ��
//----------------------------------------------------------------------//
INT32 CBillProtocol::BillEmpty(_In_ char *pcuName, BOOL toFloat)
{
	return 0;
}


//----------------------------------------------------------------------//
// Name : Eject
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: the application to force cash that has been presented
//----------------------------------------------------------------------//
INT32 CBillProtocol::Eject(_Inout_ T_BnrXfsResult* p_orResult)
{
	T_BnrXfsResult	xfs_ret = BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : Reject
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: the application to clear the intermediate stacker
//----------------------------------------------------------------------//
INT32 CBillProtocol::Reject(_Inout_ T_BnrXfsResult* p_orResult)
{
	T_BnrXfsResult	xfs_ret = BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : Retract
// IN	: 
// OUT	: T_BnrXfsResult ��� ��
// DESC	: application to force cash that has been presented to be retracted.
//----------------------------------------------------------------------//
INT32 CBillProtocol::Retract(_Inout_ T_BnrXfsResult* p_orResult)
{
	T_BnrXfsResult	xfs_ret = BXR_NO_ERROR;
	return xfs_ret;
}


//----------------------------------------------------------------------//
// Name : GetModuleStatus
// IN	: 
// OUT	: 
// DESC	: ��� ���� ������ ������
//----------------------------------------------------------------------//

T_BnrXfsResult CBillProtocol::GetModuleStatus(_In_ UINT32 moduleId, _Inout_ T_ModuleStatus* p_status)
{
	T_BnrXfsResult	xfs_ret=BXR_NO_ERROR;
	return xfs_ret;
}

//----------------------------------------------------------------------//
// Name : GetModuleID
// IN	: 
// OUT	: 
// DESC	: ����ȯ���� ��� �� ���� ID�� ����
//----------------------------------------------------------------------//
T_BnrXfsResult CBillProtocol::GetModules(_Inout_ T_ModuleIdList* p_module_list)
{		
	ZeroMemory(p_module_list, sizeof(T_ModuleIdList));
	return (T_BnrXfsResult) p_module_list;
}

//////////////////////////////////////////////////////////////////////////
// Serial ���

// ��� ��Ʈ ���� �̺�Ʈ �ε��� ����
static const INT EVENT_RECEIVE = 0;
static const INT EVENT_CLOSE   = 1;

/*
 * Name: Connect
 * IN  :
 * Out : ��� ���� �� ���� ���� ����, (TRUE:����, FALSE:����)
 * Desc: �ø��� ��� ���� �� ����
 */
BOOL CBillProtocol::Connect( CSerial::EBaudrate eBaudrate /*= CSerial::EBaud9600*/,
							 CSerial::EDataBits eDataBits /*= CSerial::EData8*/,
							 CSerial::EParity eParity	  /*= CSerial::EParNone*/,
							 CSerial::EStopBits eStopBits /*= CSerial::EStop1*/
							 )
{		
	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	
		Disconnect();
		CreateRecvEvent();
	}
	LONG lErrorCode = Open(m_nPortCom, 0, 0, TRUE);	
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	lErrorCode = SetupHandshaking(EHandshakeOff);	
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}

	lErrorCode = Setup(eBaudrate, eDataBits, eParity, eStopBits );
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	lErrorCode = SetupWriteTimeouts(50, 1000);
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	lErrorCode = Purge();
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if(m_pThread != NULL)
	{
		m_pThread->ResumeThread();	
	}
	return TRUE;
}

/*
 * Name: CalcBCC
 * IN  : BYTE* _pBuffer, DWORD _dwSize
 * Out : BCC ��
 * Desc: BCC ���
 */
BYTE CBillProtocol::CalcBCC( _In_ BYTE* _pBuffer, _In_ DWORD _dwSize )
{
	BYTE    *lpSource = _pBuffer;
	BYTE    bBCC = 0;
	DWORD   dwIndex;
	for ( dwIndex = 0; dwIndex < _dwSize; dwIndex++ )
	{
		bBCC ^= *lpSource;
		lpSource++;
	}
	return bBCC;
}

/*
 * Name: ConnectToDevice
 * IN  : ���� �� ����
 * Out : ���� ���� ����(TRUE: ����, FALSE:���� ����)
 * Desc: ��� ����
 */
BOOL CBillProtocol::ConnectToDevice(CSerial::EBaudrate eBaudrate /* = CSerial::EBaud9600 */, 
									CSerial::EDataBits eDataBits /* = CSerial::EData8 */, 
									CSerial::EParity eParity	 /* = CSerial::EParEven */, 
									CSerial::EStopBits eStopBits /* = CSerial::EStop1 */)
{
	if(CSerial::CheckDeviceOpen() == TRUE)
	{
		DeleteRecvEvent();
		CSerial::Close();
		CreateRecvEvent();
	}

	// �ø��� ��� ����
	LONG lErrorCode = CSerial::Open(m_nPortCom, 0, 0, TRUE);
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	
	// ��� ����
	lErrorCode = CSerial::Setup(eBaudrate, eDataBits, eParity, eStopBits );
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if(m_pThread != NULL){
		m_pThread->ResumeThread();	
	}
	return TRUE;
}

VOID  CBillProtocol::Disconnect()
{
	Close();
}


/*
 * Name: CreateRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
BOOL CBillProtocol::CreateRecvEvent()
{
	// overlapped operatoin�� ���� �ڵ� ����
	m_hOverlapped = ::CreateEvent(0, TRUE, FALSE, NULL);
	if(m_hOverlapped == 0)
	{
		return FALSE;
	}		
	// ������ ���Ḧ ���� �ڵ� ����
	m_hStop = ::CreateEvent(0, TRUE, FALSE, NULL);
	if(m_hStop == 0)
	{
		return FALSE;
	}

	// ������ ����
	if(m_pThread == NULL)
	{
		m_pThread = AfxBeginThread(_threadEntry, this, 0, 0, CREATE_SUSPENDED);	
		m_pThread->m_bAutoDelete = FALSE;	
		m_pThread->ResumeThread();
	}
	return TRUE;
}

/*
 * Name: DeleteRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
VOID CBillProtocol::DeleteRecvEvent()
{
	if(m_pThread != NULL)
	{
		::SetEvent(m_hStop);

		DWORD dwRet = ::WaitForSingleObject(m_pThread->m_hThread, 5000L);
		if(dwRet == WAIT_TIMEOUT){
			::TerminateThread(m_pThread, 1);
		}

		SAFE_DELETE(m_pThread);
	}

	if(m_hOverlapped){
		CloseHandle(m_hOverlapped);
		m_hOverlapped = 0;
	}

	if(m_hStop){
		CloseHandle(m_hStop);
		m_hStop = 0;
	}
}

/*
 * Name: SendCmdToBNK
 * IN  : BYTE _cmd(����ó����ġ Ŀ���), WORD _wEnableBankNoteMask(���� ������ ���� Ÿ�� mask)
 * Out : TRUE-����, FALSE-����
 * Desc: ����ó����ġ�� ��� ����, ����� BV-6200
 */
BOOL CBillProtocol::SendCmdToBNK(_In_ const BYTE _cmd, _In_opt_ WORD _wEnableBankNoteMask/*=0*/)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		return FALSE;
	}

	BOOL _bRet = ClearBuffer();
	if(_bRet == FALSE) return _bRet;

	// Text ����
	BYTE _send_buf[MAX_BUFFER]; 
	INT _send_len = 0;

	memset(_send_buf, 0x00, sizeof(_send_buf));

	// Data Block Format ����
	_send_buf[_send_len++] = DLE;
	_send_buf[_send_len++] = STX;
	_send_buf[_send_len++] = _cmd;
	if(_wEnableBankNoteMask != 0)
	{
		_send_buf[_send_len++] = LOBYTE(_wEnableBankNoteMask);
		_send_buf[_send_len++] = HIBYTE(_wEnableBankNoteMask);
	}
	else
	{
		if(_cmd == BNK_CMD_MEM_TRANSFER)
		{
			_send_buf[_send_len++] = 0x40;
			_send_buf[_send_len++] = 0x00;
		}
	}
	_send_buf[_send_len++] = DLE;
	_send_buf[_send_len++] = ETX;
	_send_buf[_send_len++] = CalcBCC(_send_buf+2, _send_len-2);


	LONG lErrCode = CSerial::Write(_send_buf, _send_len);
	if(lErrCode != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}

/*
 * Name: SendTransCtrlChar
 * IN  : BYTE _cmd(����ó����ġ Ŀ���)
 * Out : TRUE-����, FALSE-����
 * Desc: ���� ���� ����  
 */
BOOL CBillProtocol::SendTransCtrlChar(_In_ BYTE* _symbol, _In_opt_ INT _length/*=1*/)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		return FALSE;
	}

	BOOL _bRet = ClearBuffer();
	if(_bRet == FALSE) return _bRet;

	CString _strLog(_T(""));
	_strLog.Format(_T("| Send => | Length: %d, "), _length);
	for(INT i=0; i<_length; i++)
		_strLog.AppendFormat(_T("[%02X]"), _symbol[i]);

	LONG lErrorCode = CSerial::Write(_symbol, _length);
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	
	return TRUE;
}

/*
 * Name: ClearBuffer
 * IN  :
 * Out : ���� Ŭ���� ���
 * Desc: ��� ��� ���� Ŭ����
 */
BOOL CBillProtocol::ClearBuffer()
{
	LONG lErrorCode = CSerial::Purge();
	if (lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}	

	return TRUE;
}

/*
 * Name: _threadEntry
 * IN  : LPVOID pParm(CBillProtocol ��ü ����Ʈ)
 * Out :
 * Desc: ������ 
 */
UINT CBillProtocol::_threadEntry(_In_ LPVOID pParm)
{ 
	CBillProtocol *pSelf = (CBillProtocol*)(pParm);
	pSelf->RecvEventWorking();
	return 0;
}

/*
 * Name: RecvEventWorking
 * IN  :
 * Out :
 * Desc: ���� ���� �̺�Ʈ ó�� �Լ�, _threadEntry �����忡�� ȣ��
 */
VOID CBillProtocol::RecvEventWorking(void)
{
	CString		strLog(_T(""));		
	OVERLAPPED	ov = {0};
	HANDLE		ahWait[2];
	DWORD		dwRet = 0;			
	INT			nEventIndex = 0;
	BOOL        _bReceptible = FALSE;

	// �̺�Ʈ �ڵ� ����
	ov.hEvent = m_hOverlapped;
	ahWait[0] = m_hOverlapped;
	ahWait[1] = m_hStop;	
	
	while(TRUE)
	{
		if(CSerial::CheckDeviceOpen() == FALSE)
		{
			if(g_bBillThreadExit){
				return;
			}

			::Sleep(500);
			continue;
		}

		LONG lLastError = CSerial::WaitEvent(&ov);
		if(lLastError != ERROR_SUCCESS)
		{
			if(g_bBillThreadExit){
				return;
			}

			if(lLastError == ERROR_IO_INCOMPLETE){
			}else if(lLastError == ERROR_IO_PENDING){
			}else{
				if(lLastError == ERROR_INVALID_FUNCTION){
				}else{
				}				
				// �Ϸ� �ݹ� ȣ��
				CompleteReceiveData(lLastError, 0x00, 0);
				return;
			}
		}

		dwRet = ::WaitForMultipleObjects(2, ahWait, FALSE, INFINITE);

		nEventIndex = (INT)(dwRet - WAIT_OBJECT_0);

		::ResetEvent( ahWait[nEventIndex] );
		
		switch(nEventIndex)
		{
			
		case EVENT_RECEIVE:
			{
				// �̺�Ʈ ����
				const CSerial::EEvent eEvent = CSerial::GetEventType();

				// ������ ���� �̺�Ʈ�� ���
				if (eEvent & CSerial::EEventRecv)
				{
					DWORD dwBytesRead = 0;	// �ø��� ����� ���� ���� �� ������ ũ��
					BYTE recvBuf[101];		// �ø��� ����� ���� ���� �� ������ ����
					memset(recvBuf, 0x00, sizeof(recvBuf));

					do 
					{						
						lLastError = CSerial::Read(recvBuf, sizeof(recvBuf)-1, &dwBytesRead);	
						
						if (lLastError != ERROR_SUCCESS)
						{		
							// �Ϸ� �ݹ� ȣ��
							CompleteReceiveData(lLastError, recvBuf, dwBytesRead);
							break;
						}	
						else
						{
							if(dwBytesRead > 0)
							{
								strLog = _T("| Recv <= | Length: ");

								// �α� ���
								strLog.AppendFormat(_T("%u, "), dwBytesRead);
								for(INT i=0; i<(INT)(dwBytesRead); i++){
									strLog.AppendFormat(_T("[%02X]"), recvBuf[i]);
								}					
								// �Ϸ� �ݹ� ȣ��
								CompleteReceiveData(lLastError, recvBuf, dwBytesRead);
							}
						}
					} while (dwBytesRead > 0);
				}//end if
				break;
			}
		case EVENT_CLOSE:
			{
				return;
			}
		default:
			{
				// Something went wrong				
				break;
			}
		}//end switch
	}//end while
}
