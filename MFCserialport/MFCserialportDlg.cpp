
// MFCserialportDlg.cpp : 구현 파일
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

#define EXECUTE_SUCCESS					1		// 오류 없음
#define PORT_FIND_WAITTIME				2500	// 포트 탐색시간
#define PORT_RESPONS_TIME				1000	// 포트 응답시간
#define CLOSE_OPEN_TIME					100		// 포트 닫고 열리는시간
#define PERSENT_VALUE					100		// 바 게이지 최대값

CCriticalSection CMFCserialportDlg::m_cs;
ST_CONLUX_COM_INFO* CMFCserialportDlg::m_pstPack = NULL;

// 지폐처리장치 데이터 블럭 구조체
ST_BILL_DATA_FORMAT* CMFCserialportDlg::m_pDataFmt = NULL;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMFCserialportDlg 대화 상자

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


// CMFCserialportDlg 메시지 처리기

BOOL CMFCserialportDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	m_progress_coin.SetRange(0, PERSENT_VALUE);//COIN 바 초기화
	m_progress_bill.SetRange(0, PERSENT_VALUE);//BILL 바 초기화
	m_progress_rf.SetRange(0, PERSENT_VALUE);//RF 바 초기화
	m_progress_io.SetRange(0, PERSENT_VALUE);//IO 바 초기화
	m_progress_all.SetRange(0, PERSENT_VALUE*4);//전체 바 초기화(전체바의 최대값은 일반바의 4배값)
	
	//동전처리장치
	m_coin_protocol = new CCoinProtocol();	
	m_coin_protocol->CreateRecvEvent();	
	m_coin_protocol->CoinRegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::CoinReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	//지폐처리장치
	m_bill_protocol = new CBillProtocol();
	m_bill_protocol->CreateRecvEvent();
	m_bill_protocol->BillRegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::BillReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
	//보충장치
	m_rf_protocol = new CRechargeProtocol();
	m_rf_protocol->CreateRecvEvent();
	m_rf_protocol->RegisterReceiveFuncCharge(std::bind(&CMFCserialportDlg::RFReponseAnalysis, this, 
	std::placeholders::_1,	std::placeholders::_2, std::placeholders::_3));

	//IO장치
	m_io_protocol = new CIOProtocol();
	m_io_protocol->CreateRecvEvent();
	m_io_protocol->IORegCallbackFunc_RecvComp(std::bind(&CMFCserialportDlg::IOReponseAnalysis, this, 
	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_pDataFmt = new ST_BILL_DATA_FORMAT;
	ZeroMemory(m_pDataFmt, sizeof(ST_BILL_DATA_FORMAT));
	//통신 데이터 블럭 구조체 생성
	Ini_Get();//이전 INI기록
	Font_Make();//폰트 만들기
	CreateLogFolder();//로그 폴더 생성
	CreateLogFile();//로그 파일 생성

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CMFCserialportDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CMFCserialportDlg::OnOK(){//enter눌러도 종료 안되도록
	//CDialog::OnOK();
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CMFCserialportDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCserialportDlg::OnTimer(UINT_PTR nIDEvnet)//시간설정
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

void CMFCserialportDlg::Font_Make(){//폰트 만들기
	m_font.CreatePointFont(150, "굴림");
	m_button_start.SetFont(&m_font, TRUE);//폰트설정
	GetDlgItem(IDC_BUTTON_START)->SetFont(&m_font, TRUE);//폰트설정
	m_font.CreatePointFont(125, "굴림");
	m_static_before_ini.SetFont(&m_font, TRUE);//폰트설정
	m_static_after_ini.SetFont(&m_font, TRUE);//폰트설정
	m_static_coin.SetFont(&m_font, TRUE);//폰트설정
	m_static_bill.SetFont(&m_font, TRUE);//폰트설정
	m_static_rf.SetFont(&m_font, TRUE);//폰트설정
	m_static_io.SetFont(&m_font, TRUE);//폰트설정
	m_static_all_ing.SetFont(&m_font, TRUE);//폰트설정
	m_static_coin_ing.SetFont(&m_font, TRUE);//폰트설정
	m_static_bill_ing.SetFont(&m_font, TRUE);//폰트설정
	m_static_rf_ing.SetFont(&m_font, TRUE);//폰트설정
	m_static_io_ing.SetFont(&m_font, TRUE);//폰트설정
}

void CMFCserialportDlg::Coinport_Find(int Port)//동전 처리 장치 포트 찾는 함수
{
	int find_flag = 0;
	all_persent = ((PERSENT_VALUE / (Count_Port+1)) * Port) + (PERSENT_VALUE * success_part);//전체 게이지크기는 현재배열원소위치/배열크기+1 + 성공포트개수에 비례
	m_progress_all.SetPos(all_persent);//전체 바 설정
	Port_View.Format(_T("전체의 진행속도 : (%d)퍼센트"), all_persent/=4);//진행과정
	m_static_all_ing.SetWindowTextA(Port_View);//진행과정 출력
	WriteLogFile(Port_View);	//로그 출력
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//게이지크기는 현재배열원소위치/배열크기+1 비례
	m_progress_coin.SetPos(persent);//바 설정
	Port_View.Format(_T("동전 처리 장치 진행속도 : (%d)퍼센트"), persent);//진행과정
	m_static_coin_ing.SetWindowTextA(Port_View);//진행과정 출력
	WriteLogFile(Port_View);	//로그 출력
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//탐색할 포트값이 어떤 장치 포트에 속한다면
		find_flag = 1;
	}
	if(find_flag == 0){//탐색할 포트값이 어떤 장치 포트에 속하지 않을경우 탐색 시작
		m_coin_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//닫고 열리는데 사이 시간
		m_coin_protocol->SetCOMPort(Arr_Port[Port]);//포트배열 설정
		result.Format(_T("%s \r\n 시리얼 포트 번호 연결 시도 %d (동전 처리 장치) "), result, Arr_Port[Port]);//출력
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("시리얼 포트 번호 연결 시도 %d (동전 처리 장치) "), Arr_Port[Port]);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		if(m_coin_protocol->Connect_Conlux() == 1)
		{//  시리얼 통신 접속 및 설정	//연결됐을때
			Sleep(PORT_RESPONS_TIME);
			m_coin_protocol->SendCmdToConlux(CCoinProtocol::CONLUX_CMD_STANDBY);
		}
		if(Port==Count_Port-1){//포인터배열 마지막 값일 경우
			Sleep(PORT_RESPONS_TIME);
			if(COIN_Port_Num<0){//포인터 배열 탐색을 해도 못찾았을경우
				m_progress_coin.SetPos(0);//찾지 못했으므로 0으로 설정
				INDEX_Port_Num=-1;//탐색변수 초기화
				KillTimer(1);//타이머 정지
				result.Format(_T("%s \r\n 동전 처리 장치의 포트 번호는 찾지 못했습니다. "), result);//출력
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" 동전 처리 장치 포트는 찾지 못했습니다. "));//출력
				WriteLogFile(Port_View);	//로그 출력
				m_static_coin.SetWindowTextA(Port_View);
				Port_View.Format(_T("동전 처리 장치 진행속도 : (0)퍼센트"));//진행과정
				m_static_coin_ing.SetWindowTextA(Port_View);//진행과정 출력
				WriteLogFile(Port_View);	//로그 출력
				m_coin_protocol->Disconnect();//못찾았으므로 연결해제
				SetTimer(3, PORT_FIND_WAITTIME, NULL);//보충포트 탐색
			}
		}
	}
}

void CMFCserialportDlg::Billport_Find(int Port)//지폐 처리 장치 포트 찾는 함수
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//전체 게이지크기는 현재배열원소위치/배열크기+1 + 성공포트개수에 비례
	m_progress_all.SetPos(all_persent);//전체 바 설정
	Port_View.Format(_T("전체의 진행속도 : (%d)퍼센트"), all_persent/=4);//진행과정
	m_static_all_ing.SetWindowTextA(Port_View);//진행과정 출력
	WriteLogFile(Port_View);	//로그 출력
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//게이지크기는 현재배열원소위치/배열크기+1 비례
	m_progress_bill.SetPos(persent);//바 설정
	Port_View.Format(_T("지폐 처리 장치 진행속도 : (%d)퍼센트"), persent);//진행과정
	m_static_bill_ing.SetWindowTextA(Port_View);//진행과정 출력
	WriteLogFile(Port_View);	//로그 출력
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//탐색할 포트값이 어떤 장치 포트에 속한다면
		find_flag = 1;
	}
	if(find_flag == 0){//탐색할 포트값이 어떤 장치 포트에 속하지 않을경우 탐색 시작
		m_bill_protocol->Disconnect();//연결해제
		Sleep(CLOSE_OPEN_TIME);//닫고 열리는데 사이 시간
		m_bill_protocol->SetCOMPort(Arr_Port[Port]);//포트배열 설정
		result.Format(_T("%s \r\n 시리얼 포트 번호 연결 시도 %d (지폐 처리 장치) "), result, Arr_Port[Port]);//출력
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("시리얼 포트 번호 연결 시도 %d (지폐 처리 장치) "), Arr_Port[Port]);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		if(m_bill_protocol->ConnectToDevice() == 1)	
		{//  시리얼 통신 접속 및 설정	//연결됐을때
			Sleep(PORT_RESPONS_TIME/2);
			BYTE _byteCmd = ENQ;
			SendCmdToBNK(ENQ, 1000, FALSE);//ENQ전송
		}
		if(Port==Count_Port-1){//포인터배열 마지막 값일 경우
			Sleep(PORT_RESPONS_TIME*2);
			if(BILL_Port_Num<0){//포인터 배열 탐색을 해도 못찾았을경우
				m_progress_bill.SetPos(0);//찾지 못했으므로 0으로 설정
				INDEX_Port_Num=-1;//탐색변수 초기화
				KillTimer(2);//타이머 정지
				result.Format(_T("%s \r\n 지폐 처리 장치의 포트 번호는 찾지 못했습니다. "), result);//출력
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" 지폐 처리 장치 포트는 찾지 못했습니다. "));//출력
				m_static_bill.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//로그 출력
				Port_View.Format(_T("지폐 처리 장치 진행속도 : (0)퍼센트"));//진행과정
				m_static_bill_ing.SetWindowTextA(Port_View);//진행과정 출력
				WriteLogFile(Port_View);	//로그 출력
				m_bill_protocol->Disconnect();//못찾았으므로 연결해제
				Ini_Write();
			}
		}
	}
}

void CMFCserialportDlg::RFport_Find(int Port)//보충 처리 장치 포트 찾는 함수
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//전체 게이지크기는 현재배열원소위치/배열크기+1 + 성공포트개수에 비례
	m_progress_all.SetPos(all_persent);//전체 바 설정
	Port_View.Format(_T("전체의 진행속도 : (%d)퍼센트"), all_persent/=4);//진행과정
	WriteLogFile(Port_View);	//로그 출력
	m_static_all_ing.SetWindowTextA(Port_View);//진행과정 출력
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//게이지크기는 현재배열원소위치/배열크기+1 비례
	m_progress_rf.SetPos(persent);//바 설정
	Port_View.Format(_T("보충 장치 진행속도 : (%d)퍼센트"), persent);//진행과정
	WriteLogFile(Port_View);	//로그 출력
	m_static_rf_ing.SetWindowTextA(Port_View);//진행과정 출력
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//탐색할 포트값이 어떤 장치 포트에 속한다면
		find_flag = 1;
	}
	if(find_flag == 0){//탐색할 포트값이 어떤 장치 포트에 속하지 않을경우 탐색 시작
		m_rf_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//닫고 열리는데 사이 시간
		m_rf_protocol->SetCOMPort(Arr_Port[Port]);//포트배열 설정
		result.Format(_T("%s \r\n 시리얼 포트 번호 연결 시도 %d (보충 장치) "), result, Arr_Port[Port]);//출력
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("시리얼 포트 번호 연결 시도 %d (보충 장치) "), Arr_Port[Port]);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		if(m_rf_protocol->Connect() == 1)
		{//  시리얼 통신 접속 및 설정	//연결됐을때
			Sleep(PORT_RESPONS_TIME/2);
			ZeroMemory(&m_samInfo, sizeof(T_SAM_INFO_REQ));
			TCHAR cur_sam = 'C';
			m_samInfo.lsam_type = cur_sam;
			m_rf_protocol->SendPacket(CRechargeProtocol::CHARGE_CMD_SAM_INFO, (BYTE *)&m_samInfo, sizeof(T_SAM_INFO_REQ));
		}
		if(Port==Count_Port-1){//포인터 마지막 원소일 경우
			Sleep(PORT_RESPONS_TIME*2);
			if(RF_Port_Num<0){//포인터 배열 탐색을 해도 포트번호를 못찾았을경우
				m_progress_rf.SetPos(0);//찾지 못했으므로 0으로 설정
				INDEX_Port_Num=-1;//탐색변수 초기화
				KillTimer(3);//IO 타이머 정지
				result.Format(_T("%s \r\n RF 장치의 포트 번호는 찾지 못했습니다. "), result);//출력
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" 보충 장치 포트는 찾지 못했습니다. "));//출력
				m_static_rf.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//로그 출력
				Port_View.Format(_T("보충 장치 진행속도 : (0)퍼센트"));//진행과정
				WriteLogFile(Port_View);	//로그 출력
				m_static_rf_ing.SetWindowTextA(Port_View);//진행과정 출력
				m_rf_protocol->Disconnect();//못찾았으므로 연결해제
				SetTimer(2, PORT_FIND_WAITTIME, NULL);//지폐포트 탐색
			}
		}
	}
}

void CMFCserialportDlg::IOport_Find(int Port)//IO 장치 포트 찾는 함수
{
	CString tmpStr;
	int find_flag = 0;
	all_persent = (PERSENT_VALUE / (Count_Port+1)) * Port + (PERSENT_VALUE * success_part);//전체 게이지크기는 현재배열원소위치/배열크기+1 + 성공포트개수에 비례
	m_progress_all.SetPos(all_persent);//전체 바 설정
	Port_View.Format(_T("전체의 진행속도 : (%d)퍼센트"), all_persent/=4);//진행과정
	WriteLogFile(Port_View);	//로그 출력
	m_static_all_ing.SetWindowTextA(Port_View);//진행과정 출력
	persent = (PERSENT_VALUE / (Count_Port+1)) * Port;//게이지크기는 현재배열원소위치/배열크기+1 비례
	m_progress_io.SetPos(persent);//바 설정
	Port_View.Format(_T("IO 장치 진행속도 : (%d)퍼센트"), persent);//진행과정
	m_static_io_ing.SetWindowTextA(Port_View);//진행과정 출력
	WriteLogFile(Port_View);	//로그 출력
	if( (Arr_Port[Port]==COIN_Port_Num)||
		(Arr_Port[Port]==BILL_Port_Num)||
		(Arr_Port[Port]==RF_Port_Num)||
		(Arr_Port[Port]==IO_Port_Num)){//탐색할 포트값이 어떤 장치 포트에 속한다면
		find_flag = 1;
	}
	if(find_flag == 0){//탐색할 포트값이 어떤 장치 포트에 속하지 않을경우 탐색 시작
		m_io_protocol->Disconnect();
		Sleep(CLOSE_OPEN_TIME);//닫고 열리는데 사이 시간
		m_io_protocol->SetCOMPort(Arr_Port[Port]);//포트배열 설정
		result.Format(_T("%s \r\n 시리얼 포트 번호 연결 시도 %d (IO 장치) "), result, Arr_Port[Port]);//출력
		Log.Format(_T("시리얼 포트 번호 연결 시도 %d (IO 장치) "), Arr_Port[Port]);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		m_edit_rcv_view.SetWindowTextA(result);
		if(m_io_protocol->ConnectACM() == 1)
		{//  시리얼 통신 접속 및 설정	//연결됐을때
			//tmpStr.Format(_T("%s /r/nArr_Port(%d) : %d"), tmpStr, Count_Port, port);//출력
			Sleep(PORT_RESPONS_TIME/2);//1초대기
			m_io_protocol->SendInputCmdToIO(CIOProtocol::IO_CMD_PACKET_STATUS);//IO포트에게 메시지전송
		}
		if(Port==Count_Port-1){//포인터 마지막 원소일 경우
			Sleep(PORT_RESPONS_TIME*2);
			if(IO_Port_Num<0){//포인터 배열 탐색을 해도 포트번호를 못찾았을경우
				m_progress_io.SetPos(0);//찾지 못했으므로 0으로 설정
				INDEX_Port_Num=-1;//탐색변수 초기화
				KillTimer(4);//IO 타이머 정지
				result.Format(_T("%s \r\n IO 장치의 포트 번호는 찾지 못했습니다. "), result);//출력
				m_edit_rcv_view.SetWindowTextA(result);
				Port_View.Format(_T(" IO 장치 포트는 찾지 못했습니다. "));//출력
				m_static_io.SetWindowTextA(Port_View);
				WriteLogFile(Port_View);	//로그 출력
				Port_View.Format(_T("IO 장치 진행속도 : (0)퍼센트"));//진행과정
				m_static_io_ing.SetWindowTextA(Port_View);//진행과정 출력
				WriteLogFile(Port_View);	//로그 출력
				m_io_protocol->Disconnect();//못찾았으므로 연결해제
				SetTimer(1, PORT_FIND_WAITTIME, NULL);//동전포트 탐색
			}
		}
	}
}

void CMFCserialportDlg::Port_Direct()//포트 탐색
{
	int i;
	for(i = 0; i< 11; i++){
		Sleep(50);
		m_coin_protocol->SetCOMPort(i);//포트배열 설정
		if(m_coin_protocol->Connect_Conlux() == 1){
			Arr_Port[Count_Port] = i;//열렸을 경우 해당포트값을 포트배열에 담는다 
			result.Format(_T("%s Arr_Port(%d) : %d \r\n"), result, Count_Port, i);//출력
			Log.Format(_T("Arr_Port(%d) : %d"), Count_Port, i);	//로그 내용 
			Count_Port++;//포트배열개수 1증가
			m_edit_rcv_view.SetWindowTextA(result);			
			WriteLogFile(Log);	//로그 출력
		}
		m_coin_protocol->Disconnect();
	}
}

//  동전 시리얼 수신 함수
VOID CMFCserialportDlg::CoinReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength)
{
	if(Coin_flag==FALSE){//처음 동전 포트를 찾은경우
		success_part++;//포트 찾은경우 현재까지 포트 찾은 개수 증가
		KillTimer(1);
		m_progress_coin.SetPos(PERSENT_VALUE);//찾았으므로 100%으로 설정
		Port_View.Format(_T("동전 처리 장치 진행속도 : (100)퍼센트"));//찾았으므로 100%으로 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_coin_ing.SetWindowTextA(Port_View);//진행과정 출력
		INDEX_Port_Num = -1;
		COIN_Port_Num = m_coin_protocol->GetCOMPort();
		result.Format(_T("%s \r\n 시리얼 포트 번호 %d 동전 처리 장치와 연결 성공 "), result, COIN_Port_Num);//출력
		Log.Format(_T("시리얼 포트 번호 %d 동전 처리 장치와 연결 성공 "), COIN_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(3, 2500, NULL);
		Port_View.Format(_T(" 동전 처리 장치 포트 : %d "), COIN_Port_Num);//동전 결과 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_coin.SetWindowTextA(Port_View);
		m_coin_protocol->Disconnect();//연결해제
		Coin_flag=TRUE;//동전 포트 찾았으므로 플래그 참으로
	}
}

//  지폐 시리얼 수신 함수
VOID CMFCserialportDlg::BillReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength)
{
	if(Bill_flag==FALSE){//처음 지폐 포트를 찾은경우
		success_part++;//포트 찾은경우 현재까지 포트 찾은 개수 증가
		KillTimer(2);
		m_progress_bill.SetPos(PERSENT_VALUE);//찾았으므로 100%으로 설정
		Port_View.Format(_T("지폐 처리 장치 진행속도 : (100)퍼센트"));//찾았으므로 100%으로 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_bill_ing.SetWindowTextA(Port_View);//진행과정 출력
		INDEX_Port_Num = -1;
		BILL_Port_Num = m_bill_protocol->GetCOMPort();
		result.Format(_T("%s \r\n 시리얼 포트 번호 %d 지폐 처리 장치와 연결 성공 "), result, BILL_Port_Num);//출력
		Log.Format(_T("시리얼 포트 번호 %d 지폐 처리 장치와 연결 성공 "), BILL_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		m_edit_rcv_view.SetWindowTextA(result);
		Port_View.Format(_T(" 지폐 처리 장치 포트 : %d "), BILL_Port_Num);//지폐 결과 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_bill.SetWindowTextA(Port_View);
		if(success_part>2){//나머지3개의포트를 다찾았을경우
			m_progress_all.SetPos(PERSENT_VALUE * 4);//전부 찾았으므로 100퍼
			Port_View.Format(_T("전체의 진행속도 : (100)퍼센트 - 모든 포트를 찾았습니다."));//진행과정(전부 찾았으므로 100퍼)
			WriteLogFile(Port_View);	//로그 출력
			m_static_all_ing.SetWindowTextA(Port_View);//진행과정 출력
		}
		m_bill_protocol->Disconnect();//연결해제
		Sleep(PORT_RESPONS_TIME);//기록전 시간텀
		if(Ini_Write()==1){
			result.Format(_T("%s \r\n INI기록하였습니다. "), result);//INI 출력
			Log.Format(_T("INI기록하였습니다."));	//로그 내용 
			WriteLogFile(Log);	//로그 출력
			m_edit_rcv_view.SetWindowTextA(result);
		};
		Bill_flag=TRUE;//지폐 포트 찾았으므로 플래그 참으로
	}
}

//  보충처리장치 시리얼 수신 함수
VOID CMFCserialportDlg::RFReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength){
//VOID CMFCserialportDlg::RFReponseAnalysis(_In_ BYTE *pData, _In_ INT nLength, _In_ INT nResult ){
	if(RF_flag==FALSE){//처음 보충 포트를 찾은경우
		success_part++;//포트 찾은경우 현재까지 포트 찾은 개수 증가
		KillTimer(3);
		m_progress_rf.SetPos(PERSENT_VALUE);//찾았으므로 100%으로 설정
		Port_View.Format(_T("보충 장치 진행속도 : (100)퍼센트"));//찾았으므로 100%으로 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_rf_ing.SetWindowTextA(Port_View);//진행과정 출력
		INDEX_Port_Num = -1;
		RF_Port_Num = m_rf_protocol->GetCOMPort();
		result.Format(_T("%s \r\n 시리얼 포트 번호 %d 보충 처리 장치와 연결 성공 "), result, RF_Port_Num);//출력
		Log.Format(_T("시리얼 포트 번호 %d 보충 처리 장치와 연결 성공 "), RF_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(2, 2500, NULL);
		Port_View.Format(_T(" 보충 장치 포트 : %d "), RF_Port_Num);//보충 결과 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_rf.SetWindowTextA(Port_View);
		m_rf_protocol->Disconnect();//연결해제
		RF_flag=TRUE;//보충 포트 찾았으므로 플래그 참으로
	}
}

//  IO 장치 시리얼 수신 함수
VOID CMFCserialportDlg::IOReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength){
	if(IO_flag==FALSE){//처음 IO 포트를 찾은경우
		success_part++;//포트 찾은경우 현재까지 포트 찾은 개수 증가
		KillTimer(4);
		m_progress_io.SetPos(PERSENT_VALUE);//찾았으므로 100%으로 설정
		Port_View.Format(_T("IO 장치 진행속도 : (100)퍼센트"));//찾았으므로 100%으로 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_io_ing.SetWindowTextA(Port_View);//진행과정 출력
		INDEX_Port_Num = -1;
		IO_Port_Num = m_io_protocol->GetCOMPort();
		result.Format(_T("%s \r\n 시리얼 포트 번호 %d IO 처리 장치와 연결 성공 "), result, IO_Port_Num);//출력
		Log.Format(_T("시리얼 포트 번호 %d IO 장치와 연결 성공 "), IO_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		m_edit_rcv_view.SetWindowTextA(result);
		SetTimer(1, 2500, NULL);
		Port_View.Format(_T(" IO 장치 포트 : %d "), IO_Port_Num);//IO 결과 출력
		WriteLogFile(Port_View);	//로그 출력
		m_static_io.SetWindowTextA(Port_View);
		m_io_protocol->Disconnect();//연결해제
		IO_flag=TRUE;//IO 포트 찾았으므로 플래그 참으로
	}
}

afx_msg LRESULT CMFCserialportDlg::TestMessage(WPARAM wParam, LPARAM lParam)//동전처리장치 사용자정의 메시지
{
	INDEX_Port_Num++;
	Coinport_Find(INDEX_Port_Num);
	if(COIN_Port_Num>=0){
		KillTimer(1);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::BillTestMessage(WPARAM wParam, LPARAM lParam)//지폐처리장치 사용자정의 메시지
{
	INDEX_Port_Num++;
	Billport_Find(INDEX_Port_Num);
	if(BILL_Port_Num>=0){
		KillTimer(2);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::RFTestMessage(WPARAM wParam, LPARAM lParam)//보충장치 사용자정의 메시지
{
	INDEX_Port_Num++;
	RFport_Find(INDEX_Port_Num);
	if(RF_Port_Num>=0){
		KillTimer(3);
		INDEX_Port_Num=-1;
	}
	return 0;
}

afx_msg LRESULT CMFCserialportDlg::IOTestMessage(WPARAM wParam, LPARAM lParam)//IO장치 사용자정의 메시지
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
 * IN  : const BYTE _cmd(지폐처리장치 명령), BOOL _bTCC(전송 타입이 통신 제어 문자인지 판단)
 * Out : 전송 성공 여부(TRUE:성공, FALSE:실패)
 * Desc: 지폐처리장치 명령 송신
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

	// 전송 제어 문자 송신인 경우
	if(_bTCC == TRUE)
	{
		INT _length = 0;
		BYTE _symbol[128];
		memset(_symbol, 0x00, sizeof(_symbol));

		if(_cmd == ENQ){//[ENQ] 전송
		}
		else if(_cmd == 0x30){//[DLE0] 전송
		}
		else if(_cmd == 0x31){//[DLE1] 전송
		}
		else if(_cmd == EOT){//[EOT] 전송
		}
		else{//[%02X] 전송
		}

		if(_cmd == 0x30 || _cmd == 0x31)
		{// DLE 0 또는 DLE 1
			_symbol[0] = DLE;
			_symbol[1] = _cmd;
			_length = 2;
		}
		else
		{// 기타 전송 제어 문자
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
			// Authorized 명령 경우 지폐 권종 mask 값 전달
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
	// 타이머 가동
	Sleep(CLOSE_OPEN_TIME);
	return TRUE;
}

INT CMFCserialportDlg::Ini_Get()//이전 ini 포트기록
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
	Result_MSG.Format(_T("이전 INI 포트 값\r\n"));//출력

	Result_MSG.Format(_T("%sIO 장치 : %d\r\n"), Result_MSG, Before_IO_Port_Num);//출력
	Log.Format(_T("IO 장치 : %d"), Before_IO_Port_Num);//로그 내용 
	WriteLogFile(Log);	//로그 출력
	Result_MSG.Format(_T("%s동전 처리 장치 : %d\r\n"), Result_MSG, Before_COIN_Port_Num);//출력
	Log.Format(_T("동전 처리 장치 : %d"), Before_COIN_Port_Num);//로그 내용 
	WriteLogFile(Log);	//로그 출력
	Result_MSG.Format(_T("%s지폐 처리 장치 : %d\r\n"), Result_MSG, Before_BILL_Port_Num);//출력
	Log.Format(_T("지폐 처리 장치 : %d"), Before_BILL_Port_Num);//로그 내용 
	WriteLogFile(Log);	//로그 출력
	Result_MSG.Format(_T("%s보충 장치 : %d\r\n"), Result_MSG, Before_RF_Port_Num);//출력
	Log.Format(_T("보충 장치 : %d"), Before_RF_Port_Num);//로그 내용 
	WriteLogFile(Log);	//로그 출력
	
	m_static_before_ini.SetWindowTextA(Result_MSG);
	WriteLogFile(Result_MSG);//로그 기록
	return 1;
}

INT CMFCserialportDlg::Ini_Write()//ini 포트기록
{	
	CString Result_MSG;
	Result_MSG.Format(_T("탐색 후 INI 포트 값\r\n"));//출력
	Log.Format(_T("탐색 후 INI 포트 값"));	//로그 내용 
	WriteLogFile(Log);	//로그 출력
	if(IO_Port_Num>=0){
		Result_MSG.Format(_T("%sIO 장치 : %d\r\n"), Result_MSG, IO_Port_Num);//출력
		Log.Format(_T("IO 장치 : %d"), IO_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	else{
		Result_MSG.Format(_T("%sIO 장치 포트는\r\n 찾지 못했습니다.\r\n"), Result_MSG);//출력
		Log.Format(_T("IO 장치 포트는 찾지 못했습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	if(COIN_Port_Num>=0){
		Result_MSG.Format(_T("%s동전 처리 장치 : %d\r\n"), Result_MSG, COIN_Port_Num);//출력
		Log.Format(_T("동전 처리 장치 : %d"), COIN_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	else{
		Result_MSG.Format(_T("%s동전 처리 장치 포트는\r\n 찾지 못했습니다.\r\n"), Result_MSG);//출력
		Log.Format(_T("동전 처리 장치 포트는 찾지 못했습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	if(BILL_Port_Num>=0){
		Result_MSG.Format(_T("%s지폐 처리 장치 : %d\r\n"), Result_MSG, BILL_Port_Num);//출력
		Log.Format(_T("지폐 처리 장치 : %d"), BILL_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	else{
		Result_MSG.Format(_T("%s지폐 처리 장치 포트는\r\n 찾지 못했습니다.\r\n"), Result_MSG);//출력
		Log.Format(_T("지폐 처리 장치 포트는 찾지 못했습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	if(RF_Port_Num>=0){
		Result_MSG.Format(_T("%s보충 장치 : %d\r\n"), Result_MSG, RF_Port_Num);//출력
		Log.Format(_T("보충 장치 : %d"), RF_Port_Num);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	else{
		Result_MSG.Format(_T("%s보충 장치 포트는\r\n 찾지 못했습니다.\r\n"), Result_MSG);//출력
		Log.Format(_T("보충 장치 포트는 찾지 못했습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	Result_MSG.Format(_T("%s 탐색한 포트값을\n 적용하시려면\r\n 아래 버튼을\n 클릭하세요.\r\n"), Result_MSG);//출력
	m_static_after_ini.SetWindowTextA(Result_MSG);
	m_button_start.EnableWindow();
	return 1;
}

void CMFCserialportDlg::OnBnClickedButtonStart()//포트 탐색 시작
{
	Log.Format(_T("시작 버튼 클릭"));	//로그 내용 
	WriteLogFile(Log);	//로그 출력
	Port_Direct();//연결된 포트 찾기
	if(Count_Port>0){
		result.Format(_T("%s 연결된 Port의 개수 %d 개"), result, Count_Port);//출력
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("연결된 Port의 개수 %d "), Count_Port);	//로그 내용 
		WriteLogFile(Log);	//로그 출력
		SetTimer(4, PORT_FIND_WAITTIME, NULL);//IO포트 탐색
	}
	else{
		result.Format(_T("%s 연결된 Port가 없습니다. \r\n"), result);//출력
		m_edit_rcv_view.SetWindowTextA(result);
		Log.Format(_T("연결된 Port가 없습니다. "));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CMFCserialportDlg::OnBnClickedButtonIni()
{
	Log.Format(_T("탐색한 값을 INI에 적용하는 버튼 클릭"));	//로그 내용 
	WriteLogFile(Log);	//로그 출력
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
		AfxMessageBox("이전의 포트값과 탐색한 포트값이 다르지 않습니다.");
		Log.Format(_T("이전의 포트값과 탐색한 포트값이 다르지 않습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
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
		AfxMessageBox("INI에 기록하였습니다.");
		Log.Format(_T("탐색한 포트값으로 INI에 기록하였습니다."));	//로그 내용 
		WriteLogFile(Log);	//로그 출력
	}
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CMFCserialportDlg::CreateLogFolder(){//로그 폴더 생성
	currentTime = COleDateTime::GetCurrentTime();//현재시간 가져오기

	Now_Date = (currentTime.GetYear() * 10000) + (currentTime.GetMonth()* 100) + currentTime.GetDay();//시작 시간 변경

	Date_Folder_Name.Format(_T("%d%02d%d")
	, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());//날짜 이름 폴더
	Date_Folder_Name.Format(_T("C:\\SBS\\PORT_SCANNER\\Log\\%s"), Date_Folder_Name);//Log/날짜이름폴더 생성

	CreateFolder(Date_Folder_Name);
}

void CMFCserialportDlg::CreateLogFile()//로그 파일 생성
{
 //CFile file;
	CStdioFile file;
	CString strLogPath;
	CStringA strTemp;

	currentTime = COleDateTime::GetCurrentTime();//현재시간 가져오기

	strLogPath.Format(_T("%s\\%04d%02d%02d.txt")//파일 경로 지정
		, Date_Folder_Name, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());
	if (file.Open(strLogPath, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeText) != FALSE){
		strTemp.Format("\r\n\r\n============ ACM PORT SCANNER Log Start ============\r\n\r\n");
		file.SeekToEnd();
		file.Write(strTemp, strTemp.GetLength());
		file.Close();
	}
}

void CMFCserialportDlg::WriteLogFile(CStringA strContents)//로그내 기록
{
 //CFile file;
	CStdioFile file;
	CString strLogPath;
	CStringA strTemp;

	currentTime = COleDateTime::GetCurrentTime();//현재 시간 가져오기

	strLogPath.Format(_T("%s\\%04d%02d%02d.txt")//파일 경로 지정
	, Date_Folder_Name, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay());
	
	if (file.Open(strLogPath, CFile::modeWrite | CFile::modeNoTruncate | CFile::modeWrite | CFile::typeText) != FALSE){//파일이 열린다면

		file.SeekToEnd();
		
		strTemp.Format("[%04d/%02d/%02d %02d:%02d:%02d] %s\r\n"
		, currentTime.GetYear(), currentTime.GetMonth(), currentTime.GetDay()
		, currentTime.GetHour(), currentTime.GetMinute(), currentTime.GetSecond()
		, strContents);
		
		file.Write(strTemp, strTemp.GetLength());
		file.Close();
	}
	else{//열리지 않는다면
		int write_time = (currentTime.GetYear() * 10000) + (currentTime.GetMonth()* 100) + currentTime.GetDay();//기록 시간
		if(write_time > Now_Date){//날짜가 지났으면
			Now_Date = write_time;//날짜 지나면 날짜 수정
			CreateLogFile();	//로그파일 새로 생성
			WriteLogFile("날짜 변경");//날짜변경
			WriteLogFile(strContents);//재기록
		}
	}
}

void CMFCserialportDlg::CreateFolder(CString csPath)//폴더 만드는 함수
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