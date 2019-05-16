
// UDPServer_thdDlg.h : 헤더 파일
//

#pragma once
#include "afxwin.h"

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
	CString rSocketAddr = _T("");  // 최근에 통신한 ip주소를 저장
	UINT rSocketPort = -1; // 최근에 통신한 port주소를 저장합니다. (현재 같은 pc내에서 동작해야하므로 서버포트는 6789, 클라이언트 포트는 8000 지정해준상태)

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
	CString m_edit1_text; // DDV로 메세지 길이 제한할녀석
};
