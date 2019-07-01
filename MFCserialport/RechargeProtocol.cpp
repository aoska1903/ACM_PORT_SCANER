#include "stdafx.h"
#include "RechargeProtocol.h"
#include "ASCIICode.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)              if (p) { delete (p); (p) = NULL; }
#endif

// 보충 장치
static const INT CHARGE_RECV_TIMEOUT		= 1000;

INT CRechargeProtocol::m_wait_time = 0;
static bool g_bRFThreadExit = false;

CRechargeProtocol::CRechargeProtocol(void) :
	m_bConnect(FALSE),
	m_recv_buf_len(0),
	m_send_buf_len(0),
	m_pThread(NULL)
{
	memset(&chk, 0x00, sizeof(ACM_PACKET_CHK));

	g_bRFThreadExit = false;

}


CRechargeProtocol::~CRechargeProtocol(void)
{
	g_bRFThreadExit = true;

	DeleteRecvEvent();

	Disconnect();	
}

VOID  CRechargeProtocol::NewFileLog()
{
}

/*
 * Name: CreateRecvEvent
 * IN  :
 * Out :
 * Desc: 수신 이벤트 핸들 및 작업 스레드 생성
 */
BOOL CRechargeProtocol::CreateRecvEvent()
{
	m_hOverlapped = ::CreateEvent(0, TRUE, FALSE, NULL);
	if(m_hOverlapped == 0)
	{
		return FALSE;
	}		

	// 스레드 종료를 위한 핸들 생성
	m_hStop = ::CreateEvent(0, TRUE, FALSE, NULL);
	if(m_hStop == 0)
	{
		return FALSE;
	}

	// 스레드 생성
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
 * Desc: 수신 이벤트 핸들 및 작업 스레드 해제
 */
VOID CRechargeProtocol::DeleteRecvEvent()
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
 * Name: Connect
 * IN  :
 * Out :
 * Desc: 통신 연결
 */
BOOL  CRechargeProtocol::Connect(CSerial::EBaudrate eBaudrate/* = CSerial::EBaud115200*/,
			                     CSerial::EDataBits eDataBits/* = CSerial::EData8*/,
			                     CSerial::EParity   eParity  /* = CSerial::EParNone*/,
			                     CSerial::EStopBits eStopBits/* = CSerial::EStop1*/)			                     
{
	if(CSerial::CheckDeviceOpen() == TRUE)
	{		
		DeleteRecvEvent();	

		Disconnect();

		CreateRecvEvent();
	}	

	LONG l_err_code = 0;

	l_err_code = Open(m_com_port);
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

	l_err_code = SetMask( EEventRecv);
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
		
	if(m_pThread != NULL){
		m_pThread->ResumeThread();
	}

	return TRUE;
}


VOID  CRechargeProtocol::Disconnect()
{
	Close();
}

//----------------------------------------------------------------------//
// Name : SendPacket
// IN	: IN BYTE command_code, _In_opt_ BYTE *p_data, OPTIONAL SIZE_T data_len
// OUT	: 
// DESC	: 패킷 송신 처리
//----------------------------------------------------------------------//
BOOL  CRechargeProtocol::SendPacket(IN BYTE command_code, _In_opt_ BYTE *p_data/*=NULL*/, OPTIONAL SIZE_T data_len/*=0*/)
{
	BOOL b_ret = FALSE;

	b_ret = SendText(command_code, p_data, data_len);

	if(b_ret == FALSE)
	{
		m_bConnect = FALSE;
	}

	return b_ret;	
}


VOID  CRechargeProtocol::TestMode(_In_ HWND hwnd, BOOL b_testMode)
{
	if(b_testMode == TRUE);
	else;
}


//----------------------------------------------------------------------//
// Name : CalculateBCC
// IN	: _In_ BYTE *p_data, _In_ INT n_len
// OUT	: BCC
// DESC	: BCC 구하는 함수
//----------------------------------------------------------------------//
BYTE  CRechargeProtocol::CalculateBCC(_In_ BYTE *p_data, _In_ INT n_len)
{	
	BYTE    *lpSource = p_data;
	BYTE    bBCC = 0;
	for ( INT i = 0; i<n_len; i++ )
	{
		bBCC ^= *lpSource;
		lpSource++;
	}
	return bBCC;
}

//----------------------------------------------------------------------//
// Name : SendText
// IN	: IN BYTE command_code, _In_ BYTE *p_data, IN SIZE_T data_len
// OUT	: TRUE(송신 성공), FALSE(송신 실패) 
// DESC	: 장치로 송신 할 패킷 생성 후 송신
//----------------------------------------------------------------------//
BOOL  CRechargeProtocol::SendText(IN BYTE command_code, _In_ BYTE *p_data, IN SIZE_T data_len)
{	
	ULONG	tmpDataLen;
	
	memset(m_send_buf, NULL, sizeof(m_send_buf));
	tmpDataLen		= 0;
	m_send_buf_len	= 0;

	m_send_buf[m_send_buf_len++] = STX;
	m_send_buf[m_send_buf_len++] = command_code;
	// 데이터 길이 
	m_send_buf[m_send_buf_len++] = (data_len>>8)&0xff;
	m_send_buf[m_send_buf_len++] = data_len&0xff;

	if(data_len > 0)
	{
		CopyMemory(&m_send_buf[m_send_buf_len], p_data, data_len);
		m_send_buf_len += data_len;
	}

	// ETX
	m_send_buf[m_send_buf_len++] = ETX;

	// BCC 
	m_send_buf[m_send_buf_len++] = CalculateBCC(m_send_buf, data_len+5);

	// write packet data for log
	CString packet_log = _T("");	
	packet_log=_T("| Send | => |");	
	for(INT i=0; i<(INT)m_send_buf_len; i++)
		packet_log.AppendFormat(_T("0x%02x|"),m_send_buf[i]);		

	// 4. write packet data
	
	if(WriteBlock(m_send_buf, m_send_buf_len, INFINITE) != ERROR_SUCCESS)
	{
		DWORD dwError = ::GetLastError();		
		return FALSE;
	}
	
	m_wait_time = 5;

	return TRUE;
}


//----------------------------------------------------------------------//
// Name : RecvText
// IN	: 
// OUT	: 
// DESC	: 수신 패킷을 읽는 함수
//----------------------------------------------------------------------//
VOID CRechargeProtocol::RecvText()
{
	DWORD	dw_evt_mask		= 0;		// event mask
	DWORD	dw_readed		= 0;		// 수신 버퍼에서 읽어온 문자 수 
	LONG	l_retcode		= 0;		// Read 함수 결과 값
	BYTE    recv_bcc        = 0x00;
	BYTE    tmp_buf[1000];				// 수신 데이터를 저장 할 임시 로컬 버퍼
	CString slog = _T("");

	m_recv_buf_len	= 0;

	ZeroMemory(tmp_buf,    sizeof(BYTE)*1000);	
	ZeroMemory(m_recv_buf, sizeof(BYTE)*MAX_BUFFER);

	dw_readed  = 0;

	l_retcode = Read(tmp_buf, sizeof(tmp_buf)-1, &dw_readed, 0, 100);

	if ( dw_readed > 0 )
	{

		for (INT i = 0 ; i < (INT)dw_readed; i++ ) 
		{
			if ( chk_packet(tmp_buf[i]) > 0 )
			{
				m_recv_buf_len = get_packet(m_recv_buf);

				if(m_recv_buf[CHARGE_PACKET_INDEX_CMD] != CHARGE_CMD_POLL)
				{
					slog = _T("| Recv | => |");
					for(INT j=0; j<(INT)m_recv_buf_len; j++)
						slog.AppendFormat(_T("%02x "), m_recv_buf[j]);
					slog += _T("|");
				}
				ProcRecvEvent(l_retcode, m_recv_buf, m_recv_buf_len);
			}
		}//end for
	}
}


//----------------------------------------------------------------------//
// Name : _ThreadEntry
// IN	: _In_ LPVOID pParam
// OUT	: 
// DESC	: main work thread 
//----------------------------------------------------------------------//
UINT CRechargeProtocol::_ThreadEntry(_In_ LPVOID pParam)
{
	CRechargeProtocol *pSelf = static_cast<CRechargeProtocol*>(pParam);
	pSelf->_threadRecvEvent();
	return 0;
}


//----------------------------------------------------------------------//
// Name : WatchReceiveEvent
// IN	: 
// OUT	: 
// DESC	: receive event loop
//----------------------------------------------------------------------//
VOID  CRechargeProtocol::WatchReceiveEvent()
{		
	HANDLE hEvent;
	OVERLAPPED lpOverlapped;

	hEvent = ::CreateEvent(0, TRUE, FALSE, 0);
	memset(&lpOverlapped,0,sizeof(lpOverlapped));
	lpOverlapped.hEvent = hEvent;

	while(TRUE)
	{
		m_bConnect = CheckDeviceOpen();
		if(m_bConnect == FALSE)
		{
			Sleep(1000);
			continue;
		}

		if(!::WaitCommEvent(m_hFile, LPDWORD(&m_eEvent), &lpOverlapped))
		{			
			long lLastError = ::GetLastError();

			if (lLastError != ERROR_IO_PENDING)
			{
				continue;
			}

			switch (::WaitForSingleObject(lpOverlapped.hEvent, CHARGE_RECV_TIMEOUT))
			{
			case WAIT_OBJECT_0:
				{
					m_wait_time = 0;
					RecvText();
					break;
				}
			case WAIT_TIMEOUT:
				{
					// Cancel the I/O operation
					CancelCommIo();	
					break;
				}
			default:
				{
					lLastError = ::GetLastError();
					break;
				}
			}//end switch
		}
		else
			RecvText();
	}//end while
}


/*
 * Name: _threadRecvEvent
 * IN  :
 * Out :
 * Desc: 수신 이벤트 처리 함수
 */
VOID CRechargeProtocol::_threadRecvEvent()
{
	OVERLAPPED	ov = {0};
	HANDLE		ahWait[2];
	CString		strLog(_T(""));		
	DWORD		dwRet = 0;			
	INT			nEventIndex = 0;
	
	// 이벤트 핸들 저장
	ov.hEvent = m_hOverlapped;
	ahWait[0] = m_hOverlapped;
	ahWait[1] = m_hStop;	
	
	while(TRUE)
	{
		if(CSerial::CheckDeviceOpen() == FALSE)
		{
			if(g_bRFThreadExit){
				return;
			}

			::Sleep(500);
			continue;
		}

		LONG lLastError = CSerial::WaitEvent(&ov);
		if(lLastError != ERROR_SUCCESS)
		{
			if(g_bRFThreadExit){
				return;
			}

			if(lLastError == ERROR_IO_INCOMPLETE){
			}else if(lLastError == ERROR_IO_PENDING){
			}else{
				// 완료 콜백 호출
				ProcRecvEvent(0x00, 0, lLastError);

				return;
			}
		}

		dwRet = ::WaitForMultipleObjects(2, ahWait, FALSE, INFINITE);

		nEventIndex = (INT)(dwRet - WAIT_OBJECT_0);

		::ResetEvent( ahWait[nEventIndex] );

		switch(nEventIndex)
		{
		case 0:	// 시리얼 통신 이벤트
			{
				// 이벤트 저장
				const CSerial::EEvent eEvent = CSerial::GetEventType();

				// 데이터 수신 이벤트인 경우
				if (eEvent & CSerial::EEventRecv){
					RecvText();
				}//end if
				break;
			}
		case 1:	// 종료 이벤트
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


int CRechargeProtocol::chk_packet(unsigned char input)
{
	switch( chk.r_state ) 
	{
	case ACM_R_STATE_IDLE:
	case ACM_R_STATE_STX:
		//chk.dle_flag = 0; 
		chk.p_index = 0; chk.i_sub = 0 ;
		if ( input == STX )
		{
			chk.p_buffer[ chk.p_index ++]  = input;	//	input
			chk.r_state = ACM_R_STATE_CMD; // STX 수신 완료  ==> 길이 수신대기  
		}
		break;
	case ACM_R_STATE_CMD:
		chk.p_buffer[ chk.p_index ++]  = input;	//	input
		chk.r_state = ACM_R_STATE_LEN_H; // STX 수신 완료  ==> 길이 수신대기  
		break;
	case ACM_R_STATE_LEN_H:			
		chk.p_buffer[ chk.p_index ++]  = input;	//	input		
		chk.d_size  =  input<<8;
		chk.r_state = ACM_R_STATE_LEN_L;	    // LENGTH HIGH
		break;
	case ACM_R_STATE_LEN_L:			
		chk.p_buffer[ chk.p_index ++]  = input;	//	input		
		chk.d_size |= input;

		if ( chk.d_size == 0 ) 
		{
			chk.r_state = ACM_R_STATE_ETX;	// to receive etx 
		}
		else if ( (chk.d_size > ACM_MAX_PROTOCOL_SIZE) || ( chk.d_size < 0 ) )
		{
			chk.r_state = ACM_R_STATE_IDLE; 
		}
		else if ( chk.d_size > 0 ) 
		{
			chk.r_state = ACM_R_STATE_DATA;	// to receive data 
		}
		break;
	case ACM_R_STATE_DATA:

		chk.i_sub ++ ;
		chk.p_buffer[ chk.p_index ++]  = input;	//	input				
		// LEN의 길이만큼 input 받기
		if(chk.i_sub < chk.d_size)
		{
			chk.r_state = ACM_R_STATE_DATA;		
		}
		else
		{
			chk.r_state = ACM_R_STATE_ETX;
		}
		break;
	case ACM_R_STATE_ETX:
		if(input == ETX)
		{
			chk.r_state = ACM_R_STATE_BCC;
			chk.p_buffer[ chk.p_index++] = input;
		}
		else
		{
			chk.r_state = ACM_R_STATE_IDLE;
		}

		break;

	case ACM_R_STATE_BCC:
		if ( input == CalculateBCC( chk.p_buffer , chk.p_index  ))
		{ 
			chk.p_buffer[ chk.p_index++] = input;
			chk.r_state = ACM_R_STATE_IDLE ; 
			// return valid packet size ...
			return chk.p_index;
		}
		else
		{
			chk.r_state = ACM_R_STATE_IDLE ; 
		}
		break;
	default:
		// STATE MACHINE ERROR
		chk.r_state = ACM_R_STATE_IDLE ; 
		break;
	}
	return 0;
}


int CRechargeProtocol::get_packet(unsigned char * p)
{
	memcpy(p,chk.p_buffer,chk.p_index);
	return chk.p_index;	
}
