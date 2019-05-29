
// UDPServer_thdDlg.h : ��� ����
//

#pragma once
#include "afxwin.h"

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

struct ThreadArg //������ ����
{
	CStringList *pList;
	CDialogEx *pDlg;
	int Thread_run; //������ ���� ����
};

struct timerThreadArg //Ÿ�̸� ������ �μ� ����
{
	int timer_id;
	int deadline;
	int frame_seq;
	CDialogEx* pDlg;
};

class CDataSocket;

// CUDPServer_thdDlg ��ȭ ����
class CUDPServer_thdDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CUDPServer_thdDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
	

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
	CWinThread *pThread1, *pThread2, *timerThread, *pThread4; //������ ��ü �ּ�
	ThreadArg arg1, arg2, arg4; //������ ���� ����
	timerThreadArg arg3; // Ÿ�̸� ������ �μ�
	CDataSocket *m_pDataSocket;
	void ProcessReceive(CDataSocket* pSocket, int nErrorCode);
	afx_msg void OnBnClickedDisconnect();

	CString myIp;
	int myPort;
	CString peerIp = _T("");  // �ֱٿ� ����� ip�ּҸ� ����
	UINT peerPort = -1; // �ֱٿ� ����� port�ּҸ� �����մϴ�. (���� ���� pc������ �����ؾ��ϹǷ� ������Ʈ�� 6789, Ŭ���̾�Ʈ ��Ʈ�� 8000 �������ػ���)


	CArray<Packet> packet_send_buffer; // ���� ��Ŷ�� ������ ���� ���� // 1:1 chatting�̱� ������ ���� client table�� ������ �ʽ��ϴ�.

	CArray<Packet> packet_receive_buffer; // ���� ��Ŷ�� ������ ���� ����
	
	CArray<int> ack_receive_buffer; // ���� ack�޼����� frame�ѹ��� ������ ���� �����Դϴ�. ������ nack�� frame�ѹ��Դϴ�.

	void packetSegmentation(CString message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void StartTimer(unsigned int timer_id, unsigned int n); // Ÿ�̸� ����
	void StopTimer(unsigned int timer_id); // Ÿ�̸� ����

	BOOL timeout = false;

	unsigned short window_size;
	unsigned short mode;// 0-Stop&Wait, 1-GoBackN, 2-SelectiveRecject

	int current_error_frame=-1;

	bool overlap_checker_Lock = false;
	bool* overlap_checker;
	CArray<Packet> ack_send_buffer; // ack�޼����� ������ �ϴ°��, �� ���ۿ� �����ߴٰ� �����ϴ�.


	CArray<int> error_buffer; // Selective Reject�� ��������, ������ �� frame seq�� �����մϴ�.
	BOOL timer_id_checker[1000] = { 0, };
	afx_msg void OnBnClickedOpen();
};
