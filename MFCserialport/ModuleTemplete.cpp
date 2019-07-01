#include "stdafx.h"
#include "ModuleTemplete.h"
#include "Ini.h"

CCriticalSection CModuleTemplete::m_cs;
CUIntArray CModuleTemplete::m_OrderList;

//----------------------------------------------------------------------//
// Constructors && Destructor Declare
CModuleTemplete::CModuleTemplete()
{
	m_pThread = NULL;
	m_hOrder = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CModuleTemplete::~CModuleTemplete()
{		
}

//----------------------------------------------------------------------//
// Name : ExitWork
// IN	: 
// OUT	: 
// DESC	: 종료 처리 함수
//----------------------------------------------------------------------//	
VOID CModuleTemplete::ExitWork()
{
	// 명력 목록 제거
	CommandReset();
	if(m_hOrder) CloseHandle(m_hOrder);
}	

//----------------------------------------------------------------------//
// Name : CommandIn
// IN	:  UINT nCmd(수행 코드), BOOL bImmediately(최우선 수행 명령)
// OUT	: 
// DESC	: 작업 목록에 수행 코드 기록
//----------------------------------------------------------------------//
VOID CModuleTemplete::CommandIn(_In_ UINT nCmd, _In_opt_ BOOL bImmediately/*=FALSE*/)
{
	BOOL bExistCmd = FALSE;
	CSingleLock single_lock(&m_cs);
	single_lock.Lock();
	{
		// 같은 명령이 아닌 경우
		if( (m_OrderList.GetSize() > 0) &&  (m_OrderList.GetAt(m_OrderList.GetSize() - 1) == nCmd) ){
			bExistCmd = TRUE;			
		}else{
			if(bImmediately == TRUE){
				m_OrderList.InsertAt(0, nCmd);
			}else{
				m_OrderList.Add(nCmd);
			}
		}
	}
	single_lock.Unlock();
}

//----------------------------------------------------------------------//
// Name : CommandOut
// IN	: BOOL bGetNextCmd(명령 코드를 가져올지 판단하는 플래그),DWORD dwTimeOut(대기 시간)
// OUT	: 목록
// DESC	: 작업 목록에 첫 번째 수행 코드 리턴
//		  목록이 비어 있는 경우 대기 시간 만큼 대기 (Sleep 효과)
//	      대기 시간 중 목록에 수행 코드가 들어오면 해당 코드 리턴
//----------------------------------------------------------------------//
INT CModuleTemplete::CommandOut(BOOL bGetNextCmd, DWORD dwTimeOut/*= DEFAULT_WAIT_TIME*/)
{
	INT  nCmd = 0;	
	INT _size = 0;
	CSingleLock single_lock(&m_cs);
	single_lock.Lock();
	{		
		if(m_OrderList.GetSize() > 0)
		{
			nCmd = static_cast<INT>(m_OrderList.GetAt(0));
			m_OrderList.RemoveAt(0);
		}
	}
	single_lock.Unlock();
	return nCmd;
}

//----------------------------------------------------------------------//
// Name : CommandReset
// IN	: 
// OUT	: 
// DESC	: 작업 목록 리셋
//----------------------------------------------------------------------//
VOID CModuleTemplete::CommandReset()
{
	CSingleLock single_lock(&m_cs);
	single_lock.Lock();

	m_OrderList.RemoveAll();
	m_OrderList.FreeExtra();

	single_lock.Unlock();
}

//----------------------------------------------------------------------//
// Name : CreateAuditFile
// IN	: 
// OUT	: 
// DESC	: 장비 보유 회계 파일 생성
//----------------------------------------------------------------------//	
BOOL CModuleTemplete::CreateAuditFolder()
{
	TCHAR  curFolderPath[MAX_PATH];         

	// 현재 경로
	GetCurrentDirectory( MAX_PATH, curFolderPath);
	_stprintf(m_lpFolderPath, _T("%s\\Data\\AuditFile\\"), curFolderPath);
	LPTSTR  lpTemp = NULL;
	TCHAR   szCreatePath[MAX_PATH];
	DWORD   dwFileAttr;

	lpTemp = (LPTSTR)m_lpFolderPath;
	while ( TRUE )
	{
		if ( *lpTemp == _T('\\') || *lpTemp == _T('/') || *lpTemp == _T('\0') )
		{
			ZeroMemory( szCreatePath, MAX_PATH );
			_tcsncpy_s( szCreatePath, m_lpFolderPath, (lpTemp - m_lpFolderPath) );
			dwFileAttr = GetFileAttributes( szCreatePath );
			if ( (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) == FALSE || 
				(dwFileAttr == INVALID_FILE_ATTRIBUTES) == TRUE )
			{
				if ( CreateDirectory( szCreatePath, NULL ) == FALSE )
					break;
			}
		}

		if ( *lpTemp == _T('\0') )	break;
		lpTemp = _tcsinc( lpTemp);
	}
	// 회계 백업 폴더 생성
	CreateBackupAuditFolder();

	return  TRUE;
}

BOOL CModuleTemplete::CreateBackupAuditFolder()
{
	// 백업 회계 저장 폴더
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
	_stprintf(iniFilePath, _T("%s\\Ini\\afc_config.ini"), curFolderPath);				
	
	CIni _config_ini(iniFilePath);
	// 장비 타입
	int _nDevType = _config_ini.GetInt("CONFIG", "DeviceType", 4);

	// 백업 경로
	CString _strBackupFolder = _config_ini.GetString("OPERATION PARAMETER", "BackupFolder", "0");
	CString _strBackupPath; _strBackupPath.Format("");

	if(_strBackupFolder == "0" || _strBackupFolder.Find(":\\") == -1)
	{		
		// 백업 경로
		if(_nDevType == 4){			
			_strBackupFolder = _T("E:\\_Backup");			
		}else{
			_strBackupFolder = _T("D:\\_Backup");			
		}
		_config_ini.WriteString("OPERATION PARAMETER", "BackupFolder", _strBackupFolder);
	}

	_stprintf_s(m_lpBackupFolderPath, _T("%s\\Data\\AuditFile\\"), _strBackupFolder);		

	LPTSTR  lpTemp = NULL;
	TCHAR   szCreatePath[MAX_PATH];
	DWORD   dwFileAttr;

	lpTemp = (LPTSTR)m_lpBackupFolderPath;

	while ( TRUE )
	{
		if ( *lpTemp == _T('\\') || *lpTemp == _T('/') || *lpTemp == _T('\0') )
		{
			ZeroMemory( szCreatePath, MAX_PATH );
			_tcsncpy_s( szCreatePath, m_lpBackupFolderPath, (lpTemp - m_lpBackupFolderPath) );
			dwFileAttr = GetFileAttributes( szCreatePath );
			if ( (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) == FALSE || 
				(dwFileAttr == INVALID_FILE_ATTRIBUTES) == TRUE )
			{
				if ( CreateDirectory( szCreatePath, NULL ) == FALSE )
					return FALSE;
			}
		}
		if ( *lpTemp == _T('\0') )	break;
		lpTemp = _tcsinc( lpTemp);
	}
	return  TRUE;
}

//----------------------------------------------------------------------//
// Name : WriteAduit
// IN	: 
// OUT	: 
// DESC	: 장비 보유 회계 데이터 쓰기
//----------------------------------------------------------------------//	
DWORD CModuleTemplete::WriteAduit(_In_ const TCHAR *file_name, _In_ LPVOID pData, _In_ SIZE_T size)
{
	TCHAR  szFileName[MAX_PATH];
	_stprintf( szFileName, _T( "%s%s" ), m_lpFolderPath, file_name);

	FILE *p_file = fopen(szFileName, _T("wb")); 
	if(p_file == NULL)
	{
		DWORD dwErr;
		dwErr = WriteBackupAduit(file_name, pData, size);
		return dwErr;
	}

	fwrite(pData, 1, size, p_file); 
	fclose(p_file); 

	WriteBackupAduit(file_name, pData, size);

	return ERROR_SUCCESS;
}

//----------------------------------------------------------------------//
// Name : ReadAudit
// IN	: 
// OUT	: 
// DESC	: 장비 보유 회계 데이터 읽기
//----------------------------------------------------------------------//	
DWORD CModuleTemplete::ReadAudit(_In_ const TCHAR *file_name, _Inout_ LPVOID pData, _In_ SIZE_T size)
{
	TCHAR  szFileName[MAX_PATH];
	_stprintf( szFileName, _T( "%s%s" ), m_lpFolderPath, file_name);
	
	FILE *p_file = fopen(szFileName, _T("rb")); 
	if(p_file == NULL)
	{
		DWORD dwErr;
		// 백업 데이터 읽기
		dwErr = ReadBackupAudit(file_name, pData, size);	
		return dwErr;
	}

	fread(pData, 1, size, p_file); 
	fclose(p_file); 
	return ERROR_SUCCESS;
}

//----------------------------------------------------------------------//
// Name : WriteAduit
// IN	: 
// OUT	: 
// DESC	: 장비 보유 회계 데이터 쓰기
//----------------------------------------------------------------------//	
DWORD CModuleTemplete::WriteBackupAduit(_In_ const TCHAR *file_name, _In_ LPVOID pData, _In_ SIZE_T size)
{
	TCHAR  szFileName[MAX_PATH];
	_stprintf( szFileName, _T( "%s%s" ), m_lpBackupFolderPath, file_name);

	FILE *p_file = fopen(szFileName, _T("wb")); 
	if(p_file == NULL)
	{
		DWORD dwErr;
		dwErr = ::GetLastError();
		return dwErr;
	}

	fwrite(pData, 1, size, p_file); 
	fclose(p_file); 
	return ERROR_SUCCESS;
}

//----------------------------------------------------------------------//
// Name : ReadAudit
// IN	: 
// OUT	: 
// DESC	: 장비 보유 회계 데이터 읽기
//----------------------------------------------------------------------//	
DWORD CModuleTemplete::ReadBackupAudit(_In_ const TCHAR *file_name, _Inout_ LPVOID pData, _In_ SIZE_T size)
{
	TCHAR  szFileName[MAX_PATH];
	_stprintf( szFileName, _T( "%s%s" ), m_lpBackupFolderPath, file_name);

	FILE *p_file = fopen(szFileName, _T("rb")); 
	if(p_file == NULL)
	{
		DWORD dwErr;
		dwErr = ::GetLastError();
		return dwErr;
	}

	fread(pData, 1, size, p_file); 
	fclose(p_file); 
	return ERROR_SUCCESS;
}