
// UDPServer_thdDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "UDPServer_thd.h"
#include "UDPServer_thdDlg.h"
#include "afxdialogex.h"
#include "DataSocket.h"

#include <algorithm>
#include <bitset>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CCriticalSection tx_cs;
CCriticalSection rx_cs;


// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CUDPServer_thdDlg ��ȭ ����



CUDPServer_thdDlg::CUDPServer_thdDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_UDPSERVER_THD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUDPServer_thdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_tx_edit_short);
	DDX_Control(pDX, IDC_EDIT3, m_tx_edit);
	DDX_Control(pDX, IDC_EDIT2, m_rx_edit);
}

BEGIN_MESSAGE_MAP(CUDPServer_thdDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SEND, &CUDPServer_thdDlg::OnBnClickedSend)
	ON_BN_CLICKED(IDC_DISCONNECT, &CUDPServer_thdDlg::OnBnClickedDisconnect)
END_MESSAGE_MAP()


// CUDPServer_thdDlg �޽��� ó����


std::string CString_to_BinaryStr(CString message) { // CString ���� ��ȯ�ϸ� hex�� �Ǿ �����ٷο�
	std::string temp = CT2CA(message); // CString to string

	char *ptr = (char*)temp.c_str();//"GFG";

	std::string temp2 = "";

	int i;
	for (; *ptr != 0; ++ptr)
	{
		if (*ptr & 0x80 == 0x80) { // �ѱ�, �����϶�
			for (i = 7; i >= 0; --i) // 8bit
				temp2 += (*ptr & 1 << i) ? ("1") : ("0");
			++ptr;
			for (i = 7; i >= 0; --i) // 8bit
				temp2 += (*ptr & 1 << i) ? ("1") : ("0");
		}
		else { // �ƽ�Ű�ڵ�� ǥ�������Ҷ�
			for (i = 7; i >= 0; --i) // 8bit
				temp2 += (*ptr & 1 << i) ? ("1") : ("0");
		}
	}
	//std::cout<<"����ȭ�� ���ڴ�" << temp2 << "�Դϴ�.\n\n";

	return temp2;
}

std::string BinaryStr_to_CString(std::string binary_message) {
	//CString res = _T("");
	//std::string line = CT2CA(binary_message);
	//std::cout << "Enter binary string: ";
	std::string line = binary_message;
	std::string temp = "";
	bool correct = false;
	while (!correct) {
		//std::getline(std::cin, line);
		correct = std::all_of(line.begin(), line.end(), [](char c) {return c == '1' || c == '0'; });
		if (!correct)
			return ("");
		//	std::cout << "error, print only 1's or 0's: ";
	}
	std::istringstream in(line);
	std::bitset<8> bs; // 8bit
	while (in >> bs)
		//std::cout << char(bs.to_ulong());
		temp += char(bs.to_ulong());
	return temp;
}

void CUDPServer_thdDlg::packetSegmentation(CString message) {

	std::string binaried = CString_to_BinaryStr(message); // ���ڿ��� 2�������ڿ��θ���ϴ�. ���̴� 1���ڸ� 8bit�� �����ϴ�.
														  // �� ���ڸ� 8bitũ��� ����ȭ�Ͽ����Ƿ�, Packet�� data���� 80bit�� 10���� ���ڸ� �����Ͽ� ���� �� �ֽ��ϴ�.
	std::wcout << "�����÷��� �޼��� :" << (const wchar_t*)message << " �� \n"; //CString�� wcout���� ����ؾ� 16������ �ȳ���
	std::cout << "�������� " << binaried << "�Դϴ�\n";
	std::string temp = "";

	unsigned short total_packet;
	if (binaried.length() % 80 == 0) {
		total_packet = binaried.length() / 80;
	}
	else {
		total_packet = (binaried.length() / 80) + 1;
	}

	Packet newPacket = Packet();
	int packet_data_count = 0;
	int seq = 0;
	for (int i = 0; i < binaried.length(); ++i) {
		temp += binaried.at(i); // 1bit�� �߰�.
		if (temp.length() == 8) { // 8��°���� 8bit�̹Ƿ� 1byte�� ���尡��
			std::bitset<8> bits(temp); // ex) "10101010" => ���� 170 == 0b10101010
			newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]�� ���ؼ� 8bit(1byte)�� ���ڷ� ����
			packet_data_count++;
			temp = "";
			if (packet_data_count == 10) { // �Ź� 80��° bit�� �߰��Ҷ����� �̶����� ������ packet�� packet buffer�� �����մϴ�.
				newPacket.seq = ++seq; // seq�ѹ��� �߰�
				newPacket.total_sequence_number = total_packet; // ���ڿ� ����ȭ�ѰŸ� 80bit�� ������ �� ���� frame��������
				packet_send_buffer.Add(newPacket); //���ۿ� ��Ŷ �߰�
				newPacket = Packet(); // �� ��Ŷ�Ҵ�
				packet_data_count = 0;
			}
		}
	}
	if (temp.length() < 8) { // 80bit���� ���� �༮�϶��� �ٷ� ����
		newPacket.seq = ++seq; // seq�ѹ��� �߰�
		std::bitset<8> bits(temp); // ex) "10101010" => ���� 170 == 0b10101010
		newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]�� ���ؼ� 8bit(1byte)�� ���ڷ� ����
		newPacket.total_sequence_number = total_packet; // �̳༮�� ���� for������ packet�������Ƿ� �ִ� frame������ 1
		packet_send_buffer.Add(newPacket); //���ۿ� ��Ŷ �߰�
	}

	//std::cout << seq << "�� frame�� �����մϴ�.\n\nsegemenation�� packet�Դϴ�.\n";
	//std::string data_temp = "";
	//for (int k = 0; k<newPacket.total_sequence_number; ++k) {
	//	for (int i = 0; i < sizeof(newPacket.data); ++i) { // sizeof(newPacket->data) == 10
	//		std::bitset<8> bits(newPacket.data[i]);
	//		std::cout << bits << "\n";
	//		data_temp += bits.to_string(); // bitset to string
	//	}
	//}
	std::cout << "\n";

}


UINT RXThread(LPVOID arg) // �޼��� ������ �����ϴ� ���ۿ��� �� ����ϴ� ������
{
	ThreadArg *pArg = (ThreadArg *)arg;
	CStringList *plist = pArg->pList;
	CUDPServer_thdDlg *pDlg = (CUDPServer_thdDlg *)pArg->pDlg;
	std::string temp;
	while (pArg->Thread_run) {
		POSITION pos = plist->GetHeadPosition();
		POSITION current_pos;
		while (pos != NULL) {
			current_pos = pos;
			rx_cs.Lock();
			CString str = plist->GetNext(pos); // reassemble�� data�� ������ list���� 1�� ������
			temp = CT2CA(str); // CString to string
			temp = BinaryStr_to_CString(temp); // ���� ���ڿ��� ����
			rx_cs.Unlock();

			CString message;
			pDlg->m_rx_edit.GetWindowText(message);
			message += _T("<message from : ");
			message += CString(pDlg->peerIp);
			message += _T("> \r\n");
			message += CString(temp.c_str()); // string to CString
			message += "\r\n";
			pDlg->m_rx_edit.SetWindowTextW(message);
			pDlg->m_rx_edit.LineScroll(pDlg->m_rx_edit.GetLineCount());

			plist->RemoveAt(current_pos);
		}
		Sleep(10);
	}
	return 0;
}


UINT TXThread(LPVOID arg) // �޼��� ������ ������
{
	ThreadArg *pArg = (ThreadArg *)arg;
	CStringList *plist = pArg->pList;
	CUDPServer_thdDlg *pDlg = (CUDPServer_thdDlg *)pArg->pDlg;


	while (pArg->Thread_run)
	{
		POSITION pos = plist->GetHeadPosition();
		POSITION current_pos;
		while (pos != NULL)
		{
			current_pos = pos;
			tx_cs.Lock();
			CString str = plist->GetNext(pos); // Ŭ�������� ���ڿ��� plist�� ���, �װ� str�� ������
			pDlg->packetSegmentation(str); // �� CString str�� ���� segmentation�Ͽ� pack���ۿ� ����
			tx_cs.Unlock();


			CString message;
			pDlg->m_tx_edit.GetWindowText(message);
			//message += "\r\n";
			pDlg->m_tx_edit.SetWindowTextW(message);

			while (!pDlg->packet_send_buffer.IsEmpty()) { // ��Ŷ���ۿ� ���������� ����
														  //(char*)& �����ָ� ����ü ������. ���Ŵܵ� ������ �޾������
				pDlg->m_pDataSocket->SendToEx((char*)&pDlg->packet_send_buffer.GetAt(0), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
				pDlg->packet_send_buffer.RemoveAt(0); //������ ����
			}
			//pDlg->m_pDataSocket->SendToEx(str, (str.GetLength() + 1) * sizeof(TCHAR), pDlg->peerPort, pDlg->peerIp, 0); ///UDP������ ���Ͽ� �ش� ��Ʈ�� ip�ּҷ� �޼����� �����մϴ�.
			pDlg->m_tx_edit.LineScroll(pDlg->m_tx_edit.GetLineCount());

			plist->RemoveAt(current_pos);
		}
		Sleep(10);
	}return 0;
}
BOOL CUDPServer_thdDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �� ��ȭ ������ �������� �����մϴ�.  ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	CString myIp = _T("127.0.0.1");
	int myPort = 6789;
	
	/*CString str123 = _T("123127787594735479749357479579479473");
	std::cout << sizeof(str123) << "\n";*/
	
	packet_receive_buffer.RemoveAll(); //�޴� ���� �ʱ�ȭ

	CStringList* newlist = new CStringList;
	arg1.pList = newlist;
	arg1.Thread_run = 1;
	arg1.pDlg = this;

	CStringList* newlist2 = new CStringList;
	arg2.pList = newlist2;
	arg2.Thread_run = 1;
	arg2.pDlg = this;

	WSADATA wsa;
	int error_code;
	if ((error_code = WSAStartup(MAKEWORD(2, 2), &wsa)) != 0) {
		TCHAR buffer[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 256, NULL);
		AfxMessageBox(buffer, MB_ICONERROR);
	}
	
	m_pDataSocket = NULL;

	m_pDataSocket = new CDataSocket(this);

	m_pDataSocket->Create(myPort, SOCK_DGRAM); /// UDP ������ �ʱ�ȭ�մϴ�. ��Ʈ�� 6789
	//m_pDataSocket->Connect(_T(ip), 6789);
	pThread1 = AfxBeginThread(TXThread, (LPVOID)&arg1);
	pThread2 = AfxBeginThread(RXThread, (LPVOID)&arg2);
	

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

void CUDPServer_thdDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�.  ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CUDPServer_thdDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CUDPServer_thdDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CUDPServer_thdDlg::OnBnClickedSend()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	if (!(peerIp != _T("") && peerPort != -1)) { /// ������ ���� ����� Client�� ���ٸ� ������ ȣ���մϴ�.
		AfxMessageBox(_T("���� ����� Client�� �����ϴ�."));
		return;
	}
	CString tx_message;
	m_tx_edit_short.GetWindowText(tx_message);
	
	if (tx_message == "") {            //�Է�â�� �޽����� ������ �����޽��� �߻�

		AfxMessageBox(_T("������ �Է��ϼ���!"));
		return;
	}
	tx_message += _T("\r\n"); //�ٹٲ��� �߰�
	tx_cs.Lock();
	arg1.pList->AddTail(tx_message);
	tx_cs.Unlock();
	m_tx_edit_short.SetWindowText(_T(""));
	m_tx_edit_short.SetFocus();

	int len = m_tx_edit.GetWindowTextLengthW();
	m_tx_edit.SetSel(len, len);
	m_tx_edit.ReplaceSel(tx_message);
}

template<class T> // ������ ���ø�
void QSortCArray(T& t, int(__cdecl *compare)(const void *elem1, const void *elem2))
{
	if (t.GetSize() <= 0)
	{
		return;
	}
	qsort(t.GetData(), t.GetSize(), sizeof(t[0]), compare);
}

// CPacket �ڷ��� �� �Լ� �����Ŀ�
int ComparePacket(const void *elem1, const void *elem2)
{
	Packet* p1 = (Packet*)elem1;
	Packet* p2 = (Packet*)elem2;
	if (p1->seq > p2->seq) return +1;
	if (p1->seq < p2->seq) return -1;
	return 0;
}

void CUDPServer_thdDlg::ProcessReceive(CDataSocket* pSocket, int nErrorCode)
{
	TCHAR pBuf[16 + 1]; // 16byte Packet���ſ�
	int nbytes;
	memset(pBuf, 0, sizeof(pBuf));

	Packet* newPacket = new Packet();
	nbytes = pSocket->ReceiveFromEx(pBuf, sizeof(Packet), peerIp, peerPort, 0); // ��Ŷ�� char[]������ ���ƿ�
	pBuf[nbytes] = NULL;

	newPacket = (Packet*)pBuf; // Packet������ ����

	/*CIPAddressCtrl myCtrlIP;
	DWORD dwIP;
	myCtrlIP.GetAddress(&peerIp);
*/
	/*std::wcout << (const wchar_t*)peerIp.Format(_T("IP�ּ�: %d.%d.%d.%d"),FIRST_IPADDRESS(peerIp),SECOND_IPADDRESS(peerIp),THIRD_IPADDRESS(peerIp),FOURTH_IPADDRESS(peerIp));
	<< "�� ���� �� "<< newPacket->total_sequence_number <<"�� frame ������\n=> ";*/
	std::wcout << (const wchar_t*)peerIp<< "�� ���� �� "<< newPacket->total_sequence_number <<"�� frame ������\n=> ";
	std::cout<<"���� " <<newPacket->seq<<"��° frame����\n";	

	//���� packet�� ���Ͽ� 80bit data����
	std::string data_temp = "";
	for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 10
		std::bitset<8> bits(newPacket->data[i]);
		data_temp += bits.to_string(); // bitset to string
	}
	cout << "����ȭ������ 80bit :\n" << data_temp << "\n�� �޾ҽ��ϴ�.\n\n";

	packet_receive_buffer.Add(*newPacket); // ���� ��Ŷ���ۿ� �߰�.
	if (packet_receive_buffer.GetSize() == newPacket->total_sequence_number) { // 1:1�̹Ƿ� �����ſ� ����ִ°� �ٷ�üũ�ص���
		// �ִ� frame���� buffer�� ����� frame���� ������
		
		//Packet�� ���Ͽ� �������� ������
		QSortCArray(packet_receive_buffer, ComparePacket);
		//������Ŷ�鿡 ���� ��� data���� 80*total_sequence_number bit
		std::string data_temp = "";
		for(int k=0; k<newPacket->total_sequence_number; ++k){ 
			for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 10
				std::bitset<8> bits(packet_receive_buffer.GetAt(k).data[i]);
				std::cout << bits << "\n";
				data_temp += bits.to_string(); // bitset to string
			}
		}

		rx_cs.Lock();
		arg2.pList->AddTail(CString(data_temp.c_str())); // string to CString // ������ data�� �� ���ۿ� �߰�
		rx_cs.Unlock();

		packet_receive_buffer.RemoveAll(); // ���ۺ���

		// => �����ð� ���� ������ error control �ؾ��� (���߿� ����)
	}
}


void CUDPServer_thdDlg::OnBnClickedDisconnect()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	if (m_pDataSocket == NULL) {
		AfxMessageBox(_T("�̹� ���� ����"));
	}
	else {
		arg1.Thread_run = 0;
		arg2.Thread_run = 0;
		m_pDataSocket->Close();
		delete m_pDataSocket;
		m_pDataSocket = NULL;
	}
}
