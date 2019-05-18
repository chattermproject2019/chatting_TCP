
// UDPClient_thdDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CDataSocket;

struct Packet { // 16byte = 128bit
	unsigned short seq; // 2yte = 16bit
	unsigned short checksum; // 2byte = 16bit
	unsigned short total_sequence_number; // 2byte = 16bit => �� ������ frame��
	unsigned char data[10]; // 10btye = 80bit, ���� �����͸� 80bit = 10byte���尡��
	Packet() { seq = 1; checksum = 0; total_sequence_number = 0; memset(data, 0, sizeof(data)); }
};
struct ThreadArg //������ ����
{
	CStringList* pList;
	CDialogEx* pDlg;
	int Thread_run; //������ ���� ����
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
	afx_msg void OnBnClickedSend();
	afx_msg void OnBnClickedClose();
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);

	CIPAddressCtrl m_ipaddr;
	CString myIp;
	int myPort;
	CString peerIp;
	int peerPort;
	CArray<Packet> packet_send_buffer; // ���� ��Ŷ�� ������ ���� ����
	CArray<Packet> packet_receive_buffer; // ���� ��Ŷ�� ������ ���� ����

	void packetSegmentation(CString);
};
