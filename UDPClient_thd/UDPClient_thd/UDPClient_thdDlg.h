
// UDPClient_thdDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CDataSocket;

struct ThreadArg //스레드 정의
{
	CStringList* pList;
	CDialogEx* pDlg;
	int Thread_run; //스레드 제어 변수
};

struct UDP_datagram { // 보낼 패킷을 구성
	//UDP 헤더
	short checksum = 0; //checksum_field 체크섬필드는 처음엔 0

	short source_port;
	short destination_port;

	short length;
	int seq;

	int source_ip;//32bit
	int destination_ip;//32bit
	
	//Data
	CString data; //16byte짜리 데이터
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
	CWinThread *pThread1, *pThread2; //스레드 객체 주소
	ThreadArg arg1, arg2; //스레드 전달 인자
	CDataSocket *m_pDataSocket;
	CEdit m_tx_edit_short;
	CEdit m_tx_edit;
	CEdit m_rx_edit;
	CIPAddressCtrl m_ip_addr;
	afx_msg void OnBnClickedSend();
	afx_msg void OnBnClickedClose();
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);
	CIPAddressCtrl m_ipaddr;
	CString HostAddress; //전송할 ip를 저장합니다.
	UINT HostPort = 6789; // 서버의 port주소입니다.
};
