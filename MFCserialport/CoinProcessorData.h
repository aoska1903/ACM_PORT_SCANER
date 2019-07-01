#pragma once

typedef int                 INT;
typedef unsigned char       BYTE;
typedef unsigned long       DWORD;
typedef unsigned short      WORD;
typedef int                 BOOL;
typedef char TCHAR;
#pragma warning( disable : 4200 )

#define COIN_MMF_NAME		_T("COIN_PROCESSOR_MMF")
#include <WinBase.h>
//#include <MMFile.h>
// ���� ��ȯ ȣ�� ��
static const INT HOPPER_NUM = 4; 

// ���� Ÿ��
static const INT COIN_TYPE[HOPPER_NUM] = {
	10, 50, 100, 500
};

//----------------------------------------------------------------------//
// ���� ��⿡�� �ö���� ���� �ƴ� �� ���� �����ڵ�
//----------------------------------------------------------------------//
#define COIN_ERROR_CHANGE_NOT_ENOUGH			0xD1	// �����ܵ� ����
#define COIN_ERROR_CHANGE_OVER					0xD2	// �����ܵ� �ʰ�
#define COIN_ERROR_REJECT						0xD3	// �Լ��ݾ׹�ȯ ����
#define COIN_ERROR_NAK_3_TIMES					0xD4	// ��ɽ� NAK 3ȸ�̻� �ö�ö�
#define COIN_ERROR_COUNTING						0xD5	// ����̻��� �߻���

static const BYTE COIN_NO_ERROR	= 0x00;

#ifndef WIN32
	#pragma pack(push,1)
#else
	#include <pshpack1.h>
#endif

//----------------------------------------------------------------------//
// Structure Declare
//----------------------------------------------------------------------//

// ��� ����
typedef struct  
{
	WORD	wHopperNum[HOPPER_NUM];		// ��ȯ ȣ�� ����
	DWORD	dwHopperAmt[HOPPER_NUM];	// ��ȯ ȣ�� �ݾ�
	WORD	wSpareNum[2];				// ���� ȣ�� ����
	DWORD	dwSpareAmt[2];				// ���� ȣ�� �ݾ�
	WORD	wRejectNum[HOPPER_NUM];		// Reject box ���� ����
	DWORD	dwRejectAmt;				// Reject box �� �� �ݾ�
	WORD	wCoinBox[HOPPER_NUM];		// ����ȸ���� ����
	DWORD	dwCoinBoxAmt;				// ����ȸ���� �ݾ�
}T_COIN_HOLDING;


// �ܵ� ���� ��Ŷ ����
typedef struct  
{
	INT     amount;
	BYTE	b_topchanges;				// ���� �ܵ�
	BYTE	b_bottomchanges;			// ���� �ܵ�
	BYTE	b_changes[4];				// ȣ�� �ܵ�
	BYTE	b_s_changes[2];				// ���� ȣ�� �ܵ�	
}T_COIN_CHANGES;


// �ܵ� ���� ��Ŷ ����
typedef struct  
{	
	BYTE	b_topchanges;				// ���� �ܵ�
	BYTE	b_bottomchanges;			// ���� �ܵ�
	BYTE	b_changes[4];				// ȣ�� �ܵ�
	BYTE	b_s_changes[2];				// ���� ȣ�� �ܵ�	
}T_COIN_CHANGE_PACKET;


// ���� ����
typedef struct
{	
	BYTE	b_coin_type[4];				// ���� Ÿ�� �� ���� ����
	DWORD	dw_amount;					// �� �Լ� �� �ݾ�
}T_COIN_INSERT;


// ����, ����, ȸ�� ����
typedef struct
{
	INT     n_s_coin_type[2]; 
	INT	    n_coin_type[4];				// ���� Ÿ�� �� ���� ����
	INT     n_coin_box[4];
	DWORD	dw_amount;					// �� �Լ� �� �ݾ�
}T_COIN_SUPPLY, T_COIN_COLLECT, T_COINBOX_COLLECT;


// ȣ�� �� ���� ȸ�� ������ ����ü 
typedef struct  
{
	BYTE b_hopper_no;						// ȸ�� ȣ�� ��ȣ
	BYTE b_withdraw_cnt;					// ȸ�� ����	
	WORD b_hopper_withdraw_cnt[HOPPER_NUM];	// ȣ�� �� ȸ�� ī����	
	WORD b_spare_withdraw_cnt[2];			// ���� ȣ�� �� ȸ�� ī����
}T_COIN_WITHDRAW;


// ����ó����ġ ���� ���� ����ü
typedef struct 
{
	BYTE b_error_code;						// H/W ���� �ڵ�

	BYTE bCoinVault;                        // �����Ի���   FULL, NEAR FULL
	BYTE bRejectBox;                        // ȸ���� ����  Full, Near FULL
	BYTE bSpareHopper[HOPPER_NUM];			// ����ȣ�ۻ��� NEAR EMPTY, EMPTY
	BYTE bHopper[HOPPER_NUM];				// ȣ�ۻ���     NEAR EMPTY, EMPTY
	BOOL fSpareHopperError;                 // ����ȣ�� �̻�����
	BOOL fHopperError;                      // ��ȯȣ�� �̻�����
	BOOL fBeltError;                        // �ݼۺ�Ʈ �̻�����
	BOOL fValidatorError;                   // Validator �̻�����

	BOOL fSpareHopperAbnormal[HOPPER_NUM];  // ����ȣ�� �̻�(����)
	BOOL fHopperAbnormal[HOPPER_NUM];       // ��ȯȣ�� �̻�(����)   
	BOOL fCoinModuleUnLocking;				// ��� ��������
	BOOL fHopperUnLocking;					// ȣ��Ż������ 
	BOOL fShutterOpen;		
	BOOL device_connect;	
	BOOL cointube_missed;					// Ʃ�� Ż��
} T_COIN_STATUS;


// ����ó����ġ �˶�
typedef struct  
{
	TCHAR alarm_key[5];
	TCHAR modulecode;
	TCHAR alarmcode[4];	
}T_COIN_ALARM;


// ����ó����ġ ������
typedef struct  
{
	TCHAR               _ver[11];
	INT					com_port;
	INT					cmd;				// main->device ���� �ڵ�	
	INT                 dev_cmd;            // device->main
	INT					n_result;			// ���� ��� �ڵ�
	
	// audit
	DWORD				dw_unrAmt;
	DWORD				dw_overAmt;
	T_COIN_HOLDING		coin_holding;		// ������ ȸ���� ������

	// operation audit
	DWORD				change_amount;		// �� ���� �ݾ� ����
	T_COIN_INSERT		insert_data;		// ���� ����
	T_COIN_CHANGES		changes_data;		// ���� ���� ����
	T_COIN_WITHDRAW		withdraw_data;		// ȸ�� ���� (ȣ�� -> ������)
	T_COIN_SUPPLY       coin_supply;        // ���� ���� ����
	T_COINBOX_COLLECT   coin_collect;       // ������ ȸ�� ����
	
	// status	
	BOOL				device_open;
	BOOL				operationcomplete;	// ���� �Ϸ� ����(TRUE:�Ϸ�, FALSE:�̿Ϸ�)
	T_COIN_STATUS		coin_stat;			// ���� ó�� ��ġ ����
	INT					alarm_cnt;
	T_COIN_ALARM		coin_alarm[150];
}T_COIN_DATA;

#ifndef WIN32
	#pragma pack(pop)
#else
	#include <poppack.h>
#endif

//----------------------------------------------------------------------//
// PassengerUIData class declare
//----------------------------------------------------------------------//
class CCoinProcessorData
{
public:
	
	CCoinProcessorData(_In_ LPCTSTR lpMapName, IN const SIZE_T &nBytes, IN BOOL bOpen=FALSE)
	{
	}

	virtual ~CCoinProcessorData()
	{		
	}
	
	LPVOID GetDataPtr()
	{
	}

	BOOL WriteMMF(_In_ const T_COIN_DATA *DestData)
	{
	}

	BOOL ReadMMF(_Inout_ T_COIN_DATA *SrcData){
	}		
	
private:
	T_COIN_DATA *m_pData;
};

