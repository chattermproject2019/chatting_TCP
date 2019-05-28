
// UDPClient_thdDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CDataSocket;

struct Response { // 4byte, 수신에 대한 응답에 대한 정보이다. stop&wait, Go&Back, Selective Reject에 쓰인다.
	unsigned short ACK; // n번 frame에 대한 확인응답, frame받고 해당 frame의 seq를전송, seq는 항상 1부터 시작.
	bool no_error; // negative Ack로 할것인지 결정. true면 에러가 없는거고. false는 frame에 에러가 있었음을 알림.
	bool more; // RR인지 REJ인지 구별// true이면 더 수신가능함을, false이면 지금은 더 이상 수신안되고, 좀있다가 시도해줄것을 알림.

	Response() { ACK = 0; no_error = more = true; } // ACK 0이면 frame정보가 초기화되지 않은거임. seq는 1부터 시작하니깐
};

struct Packet { // 16byte = 128bit
	unsigned short seq; // 2yte = 16bit, 
	unsigned short checksum; // 2byte = 16bit
	unsigned short total_sequence_number; // 2byte = 16bit => 총 보내는 frame수
	Response response; // 4byte, 응답메세지이다. 
	unsigned char data[6]; // 6btye = 48bit, 순수 데이터만 48bit = 6byte저장가능
	Packet() { seq = 1; response = Response(); checksum = 0; total_sequence_number = 0; memset(data, 0, sizeof(data)); }
};

struct ThreadArg //스레드 인수 정의
{
	CStringList* pList;
	CDialogEx* pDlg;
	int Thread_run; //스레드 제어 변수
};

struct timerThreadArg //타이머 스레드 인수 정의
{
	int timer_id;
	int deadline;
	int frame_seq;
	CDialogEx* pDlg;
};

// CUDPClient_thdDlg 대화 상자
class CUDPClient_thdDlg : public CDialogEx
{
// 생성입니다.
public:
	CUDPClient_thdDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDPCLIENT_THD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CWinThread *pThread1, *pThread2, *timerThread; //스레드 객체 주소
	ThreadArg arg1, arg2; //스레드 전달 인자
	timerThreadArg arg3; // 타이머 스레드 인수


	CDataSocket *m_pDataSocket;
	CEdit m_tx_edit_short;
	CEdit m_tx_edit;
	CEdit m_rx_edit;
	afx_msg void OnBnClickedSend();
	afx_msg void OnBnClickedClose();
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);

	CIPAddressCtrl m_ipaddr;
	CString temp_str;
	CString myIp;
	int myPort;
	CString peerIp;
	int peerPort;
	CArray<Packet> packet_send_buffer; // 보낼 패킷을 저장해 놓는 버퍼
	CArray<Packet> packet_receive_buffer; // 받은 패킷을 저장해 놓는 버퍼
	CArray<int> ack_receive_buffer; // 받은 ack메세지의 frame넘버를 저장해 놓는 버퍼입니다. 음수면 nack의 frame넘버입니다.

	void packetSegmentation(CString);
	 
	afx_msg void OnTimer(UINT_PTR nIDEvent); // 타이머 용, 클래스 뷰에서 WM_Timer 추가하면됨
	void StartTimer(unsigned int timer_id, unsigned int n); // 타이머 시작
	void StopTimer(unsigned int timer_id); // 타이머 끄기

	BOOL timeout = false;
	//int next_sequnce; //  다음으로 보낼 frame 넘버를 저장하는 변수, frame을 보낼때, 잘 보냈거나, 에러가 나면, 보내야하는 sequence가 달라지기 때문에,  

	unsigned short window_size; // Go Back N은 최대 2^(seq넘버 bit수 =지금은 16bit) - 1 까지 가능
	unsigned short mode; // 0-Stop&Wait, 1-GoBackN, 2-SelectiveRecject

	int current_error_frame = -1;

	bool overlap_checker_Lock = false;
	bool* overlap_checker;

	CArray<Packet> ack_send_buffer; // ack메세지를 보내야 하는경우, 이 버퍼에 저장했다가 보냅니다.

	CArray<int> error_buffer; // Selective Reject용 에러버퍼, 에러가 난 frame seq를 저장합니다.

	BOOL timer_id_checker[1000] = {0,};
	BOOL mRadio;
	afx_msg void OnBnClickedButton1();
	CEdit m_windowSize;
	int m_windowSzie_val;
	afx_msg void OnBnClickedOpen();
};
