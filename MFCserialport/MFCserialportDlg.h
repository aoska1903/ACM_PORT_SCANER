
// MFCserialportDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include <functional>
#include "CoinProcessorData.h"
#include "BillProcessorData.h"
#include "afxcmn.h"
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

// ��� ������ ����ü
typedef struct _bnk_data_block_format
{
	BYTE command;
	BYTE recv_buf[128];	// ���� ������ ����
	BYTE text[128];		// ���� �� TEXT ������
	INT  length;		// ���� ������ ����
	INT  text_length;	// ���� �� TEXT ������ ����
	INT  retry_cnt;
	INT  cur_state;
}ST_BILL_DATA_FORMAT;

static const BYTE ENQ = 0x05;
static const BYTE ACK = 0x06;
static const BYTE EOT = 0x04;
static const BYTE DLE = 0x10;

// ���� ��� ������ ���� mask ��
//WORD		m_wEnableBanknoteMask_Dlg;

//----------------------------------------------------------------------//
// @ SAM ���� ��û ����ü 
//--------------------------------------------------------------------
typedef struct _sam_info_req 
{
	char          lsam_type ;        // 'U', 'L', 'T','C', �Ǵ� slot_num ;
	unsigned char lsam_pass[8];
}T_SAM_INFO_REQ;

class CCoinProtocol;
class CBillProtocol;
class CRechargeProtocol;
class CIOProtocol;
// CMFCserialportDlg ��ȭ ����
class CMFCserialportDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CMFCserialportDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_MFCSERIALPORT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT TestMessage(WPARAM wParam, LPARAM lParam);//���� ��������� �޽���
	afx_msg LRESULT BillTestMessage(WPARAM wParam, LPARAM lParam);//���� ��������� �޽���
	afx_msg LRESULT RFTestMessage(WPARAM wParam, LPARAM lParam);//������ġ ��������� �޽���
	afx_msg LRESULT IOTestMessage(WPARAM wParam, LPARAM lParam);//IO��ġ ��������� �޽���
	DECLARE_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// Member Variable
public:
	static CCriticalSection m_cs;

public:
	INT COIN_Port_Num;//����ó����ġ ��Ʈ��ȣ
	INT BILL_Port_Num;//����ó����ġ ��Ʈ��ȣ
	INT RF_Port_Num;//����ó����ġ ��Ʈ��ȣ
	INT IO_Port_Num;//IO ��Ʈ��ȣ
	INT INDEX_Port_Num;//Ž���� ��Ʈ��ȣ
	INT persent;//�� ������
	INT all_persent;//��ü�� ������
	INT success_part;//������� ��Ʈ ã�� ����

	INT Before_COIN_Port_Num;//���� ����ó����ġ ��Ʈ��ȣ
	INT Before_BILL_Port_Num;//���� ����ó����ġ ��Ʈ��ȣ
	INT Before_RF_Port_Num;//���� ����ó����ġ ��Ʈ��ȣ
	INT Before_IO_Port_Num;//���� IO ��Ʈ��ȣ

	int Arr_Port[12];//�ø��� ����Ǵ� ��Ʈ �迭
	int Count_Port;//����� ��Ʈ����
	BOOL Coin_flag;//���� ��Ʈ ã�Ҵ����� ���� ����
	BOOL Bill_flag;//���� ��Ʈ ã�Ҵ����� ���� ����
	BOOL RF_flag;//RF ��Ʈ ã�Ҵ����� ���� ����
	BOOL IO_flag;//IO ��Ʈ ã�Ҵ����� ���� ����

	// ����ó����ġ ��� �ڵ� Get/Set
	BYTE GetCoinCmd();
	void OnOK();//enter������ ���� �ȵǵ���
	
	CString result;//��� �α�
	CString Port_View;//ã�� ��Ʈ�� edit �ڽ��� ���

	void Font_Make();//��Ʈ �����

	void Coinport_Find(int Port);//���� ó�� ��ġ ��Ʈ ã�� �Լ�
	void Billport_Find(int Port);//���� ó�� ��ġ ��Ʈ ã�� �Լ�
	void RFport_Find(int Port);//���� ó�� ��ġ ��Ʈ ã�� �Լ�
	void IOport_Find(int Port);//IO ��ġ ��Ʈ ã�� �Լ�
	void Port_Direct();//��Ʈ Ž�� ����
	INT Ini_Get();//���� ini ��Ʈ���
	INT Ini_Write();//ini ��Ʈ���
	
	void CreateLogFolder();	//�α� ���� ����
	void CreateLogFile();	//�α� ���� ����
	void WriteLogFile(CStringA strContents);//�α׳� ���
	void CreateFolder(CString csPath);//���� ���� �Լ�
	CString Log;//�α� ����� ����

	CFont m_font;//��Ʈ

	CEdit m_edit_rcv_view;//������������ִ� ����
	CStatic m_static_coin;//coin �����
	CStatic m_static_bill;//bill �����
	CStatic m_static_io;  //rf �����
	CStatic m_static_rf;  //io �����
	CProgressCtrl m_progress_coin;//���� ��
	CProgressCtrl m_progress_bill;//���� ��
	CProgressCtrl m_progress_rf;  //RF ��
	CProgressCtrl m_progress_io;  //IO ��
	CProgressCtrl m_progress_all; //��ü ��
	CStatic m_static_coin_ing;//coin �������
	CStatic m_static_bill_ing;//bill �������
	CStatic m_static_rf_ing;  //rf �������
	CStatic m_static_io_ing;  //io �������
	CStatic m_static_all_ing; //��ü �������
	CStatic m_static_before_ini;//����INI��
	CStatic m_static_after_ini;//����INI��
	CButton m_button_start;//���� ��ư

	COleDateTime currentTime;//�ð� ����
	CString m_sModulePath;//log filel ���
	int Now_Date;//���� ��¥
	int Now_Transation;// ���� ����
	CString Date_Folder_Name;//Log ���� ����

	// ����ó����ġ Ŀ���
	static ST_BILL_DATA_FORMAT*	m_pDataFmt;

	afx_msg void OnTimer(UINT_PTR nIDEvnet);//�ð�����
	afx_msg void OnBnClickedButtonStart();//��Ʈ Ž�� ����
	afx_msg void OnBnClickedButtonIni();//INI ��� ����

	//����ó����ġ ���� ��� �� �м� ó��
	VOID CoinReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	//����ó����ġ ���� ��� �� �м� ó��
	VOID BillReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);
	
	//����ó����ġ ���� ��� �� �м� ó��
	//VOID RFReponseAnalysis(_In_ BYTE *pData, _In_ INT nLength, _In_ INT nResult);
	 VOID RFReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	//IO��ġ ���� ��� �� �м� ó��
	VOID IOReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	// ����ó����ġ ��� �۽�
	BOOL SendCmdToBNK(_In_ const BYTE _cmd, _In_ INT _wait_time, _In_opt_ BOOL _bTCC=TRUE);

private:
	T_COIN_DATA				*m_p_Data;
	CCoinProcessorData		*m_p_mmem;		// ����ó����ġ �޸� �� ������
	
	// ����ó����ġ ��� ���� ����ü
	static ST_CONLUX_COM_INFO *m_pstPack;

	// �ø��� ��� ���� Ŭ���� ��ü(���� ó�� ��ġ)
	CCoinProtocol* m_coin_protocol;
	
	// �ø��� ��� ���� Ŭ���� ��ü(���� ó�� ��ġ)
	CBillProtocol* m_bill_protocol;
	
	// �ø��� ��� ���� Ŭ���� ��ü(���� ��ġ)
	CRechargeProtocol* m_rf_protocol;
	
	// �ø��� ��� ���� Ŭ���� ��ü(���� ��ġ)
	CIOProtocol* m_io_protocol;

	T_SAM_INFO_REQ           m_samInfo;//RF SAM ����
};
