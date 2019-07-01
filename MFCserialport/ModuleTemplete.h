#pragma once

#pragma warning( disable : 4996 )

static const INT DEFAULT_WAIT_TIME		    = 500;
//----------------------------------------------------------------------//
// Class Declare
//----------------------------------------------------------------------//
//template<class T>

class CModuleTemplete 
{
	//----------------------------------------------------------------------//
	// Constructors && Destructor Declare
public:
	CModuleTemplete();
	virtual ~CModuleTemplete();

	//----------------------------------------------------------------------//
	// �����Լ� ����
	//----------------------------------------------------------------------//
	virtual VOID InitWork(){}					// �ʱ�ȭ �Լ�	
	virtual VOID SendCmd(IN INT nCmd) = 0;		// ��� ���� 
	virtual VOID PacketAnalysis()=0;			// ���� �ڵ尡 ���� ��� ����
												// Polling, ���� ��� �� �м� ���� ����
	virtual VOID Working() = 0;					// thread loop
	virtual VOID TestModeON(HWND hwnd){};		// Test Mode
	virtual VOID TestModeOFF(){};	

	//----------------------------------------------------------------------//
	// Operation
	//----------------------------------------------------------------------//
	VOID ExitWork();					

	VOID CommandIn(IN UINT nCmd, OPTIONAL BOOL bImmediately=FALSE);
	INT CommandOut(IN BOOL bGetNextCmd, OPTIONAL DWORD dwTimeOut = DEFAULT_WAIT_TIME);
	VOID CommandReset();

	BOOL  CreateAuditFolder();
	BOOL  CreateBackupAuditFolder();
	DWORD  WriteAduit(_In_ const TCHAR *file_name, _In_ LPVOID pData, _In_ SIZE_T size);
	DWORD  ReadAudit(_In_ const TCHAR *file_name, _Inout_ LPVOID pData, _In_ SIZE_T size);
	DWORD  WriteBackupAduit(_In_ const TCHAR *file_name, _In_ LPVOID pData, _In_ SIZE_T size);
	DWORD  ReadBackupAudit(_In_ const TCHAR *file_name, _Inout_ LPVOID pData, _In_ SIZE_T size);

	//----------------------------------------------------------------------//
	// Member Function Declare
protected:
	HANDLE						m_hExitThread;	// ���� �̺�Ʈ �ڵ�	
	CWinThread*					m_pThread;		
	TCHAR						m_lpFolderPath[MAX_PATH];
	TCHAR						m_lpBackupFolderPath[MAX_PATH];

private:
	static CCriticalSection		m_cs;			// critical section
	static CUIntArray			m_OrderList;	// �۾� ���
	HANDLE						m_hOrder;		// �̺�Ʈ �ڵ� 
};