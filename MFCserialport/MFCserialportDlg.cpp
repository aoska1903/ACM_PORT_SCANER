
// MFCserialportDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "MFCserialport.h"
#include "MFCserialportDlg.h"
#include "afxdialogex.h"
#include "CoinProtocol.h"
#include "BillProtocol.h"
#include "RechargeProtocol.h"
#include "IOProtocol.h"
#include "Ini.h"
//#include "sqlite3.h"
//#include "sqlite3.lib"

// Bit set
#define BIT_SET( arg, posn ) ((arg) |= (1L << (posn)))

#define EXECUTE_SUCCESS					1		// ���� ����
#define PORT_FIND_WAITTIME				2500	// ��Ʈ Ž���ð�
#define PORT_RESPONS_TIME				1000	// ��Ʈ ����ð�
#define CLOSE_OPEN_TIME					100		// ��Ʈ �ݰ� �����½ð�
#define PERSENT_VALUE					100		// �� ������ �ִ밪

CCriticalSection CMFCserialportDlg::m_cs;
ST_CONLUX_COM_INFO* CMFCserialportDlg::m_pstPack = NULL;

// ����ó����ġ ������ �� ����ü
ST_BILL_DATA_FORMAT* CMFCserialportDlg::m_pDataFmt = NULL;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMFCserialportDlg ��ȭ ����

CMFCserialportDlg::CMFCserialportDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCserialportDlg::IDD, pParent)
	, Count_Port(0)
	,m_coin_protocol(NULL)
	,m_bill_protocol(NULL)
	,m_rf_protocol(NULL)
	,m_io_protocol(NULL)
	,COIN_Port_Num(-1)
	,BILL_Port_Num(-1)
	,RF_Port_Num(-1)
	,IO_Port_Num(-1)
	,INDEX_Port_Num(-1)
	,Coin_flag(FALSE)
	,Bill_flag(FALSE)
	,RF_flag(FALSE)
	,IO_flag(FALSE)
	,Before_COIN_Port_Num(-1)
	,Before_BILL_Port_Num(-1)
	,Before_RF_Port_Num(-1)
	,Before_IO_Port_Num(-1)
	,success_part(0)
	,m_sModulePath(_T("Log"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCserialportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_RCV_VIEW, m_edit_rcv_view);
	DDX_Control(pDX, IDC_STATIC_COIN, m_static_coin);
	DDX_Control(pDX, IDC_STATIC_BILL, m_static_bill);
	DDX_Control(pDX, IDC_STATIC_IO, m_static_io);
	DDX_Control(pDX, IDC_STATIC_RF, m_static_rf);
	DDX_Control(pDX, IDC_PROGRESS_COIN, m_progress_coin);
	DDX_Control(pDX, IDC_PROGRESS_BILL, m_progress_bill);
	DDX_Control(pDX, IDC_PROGRESS_RF, m_progress_rf);
	DDX_Control(pDX, IDC_PROGRESS_IO, m_progress_io);
	DDX_Control(pDX, IDC_STATIC_COIN_ING, m_static_coin_ing);
	DDX_Control(pDX, IDC_STATIC_BILL_ING, m_static_bill_ing);
	DDX_Control(pDX, IDC_STATIC_RF_ING, m_static_rf_ing);
	DDX_Control(pDX, IDC_STATIC_IO_ING, m_static_io_ing);
	DDX_Control(pDX, IDC_STATIC_ALL_ING, m_static_all_ing);
	DDX_Control(pDX, IDC_PROGRESS_ALL, m_progress_all);
	DDX_Control(pDX, IDC_STATIC_BEFORE_INI, m_static_before_ini);
	DDX_Control(pDX, IDC_STATIC_AFTER_INI, m_static_after_ini);
	DDX_Control(pDX, IDC_BUTTON_INI, m_button_start);
}

BEGIN_MESSAGE_MAP(CMFCserialportDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_MESSAGE(UM_COIN_MESSAGE, &CMFCserialportDlg::TestMessage)
	ON_MESSAGE(UM_BILL_MESSAGE, &CMFCserialportDlg::BillTestMessage)
	ON_MESSAGE(UM_RF_MESSAGE, &CMFCserialportDlg::RFTestMessage)
	ON_MESSAGE(UM_IO_MESSAGE, &CMFCserialportDlg::IOTestMessage)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMFCserialportDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_INI, &CMFCserialportDlg::OnBnClickedButtonIni)
END_MESSAGE_MAP()


// CMFCserialportDlg �޽��� ó����

BOOL CMFCserialportDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	m_progress_coin.SetRange(0, PERSENT_VALUE);//COIN �� �ʱ�ȭ
	m_progress_bill.SetRange(0, PERSENT_VALUE);//BILL �� �ʱ�ȭ
	m_progress_rf.SetRange(0, PERSENT_VALUE);//RF �� �ʱ�ȭ
	m_progress_io.SetRange(0, PERSENT_VALUE);//IO �� �ʱ�ȭ
	m_progress_all.SetRange(0, PERSENT_VALUE*4);//��ü �� �ʱ�ȭ(��ü���� �ִ밪�� �Ϲݹ��� 4�谪)
	
	//����ó����ġ
	m_coin_protocol = new CCoinProtocol();	
	m_coin_protocol->CreateRecvEvent();	
	m_coin_protocol->CoinRegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::CoinReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//����ó����ġ
	m_bill_protocol = new CBillProtocol();
	m_bill_protocol->CreateRecvEvent();
	m_bill_protocol->BillRegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::BillReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//������ġ
	m_rf_protocol = new CRechargeProtocol();
	m_rf_protocol->CreateRecvEvent();
	m_rf_protocol->RegisterReceiveFuncCharge(std::bind(&CMFCserialportDlg::RFReponseAnalysis, this, 
	std::placeholders::_1,	std::placeholders::_2, std::placeholders::_3));

	//IO��ġ
	m_io_protocol = new CIOProtocol();
	m_io_protocol->CreateRecvEvent();
	m_io_protocol->IORegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::IOReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_pDataFmt = new ST_BILL_DATA_FORMAT;
	ZeroMemory(m_pDataFmt, sizeof(ST_BILL_DATA_FORMAT));
	//��� ������ �� ����ü ����
	Ini_Get();//���� INI���
	Font_Make();//��Ʈ �����
	CreateLogFolder();//�α� ���� ����
	CreateLogFile();//�α� ���� ����

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CMFCserialportDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CMFCserialportDlg::OnOK(){//enter������ ���� �ȵǵ���
	//CDialog::OnOK();
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CMFCserialportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCserialportDlg::OnTimer(UINT_PTR nIDEvnet)//�ð�����
{
	if(nIDEvnet == 0){

	}
	if(nIDEvnet == 1){
		if(INDEX_Port_Num<Count_Port){		
			this->PostMessageA(UM_COIN_MESSAGE, 0, 0);
		}
	}
	if(nIDEvnet == 2){
		if(INDEX_Port_Num<Count_Port){				
			this->PostMessageA(UM_BILL_MESSAGE, 0, 0);
		}
	}
	if(nIDEvnet == 3){
		if(INDEX_Port_Num<Count_Port){				
			this->PostMessageA(UM_RF_MESSAGE, 0, 0);
		}
	}
	if(nIDEvnet == 4){
		if(INDEX_Port_Num<Count_Port){				
			this->PostMessageA(UM_IO_MESSAGE, 0, 0);
		}
	}
	CDialog::OnTimer(nIDEvnet);
}

void CMFCserialportDlg::Font_Make(){//��Ʈ �����
	m_font.CreatePointFont(150, "����");
	m_button_start.SetFont(&m_font, TRUE);//��Ʈ����
	GetDlgItem(IDC_BUTTON_START)->SetFont(&m_font, TRUE);//��Ʈ����
	m_font.CreatePointFont(125, "����");
	m_static_before_ini.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_after_ini.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_coin.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_bill.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_rf.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_io.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_all_ing.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_coin_ing.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_bill_ing.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_rf_ing.SetFont(&m_font, TRUE);//��Ʈ����
	m_static_io_ing.SetFont(&m_font, TRUE);//��Ʈ����
}

void CMFCserialportDlg::Coinport_Find(int Port)//���� ó�� ��ġ ��Ʈ ã�� �Լ�
{
	int find_flag = 0;
	all_persent = ((PERSENT_VALUE / (Count_Port+1)) * Port) + (PERSENT_VALUE * success_part);//��ü ������ũ��� ����迭������ġ/�迭ũ��+1 + ������Ʈ������ ���
	m_progress_all.SetPos(all_persent);//��ü �� ����
	Port_View.Format(_T("��ü�� ����ӵ� : (%d)�ۼ�Ʈ"), all_persent/=4);//�������
	m_static_all_ing.SetWindowTextA(Port_View);//������� ���
	WriteLogFile(Port_View);	//�α� ���
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//������ũ��� ����迭������ġ/�迭ũ��+1 ���
	m_progress_coin.SetPos(persent);//�� ����
	Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (%d)�ۼ�Ʈ"), persent);//�������
	m_static_coin_ing.SetWindowTextA(Port_View);//������� ���
	WriteLogFile(Port_View);	//�α� ���
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ���Ѵٸ�
		find_flag = 1;
	}
	if(find_flag == 0){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ������ ������� Ž�� ����
		m_coin_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//�ݰ� �����µ� ���� �ð�
		m_coin_protocol->SetCOMPort(Arr_Port[Port]);//��Ʈ�迭 ����
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ó�� ��ġ) "), result, Arr_Port[Port]);//���
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("�ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ó�� ��ġ) "), Arr_Port[Port]);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		if(m_coin_protocol->Connect_Conlux() == 1)
		{//  �ø��� ��� ���� �� ����	//���������
			Sleep(PORT_RESPONS_TIME);
			m_coin_protocol->SendCmdToConlux(CCoinProtocol::CONLUX_CMD_STANDBY);
		}
		if(Port==Count_Port-1){//�����͹迭 ������ ���� ���
			Sleep(PORT_RESPONS_TIME);
			if(COIN_Port_Num<0){//������ �迭 Ž���� �ص� ��ã�������
				m_progress_coin.SetPos(0);//ã�� �������Ƿ� 0���� ����
				INDEX_Port_Num=-1;//Ž������ �ʱ�ȭ
				KillTimer(1);//Ÿ�̸� ����
				result.Format(_T("%s \r\n ���� ó�� ��ġ�� ��Ʈ ��ȣ�� ã�� ���߽��ϴ�. "), result);//���
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" ���� ó�� ��ġ ��Ʈ�� ã�� ���߽��ϴ�. "));//���
				WriteLogFile(Port_View);	//�α� ���
				m_static_coin.SetWindowTextA(Port_View);
				Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (0)�ۼ�Ʈ"));//�������
				m_static_coin_ing.SetWindowTextA(Port_View);//������� ���
				WriteLogFile(Port_View);	//�α� ���
				m_coin_protocol->Disconnect();//��ã�����Ƿ� ��������
				SetTimer(3, PORT_FIND_WAITTIME, NULL);//������Ʈ Ž��
			}
		}
	}
}

void CMFCserialportDlg::Billport_Find(int Port)//���� ó�� ��ġ ��Ʈ ã�� �Լ�
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//��ü ������ũ��� ����迭������ġ/�迭ũ��+1 + ������Ʈ������ ���
	m_progress_all.SetPos(all_persent);//��ü �� ����
	Port_View.Format(_T("��ü�� ����ӵ� : (%d)�ۼ�Ʈ"), all_persent/=4);//�������
	m_static_all_ing.SetWindowTextA(Port_View);//������� ���
	WriteLogFile(Port_View);	//�α� ���
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//������ũ��� ����迭������ġ/�迭ũ��+1 ���
	m_progress_bill.SetPos(persent);//�� ����
	Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (%d)�ۼ�Ʈ"), persent);//�������
	m_static_bill_ing.SetWindowTextA(Port_View);//������� ���
	WriteLogFile(Port_View);	//�α� ���
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ���Ѵٸ�
		find_flag = 1;
	}
	if(find_flag == 0){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ������ ������� Ž�� ����
		m_bill_protocol->Disconnect();//��������
		Sleep(CLOSE_OPEN_TIME);//�ݰ� �����µ� ���� �ð�
		m_bill_protocol->SetCOMPort(Arr_Port[Port]);//��Ʈ�迭 ����
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ó�� ��ġ) "), result, Arr_Port[Port]);//���
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("�ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ó�� ��ġ) "), Arr_Port[Port]);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		if(m_bill_protocol->ConnectToDevice() == 1)	
		{//  �ø��� ��� ���� �� ����	//���������
			Sleep(PORT_RESPONS_TIME/2);
			BYTE _byteCmd = ENQ;
			SendCmdToBNK(ENQ, 1000, FALSE);//ENQ����
		}
		if(Port==Count_Port-1){//�����͹迭 ������ ���� ���
			Sleep(PORT_RESPONS_TIME*2);
			if(BILL_Port_Num<0){//������ �迭 Ž���� �ص� ��ã�������
				m_progress_bill.SetPos(0);//ã�� �������Ƿ� 0���� ����
				INDEX_Port_Num=-1;//Ž������ �ʱ�ȭ
				KillTimer(2);//Ÿ�̸� ����
				result.Format(_T("%s \r\n ���� ó�� ��ġ�� ��Ʈ ��ȣ�� ã�� ���߽��ϴ�. "), result);//���
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" ���� ó�� ��ġ ��Ʈ�� ã�� ���߽��ϴ�. "));//���
				m_static_bill.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//�α� ���
				Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (0)�ۼ�Ʈ"));//�������
				m_static_bill_ing.SetWindowTextA(Port_View);//������� ���
				WriteLogFile(Port_View);	//�α� ���
				m_bill_protocol->Disconnect();//��ã�����Ƿ� ��������
				Ini_Write();
			}
		}
	}
}

void CMFCserialportDlg::RFport_Find(int Port)//���� ó�� ��ġ ��Ʈ ã�� �Լ�
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//��ü ������ũ��� ����迭������ġ/�迭ũ��+1 + ������Ʈ������ ���
	m_progress_all.SetPos(all_persent);//��ü �� ����
	Port_View.Format(_T("��ü�� ����ӵ� : (%d)�ۼ�Ʈ"), all_persent/=4);//�������
	WriteLogFile(Port_View);	//�α� ���
	m_static_all_ing.SetWindowTextA(Port_View);//������� ���
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//������ũ��� ����迭������ġ/�迭ũ��+1 ���
	m_progress_rf.SetPos(persent);//�� ����
	Port_View.Format(_T("���� ��ġ ����ӵ� : (%d)�ۼ�Ʈ"), persent);//�������
	WriteLogFile(Port_View);	//�α� ���
	m_static_rf_ing.SetWindowTextA(Port_View);//������� ���
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ���Ѵٸ�
		find_flag = 1;
	}
	if(find_flag == 0){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ������ ������� Ž�� ����
		m_rf_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//�ݰ� �����µ� ���� �ð�
		m_rf_protocol->SetCOMPort(Arr_Port[Port]);//��Ʈ�迭 ����
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ��ġ) "), result, Arr_Port[Port]);//���
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("�ø��� ��Ʈ ��ȣ ���� �õ� %d (���� ��ġ) "), Arr_Port[Port]);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		if(m_rf_protocol->Connect() == 1)
		{//  �ø��� ��� ���� �� ����	//���������
			Sleep(PORT_RESPONS_TIME/2);
			ZeroMemory(&m_samInfo, sizeof(T_SAM_INFO_REQ));
			TCHAR cur_sam = 'C';
			m_samInfo.lsam_type = cur_sam;
			m_rf_protocol->SendPacket(CRechargeProtocol::CHARGE_CMD_SAM_INFO, (BYTE *)&m_samInfo, sizeof(T_SAM_INFO_REQ));
		}
		if(Port==Count_Port-1){//������ ������ ������ ���
			Sleep(PORT_RESPONS_TIME*2);
			if(RF_Port_Num<0){//������ �迭 Ž���� �ص� ��Ʈ��ȣ�� ��ã�������
				m_progress_rf.SetPos(0);//ã�� �������Ƿ� 0���� ����
				INDEX_Port_Num=-1;//Ž������ �ʱ�ȭ
				KillTimer(3);//IO Ÿ�̸� ����
				result.Format(_T("%s \r\n RF ��ġ�� ��Ʈ ��ȣ�� ã�� ���߽��ϴ�. "), result);//���
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" ���� ��ġ ��Ʈ�� ã�� ���߽��ϴ�. "));//���
				m_static_rf.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//�α� ���
				Port_View.Format(_T("���� ��ġ ����ӵ� : (0)�ۼ�Ʈ"));//�������
				WriteLogFile(Port_View);	//�α� ���
				m_static_rf_ing.SetWindowTextA(Port_View);//������� ���
				m_rf_protocol->Disconnect();//��ã�����Ƿ� ��������
				SetTimer(2, PORT_FIND_WAITTIME, NULL);//������Ʈ Ž��
			}
		}
	}
}

void CMFCserialportDlg::IOport_Find(int Port)//IO ��ġ ��Ʈ ã�� �Լ�
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//��ü ������ũ��� ����迭������ġ/�迭ũ��+1 + ������Ʈ������ ���
	m_progress_all.SetPos(all_persent);//��ü �� ����
	Port_View.Format(_T("��ü�� ����ӵ� : (%d)�ۼ�Ʈ"), all_persent/=4);//�������
	WriteLogFile(Port_View);	//�α� ���
	m_static_all_ing.SetWindowTextA(Port_View);//������� ���
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//������ũ��� ����迭������ġ/�迭ũ��+1 ���
	m_progress_io.SetPos(persent);//�� ����
	Port_View.Format(_T("IO ��ġ ����ӵ� : (%d)�ۼ�Ʈ"), persent);//�������
	m_static_io_ing.SetWindowTextA(Port_View);//������� ���
	WriteLogFile(Port_View);	//�α� ���
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ���Ѵٸ�
		find_flag = 1;
	}
	if(find_flag == 0){//Ž���� ��Ʈ���� � ��ġ ��Ʈ�� ������ ������� Ž�� ����
		m_io_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//�ݰ� �����µ� ���� �ð�
		m_io_protocol->SetCOMPort(Arr_Port[Port]);//��Ʈ�迭 ����
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ ���� �õ� %d (IO ��ġ) "), result, Arr_Port[Port]);//���
		Log.Format(_T("�ø��� ��Ʈ ��ȣ ���� �õ� %d (IO ��ġ) "), Arr_Port[Port]);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		m_edit_rcv_view.SetWindowTextA(result);
		if(m_io_protocol->ConnectACM() == 1)
		{//  �ø��� ��� ���� �� ����	//���������
			//tmpStr.Format(_T("%s /r/nArr_Port(%d) : %d"), tmpStr, Count_Port, port);//���
			Sleep(PORT_RESPONS_TIME/2);//1�ʴ��
			m_io_protocol->SendInputCmdToIO(CIOProtocol::IO_CMD_PACKET_STATUS);//IO��Ʈ���� �޽�������
		}
		if(Port==Count_Port-1){//������ ������ ������ ���
			Sleep(PORT_RESPONS_TIME*2);
			if(IO_Port_Num<0){//������ �迭 Ž���� �ص� ��Ʈ��ȣ�� ��ã�������
				m_progress_io.SetPos(0);//ã�� �������Ƿ� 0���� ����
				INDEX_Port_Num=-1;//Ž������ �ʱ�ȭ
				KillTimer(4);//IO Ÿ�̸� ����
				result.Format(_T("%s \r\n IO ��ġ�� ��Ʈ ��ȣ�� ã�� ���߽��ϴ�. "), result);//���
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" IO ��ġ ��Ʈ�� ã�� ���߽��ϴ�. "));//���
				m_static_io.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//�α� ���
				Port_View.Format(_T("IO ��ġ ����ӵ� : (0)�ۼ�Ʈ"));//�������
				m_static_io_ing.SetWindowTextA(Port_View);//������� ���
				WriteLogFile(Port_View);	//�α� ���
				m_io_protocol->Disconnect();//��ã�����Ƿ� ��������
				SetTimer(1, PORT_FIND_WAITTIME, NULL);//������Ʈ Ž��
			}
		}
	}
}

void CMFCserialportDlg::Port_Direct()//��Ʈ Ž��
{
	int i;
	for(i = 0; i< 11; i++){
		Sleep(50);
		m_coin_protocol->SetCOMPort(i);//��Ʈ�迭 ����
		if(m_coin_protocol->Connect_Conlux() == 1){
			Arr_Port[Count_Port] = i;//������ ��� �ش���Ʈ���� ��Ʈ�迭�� ��´� 
			result.Format(_T("%s Arr_Port(%d) : %d \r\n"), result, Count_Port, i);//���
			Log.Format(_T("Arr_Port(%d) : %d"), Count_Port, i);	//�α� ���� 
			Count_Port++;//��Ʈ�迭���� 1����
			m_edit_rcv_view.SetWindowTextA(result);			
			WriteLogFile(Log);	//�α� ���
		}
		m_coin_protocol->Disconnect();
	}
}

//  ���� �ø��� ���� �Լ�
VOID CMFCserialportDlg::CoinReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength)
{
	if(Coin_flag==FALSE){//ó�� ���� ��Ʈ�� ã�����
		success_part++;//��Ʈ ã����� ������� ��Ʈ ã�� ���� ����
		KillTimer(1);
		m_progress_coin.SetPos(PERSENT_VALUE);//ã�����Ƿ� 100%���� ����
		Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (100)�ۼ�Ʈ"));//ã�����Ƿ� 100%���� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_coin_ing.SetWindowTextA(Port_View);//������� ���
		INDEX_Port_Num = -1;
		COIN_Port_Num = m_coin_protocol->GetCOMPort();
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), result, COIN_Port_Num);//���
		Log.Format(_T("�ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), COIN_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(3, 2500, NULL);
		Port_View.Format(_T(" ���� ó�� ��ġ ��Ʈ : %d "), COIN_Port_Num);//���� ��� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_coin.SetWindowTextA(Port_View);
		m_coin_protocol->Disconnect();//��������
		Coin_flag=TRUE;//���� ��Ʈ ã�����Ƿ� �÷��� ������
	}
}

//  ���� �ø��� ���� �Լ�
VOID CMFCserialportDlg::BillReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength)
{
	if(Bill_flag==FALSE){//ó�� ���� ��Ʈ�� ã�����
		success_part++;//��Ʈ ã����� ������� ��Ʈ ã�� ���� ����
		KillTimer(2);
		m_progress_bill.SetPos(PERSENT_VALUE);//ã�����Ƿ� 100%���� ����
		Port_View.Format(_T("���� ó�� ��ġ ����ӵ� : (100)�ۼ�Ʈ"));//ã�����Ƿ� 100%���� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_bill_ing.SetWindowTextA(Port_View);//������� ���
		INDEX_Port_Num = -1;
		BILL_Port_Num = m_bill_protocol->GetCOMPort();
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), result, BILL_Port_Num);//���
		Log.Format(_T("�ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), BILL_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		m_edit_rcv_view.SetWindowTextA(result);
		Port_View.Format(_T(" ���� ó�� ��ġ ��Ʈ : %d "), BILL_Port_Num);//���� ��� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_bill.SetWindowTextA(Port_View);
		if(success_part>2){//������3������Ʈ�� ��ã�������
			m_progress_all.SetPos(PERSENT_VALUE * 4);//���� ã�����Ƿ� 100��
			Port_View.Format(_T("��ü�� ����ӵ� : (100)�ۼ�Ʈ - ��� ��Ʈ�� ã�ҽ��ϴ�."));//�������(���� ã�����Ƿ� 100��)
			WriteLogFile(Port_View);	//�α� ���
			m_static_all_ing.SetWindowTextA(Port_View);//������� ���
		}
		m_bill_protocol->Disconnect();//��������
		Sleep(PORT_RESPONS_TIME);//����� �ð���
		if(Ini_Write()==1){
			result.Format(_T("%s \r\n INI����Ͽ����ϴ�. "), result);//INI ���
			Log.Format(_T("INI����Ͽ����ϴ�."));	//�α� ���� 
			WriteLogFile(Log);	//�α� ���
			m_edit_rcv_view.SetWindowTextA(result);
		};
		Bill_flag=TRUE;//���� ��Ʈ ã�����Ƿ� �÷��� ������
	}
}

//  ����ó����ġ �ø��� ���� �Լ�
VOID CMFCserialportDlg::RFReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength){
//VOID CMFCserialportDlg::RFReponseAnalysis(_In_ BYTE *pData, _In_ INT nLength, _In_ INT nResult ){
	if(RF_flag==FALSE){//ó�� ���� ��Ʈ�� ã�����
		success_part++;//��Ʈ ã����� ������� ��Ʈ ã�� ���� ����
		KillTimer(3);
		m_progress_rf.SetPos(PERSENT_VALUE);//ã�����Ƿ� 100%���� ����
		Port_View.Format(_T("���� ��ġ ����ӵ� : (100)�ۼ�Ʈ"));//ã�����Ƿ� 100%���� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_rf_ing.SetWindowTextA(Port_View);//������� ���
		INDEX_Port_Num = -1;
		RF_Port_Num = m_rf_protocol->GetCOMPort();
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), result, RF_Port_Num);//���
		Log.Format(_T("�ø��� ��Ʈ ��ȣ %d ���� ó�� ��ġ�� ���� ���� "), RF_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(2, 2500, NULL);
		Port_View.Format(_T(" ���� ��ġ ��Ʈ : %d "), RF_Port_Num);//���� ��� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_rf.SetWindowTextA(Port_View);
		m_rf_protocol->Disconnect();//��������
		RF_flag=TRUE;//���� ��Ʈ ã�����Ƿ� �÷��� ������
	}
}

//  IO ��ġ �ø��� ���� �Լ�
VOID CMFCserialportDlg::IOReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength){
	if(IO_flag==FALSE){//ó�� IO ��Ʈ�� ã�����
		success_part++;//��Ʈ ã����� ������� ��Ʈ ã�� ���� ����
		KillTimer(4);
		m_progress_io.SetPos(PERSENT_VALUE);//ã�����Ƿ� 100%���� ����
		Port_View.Format(_T("IO ��ġ ����ӵ� : (100)�ۼ�Ʈ"));//ã�����Ƿ� 100%���� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_io_ing.SetWindowTextA(Port_View);//������� ���
		INDEX_Port_Num = -1;
		IO_Port_Num = m_io_protocol->GetCOMPort();
		result.Format(_T("%s \r\n �ø��� ��Ʈ ��ȣ %d IO ó�� ��ġ�� ���� ���� "), result, IO_Port_Num);//���
		Log.Format(_T("�ø��� ��Ʈ ��ȣ %d IO ��ġ�� ���� ���� "), IO_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(1, 2500, NULL);
		Port_View.Format(_T(" IO ��ġ ��Ʈ : %d "), IO_Port_Num);//IO ��� ���
		WriteLogFile(Port_View);	//�α� ���
		m_static_io.SetWindowTextA(Port_View);
		m_io_protocol->Disconnect();//��������
		IO_flag=TRUE;//IO ��Ʈ ã�����Ƿ� �÷��� ������
	}
}

afx_msg LRESULT CMFCserialportDlg::TestMessage(WPARAM wParam, LPARAM lParam)//����ó����ġ ��������� �޽���
{
	INDEX_Port_Num++;
	Coinport_Find(INDEX_Port_Num);
	if(COIN_Port_Num>=0){
		KillTimer(1);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::BillTestMessage(WPARAM wParam, LPARAM lParam)//����ó����ġ ��������� �޽���
{
	INDEX_Port_Num++;
	Billport_Find(INDEX_Port_Num);
	if(BILL_Port_Num>=0){
		KillTimer(2);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::RFTestMessage(WPARAM wParam, LPARAM lParam)//������ġ ��������� �޽���
{
	INDEX_Port_Num++;
	RFport_Find(INDEX_Port_Num);
	if(RF_Port_Num>=0){
		KillTimer(3);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::IOTestMessage(WPARAM wParam, LPARAM lParam)//IO��ġ ��������� �޽���
{
	INDEX_Port_Num++;
	IOport_Find(INDEX_Port_Num);
	if(IO_Port_Num>=0){
		KillTimer(4);
		INDEX_Port_Num=-1;
	}	
	return 0;
}

/*
 * Name: SendCmdToBNK
 * IN  : const BYTE _cmd(����ó����ġ ���), BOOL _bTCC(���� Ÿ���� ��� ���� �������� �Ǵ�)
 * Out : ���� ���� ����(TRUE:����, FALSE:����)
 * Desc: ����ó����ġ ��� �۽�
 */
BOOL CMFCserialportDlg::SendCmdToBNK(_In_ const BYTE _cmd, _In_ INT _wait_time, _In_opt_ BOOL _bTCC/*=TRUE*/)
{
	Sleep(CLOSE_OPEN_TIME);
	CSingleLock _cs(&m_cs);
	_cs.Lock();
	{
		memset(m_pDataFmt->recv_buf, 0x00, sizeof(m_pDataFmt->recv_buf));
		m_pDataFmt->length = 0;
	}
	_cs.Unlock();

	BOOL _bRet = TRUE;

	// ���� ���� ���� �۽��� ���
	if(_bTCC == TRUE)
	{
		INT _length = 0;
		BYTE _symbol[128];
		memset(_symbol, 0x00, sizeof(_symbol));

		if(_cmd == ENQ){//[ENQ] ����
		}
		else if(_cmd == 0x30){//[DLE0] ����
		}
		else if(_cmd == 0x31){//[DLE1] ����
		}
		else if(_cmd == EOT){//[EOT] ����
		}
		else{//[%02X] ����
		}

		if(_cmd == 0x30 || _cmd == 0x31)
		{// DLE 0 �Ǵ� DLE 1
			_symbol[0] = DLE;
			_symbol[1] = _cmd;
			_length = 2;
		}
		else
		{// ��Ÿ ���� ���� ����
			_symbol[0] = _cmd;
			_length = 1;
		}

		if(m_bill_protocol == NULL){
			return FALSE;
		}

		_bRet = m_bill_protocol->SendTransCtrlChar(_symbol, _length);
		if(_bRet == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		if(m_bill_protocol)
		{
			// Authorized ��� ��� ���� ���� mask �� ����
			if(_cmd == BNK_BILL_COMMAND::BNK_CMD_AUTHORIZED){
				WORD		m_wEnableBanknoteMask_Dlg;
				BIT_SET( m_wEnableBanknoteMask_Dlg, 0 );
				_bRet = m_bill_protocol->SendCmdToBNK(_cmd, m_wEnableBanknoteMask_Dlg);
			}else{
				_bRet = m_bill_protocol->SendCmdToBNK(_cmd);
			}

			if(_bRet == FALSE){
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	// Ÿ�̸� ����
	Sleep(CLOSE_OPEN_TIME);
	return TRUE;
}

INT CMFCserialportDlg::Ini_Get()//���� ini ��Ʈ���
{	
	TCHAR  curFolderPath[MAX_PATH];         
	TCHAR  iniFilePath[MAX_PATH];
	ZeroMemory(curFolderPath, sizeof(curFolderPath));
	ZeroMemory(iniFilePath, sizeof(iniFilePath));
	GetCurrentDirectory( MAX_PATH, curFolderPath);
	_stprintf(iniFilePath, _T("C:\\AFC\\Ini\\DeviceConfig.ini"), curFolderPath);		

	CIni _ini;
	INT _com_port = 0;
	_ini.SetPathName(iniFilePath);
	Before_COIN_Port_Num	= _ini.GetInt(_T("COM PORT"), _T("Coin Port"),		-1);
	Before_BILL_Port_Num	= _ini.GetInt(_T("COM PORT"), _T("Bill Port"),		-1);
	Before_RF_Port_Num		= _ini.GetInt(_T("COM PORT"), _T("Recharge Port"),	-1);
	Before_IO_Port_Num		= _ini.GetInt(_T("COM PORT"), _T("IO Port"),		-1);

	CString Result_MSG;
	Result_MSG.Format(_T("���� INI ��Ʈ ��\r\n"));//���

	Result_MSG.Format(_T("%sIO ��ġ : %d\r\n"), Result_MSG, Before_IO_Port_Num);//���
	Log.Format(_T("IO ��ġ : %d"), Before_IO_Port_Num);//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	Result_MSG.Format(_T("%s���� ó�� ��ġ : %d\r\n"), Result_MSG, Before_COIN_Port_Num);//���
	Log.Format(_T("���� ó�� ��ġ : %d"), Before_COIN_Port_Num);//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	Result_MSG.Format(_T("%s���� ó�� ��ġ : %d\r\n"), Result_MSG, Before_BILL_Port_Num);//���
	Log.Format(_T("���� ó�� ��ġ : %d"), Before_BILL_Port_Num);//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	Result_MSG.Format(_T("%s���� ��ġ : %d\r\n"), Result_MSG, Before_RF_Port_Num);//���
	Log.Format(_T("���� ��ġ : %d"), Before_RF_Port_Num);//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	
	m_static_before_ini.SetWindowTextA(Result_MSG);
	WriteLogFile(Result_MSG);//�α� ���
	return 1;
}

INT CMFCserialportDlg::Ini_Write()//ini ��Ʈ���
{	
	CString Result_MSG;
	Result_MSG.Format(_T("Ž�� �� INI ��Ʈ ��\r\n"));//���
	Log.Format(_T("Ž�� �� INI ��Ʈ ��"));	//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	if(IO_Port_Num>=0){
		Result_MSG.Format(_T("%sIO ��ġ : %d\r\n"), Result_MSG, IO_Port_Num);//���
		Log.Format(_T("IO ��ġ : %d"), IO_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	else{
		Result_MSG.Format(_T("%sIO ��ġ ��Ʈ��\r\n ã�� ���߽��ϴ�.\r\n"), Result_MSG);//���
		Log.Format(_T("IO ��ġ ��Ʈ�� ã�� ���߽��ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	if(COIN_Port_Num>=0){
		Result_MSG.Format(_T("%s���� ó�� ��ġ : %d\r\n"), Result_MSG, COIN_Port_Num);//���
		Log.Format(_T("���� ó�� ��ġ : %d"), COIN_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	else{
		Result_MSG.Format(_T("%s���� ó�� ��ġ ��Ʈ��\r\n ã�� ���߽��ϴ�.\r\n"), Result_MSG);//���
		Log.Format(_T("���� ó�� ��ġ ��Ʈ�� ã�� ���߽��ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	if(BILL_Port_Num>=0){
		Result_MSG.Format(_T("%s���� ó�� ��ġ : %d\r\n"), Result_MSG, BILL_Port_Num);//���
		Log.Format(_T("���� ó�� ��ġ : %d"), BILL_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	else{
		Result_MSG.Format(_T("%s���� ó�� ��ġ ��Ʈ��\r\n ã�� ���߽��ϴ�.\r\n"), Result_MSG);//���
		Log.Format(_T("���� ó�� ��ġ ��Ʈ�� ã�� ���߽��ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	if(RF_Port_Num>=0){
		Result_MSG.Format(_T("%s���� ��ġ : %d\r\n"), Result_MSG, RF_Port_Num);//���
		Log.Format(_T("���� ��ġ : %d"), RF_Port_Num);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	else{
		Result_MSG.Format(_T("%s���� ��ġ ��Ʈ��\r\n ã�� ���߽��ϴ�.\r\n"), Result_MSG);//���
		Log.Format(_T("���� ��ġ ��Ʈ�� ã�� ���߽��ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	Result_MSG.Format(_T("%s Ž���� ��Ʈ����\n �����Ͻ÷���\r\n �Ʒ� ��ư��\n Ŭ���ϼ���.\r\n"), Result_MSG);//���
	m_static_after_ini.SetWindowTextA(Result_MSG);
	m_button_start.EnableWindow();
	return 1;
}

void CMFCserialportDlg::OnBnClickedButtonStart()//��Ʈ Ž�� ����
{
	Log.Format(_T("���� ��ư Ŭ��"));	//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	Port_Direct();//����� ��Ʈ ã��
	if(Count_Port>0){
		result.Format(_T("%s ����� Port�� ���� %d ��"), result, Count_Port);//���
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("����� Port�� ���� %d "), Count_Port);	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
		SetTimer(4, PORT_FIND_WAITTIME, NULL);//IO��Ʈ Ž��
	}
	else{
		result.Format(_T("%s ����� Port�� �����ϴ�. \r\n"), result);//���
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("����� Port�� �����ϴ�. "));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}

void CMFCserialportDlg::OnBnClickedButtonIni()
{
	Log.Format(_T("Ž���� ���� INI�� �����ϴ� ��ư Ŭ��"));	//�α� ���� 
	WriteLogFile(Log);	//�α� ���
	TCHAR  curFolderPath[MAX_PATH];         
	TCHAR  iniFilePath[MAX_PATH];
	ZeroMemory(curFolderPath, sizeof(curFolderPath));
	ZeroMemory(iniFilePath, sizeof(iniFilePath));
	GetCurrentDirectory( MAX_PATH, curFolderPath);
	_stprintf(iniFilePath, _T("C:\\AFC\\Ini\\DeviceConfig.ini"), curFolderPath);		

	if( (COIN_Port_Num==Before_COIN_Port_Num)&&
	    (BILL_Port_Num==Before_BILL_Port_Num)&&
		(RF_Port_Num==Before_RF_Port_Num)&&
		(IO_Port_Num==Before_IO_Port_Num))
	{
		AfxMessageBox("������ ��Ʈ���� Ž���� ��Ʈ���� �ٸ��� �ʽ��ϴ�.");
		Log.Format(_T("������ ��Ʈ���� Ž���� ��Ʈ���� �ٸ��� �ʽ��ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	else
	{
		CIni _ini;
		INT _com_port = 0;
		_ini.SetPathName(iniFilePath);
		_ini.WriteInt(_T("COM PORT"), _T("Coin Port"),     COIN_Port_Num);
		_ini.WriteInt(_T("COM PORT"), _T("Bill Port"),     BILL_Port_Num);
		_ini.WriteInt(_T("COM PORT"), _T("Recharge Port"), RF_Port_Num);
		_ini.WriteInt(_T("COM PORT"), _T("IO Port"),       IO_Port_Num);
		AfxMessageBox("INI�� ����Ͽ����ϴ�.");
		Log.Format(_T("Ž���� ��Ʈ������ INI�� ����Ͽ����ϴ�."));	//�α� ���� 
		WriteLogFile(Log);	//�α� ���
	}
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}

void CMFCserialportDlg::CreateLogFolder(){//�α� ���� ����
	currentTime = COleDateTime::GetCurrentTime();//����ð� ��������

	Now_Date = (currentTime.GetYear() * 10000) + (currentTime.GetMonth()* 100) + currentTime.GetDay();//���� �ð� ����

	Date_Folder_Name.Format(_T("%d%02d%d")
	, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());//��¥ �̸� ����
	Date_Folder_Name.Format(_T("C:\\SBS\\PORT_SCANNER\\Log\\%s"), Date_Folder_Name);//Log/��¥�̸����� ����

	CreateFolder(Date_Folder_Name);
}

void CMFCserialportDlg::CreateLogFile()//�α� ���� ����
{
 //CFile file;
	CStdioFile file;
	CString strLogPath;
	CStringA strTemp;

	currentTime = COleDateTime::GetCurrentTime();//����ð� ��������

	strLogPath.Format(_T("%s\\%04d%02d%02d.txt")//���� ��� ����
		, Date_Folder_Name, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());
	if (file.Open(strLogPath, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeText) != FALSE){
		strTemp.Format("\r\n\r\n============ ACM PORT SCANNER Log Start ============\r\n\r\n");
		file.SeekToEnd();
		file.Write(strTemp, strTemp.GetLength());
		file.Close();
	}
}

void CMFCserialportDlg::WriteLogFile(CStringA strContents)//�α׳� ���
{
 //CFile file;
	CStdioFile file;
	CString strLogPath;
	CStringA strTemp;

	currentTime = COleDateTime::GetCurrentTime();//���� �ð� ��������

	strLogPath.Format(_T("%s\\%04d%02d%02d.txt")//���� ��� ����
	, Date_Folder_Name, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());
	
	if (file.Open(strLogPath, CFile::modeWrite | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeText) != FALSE){//������ �����ٸ�

		file.SeekToEnd();
		
		strTemp.Format("[%04d/%02d/%02d %02d:%02d:%02d] %s\r\n"
		, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay()
		, currentTime.GetHour(), currentTime.GetMinute(), currentTime.GetSecond()
		, strContents);
		
		file.Write(strTemp, strTemp.GetLength());
		file.Close();
	}
	else{//������ �ʴ´ٸ�
		int write_time = (currentTime.GetYear() * 10000) + (currentTime.GetMonth()* 100) + currentTime.GetDay();//��� �ð�
		if(write_time > Now_Date){//��¥�� ��������
			Now_Date = write_time;//��¥ ������ ��¥ ����
			CreateLogFile();	//�α����� ���� ����
			WriteLogFile("��¥ ����");//��¥����
			WriteLogFile(strContents);//����
		}
	}
}

void CMFCserialportDlg::CreateFolder(CString csPath)//���� ����� �Լ�
{
    // UpdateData(TRUE);
    // csPath = m_csTopFolderName + csPath; 
    CString csPrefix(_T("")), csToken(_T(""));
    int nStart = 0, nEnd;
    while( (nEnd = csPath.Find('\\', nStart)) >= 0)
    {
        CString csToken = csPath.Mid(nStart, nEnd-nStart);
        CreateDirectory(csPrefix + csToken, NULL);

        csPrefix += csToken;
        csPrefix += _T("\\");
        nStart = nEnd+1;
    } 
    csToken = csPath.Mid(nStart);
    CreateDirectory(csPrefix + csToken, NULL);
}