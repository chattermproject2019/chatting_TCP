
// UDPClient_thdDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CDataSocket;

struct ThreadArg //������ ����
{
	CStringList* pList;
	CDialogEx* pDlg;
	int Thread_run; //������ ���� ����
};

struct UDP_datagram { // ���� ��Ŷ�� ����
	//UDP ���
	short checksum = 0; //checksum_field üũ���ʵ�� ó���� 0

	short source_port;
	short destination_port;

	short length;
	int seq;

	int source_ip;//32bit
	int destination_ip;//32bit
	
	//Data
	CString data; //16byte¥�� ������
};


// CUDPClient_thdDlg ��ȭ ����
class CUDPClient_thdDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CUDPClient_thdDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDPCLIENT_THD_DIALOG };
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
	CWinThread *pThread1, *pThread2; //������ ��ü �ּ�
	ThreadArg arg1, arg2; //������ ���� ����
	CDataSocket *m_pDataSocket;
	CEdit m_tx_edit_short;
	CEdit m_tx_edit;
	CEdit m_rx_edit;
	CIPAddressCtrl m_ip_addr;
	afx_msg void OnBnClickedSend();
	afx_msg void OnBnClickedClose();
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);
	CIPAddressCtrl m_ipaddr;
	CString HostAddress; //������ ip�� �����մϴ�.
	UINT HostPort = 6789; // ������ port�ּ��Դϴ�.
};
