#pragma once
#include "moduletemplete.h"
#include "CoinProcessorData.h"
#include "TemplateTimer.h"
#include <functional>
//#include "Ini.h"

typedef std::function<VOID(_In_ const T_COIN_DATA*)> T_OPERATION_COMPLETE;

#ifndef WIN32
#pragma pack(push,1)
#else
#include <pshpack1.h>
#endif

//����ó����ġ ����
typedef struct _COIN_STATUS 
{
	UINT	CREMON:				1;	// ��ȭ���԰��� ����
	UINT	INVENTORY:			1;	// �κ��丮 ���� ����
	UINT	CHANGE:				1;	// �ܵ� ���� ���� ����
	UINT	RETURN_SWON:		1;	// ��ȯ ����ġ ON 
	UINT	CHANGE_COMPLETE:	1;	// ���� ���� 
	UINT	CLEAR_COMPLETE:		1;	// Ŭ���� �Ϸ�
	UINT	INVENTORY_STAT:		1;	// �κ��丮 ���� ���� ����
	UINT	U8:					1;	// �׻� '0'
}T_CONLUX_STATUS;

union U_CONLUX_STATUS
{
	BYTE				data;
	T_CONLUX_STATUS		bit_data;
};

//����ó����ġ �̻� ������
typedef struct _COIN_ERROR 
{
	UINT	COINCHANGER:		1;	// ��ǥ �̻�
	UINT	WORKABLE:			1;	// ���۰�
	UINT	U3:					1;	// ����
	UINT	ACCEPTOR:			1;	// �Ǽ��� �̻�
	UINT	EMPTYSW_10:			1;	// ����Ƽ ����ġ �̻� (10��)
	UINT	EMPTYSW_50:			1;	// ����Ƽ ����ġ �̻� (50��)
	UINT	EMPTYSW_100:		1;	// ����Ƽ ����ġ �̻� (100��)
	UINT	EMPTYSW_500:		1;	// ����Ƽ ����ġ �̻� (500��)
	UINT	U9:					1;	// ����
	UINT	U10:				1;	// ����
	UINT	U11:				1;	// ����
	UINT	U12:				1;	// ����
	UINT	RETURN_SW:			1;	// ��ȯ ����ġ �̻�
	UINT	COIN_RETURN_ERROR:	1;	// ��ȭ ���� �ҷ�
	UINT	SAFTY_SW:			1;	// ����Ƽ ����ġ �̻�
	UINT	U16:				1;	// ����
}T_CONLUX_ERROR;

union U_CONLUX_ERROR
{
	BYTE				data[2];
	T_CONLUX_ERROR		bit_data;
};

// ��� ������ ����ü
typedef struct _st_com_info
{
	BYTE coin_command;	// ����ó����ġ �ڸ��
	BYTE response_type;	// ���� (ie, ACK11, ACK22, ...) 	
	BYTE data_cmd;		// ������ Ŀ���
	BYTE ctrl_type;		// ������� Ÿ��(0: CREM OFF, 1:CREM ON, 2:CLEAR, ...) 
	BYTE recv_buf[128]; // ���� ������
	INT retry_cnt;		// ��õ� Ƚ��
	INT data_size;		// ���� ������ ũ��
	bool is_data_send;	// ������ ���� ����(false: ������ ���� ����, true:������ ���� �� ���� ���)
}ST_CONLUX_COM_INFO;

// restore packing
#ifndef WIN32
#pragma pack(pop)
#else
#include <poppack.h>
#endif

class CCoinProtocol;

class CConluxCoinModule :
	public CModuleTemplete
{
public:
	CConluxCoinModule(void);
	virtual ~CConluxCoinModule(void);

//////////////////////////////////////////////////////////////////////////
// Member Function
public:
	// ��� ���� �Ϸ� �÷��� �� ��������
	BOOL GetOperationCompleteFlag();

	// �ʱ�ȭ �Լ�
	VOID InitWork();	

	// ���� ��� �� �м� ó��
	VOID ReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	// �޸�, ��ü ���� �� ���� �Լ�
	VOID ReleaseWork();	

	// ��� ���� �Ϸ� �� ȣ��Ǵ� �ݹ� �Լ� ���
	VOID RegCallbackFunc_OperComp(T_OPERATION_COMPLETE _func){
		OperationComplete = std::move(_func);
	}

	// ���� �ݾ� ���� ����
	VOID SetChangesData(_In_ const T_COIN_CHANGES *pChanges);
	VOID SetChangesData(_In_ const INT pChanges);

	// ��Ʈ ���� �Լ�
	VOID SetComPort(_In_ INT _nPort){
		m_nComPortNo = _nPort;
	}
	
	// �α� ���� ����Ʈ ����
	//VOID SetFileLog(_In_ CFileLog* _pLog);
	
	// ���� ��� �� ����
	VOID SetResult(_In_ const INT _ret);

	// Ÿ�Ӿƿ� ���� Ÿ�̸� ����/����
	VOID TimeoutTimerStart(_In_ INT nInterval=1000);
	VOID TimeoutTimerStop();	
	VOID TimeoutEvent();

	// ���� �ݱ� ��� Ÿ�̸� ����/����
	VOID ShutterCloseTimerStart(_In_ INT nInterval=500);
	VOID ShutterCloseTimerStop();	
	VOID ShutterCloseEvent();

	// �׽�Ʈ �α� ���
	VOID TestModeLog(_In_ HWND _hwnd, _In_opt_ BOOL _bTestMode=TRUE);
	
private:	
	// BCD -> DEC Convert (256 �̸�)
	static inline int bcd2dec(BYTE hex){
		return (((hex & 0xF0) >> 4) * 10 + (hex & 0x0F));		
	}  

	// DEC -> BCD Convert (100 �̸�)
	static inline BYTE dec2bcd(INT dec){
		BYTE upper = dec / 10 ;
		BYTE lower = dec % 10 ; 
		return (upper << 4) | lower ; 
	}

	// �˶� �߻� �� ����
	VOID AlarmOccurred(_In_ TCHAR* pCode, _In_opt_ BOOL bRelease=FALSE);

	// ȸ�� ����
	VOID AuditData();

	// ����, ���� �� ȸ�� ������ �ʱ�ȭ
	VOID ClearAuditData();

	// ����ó����ġ�� ��� �۽�
	BOOL CmdSendToCoin();

	// ����ó����ġ ������ ��� �۽�
	BOOL DataCmdSendToCoin(_In_ const INT _coin_cmd);

	// ����ü������ ������ �۽� �÷��� ����
	bool IsDataSend();

	// ��Ŷ �м� �Լ�
	VOID PacketAnalysis(){}
	INT StatusPacketAnalysis();				

	// ���� �� ���� ��� ������ �м�
	VOID RecvDataAnalysis(_In_ const BYTE *pData, _In_ const INT nSize);

	// ������ ��� ��õ�
	VOID RetryCOM(_In_opt_ BOOL _bNotConnectError=FALSE);

	// ��û ��� ��� ��� �� �̺�Ʈ ȣ��
	VOID ResponseResultIn();			

	// ���� �޸� �ʿ� ��û ��� ������ ���� 
	VOID ResponseResultOut(){};			

	// Retry ī���� �ʱ�ȭ
	VOID ResetRetryCnt();

	// ��û ��� �ڵ� Get/Set
	INT GetCommand();
	VOID SetCommand(_In_ INT &_cmd);	

	// ����ó����ġ ��� �ڵ� Get/Set
	BYTE GetCoinCmd();
	VOID SetCoinCmd(_In_ BYTE _cmd, _In_opt_ BYTE _data_cmd = 0x00, _In_opt_ BYTE _ctrl_type=0xFF);	

	// ���� ������� Ŀ���
	BYTE GetCoinCtrlCmd();

	// ������ Ŀ��Ʈ ����
	BYTE GetDataCmd();

	// ��û ��� �ڵ� �м� �� ����
	VOID SendCmd(_In_ INT _command);

	// ���� ��� ���� ���� �Լ�
	BOOL SendCoinCmd(_In_ BOOL _bDataCmd, _In_opt_ BYTE _cmd, _In_opt_ BYTE _data_cmd = 0x00, _In_opt_ BYTE _ctrl_cmd=0xFF);

	// ��� ���� �Ϸ� �÷��� ����
	VOID SetOperationCompleteFlag(_In_ BOOL bComplete);

	// ������ �Լ� (Working ȣ��)
	static UINT _threadEntry(_In_ LPVOID);

	// ��ŷ ������
	VOID Working();				 

	// �ŷ� ��� ����
	INT procCancel();

	// ���� ��� ����
	INT procChange();

	// Ŭ���� ����
	INT procClear();

	// ���� �ݱ�
	INT procClose();

//////////////////////////////////////////////////////////////////////////
// Member Variable
public:
	static CCriticalSection m_cs;

private:
	// ��Ʈ ��ȣ
	INT m_nComPortNo;

	// �ø��� ��� ���� Ŭ���� ��ü
	CCoinProtocol* m_protocol;

	// ����ó����ġ ���� ������ ����ü ����Ʈ		
	static T_COIN_DATA *m_pData;	

	// ����ó����ġ ��� ���� ����ü
	static ST_CONLUX_COM_INFO *m_pstPack;

	// ���� ��� �Ϸ� �ݹ� �Լ�
	T_OPERATION_COMPLETE OperationComplete;

	// �ø��� ��� Ÿ�Ӿƿ� ���� Ÿ�̸�
	//TTimer<CConluxCoinModule> m_tm_timeout;

	// ���� �ݱ� ��� Ÿ�Ӿƿ� 
	//TTimer<CConluxCoinModule> m_tm_close;

	// ����ó����ġ ����
	U_CONLUX_STATUS m_status;
	U_CONLUX_STATUS  m_bef_stat;

	// ����ó����ġ ����
	U_CONLUX_ERROR  m_error_stat;
	U_CONLUX_ERROR  m_bef_error_stat;

	// ����ó����ġ �˶� ��(Map)
	CMap<CString, LPCTSTR, T_COIN_ALARM, T_COIN_ALARM&> map_alarm;

	// ���� ���� �ݾ� ���� ������
	T_COIN_CHANGES m_changes;

	// ���� Ʃ�� Ż�� ���� �÷���
	BOOL m_bCoinTubeMissed;

	// ���� �ݱ� ���� �÷���
	BOOL m_bCloseShutter;

	// ���� �ŷ� ��� ���� �� �ݾ� ��ȯ ���� �Ǵ��ϴ� �÷���
	BOOL m_bCointReturn; // TRUE: �ݾ� ��ȯ, FALSE: �ݾ� ��ȯ ����

	// ���� �ݾ� ���� ���� ������ �Ǵ��ϴ� �÷���
	BOOL m_bChangeComplete;
	
	// ���� ���� �غ� �÷���
	bool m_bCoinReady;

	// ���� ���� �÷���
	bool m_bTestMode;

	// ���� ���� ���� ���
	INT m_nCoinChangeRet;

	// �� �ݾ� �ӽ� ����
	INT m_nTmpAmt;

	BYTE m_nCurCtrlCmd;

	//CIni m_ini_ver;
};

