
// UDPServer_thdDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"

struct ThreadArg //������ ����
{
	CStringList *pList;
	CDialogEx *pDlg;
	int Thread_run; //������ ���� ����
};

class CDataSocket;

// CUDPServer_thdDlg ��ȭ ����
class CUDPServer_thdDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CUDPServer_thdDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
	CString rSocketAddr = _T("");  // �ֱٿ� ����� ip�ּҸ� ����
	UINT rSocketPort = -1; // �ֱٿ� ����� port�ּҸ� �����մϴ�. (���� ���� pc������ �����ؾ��ϹǷ� ������Ʈ�� 6789, Ŭ���̾�Ʈ ��Ʈ�� 8000 �������ػ���)

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDPSERVER_THD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
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
	CWinThread *pThread1, *pThread2; //������ ��ü �޼�
	ThreadArg arg1, arg2; //������ ���� ����
	CDataSocket *m_pDataSocket;
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);
	afx_msg void OnBnClickedDisconnect();
	CString m_edit1_text; // DDV�� �޼��� ���� �����ҳ༮
};
