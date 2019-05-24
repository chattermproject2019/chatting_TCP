
// UDPServer_thdDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"

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

struct ThreadArg //스레드 정의
{
	CStringList *pList;
	CDialogEx *pDlg;
	int Thread_run; //스레드 제어 변수
};

class CDataSocket;

// CUDPServer_thdDlg 대화 상자
class CUDPServer_thdDlg : public CDialogEx
{
// 생성입니다.
public:
	CUDPServer_thdDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDPSERVER_THD_DIALOG };
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
	CEdit m_tx_edit_short;
	CEdit m_tx_edit;
	CEdit m_rx_edit;
	afx_msg void OnBnClickedSend();
	CWinThread *pThread1, *pThread2; //스레드 객체 줏소
	ThreadArg arg1, arg2; //스레드 전달 인자
	CDataSocket *m_pDataSocket;
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);
	afx_msg void OnBnClickedDisconnect();

	CString myIp;
	int myPort;
	CString peerIp = _T("");  // 최근에 통신한 ip주소를 저장
	UINT peerPort = -1; // 최근에 통신한 port주소를 저장합니다. (현재 같은 pc내에서 동작해야하므로 서버포트는 6789, 클라이언트 포트는 8000 지정해준상태)


	CArray<Packet> packet_send_buffer; // 보낼 패킷을 저장해 놓는 버퍼 // 1:1 chatting이기 때문에 따로 client table은 만들지 않습니다.

	CArray<Packet> packet_receive_buffer; // 받은 패킷을 저장해 놓는 버퍼
	
	void packetSegmentation(CString message);
};
