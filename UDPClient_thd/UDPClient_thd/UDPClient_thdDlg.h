
// UDPClient_thdDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

class CDataSocket;

struct Response { // 4byte, ���ſ� ���� ���信 ���� �����̴�. stop&wait, Go&Back, Selective Reject�� ���δ�.
	unsigned short ACK; // n�� frame�� ���� Ȯ������, frame�ް� �ش� frame�� seq������, seq�� �׻� 1���� ����.
	bool no_error; // negative Ack�� �Ұ����� ����. true�� ������ ���°Ű�. false�� frame�� ������ �־����� �˸�.
	bool more; // RR���� REJ���� ����// true�̸� �� ���Ű�������, false�̸� ������ �� �̻� ���žȵǰ�, ���ִٰ� �õ����ٰ��� �˸�.

	Response() { ACK = 0; no_error = more = true; } // ACK 0�̸� frame������ �ʱ�ȭ���� ��������. seq�� 1���� �����ϴϱ�
};

struct Packet { // 16byte = 128bit
	unsigned short seq; // 2yte = 16bit, 
	unsigned short checksum; // 2byte = 16bit
	unsigned short total_sequence_number; // 2byte = 16bit => �� ������ frame��
	Response response; // 4byte, ����޼����̴�. 
	unsigned char data[6]; // 6btye = 48bit, ���� �����͸� 48bit = 6byte���尡��
	Packet() { seq = 1; response = Response(); checksum = 0; total_sequence_number = 0; memset(data, 0, sizeof(data)); }
};

struct ThreadArg //������ �μ� ����
{
	CStringList* pList;
	CDialogEx* pDlg;
	int Thread_run; //������ ���� ����
};

struct timerThreadArg //Ÿ�̸� ������ �μ� ����
{
	int timer_id;
	int deadline;
	int frame_seq;
	CDialogEx* pDlg;
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
	CWinThread *pThread1, *pThread2, *timerThread; //������ ��ü �ּ�
	ThreadArg arg1, arg2; //������ ���� ����
	timerThreadArg arg3; // Ÿ�̸� ������ �μ�


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
	CArray<Packet> packet_send_buffer; // ���� ��Ŷ�� ������ ���� ����
	CArray<Packet> packet_receive_buffer; // ���� ��Ŷ�� ������ ���� ����
	CArray<int> ack_receive_buffer; // ���� ack�޼����� frame�ѹ��� ������ ���� �����Դϴ�. ������ nack�� frame�ѹ��Դϴ�.

	void packetSegmentation(CString);
	 
	afx_msg void OnTimer(UINT_PTR nIDEvent); // Ÿ�̸� ��, Ŭ���� �信�� WM_Timer �߰��ϸ��
	void StartTimer(unsigned int timer_id, unsigned int n); // Ÿ�̸� ����
	void StopTimer(unsigned int timer_id); // Ÿ�̸� ����

	BOOL timeout = false;
	//int next_sequnce; //  �������� ���� frame �ѹ��� �����ϴ� ����, frame�� ������, �� ���°ų�, ������ ����, �������ϴ� sequence�� �޶����� ������,  

	unsigned short window_size; // Go Back N�� �ִ� 2^(seq�ѹ� bit�� =������ 16bit) - 1 ���� ����
	unsigned short mode; // 0-Stop&Wait, 1-GoBackN, 2-SelectiveRecject

	int current_error_frame = -1;

	bool overlap_checker_Lock = false;
	bool* overlap_checker;

	CArray<Packet> ack_send_buffer; // ack�޼����� ������ �ϴ°��, �� ���ۿ� �����ߴٰ� �����ϴ�.

	CArray<int> error_buffer; // Selective Reject�� ��������, ������ �� frame seq�� �����մϴ�.

	BOOL timer_id_checker[1000] = {0,};
	BOOL mRadio;
	afx_msg void OnBnClickedButton1();
	CEdit m_windowSize;
	int m_windowSzie_val;
	afx_msg void OnBnClickedOpen();
};
