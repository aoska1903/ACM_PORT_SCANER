
// MFCserialportDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include <functional>
#include "CoinProcessorData.h"
#include "BillProcessorData.h"
#include "afxcmn.h"
// 통신 데이터 구조체
typedef struct _st_com_info
{
	BYTE coin_command;	// 동전처리장치 코멘드
	BYTE response_type;	// 응답 (ie, ACK11, ACK22, ...) 	
	BYTE data_cmd;		// 데이터 커멘드
	BYTE ctrl_type;		// 제어데이터 타입(0: CREM OFF, 1:CREM ON, 2:CLEAR, ...) 
	BYTE recv_buf[128]; // 응답 데이터
	INT retry_cnt;		// 재시도 횟수
	INT data_size;		// 응답 데이터 크기
	bool is_data_send;	// 데이터 전송 유무(false: 데이터 전송 없음, true:데이터 전송 후 응답 대기)
}ST_CONLUX_COM_INFO;

// 통신 데이터 구조체
typedef struct _bnk_data_block_format
{
	BYTE command;
	BYTE recv_buf[128];	// 수신 데이터 버퍼
	BYTE text[128];		// 수신 된 TEXT 데이터
	INT  length;		// 수신 데이터 길이
	INT  text_length;	// 수신 된 TEXT 데이터 길이
	INT  retry_cnt;
	INT  cur_state;
}ST_BILL_DATA_FORMAT;

static const BYTE ENQ = 0x05;
static const BYTE ACK = 0x06;
static const BYTE EOT = 0x04;
static const BYTE DLE = 0x10;

// 투입 허용 가능한 권종 mask 값
//WORD		m_wEnableBanknoteMask_Dlg;

//----------------------------------------------------------------------//
// @ SAM 정보 요청 구조체 
//--------------------------------------------------------------------
typedef struct _sam_info_req 
{
	char          lsam_type ;        // 'U', 'L', 'T','C', 또는 slot_num ;
	unsigned char lsam_pass[8];
}T_SAM_INFO_REQ;

class CCoinProtocol;
class CBillProtocol;
class CRechargeProtocol;
class CIOProtocol;
// CMFCserialportDlg 대화 상자
class CMFCserialportDlg : public CDialogEx
{
// 생성입니다.
public:
	CMFCserialportDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
	enum { IDD = IDD_MFCSERIALPORT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT TestMessage(WPARAM wParam, LPARAM lParam);//동전 사용자정의 메시지
	afx_msg LRESULT BillTestMessage(WPARAM wParam, LPARAM lParam);//지폐 사용자정의 메시지
	afx_msg LRESULT RFTestMessage(WPARAM wParam, LPARAM lParam);//보충장치 사용자정의 메시지
	afx_msg LRESULT IOTestMessage(WPARAM wParam, LPARAM lParam);//IO장치 사용자정의 메시지
	DECLARE_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// Member Variable
public:
	static CCriticalSection m_cs;

public:
	INT COIN_Port_Num;//동전처리장치 포트번호
	INT BILL_Port_Num;//지폐처리장치 포트번호
	INT RF_Port_Num;//보충처리장치 포트번호
	INT IO_Port_Num;//IO 포트번호
	INT INDEX_Port_Num;//탐색할 포트번호
	INT persent;//바 게이지
	INT all_persent;//전체바 게이지
	INT success_part;//현재까지 포트 찾은 개수

	INT Before_COIN_Port_Num;//이전 동전처리장치 포트번호
	INT Before_BILL_Port_Num;//이전 지폐처리장치 포트번호
	INT Before_RF_Port_Num;//이전 보충처리장치 포트번호
	INT Before_IO_Port_Num;//이전 IO 포트번호

	int Arr_Port[12];//시리얼 연결되는 포트 배열
	int Count_Port;//연결된 포트개수
	BOOL Coin_flag;//동전 포트 찾았는지에 대한 변수
	BOOL Bill_flag;//지폐 포트 찾았는지에 대한 변수
	BOOL RF_flag;//RF 포트 찾았는지에 대한 변수
	BOOL IO_flag;//IO 포트 찾았는지에 대한 변수

	// 동전처리장치 명령 코드 Get/Set
	BYTE GetCoinCmd();
	void OnOK();//enter눌러도 종료 안되도록
	
	CString result;//결과 로그
	CString Port_View;//찾은 포트값 edit 박스에 출력

	void Font_Make();//폰트 만들기

	void Coinport_Find(int Port);//동전 처리 장치 포트 찾는 함수
	void Billport_Find(int Port);//지폐 처리 장치 포트 찾는 함수
	void RFport_Find(int Port);//지폐 처리 장치 포트 찾는 함수
	void IOport_Find(int Port);//IO 장치 포트 찾는 함수
	void Port_Direct();//포트 탐색 변수
	INT Ini_Get();//이전 ini 포트기록
	INT Ini_Write();//ini 포트기록
	
	void CreateLogFolder();	//로그 폴더 생성
	void CreateLogFile();	//로그 파일 생성
	void WriteLogFile(CStringA strContents);//로그내 기록
	void CreateFolder(CString csPath);//폴더 생성 함수
	CString Log;//로그 기록할 변수

	CFont m_font;//폰트

	CEdit m_edit_rcv_view;//진행과정보여주는 변수
	CStatic m_static_coin;//coin 결과값
	CStatic m_static_bill;//bill 결과값
	CStatic m_static_io;  //rf 결과값
	CStatic m_static_rf;  //io 결과값
	CProgressCtrl m_progress_coin;//동전 바
	CProgressCtrl m_progress_bill;//지폐 바
	CProgressCtrl m_progress_rf;  //RF 바
	CProgressCtrl m_progress_io;  //IO 바
	CProgressCtrl m_progress_all; //전체 바
	CStatic m_static_coin_ing;//coin 진행상태
	CStatic m_static_bill_ing;//bill 진행상태
	CStatic m_static_rf_ing;  //rf 진행상태
	CStatic m_static_io_ing;  //io 진행상태
	CStatic m_static_all_ing; //전체 진행상태
	CStatic m_static_before_ini;//이전INI값
	CStatic m_static_after_ini;//현재INI값
	CButton m_button_start;//시작 버튼

	COleDateTime currentTime;//시간 변수
	CString m_sModulePath;//log filel 경로
	int Now_Date;//오늘 날짜
	int Now_Transation;// 현재 상태
	CString Date_Folder_Name;//Log 하위 폴더

	// 지폐처리장치 커멘드
	static ST_BILL_DATA_FORMAT*	m_pDataFmt;

	afx_msg void OnTimer(UINT_PTR nIDEvnet);//시간설정
	afx_msg void OnBnClickedButtonStart();//포트 탐색 시작
	afx_msg void OnBnClickedButtonIni();//INI 기록 시작

	//동전처리장치 수신 결과 후 분석 처리
	VOID CoinReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	//지폐처리장치 수신 결과 후 분석 처리
	VOID BillReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);
	
	//보충처리장치 수신 결과 후 분석 처리
	//VOID RFReponseAnalysis(_In_ BYTE *pData, _In_ INT nLength, _In_ INT nResult);
	 VOID RFReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	//IO장치 수신 결과 후 분석 처리
	VOID IOReponseAnalysis(_In_ INT nResult, _In_ BYTE *pData, _In_ INT nLength);

	// 지폐처리장치 명령 송신
	BOOL SendCmdToBNK(_In_ const BYTE _cmd, _In_ INT _wait_time, _In_opt_ BOOL _bTCC=TRUE);

private:
	T_COIN_DATA				*m_p_Data;
	CCoinProcessorData		*m_p_mmem;		// 동전처리장치 메모리 맵 포인터
	
	// 동전처리장치 통신 정보 구조체
	static ST_CONLUX_COM_INFO *m_pstPack;

	// 시리얼 통신 헬퍼 클래스 객체(동전 처리 장치)
	CCoinProtocol* m_coin_protocol;
	
	// 시리얼 통신 헬퍼 클래스 객체(지폐 처리 장치)
	CBillProtocol* m_bill_protocol;
	
	// 시리얼 통신 헬퍼 클래스 객체(보충 장치)
	CRechargeProtocol* m_rf_protocol;
	
	// 시리얼 통신 헬퍼 클래스 객체(보충 장치)
	CIOProtocol* m_io_protocol;

	T_SAM_INFO_REQ           m_samInfo;//RF SAM 정보
};
