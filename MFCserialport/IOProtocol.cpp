#include "stdafx.h"
#include "IOProtocol.h"
#include "ASCIICode.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)              if (p) { delete (p); (p) = NULL; }
#endif

static const INT WAIT_HALF_SECOND			= 500;
// IO
static const INT IO_RECV_TIMEOUT		    = 10000;

// �̺�Ʈ �ε��� ����
static const INT EVENT_RECEIVE = 0;
static const INT EVENT_CLOSE   = 1;
static const INT EVENT_RESET   = 2;

static bool g_thread_exit = false;

// �����忡�� ȣ���� �̺�Ʈ �Լ� ����
BOOL m_bCallACMEvent = FALSE;

CIOProtocol::CIOProtocol(void) :
	m_bConnect(FALSE),
	m_bSequnceNum(0),
	m_pThread(NULL)	
{
	g_thread_exit = false;
	NewLog();
}


CIOProtocol::~CIOProtocol(void)
{
	g_thread_exit = true;

	Disconnect();

	DeleteRecvEvent();
}


VOID CIOProtocol::NewLog()
{
}


//----------------------------------------------------------------------//
// Name : Connect
// IN	: CSerial::EBaudrate eBaudrate,  CSerial::EDataBits eDataBits,
//	 	  CSerial::EParity   eParity, CSerial::EStopBits eStopBits, INT com_port_no
// OUT	: 
// DESC	: ���� 
//----------------------------------------------------------------------//
BOOL  CIOProtocol::Connect(CSerial::EBaudrate eBaudrate /*= CSerial::EBaud9600*/,
							   CSerial::EDataBits eDataBits /*= CSerial::EData8*/,
							   CSerial::EParity   eParity   /*= CSerial::EParNone*/,
							   CSerial::EStopBits eStopBits /*= CSerial::EStop1*/,
							   INT com_port_no/*=3*/)
{
	// ������ ȣ�� �Լ� ����
	m_bCallACMEvent = FALSE;

	m_com_port = com_port_no;

	LONG l_err_code = 0;
	m_bConnect		= FALSE;

	bool bReset = false;
	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	

		Disconnect();

		CreateRecvEvent();
	}

	l_err_code = Open(com_port_no);
	if(l_err_code != ERROR_SUCCESS)
	{
		return FALSE;
	}

	l_err_code = Setup(eBaudrate, eDataBits, eParity, eStopBits);
	if(l_err_code != ERROR_SUCCESS)
	{	
		return FALSE;
	}

	l_err_code = SetupHandshaking(EHandshakeOff);
	if(l_err_code != ERROR_SUCCESS)
	{
		return FALSE;
	}

	DWORD dw_Multiplier = 2 * CBR_9600 / eBaudrate;
	l_err_code = SetupWriteTimeouts(dw_Multiplier, 0);	
	if(l_err_code != ERROR_SUCCESS)
	{	
		return FALSE;
	}

	l_err_code = Purge();
	if(l_err_code != ERROR_SUCCESS)
	{		
		return FALSE;
	}

	m_bConnect = TRUE;

	if(m_pThread != NULL)
	{
		m_pThread->ResumeThread();	
	}
		
	return TRUE;
}


VOID CIOProtocol::Disconnect()
{
	CSerial::Close();

	m_bConnect = FALSE;
}

/*
 * Name: CreateRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
BOOL CIOProtocol::CreateRecvEvent()
{
	// overlapped operatoin�� ���� �ڵ� ����
	m_hOverlapped = ::CreateEvent(0, TRUE, FALSE, NULL);
	if(m_hOverlapped == 0)
	{
		return FALSE;
	}		

	// ������ ���Ḧ ���� �ڵ� ����
	m_hStop = ::CreateEvent(0, FALSE, FALSE, NULL);
	if(m_hStop == 0)
	{
		return FALSE;
	}

	// ������ ����
	if(m_pThread == NULL)
	{
		m_pThread = AfxBeginThread(_ThreadEntry, this, 0, 0, CREATE_SUSPENDED);	
		m_pThread->m_bAutoDelete = FALSE;	
	}

	return TRUE;
}

/*
 * Name: DeleteRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
VOID CIOProtocol::DeleteRecvEvent()
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

//----------------------------------------------------------------------//
// Name : SendPacket
// IN	: IN BYTE command_code, _In_opt_ BYTE *p_data, OPTIONAL SIZE_T data_len
// OUT	: 
// DESC	: ��Ŷ �۽� ó��
//----------------------------------------------------------------------//
BOOL  CIOProtocol::SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data/*=NULL*/, OPTIONAL SIZE_T data_len/*=0*/)
{
	BOOL b_ret = FALSE;

	for(INT retry_count=0; retry_count < IO_RETRY_COUNT; retry_count++)
	{
		b_ret = SendText(command_code, p_data, data_len);
		if(b_ret == TRUE)
			break;

		Sleep(WAIT_HALF_SECOND);
	}

	if(b_ret == FALSE)
	{
		m_bConnect = FALSE;
	}

	return b_ret;	
}


//----------------------------------------------------------------------//
// Name : GetSequenceNum
// IN	: 
// OUT	: message sequence number
// DESC	: message ���� Ƚ���� ����
//----------------------------------------------------------------------//
BYTE CIOProtocol::GetSequenceNum()
{
	if(m_bSequnceNum == 0xFF) 
		m_bSequnceNum = 0x00;
	else
		m_bSequnceNum++;

	return m_bSequnceNum;
}


//----------------------------------------------------------------------//
// Name : CalculateBCC
// IN	: _In_ BYTE *p_data, _In_ INT n_len
// OUT	: BCC
// DESC	: BCC ���ϴ� �Լ�
//----------------------------------------------------------------------//
BYTE  CIOProtocol::CalculateBCC(_In_ BYTE *p_data, _In_ INT n_len)
{	
	BYTE    *lpSource = p_data;
	BYTE    bBCC = 0;

	for ( INT i = 0; i<n_len; i++ )
	{
		bBCC ^= p_data[i];
		lpSource++;
	}

	return bBCC;
}


INT CIOProtocol::EncodeEscape(_In_ BYTE *p_dst, _In_ BYTE *p_src, IN INT data_len)
{
	int DleCnt = 0;

	p_dst[0] = p_src[0];
	for (int i=1; i<data_len-1; i++)
	{
		if (p_src[i] == DLE)
		{
			p_dst[i + DleCnt++] = DLE;
			p_dst[i + DleCnt]   = DLE^0x40;
		}
		else if (p_src[i] == STX)
		{
			p_dst[i + DleCnt++] = DLE;
			p_dst[i + DleCnt]   = STX^0x40;
		}
		else if (p_src[i] == ETX)
		{
			p_dst[i + DleCnt++] = DLE;
			p_dst[i + DleCnt]   = ETX^0x40;
		}
		else
			p_dst[i + DleCnt] = p_src[i];
	}

	p_dst[data_len-1+DleCnt] = p_src[data_len-1];

	return DleCnt;
}

INT CIOProtocol::ConvertEscape(_In_ BYTE *p_dst, _In_ BYTE *p_src, IN INT src_len)
{
	int DleCnt = 0;
	int nCnt   = 0;
	BYTE DLE_ESC = DLE ^ 0x40;
	BYTE STX_ESC = STX ^ 0x40;
	BYTE ETX_ESC = ETX ^ 0x40;

	p_dst[nCnt++] = p_src[0];

	for (int i=1; i<src_len-1; i++)
	{
		if ((p_src[i] == DLE) && (p_src[i+1] == DLE_ESC))
		{
			p_dst[nCnt++] = DLE;
			i++; DleCnt++;
		}
		else if ((p_src[i] == DLE) && (p_src[i+1] == STX_ESC))
		{
			p_dst[nCnt++] = STX;
			i++; DleCnt++;
		}
		else if ((p_src[i] == DLE) && (p_src[i+1] == ETX_ESC))
		{
			p_dst[nCnt++] = ETX;
			i++; DleCnt++;
		}
		else
		{
			p_dst[nCnt++] = p_src[i];
		}
	}
	p_dst[nCnt] = p_src[src_len-1];

	return DleCnt;
}

BOOL CIOProtocol::SendText(IN BYTE command_code, _In_ BYTE *p_data, IN SIZE_T data_len)
{
	INT		tmpDataSize  = 0;
	INT		tmpDataLen   = 0;
	INT		tmpAddLength = 0;
	BYTE	tBuf[MAX_BUFFER];
	
	m_send_buf_len = 0;

	ZeroMemory(tBuf, sizeof(BYTE)*MAX_BUFFER);
	ZeroMemory(m_send_buf, sizeof(BYTE)*MAX_BUFFER);
	
	tBuf[tmpDataSize++] = STX;
	tBuf[tmpDataSize++] = command_code;
	tBuf[tmpDataSize++] = GetSequenceNum();
	tBuf[tmpDataSize++] = 0x00;
	tBuf[tmpDataSize++] = 0x00;

	if(data_len > 0)
	{
		CopyMemory(&tBuf[tmpDataSize], p_data, sizeof(BYTE)*data_len);
		tmpDataSize += data_len;		
		tmpDataLen  += data_len;
	}

	//Data Length Setting
	tBuf[IO_PACKET_INDEX_LENGTH]	= HIBYTE(tmpDataLen);
	tBuf[IO_PACKET_INDEX_LENGTH+1]  = LOBYTE(tmpDataLen);
	tBuf[tmpDataSize++] = CalculateBCC(tBuf, tmpDataSize);	
	tBuf[tmpDataSize++] = ETX;

	//DLE Convert
	tmpAddLength   = EncodeEscape(m_send_buf, tBuf, tmpDataSize);
	m_send_buf_len = tmpDataSize + tmpAddLength;

	if(WriteBlock(m_send_buf, m_send_buf_len, INFINITE) != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();
		if(dwError == ERROR_IO_INCOMPLETE || dwError == ERROR_IO_PENDING){
			return TRUE;
		}		
		return FALSE;
	}

	return TRUE;
}


VOID CIOProtocol::RecvText()
{
	DWORD	dw_evt_mask		= 0;		// event mask
	DWORD	dw_readed		= 0;		// ���� ���ۿ��� �о�� ���� �� 
	BYTE	b_readchar		= 0;		// ���� ���ۿ��� ���� ����
	INT		data_cnt		= 0;		// ���� Loop�� ���� ���� ��
	LONG	l_retcode		= 0;		// Read �Լ� ��� ��
	LONG    l_waitevt_ret   = 0;		// ���� �̺�Ʈ ��� ��� ��
	BOOL    b_waitevt_ret   = FALSE;
	BOOL	b_enable_write  = FALSE;	// ���� ���ۿ��� ���� ���� �ӽ� ���� ���ۿ� ���� �Ǵ�
	BOOL    b_request_resned= FALSE;	// �� �۽� ��û
	BYTE    tmp_buf[MAX_BUFFER];		// ���� �����͸� ���� �� �ӽ� ���� ����

	CString slog			= _T("");			

	dw_evt_mask = GetEventMask();
	if( (dw_evt_mask & EV_RXCHAR) == EV_RXCHAR)
	{	
		b_enable_write  = FALSE;
		b_request_resned= FALSE;
		data_cnt		= 0;			
		m_recv_buf_len	= 0;
		ZeroMemory(tmp_buf, MAX_BUFFER);	

		slog.Format(_T("%s packet = |"), _T("Receive"));		

		while( data_cnt < MAX_BUFFER )
		{
			b_readchar = 0x00;							
			dw_readed  = 0;

			l_retcode  = Read(&b_readchar, 1, &dw_readed);

			if(l_retcode == ERROR_SUCCESS)
			{
				// ���� ���ۿ��� �о���� ���� ���� '1'�� ���
				if(dw_readed == 1)
				{
					// ���� ���ڰ� 'STX'�� ��� 
					// ���� ���� ���ۿ��� �о���� �����͸� ���� ���ۿ� ���
					if(b_readchar == STX && m_recv_buf_len == 0)
						b_enable_write = TRUE;

					if(b_readchar == NAK && m_recv_buf_len == 0)
					{
						b_request_resned = TRUE;
						break;
					}

					if(b_enable_write == TRUE)
					{
						tmp_buf[m_recv_buf_len++] = b_readchar;
						slog.AppendFormat(_T("0x%02x|"), b_readchar);
						if(b_readchar == ETX)
						{
							//m_log.WriteLog(slog);
							break;
						}
					}
				}

				data_cnt++;				
			}
			else
			{
				break;
			}
		}//end while

		if(b_enable_write == TRUE)
		{
			b_enable_write = FALSE;

			SendCtrlChar(ACK);

			Sleep(100);

			ZeroMemory(m_recv_buf, MAX_BUFFER);	

			ConvertEscape(m_recv_buf, tmp_buf, m_recv_buf_len);
		}

		if(b_request_resned == TRUE)
		{
			b_request_resned = FALSE;
		}
	}//end if
}


//----------------------------------------------------------------------//
// Name : SendCtrlChar
// IN	: IN BYTE b_char
// OUT	: TRUE(�۽� ����), FALSE(�۽� ����) 
// DESC	: �����ǹ߸���ġ�� ������� �۽�
//----------------------------------------------------------------------//
BOOL  CIOProtocol::SendCtrlChar(IN BYTE b_char)
{
	if(WriteBlock(&b_char, 1, INFINITE) != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------//
// Name : _ThreadEntry
// IN	: _In_ LPVOID pParam
// OUT	: 
// DESC	: main work thread 
//----------------------------------------------------------------------//
UINT CIOProtocol::_ThreadEntry(_In_ LPVOID pParam)
{
	CIOProtocol *pSelf = static_cast<CIOProtocol*>(pParam);
	pSelf->_ReceiveEvent();
	return 0;
}

//----------------------------------------------------------------------//
// Name : WatchReceiveEvent
// IN	: 
// OUT	: 
// DESC	: receive event loop
//----------------------------------------------------------------------//
VOID  CIOProtocol::WatchReceiveEvent()
{		
	HANDLE hEvent;
	OVERLAPPED lpOverlapped;

	hEvent = ::CreateEvent(0, TRUE, FALSE, 0);
	memset(&lpOverlapped,0,sizeof(lpOverlapped));
	lpOverlapped.hEvent = hEvent;

	OVERLAPPED	ov = {0};
	HANDLE		ahWait[2];
	// �̺�Ʈ �ڵ� ����
	ov.hEvent = m_hOverlapped;
	ahWait[0] = m_hOverlapped;
	ahWait[1] = m_hStop;	

	while(TRUE)
	{

		if(!::WaitCommEvent(m_hFile, LPDWORD(&m_eEvent), &lpOverlapped))
		{
			long lLastError = ::GetLastError();

			if (lLastError != ERROR_IO_PENDING)
			{
				m_bConnect = FALSE;		
				CloseHandle(hEvent);
				return;
			}

			switch (::WaitForSingleObject(lpOverlapped.hEvent, IO_RECV_TIMEOUT))
			{
			case WAIT_OBJECT_0:
				{
					m_bConnect = TRUE;
					RecvText();
					break;
				}
			case WAIT_TIMEOUT:
				{				
					CancelCommIo();								
					ZeroMemory(m_recv_buf, MAX_BUFFER);	
					m_bConnect = FALSE;
					break;
				}
			default:
				{
					lLastError = ::GetLastError();
					break;
				}
			}
		}
		else
		{
			RecvText();
		}
	}//end while
}

//----------------------------------------------------------------------//
// ACM IO �ø��� ��� ��� ����
//----------------------------------------------------------------------//

/*
 * Name:ClearBuffer
 * IN  :
 * Out :���� Ŭ���� ���
 * Desc:���� Ŭ����
 */
BOOL CIOProtocol::ClearBuffer()
{
	LONG lErrorCode = CSerial::Purge();
	if (lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}	

	return TRUE;
}

/*
 * Name: ConnectACM
 * IN  :
 * Out :
 * Desc: ACM IO �ø��� ��� ����
 */
BOOL CIOProtocol::ConnectACM(CSerial::EBaudrate eBaudrate /* = CSerial::EBaud4800 */, 
							 CSerial::EDataBits eDataBits /* = CSerial::EData8 */, 
							 CSerial::EParity eParity	  /* = CSerial::EParEven */, 
							 CSerial::EStopBits eStopBits /* = CSerial::EStop1 */)
{
	// �����忡�� ȣ�� �� �Լ� Ÿ�� ���� 
	m_bCallACMEvent = TRUE;

	bool bReset = false;

	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	
		
		Disconnect();

		CreateRecvEvent();
	}
	else
	{
		if(m_pThread == NULL){
			CreateRecvEvent();
		}
	}

	// �ø��� ����
	LONG lErrorCode = CSerial::Open(m_com_port, 0, 0, TRUE);
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
	
	lErrorCode = CSerial::Purge();
	if (lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}	
	
	if(m_pThread != NULL){
		m_pThread->ResumeThread();	
	}

	return TRUE;
}

/*
 * Name: SendPowerCtrlToIO
 * IN  :
 * Out :
 * Desc: IO ���忡 ��� �۽�
 */
BOOL CIOProtocol::SendPowerCtrlToIO(_In_ BYTE _cmd, _In_ BYTE _ctrl_cmd)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		m_bConnect = FALSE;
		return FALSE;
	}

	BOOL bRet = ClearBuffer();
	if(bRet == FALSE) return FALSE;

	BYTE send_pack[5] = {0x00};

	send_pack[0] = STX;
	send_pack[1] = _cmd;
	send_pack[2] = (BYTE)(_ctrl_cmd);
	send_pack[3] = ETX;
	send_pack[4] = CalculateBCC(send_pack, 4);

	LONG lErrorCode = CSerial::Write(send_pack, sizeof(send_pack));
	if(lErrorCode != ERROR_SUCCESS)
	{
		if(lErrorCode != ERROR_IO_INCOMPLETE && lErrorCode != ERROR_IO_PENDING)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CIOProtocol::SendInputCmdToIO(_In_ const BYTE _cmd)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		m_bConnect = FALSE;
		return FALSE;
	}

	BOOL bRet = ClearBuffer();
	if(bRet == FALSE) return FALSE;

	BYTE send_pack[4] = {0x00};

	send_pack[0] = STX;
	send_pack[1] = _cmd;
	send_pack[2] = ETX;
	send_pack[3] = CalculateBCC(send_pack, 3);

	LONG lErrorCode = CSerial::Write(send_pack, sizeof(send_pack));

	if(lErrorCode != ERROR_SUCCESS)
	{
		if(lErrorCode != ERROR_IO_INCOMPLETE && lErrorCode != ERROR_IO_PENDING)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*
 * Name: SendCmdToCardDispenser
 * IN  :
 * Out :
 * Desc: ī�� ���漭�� ��� �۽�
 */
BOOL CIOProtocol::SendCmdToCardDispenser(_In_ const BYTE _cmd, _In_ BYTE _devid)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		m_bConnect = FALSE;
		return FALSE;
	}

	BOOL bRet = ClearBuffer();
	if(bRet == FALSE) return FALSE;

	BYTE send_pack[5];
	send_pack[0] = STX;
	send_pack[1] = _devid;
	send_pack[2] = _cmd;
	send_pack[3] = ETX;
	send_pack[4] = CalculateBCC(send_pack, 4);

	LONG lErrorCode = CSerial::Write(send_pack, sizeof(send_pack));
	if(lErrorCode != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_IO_INCOMPLETE && dwError != ERROR_IO_PENDING)
		{
			return FALSE;
		}
	}

	CString _sLog = _T("| Module => Protocol | Command: ");
	for(INT i=0; i<5; i++){
		_sLog.AppendFormat(_T("[%02X]"), send_pack[i]);
	}
	return TRUE;
}

/*
 * Name: _ReceiveEvent
 * IN  :
 * Out :
 * Desc: ������ ���� �̺�Ʈ, ACM IO �����
 */
void CIOProtocol::_ReceiveEvent()
{
	OVERLAPPED	ov = {0};
	HANDLE		ahWait[2];
	CString		strLog(_T(""));		
	DWORD		dwRet = 0;			
	BYTE		_tmpBuf[MAX_BUFFER] = {NULL};
	INT			nEventIndex = 0;
	BOOL		_enableWriteData = FALSE;

	m_recv_buf_len = 0;

	// �̺�Ʈ �ڵ� ����
	ov.hEvent = m_hOverlapped;
	ahWait[0] = m_hOverlapped;
	ahWait[1] = m_hStop;	

	while(TRUE)
	{
		if(CSerial::CheckDeviceOpen() == FALSE)
		{
			if(g_thread_exit){
				return;
			}

			::Sleep(500);
			continue;
		}

		LONG lLastError = CSerial::WaitEvent(&ov);
		
		if(lLastError != ERROR_SUCCESS)
		{
			if(g_thread_exit){
				return;
			}

			if(lLastError == ERROR_IO_INCOMPLETE){
			}else if(lLastError == ERROR_IO_PENDING){
			}else{
				if(lLastError == ERROR_INVALID_FUNCTION){
				}else{
				}				

				// �Ϸ� �ݹ� ȣ��
				if(	m_bCallACMEvent == TRUE){
					OperationComplete(lLastError, 0x00, 0);
				}else{
				}
				
				return;
			}
		}

		dwRet = ::WaitForMultipleObjects(2, ahWait, FALSE, INFINITE);

		nEventIndex = (INT)(dwRet - WAIT_OBJECT_0);

		::ResetEvent( ahWait[nEventIndex] );

		switch(nEventIndex)
		{
		case EVENT_RECEIVE:	// �ø��� ��� �̺�Ʈ
			{
				strLog = _T("");

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
							if(	m_bCallACMEvent == TRUE){									
								OperationComplete(lLastError, recvBuf, dwBytesRead);
							}else{
							}
							break;
						}	
						else
						{
							if(dwBytesRead > 0)
							{
								// �Ϸ� �ݹ� ȣ�� (ACM)
								if(	m_bCallACMEvent == TRUE)
								{									
									// �α� ���
									strLog.Format(_T("Length:%u, Recv:"), dwBytesRead);
									for(INT i=0; i<(INT)(dwBytesRead); i++){
										strLog.AppendFormat(_T("[%02X]"), recvBuf[i]);
									}					
									OperationComplete(lLastError, recvBuf, dwBytesRead);
								}
								else
								{
									BOOL _bSendData = FALSE;

									// �α� ���
									strLog.Format(_T("Length:%u, Recv:"), dwBytesRead);
									for(INT i=0; i<(INT)(dwBytesRead); i++)
									{
										strLog.AppendFormat(_T("[%02X]"), recvBuf[i]);

										_tmpBuf[m_recv_buf_len] = recvBuf[i];
										
										if(_tmpBuf[m_recv_buf_len] == STX && m_recv_buf_len == 0)
										{
											_enableWriteData = TRUE;
										}
										else if(_tmpBuf[m_recv_buf_len] == NAK && m_recv_buf_len == 0)
										{
											m_recv_buf_len++;

											_bSendData = TRUE;

											_enableWriteData = FALSE;
											
											break;
										}				

										if(_tmpBuf[m_recv_buf_len] == ETX && _enableWriteData == TRUE)
										{
											m_recv_buf_len++;

											SendCtrlChar(ACK);

											Sleep(100);

											ConvertEscape(m_recv_buf, _tmpBuf, m_recv_buf_len);

											_bSendData = TRUE;

											_enableWriteData = FALSE;

											break;
										}

										m_recv_buf_len++;
									}//end for					

									if(_bSendData == TRUE){
									}									

									if(_enableWriteData == FALSE)
									{
										m_recv_buf_len = 0;

										memset(m_recv_buf, 0x00, sizeof(m_recv_buf));	

										memset(_tmpBuf, 0x00, sizeof(_tmpBuf));
									}
								}
							}//end if(dwBytesRead > 0)
						}// end else
					} while (dwBytesRead > 0);
				}//end if

				break;
			}
		case EVENT_CLOSE:	// ���� �̺�Ʈ
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