#include "stdafx.h"
#include "ASCIICode.h"
#include "CoinProtocol.h"

// �̺�Ʈ �ε��� ����
static const INT EVENT_RECEIVE = 0;
static const INT EVENT_CLOSE   = 1;
static const INT EVENT_RESET   = 2;

// conlux ������ ���� RTS ��ȣ 
static const DWORD RTS_HIGH = RTS_CONTROL_ENABLE;
static const DWORD RTS_LOW  = RTS_CONTROL_DISABLE;
static bool g_bCoinThreadExit = false;

CCoinProtocol::CCoinProtocol() :	
	m_flagConnect(FALSE),	
	m_bSequnceNum(0x00),
	m_recv_buf_len(0),
	m_send_buf_len(0),
	m_hOverlapped(NULL),
	m_hStop(NULL),
	m_pThread(NULL)
{	
	CreateLog();
}

CCoinProtocol::~CCoinProtocol(void)
{
	g_bCoinThreadExit = true;

	DeleteRecvEvent();

	Disconnect();	
}

/*
 * Name:ClearBuffer
 * IN  :
 * Out :���� Ŭ���� ���
 * Desc:���� Ŭ����
 */
BOOL CCoinProtocol::ClearBuffer()
{
	LONG lErrorCode = CSerial::Purge();
	if (lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}	
	return TRUE;
}

/*
 * Name: CreateLog
 * IN  : -
 * Out : -
 * Desc: �α� ���� ����
 */
VOID CCoinProtocol::CreateLog()
{
}

/*
 * Name: CreateRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
BOOL CCoinProtocol::CreateRecvEvent()
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
	}
	return TRUE;
}

/*
 * Name: DeleteRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ �ڵ� �� �۾� ������ ����
 */
VOID CCoinProtocol::DeleteRecvEvent()
{
	if(m_pThread != NULL)
	{
		g_bCoinThreadExit = true;

		::SetEvent(m_hStop);

		DWORD dwRet = ::WaitForSingleObject(m_pThread->m_hThread, 5000L);
		if(dwRet == WAIT_TIMEOUT){
			::TerminateThread(m_pThread, 1);
		}
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
 * Name: Disconnect
 * IN  : 
 * Out :
 * Desc: ���� ����
 */
VOID CCoinProtocol::Disconnect()
{
	CSerial::Close();
	m_flagConnect = FALSE;		
}

/*
 * Name: GetLRC
 * IN  : BYTE *pbData(���� ������), int iCount(������ ũ��)
 * Out :
 * Desc: �ܷ��� ��ǰ ������üũ�ڵ� ���ϱ�
 */
BYTE CCoinProtocol::GetLRC(_In_ const BYTE* pbData, _In_ INT iCount)
{
	if(pbData == NULL) return 0x00;

	BYTE chLRC = 0;
	while(iCount > 0)
	{
		chLRC ^= *pbData++;
		iCount--;
	}

	return chLRC;
}

VOID  CCoinProtocol::TestMode(_In_ HWND hwnd, BOOL b_testMode)
{
	if(b_testMode == TRUE);
	else;
}

//----------------------------------------------------------------------//
// ���FA ����ó����ġ Operation

/*
 * Name: Connect
 * IN  :
 * Out : ��� ���� �� ���� ���� ����, (TRUE:����, FALSE:����)
 * Desc: �ø��� ��� ���� �� ����
 */
BOOL CCoinProtocol::Connect( CSerial::EBaudrate eBaudrate /* = CSerial::EBaud19200 */, 
							 CSerial::EDataBits eDataBits /* = CSerial::EData8 */, 
							 CSerial::EParity eParity	  /* = CSerial::EParNone */, 
							 CSerial::EStopBits eStopBits /* = CSerial::EStop1 */,
							 INT com_port_no)
{		
	m_port_no = com_port_no;
	
	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	

		Disconnect();

		CreateRecvEvent();
	}

	//LONG lErrorCode = Open(com_port_no, 0, 0, TRUE);	
	LONG lErrorCode = Open(com_port_no);	
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

	m_bSequnceNum = 0x00;
	
	m_flagConnect = TRUE;

	if(m_pThread != NULL)
	{
		m_pThread->ResumeThread();	
	}
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : SendText
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ�� �����͸� �۽�
//----------------------------------------------------------------------//
BOOL CCoinProtocol::SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data/*=NULL*/, IN SIZE_T data_len/*=0*/)
{
	if(CheckDeviceOpen() == FALSE) return FALSE;

	CSerial::Purge();

	m_flagConnect = FALSE;

	for(INT retry_count=0; retry_count < COIN_RETRY_COUNT; retry_count++)
	{
		if(SendText(command_code, p_data, data_len) == TRUE)
		{
			m_flagConnect = TRUE;

			BYTE b_read_byte = 0x00;
			b_read_byte = RecvUnit();

			if(b_read_byte == ACK || b_read_byte == BEL)
			{
				SendCtrlChar(EOT);
				return TRUE;
			}
		}

		Sleep(500);
	}
	return m_flagConnect;
}

//----------------------------------------------------------------------//
// Name : SendPolling
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ�� ����(polling) �۽�
//----------------------------------------------------------------------//
BOOL CCoinProtocol::SendPolling()
{
	if(CheckDeviceOpen() == FALSE) return FALSE;

	T_COIN_TEXT_POLLING_FORMAT send_data;
	ZeroMemory(&send_data, sizeof(T_COIN_TEXT_POLLING_FORMAT));

	send_data.start_code	= ENQ;
	send_data.length		= 0x03;
	send_data.text_type		= COIN_TEXT_TYPE_POLLING;
	send_data.sequence_num	= GetSequenceNum();
	
	m_send_buf_len = sizeof(T_COIN_TEXT_POLLING_FORMAT);
	ZeroMemory(m_send_buf, COIN_MAX_BUFSIZE);
	CopyMemory(&m_send_buf[0], &send_data, sizeof(T_COIN_TEXT_POLLING_FORMAT));

	// CRC setting
	WORD wBCC = crc16_ccitt(m_send_buf + 1, 3);
	m_send_buf[m_send_buf_len]		= HIBYTE(wBCC);
	m_send_buf[m_send_buf_len+1]	= LOBYTE(wBCC);
	
	m_send_buf_len = m_send_buf_len + 2;

	INT retry_cnt = 1;

	do{
		if(WriteBlock(&m_send_buf[0], m_send_buf_len, 500) != ERROR_SUCCESS)
		{
			DWORD dwError = ::GetLastError();

			if(retry_cnt == 3)
			{
				m_flagConnect = FALSE;
				return FALSE;
			}
			else
			{
				retry_cnt++;
				Sleep(500);
			}
		}
		else break;		
	}while(retry_cnt < 4);

	m_flagConnect = TRUE;

	return TRUE;
}

//----------------------------------------------------------------------//
// �ܶ��� ����ó����ġ Operation

/*
 * Name: Connect_Conlux
 * IN  : ���� �� ����
 * Out : ��� ���� �� ���� ��� (TRUE: ����, FALSE: ����)
 * Desc: �ܶ��� ����ó����ġ �ø��� ��� ���� �� ����
 */
BOOL CCoinProtocol::Connect_Conlux(CSerial::EBaudrate eBaudrate /* = CSerial::EBaud4800 */, 
								   CSerial::EDataBits eDataBits /* = CSerial::EData8 */, 
								   CSerial::EParity eParity	  /* = CSerial::EParEven */, 
								   CSerial::EStopBits eStopBits /* = CSerial::EStop1 */)
{
	bool bReset = false;
	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	
		
		Disconnect();

		CreateRecvEvent();
	}

	// �ø��� ����
	LONG lErrorCode = CSerial::Open(m_port_no, 0, 0, TRUE);
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
	// ��� ���� �帧 ����
	lErrorCode = CSerial::SetupHandshaking(EHandshakeOff);
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	SetRtsControl(RTS_HIGH);

	lErrorCode = CSerial::Purge();
	if (lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}	

	m_flagConnect = TRUE;

	if(m_pThread != NULL)
	{
		m_pThread->ResumeThread();	
	}

	return TRUE;
}

/*
 * Name: SendCmdToConlux
 * IN  : const BYTE cmd_code, ��� �ڵ�(����: CONLUX_COIN_COMMAND)
 * Out :
 * Desc: �ܶ��� ����ó����ġ ��� �۽�
 */
BOOL CCoinProtocol::SendCmdToConlux(_In_ const BYTE cmd_code)
{	
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		m_flagConnect = FALSE;
		return FALSE;
	}

	BYTE normal_cmd = cmd_code;
	BYTE reverse_cmd= ~normal_cmd;

	BOOL bRet = ClearBuffer();
	if(bRet == FALSE) return FALSE;

	SetRtsControl(RTS_LOW);

	LONG lErrorCode = CSerial::Write(&normal_cmd, sizeof(BYTE));
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}

	::Sleep(2);

	SetRtsControl(RTS_HIGH);

	lErrorCode = CSerial::Write(&reverse_cmd, sizeof(BYTE));
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	
	return TRUE;
}

/*
 * Name: SendDataToConlux
 * IN  : _In_ BYTE* pbyteData(������), _In_ INT nSize(������ ũ��)
 * Out :
 * Desc: �ܶ��� ����ó����ġ ������ ��� �۽�
 */
BOOL CCoinProtocol::SendDataToConlux(_In_ BYTE* pbyteData, _In_ INT nSize)
{	
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		m_flagConnect = FALSE;
		return FALSE;
	}

	LONG lErrorCode = CSerial::Write(pbyteData, nSize);
	if(lErrorCode != ERROR_SUCCESS)
	{
		return FALSE;
	}
	
	CString strLog;
	strLog = _T("| Module => Protocol | Data: ");
	for(INT i=0; i<nSize; i++){
		strLog.AppendFormat(_T("[%02X]"), pbyteData[i]);		
	}
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : RecvPacket
// IN	: _Out_ BYTE *p_byte(������), _Out_ SIZE_T *p_len(������ ����), IN DWORD dw_timeout
// OUT	: ���� ����
// DESC	: ����ó����ġ ������ ����
//----------------------------------------------------------------------//
BOOL CCoinProtocol::RecvPacket(_Inout_ BYTE *p_byte, _Inout_ SIZE_T *p_len, IN DWORD dw_timeout)
{
	if(CheckDeviceOpen() == FALSE) return FALSE;

	BOOL b_ret = RecvText(dw_timeout);
	
	if(b_ret == FALSE)
	{
		*p_len = 0;
	}
	else
	{
		CopyMemory(p_byte, m_recv_buf, m_recv_buf_len);
		*p_len = m_recv_buf_len;
	}
	return b_ret;
}



//////////////////////////////////////////////////////////////////////////
// private member function

//----------------------------------------------------------------------//
// Name : crc16_ccitt
// IN	: BYTE* databuf(��� ��� data), INT nlen(��� data ����)
// OUT	: ���� crc ��
// DESC	: crc ���� ����Ѵ�.
//----------------------------------------------------------------------//
USHORT CCoinProtocol::crc16_ccitt(_In_ const BYTE* databuf, IN INT nlen)
{
	USHORT	tCrc = 0;

	for(int i=0; i<nlen; i++)
		tCrc = (tCrc<<8) ^ crc16tab[((tCrc>>8) ^ *(char *)databuf++)&0x00FF];

	return tCrc;
}

//----------------------------------------------------------------------//
// Name : GetSequenceNum
// IN	: 
// OUT	: message sequence number
// DESC	: message ���� Ƚ���� ����
//----------------------------------------------------------------------//
BYTE CCoinProtocol::GetSequenceNum()
{
	if(m_bSequnceNum == 0xFF) 
		m_bSequnceNum = 0x00;
	else
		m_bSequnceNum++;

	return m_bSequnceNum;
}

//----------------------------------------------------------------------//
// Name : ReadByte
// IN	: IN DWORD dwTimeOut(Ÿ�Ӿƿ�)
// OUT	: 1Byte ���� ������
// DESC	: ����ó����ġ���� 1Byte�� �о��
//----------------------------------------------------------------------//
INT CCoinProtocol::ReadByte(IN DWORD dwTimeOut)
{
	SetupReadTimeouts(0, 0, dwTimeOut);

	BYTE b_read_char = 0x00;
	DWORD dw_readed;

	LONG l_retcode = Read(&b_read_char, 1, &dw_readed, 0, dwTimeOut);

	if(l_retcode == ERROR_SUCCESS && dw_readed == 1) 
		return b_read_char;

	if(l_retcode == ERROR_TIMEOUT);
	else;

	return -1;
}

//----------------------------------------------------------------------//
// Name : RecvUnit
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ���� ���� ACK, NAK, EOT, BEL �ڷ� �۽�
//----------------------------------------------------------------------//
INT CCoinProtocol::RecvUnit()
{
	INT n_read_char = 0x00;

	n_read_char = ReadByte(500);

	read_ACK = 0;
	switch( n_read_char )
	{
	case ACK: 
		return 1;
		break;
	case NAK: 
		return 1;
		break;
	case EOT: 
		return 1;
		break;
	case BEL: 
		return 1;
		break;
	case -1 : break;		
	default:
		{
			break;
		}
	}//end switch

	return n_read_char;
}

//----------------------------------------------------------------------//
// Name : RecvText
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ���� ���� ������ ����
//----------------------------------------------------------------------//
BOOL CCoinProtocol::RecvText(IN DWORD dwTimeOut)
{
	CString slog = _T("");	
	BYTE	b_readchar = 0x00;
	DWORD	dw_readed;

	m_recv_buf_len = 0;
	ZeroMemory(m_recv_buf, COIN_MAX_BUFSIZE);

	LONG l_retcode = Read(&b_readchar, 1, &dw_readed, 0, 500);

	slog.Format(_T("Recv Packet : [%02X]"), b_readchar);

	if(l_retcode == ERROR_SUCCESS && dw_readed == 1) 
	{
		if(b_readchar == ENQ)
		{
			m_recv_buf[0] = b_readchar;
			m_recv_buf_len++;			
		}
		else
		{
			return FALSE;
		}
	}
	else
		return FALSE;


	b_readchar = 0x00;
	l_retcode = Read(&b_readchar, 1, &dw_readed, 0, 500);	
	slog.AppendFormat(_T(" [%02X]"), b_readchar);
	if(l_retcode == ERROR_SUCCESS && dw_readed == 1) 
	{
		m_recv_buf[1] = b_readchar;
		m_recv_buf_len++;			
	}
	else
		return FALSE;

	INT n_datalen = static_cast<INT>(m_recv_buf[1]);	// ������ ����

	for(INT i=0; i<n_datalen+1; i++)
	{
		b_readchar = 0x00;
		l_retcode = Read(&b_readchar, 1, &dw_readed, 0, 500);	
		slog.AppendFormat(_T(" [%02X]"), b_readchar);
		if(l_retcode == ERROR_SUCCESS && dw_readed == 1) 
		{
			m_recv_buf[i+2] = b_readchar;
			m_recv_buf_len++;
		}
		else
			return FALSE;
	}//end for

	WORD    calc_bcc = 0;
	BYTE    bcc[2];

	calc_bcc = crc16_ccitt(m_recv_buf+1, n_datalen);

	bcc[0] = HIBYTE(calc_bcc);
	bcc[1] = LOBYTE(calc_bcc);

	slog.AppendFormat(_T(" [%02X] [%02X]"), bcc[0], bcc[1]);

	if( (m_recv_buf[n_datalen+1] != bcc[0]) ||
		(m_recv_buf[n_datalen+2] != bcc[1]) )
	{
		SendCtrlChar(NAK);
		return FALSE;
	}

	// Polling�� �ƴ� ������ ������ ���
	if(n_datalen > 3) 
	{
		SendCtrlChar(ACK);
		RecvUnit();
	}		

	return TRUE;
}

//----------------------------------------------------------------------//
// Name : SendText
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ�� �����͸� �۽�
//----------------------------------------------------------------------//
BOOL CCoinProtocol::SendText(IN BYTE command_code, _In_opt_ BYTE *p_data, IN SIZE_T data_len)
{
	CString slog = _T("");
	
	m_send_buf_len = sizeof(T_COIN_TEXT_FORMAT);

	T_COIN_TEXT_FORMAT send_data;
	ZeroMemory(&send_data, sizeof(T_COIN_TEXT_FORMAT));

	send_data.start_code	= ENQ;
	send_data.length		= static_cast<BYTE>(data_len) + 6;
	send_data.text_type		= COIN_TEXT_TYPE_COMMAND;
	send_data.sequence_num	= GetSequenceNum();
	if(command_code == COIN_CMD_PACKET_COIN_CHANGE)
	{
		send_data.f1 = 0x0B;
	}
	else if(command_code == COIN_CMD_PACKET_WITHDRAW)
	{
		send_data.f1 = 0x05;
	}
	else
	{							
		send_data.f1 = 0x03; 
	}
	send_data.f2			= 0x02;
	send_data.text_data		= command_code;

	m_send_buf_len = sizeof(T_COIN_TEXT_FORMAT) + data_len;

	ZeroMemory(m_send_buf, COIN_MAX_BUFSIZE);
	CopyMemory(&m_send_buf[0], &send_data, sizeof(T_COIN_TEXT_FORMAT));
	if(data_len > 0)
		CopyMemory(&m_send_buf[sizeof(T_COIN_TEXT_FORMAT)], p_data, data_len);

	WORD w_bcc = crc16_ccitt(m_send_buf+1, data_len+6);

	m_send_buf[m_send_buf_len]		= HIBYTE(w_bcc);
	m_send_buf[m_send_buf_len+1]	= LOBYTE(w_bcc);

	m_send_buf_len = m_send_buf_len + 2;

	CString s_packet = _T("");
	s_packet = (_T("Send packet: "));
	for(INT i=0; i<(INT)m_send_buf_len; i++)
		s_packet.AppendFormat(_T("[%02x]"), m_send_buf[i]);
	
	LONG lErrorCode = CSerial::Write(m_send_buf, m_send_buf_len);
	if(lErrorCode != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();
		
		return FALSE;
	}	
	return TRUE;
}

//----------------------------------------------------------------------//
// Name : SendCtrlChar
// IN	: 
// OUT	: ���� ����
// DESC	: ����ó����ġ�� ���� ��ȣ (EOT, ACK, NAK, ENQ,...) �۽�
//----------------------------------------------------------------------//
BOOL CCoinProtocol::SendCtrlChar(IN BYTE b_buf)
{
	if(CSerial::Write(&b_buf, 1) != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();
		return FALSE;
	}

	return TRUE;
}

/*
 * Name: SetRtsControl
 * IN  : DWORD _val, (RTS_HIGH: High, RTS_LOW: Low)
 * Out :
 * Desc: RTS ��ȣ High, Low ����
 */
VOID CCoinProtocol::SetRtsControl(_In_ DWORD _val)
{
	if(CSerial::CheckDeviceOpen() == FALSE)
	{
		return;
	}

	CDCB dcb;

	if (!::GetCommState(CSerial::m_hFile,&dcb))
	{		
	}

	// rts ��ȣ ����
	dcb.fRtsControl = _val;

	// Set the new DCB structure
	if (!::SetCommState(CSerial::m_hFile,&dcb))
	{
	}
}

/*
 * Name: _threadEntry
 * IN  : LPVOID pParm, ������ ���� ����
 * Out :
 * Desc: ������ ���� 
 */
UINT CCoinProtocol::_threadEntry(_In_ LPVOID pParm)
{ 
	CCoinProtocol *pSelf = (CCoinProtocol*)(pParm);
	pSelf->_threadRecvEvent();
	return 0;
}

/*
 * Name: _threadRecvEvent
 * IN  :
 * Out :
 * Desc: ���� �̺�Ʈ ó�� �Լ�
 */
VOID CCoinProtocol::_threadRecvEvent()
{
	OVERLAPPED	ov = {0};
	HANDLE		ahWait[2];
	CString		strLog(_T(""));		
	DWORD		dwRet = 0;			
	INT			nEventIndex = 0;
	
	// �̺�Ʈ �ڵ� ����
	ov.hEvent = m_hOverlapped;
	ahWait[0] = m_hOverlapped;
	ahWait[1] = m_hStop;	
	
	while(TRUE)
	{
		if(CSerial::CheckDeviceOpen() == FALSE)
		{
			if(g_bCoinThreadExit){
				return;
			}

			::Sleep(500);
			continue;
		}

		LONG lLastError = CSerial::WaitEvent(&ov);
		if(lLastError != ERROR_SUCCESS)
		{
			if(g_bCoinThreadExit){
				return;
			}

			if(lLastError == ERROR_IO_INCOMPLETE){
			}else if(lLastError == ERROR_IO_PENDING){
			}else{
				// �Ϸ� �ݹ� ȣ��
				CompleteReceiveData(lLastError, 0x00, 0);

				return;
			}
		}

		dwRet = ::WaitForMultipleObjects(2, ahWait, FALSE, INFINITE);
		
		nEventIndex = (INT)(dwRet - WAIT_OBJECT_0);
		
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
							CompleteReceiveData(lLastError, recvBuf, dwBytesRead);
							break;
						}	
						else
						{
							if(dwBytesRead > 0)
							{
								// �α� ���
								strLog.Format(_T("Length:%u, Recv:"), dwBytesRead);
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

