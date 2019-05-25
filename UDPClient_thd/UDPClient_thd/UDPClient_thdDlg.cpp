
// UDPClient_thdDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "UDPClient_thd.h"
#include "UDPClient_thdDlg.h"
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
CCriticalSection Data_socket_cs; //Data Socket tx로도 보내고, ACK반응용으로도 쓰이기때문에 중복피하기 위함.
CCriticalSection ACK_CHECK_cs; // ACK메세지를 수신했는지를 체크하기위한 cs.
//CCriticalSection sequence_cs; // 다음 seq number, timer스레드에서 중복해서 수정하게 하지 않기 위함.


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
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


// CUDPClient_thdDlg 대화 상자



CUDPClient_thdDlg::CUDPClient_thdDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_UDPCLIENT_THD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUDPClient_thdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_tx_edit_short);
	DDX_Control(pDX, IDC_EDIT3, m_tx_edit);
	DDX_Control(pDX, IDC_EDIT2, m_rx_edit);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ipaddr);
}

BEGIN_MESSAGE_MAP(CUDPClient_thdDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SEND, &CUDPClient_thdDlg::OnBnClickedSend)
	ON_BN_CLICKED(IDC_CLOSE, &CUDPClient_thdDlg::OnBnClickedClose)
	ON_WM_TIMER()
END_MESSAGE_MAP()


std::string CString_to_BinaryStr(CString message) { // CString 으로 반환하면 hex로 되어서 연산까다로움
	
	std::string temp = CT2CA(message); // CString to string

	char *ptr = (char*)temp.c_str();//"GFG";

	std::string temp2 = "";

	int i;
	for (; *ptr != 0; ++ptr)
	{
		for (i = 7; i >= 0; --i) // 8bit
			temp2 += (*ptr & 1 << i) ? ("1") : ("0");
	}
	//std::cout<<"이진화된 문자는" << temp2 << "입니다.\n\n";
	
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
		correct = std::all_of(line.begin(), line.end(), [](char c){return c =='1'||c=='0';});
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

unsigned short checksum_packet(unsigned short* packet, int length) { // checksum char*으로 구현하면 struct에서 char*로 바꿀때 16byte가 8,8로 쪼개질때 순서가 바뀌어서 short*로 구현
	unsigned long sum = 0;
	unsigned long carry;
	for (int i = 0; i<length; i++)
	{
		sum += packet[i];
	}
	carry = (sum) >> 16;
	sum = sum & 0xffff;
	sum += carry;

	unsigned short result = (unsigned short)(~(sum & 0xffff));
	return result;
}

void CUDPClient_thdDlg::packetSegmentation(CString message) {

	std::string binaried = CString_to_BinaryStr(message); // 문자열을 2진수문자열로만듦니다. 길이는 1문자를 8bit씩 나눕니다.
	// 각 문자를 8bit크기로 이진화하였으므로, Packet의 data에는 48bit즉 10개의 문자를 저장하여 보낼 수 있습니다.
	std::wcout<<"보내시려는 메세지 :" << (const wchar_t*)message << " 는 \n"; //CString은 wcout으로 출력해야 16진수로 안나옴
	std::cout << "이진수로 " << binaried << "입니다\n";
	std::string temp = "";

	unsigned short total_packet;
	if (binaried.length() % 48 == 0) { 
		total_packet = binaried.length() / 48;
	}
	else {
		total_packet = (binaried.length() / 48) + 1;
	}

	Packet newPacket = Packet();
	int packet_data_count = 0;
	int seq = 0;
	for (int i = 0; i < binaried.length(); ++i) {
		temp += binaried.at(i); // 1bit씩 추가.
		if (temp.length() == 8) { // 8번째마다 8bit이므로 1byte에 저장가능
			std::bitset<8> bits(temp); // ex) "10101010" => 숫자 170 == 0b10101010
			newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]에 대해서 8bit(1byte)씩 숫자로 저장
			packet_data_count++;
			temp = "";
			if (packet_data_count == 6) { // 매번 48번째 bit를 추가할때마다 이때까지 저장한 packet을 packet buffer에 저장합니다.
				
				newPacket.seq = ++seq; // seq넘버도 추가
				newPacket.total_sequence_number = total_packet; // 문자열 이진화한거를 80bit로 나누면 총 보낼 frame개수나옴
				
				newPacket.checksum = 0;
				unsigned short* short_packet = (unsigned short*)&newPacket;
				newPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
				printf("보내려는 패킷의 체크섬 %x\n", newPacket.checksum);

				packet_send_buffer.Add(newPacket); //버퍼에 패킷 추가

				newPacket = Packet(); // 새 패킷할당
				packet_data_count = 0;
			}
		}
	}
	if ((temp.length() < 8)) { // 8bit보다 작게 저장된 패킷일때는 바로 보냄
		newPacket.seq = ++seq; // seq넘버도 추가
		std::bitset<8> bits(temp); // ex) "10101010" => 숫자 170 == 0b10101010
		newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]에 대해서 8bit(1byte)씩 숫자로 저장
		newPacket.total_sequence_number = total_packet; // 이녀석은 위에 for문돌때 packet못보내므로 최대 frame개수는 1
		
		newPacket.checksum = 0;
		unsigned short* short_packet = (unsigned short*)&newPacket;
		newPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
		printf("보내려는 패킷의 체크섬 %x", newPacket.checksum);
		
		packet_send_buffer.Add(newPacket); //버퍼에 패킷 추가
	}

	//arg1.Thread_run = false;

	std::cout << "\n";

}

// CUDPClient_thdDlg 메시지 처리기
UINT timer_thread_func(LPVOID arg) { // 타이머 스레드
	timerThreadArg *pArg = (timerThreadArg *)arg;
	CUDPClient_thdDlg *pDlg = (CUDPClient_thdDlg *)pArg->pDlg;

	pDlg->StartTimer(pDlg->arg3.timer_id, pDlg->arg3.deadline); // 타이머 시작, deadline주기로 OnTime함수 실행

	//AfxEndThread(0, false);

	return 0;
}


UINT RXThread(LPVOID arg) //RXThread 함수 정의
{
	ThreadArg *pArg = (ThreadArg *)arg;
	CStringList *plist = pArg->pList;
	CUDPClient_thdDlg *pDlg = (CUDPClient_thdDlg *)pArg->pDlg;
	std::string temp;
	while (pArg->Thread_run) {
		POSITION pos = plist->GetHeadPosition();
		POSITION current_pos;
		while (pos != NULL) {
			current_pos = pos;
			rx_cs.Lock();
			CString str = plist->GetNext(pos); // reassemble한 data를 저장한 list에서 1개 꺼내옴
			temp = CT2CA(str); // CString to string
			temp = BinaryStr_to_CString(temp); // 실제 문자열로 복구
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


UINT TXThread(LPVOID arg) //TXThread 함수 정의
{
	ThreadArg *pArg = (ThreadArg *)arg;
	CStringList *plist = pArg->pList;
	CUDPClient_thdDlg *pDlg = (CUDPClient_thdDlg *)pArg->pDlg;
	

	while (pArg->Thread_run)
	{
		POSITION pos = plist->GetHeadPosition();
		POSITION current_pos;
		
		while (pos != NULL)
		{
			current_pos = pos;
			tx_cs.Lock();
			CString str = plist->GetNext(pos); // 클릭했을때 문자열이 plist에 담김, 그걸 str로 가져옴
			pDlg->packetSegmentation(str); /// 그 CString str에 대해 segmentation하여 pack버퍼에 넣음
			tx_cs.Unlock();

			// message가 여러개의 frame으로 쪼개진 상황

			CString message;
			pDlg->m_tx_edit.GetWindowText(message);
			message += "\r\n";
			pDlg->m_tx_edit.SetWindowTextW(message);
			
			//while(!pDlg->packet_send_buffer.IsEmpty()){ // 패킷버퍼에 뭔가있으면 보냄
			for (int i = 0; i < pDlg->packet_send_buffer.GetSize(); ++i) {
				//(char*)& 안해주면 구조체 못보냄. 수신단도 저렇게 받아줘야함
				pDlg->timeout = false;
				std::cout << pDlg->packet_send_buffer.GetAt(i).seq << " 번 frame을 보냅니다.\n";
				Data_socket_cs.Lock();
				pDlg->m_pDataSocket->SendToEx((char*)&pDlg->packet_send_buffer.GetAt(i), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
				Data_socket_cs.Unlock();

				/*sequence_cs.Lock();
				pDlg->next_sequnce = i;
				sequence_cs.Unlock();*/

				/*stop&wait ack메세지 수신기다림*/
				std::cout << "Ack메세지를 기다리고 있습니다...\n";
				
				int timer_id = (int)(rand() * 1000); // 랜덤으로 id 생성, 중복되지 않게 수정하기
				
				pDlg->arg3.deadline = 1000; // 1초가 deadline입니다.
				pDlg->arg3.timer_id = timer_id;
				//타이머 스레드 바로 시작.
				pDlg->arg3.frame_seq = i;
				
				pDlg->timerThread = AfxBeginThread(timer_thread_func, (LPVOID)&pDlg->arg3, NULL);
				
				pDlg->StartTimer(timer_id, pDlg->arg3.deadline); // 타이머 시작, deadline주기로 OnTime함수 실행

				while (pDlg->ack_receive_buffer.IsEmpty()) { // ack버퍼가 비어있음.
					if (pDlg->timeout == true) { // 버퍼에 아무것도 없는 상태로, 시간지나면 expire
						std::cout << "Expired!\n";
						break;
					}
				}

				//위while문을 빠져나오는 경우는 ack버퍼에 무언가 추가 되었거나, timeout되었거나 둘중하나임
				pDlg->StopTimer(pDlg->arg3.timer_id); //timer종료 

				if (!pDlg->ack_receive_buffer.IsEmpty()) { // ack 버퍼에 무언가가 도착했음.
					if (pDlg->ack_receive_buffer.GetAt(0) > 0) { // 받은 메세지가 ack였다
						std::cout << "ack메세지를 받았으므로, 보냈던 " << pDlg->ack_receive_buffer.GetAt(0) << "번 frame을 확정짓습니다.\n";
						pDlg->ack_receive_buffer.RemoveAt(0); // ack 수신확인한거 clear
						//pArg->frame_seq; // 그대로있어야 tx에서 i++되서 다음 seq로 넘어감
					}
					else if ((pDlg->ack_receive_buffer.GetAt(0) < 0)) { // 받은메세지가 nack였으면 똑같은거 한번 더 보냄
						std::cout << "nack메세지를 받았으므로 보냈던" << -1 * pDlg->ack_receive_buffer.GetAt(0) << "번 frame을 다시 보냅니다.\n";
						--i;
						//pArg->frame_seq--; // 한번 더 보내기 위해
						pDlg->ack_receive_buffer.RemoveAt(0); // ack 수신확인한거 clear
															  //break; // 똑같은(nack) frame보내기위해 break;
					}
				}
				else if (pDlg->timeout == true) { // 비록 ack메세지는 못받았지만, timeout은 패킷loss이므로 재전송해줘야함
					std::cout << "timeout이므로 받았으므로 보냈던 " << pDlg->packet_send_buffer.GetAt(i).seq << "번 frame을 다시 보냅니다.\n";
					//pArg->frame_seq--; // 한번 더 보내기 위해
					i--;
				}

			}

			pDlg->packet_send_buffer.RemoveAll(); // for문이기 때문에 다보냈으면 다 제거

			//pDlg->m_pDataSocket->SendToEx(str, (str.GetLength() + 1) * sizeof(TCHAR), pDlg->peerPort, pDlg->peerIp, 0); ///UDP소켓을 통하여 해당 포트와 ip주소로 메세지를 전송합니다.
			pDlg->m_tx_edit.LineScroll(pDlg->m_tx_edit.GetLineCount());

			plist->RemoveAt(current_pos);
		}
		Sleep(10);
	}return 0;
}

BOOL CUDPClient_thdDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	/// TODO: 여기에 추가 초기화 작업을 추가합니다.


	ack_receive_buffer.RemoveAll();
	packet_send_buffer.RemoveAll(); // packet buffer 초기화

	m_ipaddr.SetWindowTextW(_T("127.0.0.1"));

	peerPort = 6789; // 상대방 포트
	m_ipaddr.GetWindowTextW(peerIp);

	m_pDataSocket = new CDataSocket(this);
	m_pDataSocket->Create(8000, SOCK_DGRAM); // 내 포트
	
	

	CStringList* newlist = new CStringList;
	arg1.pList = newlist;
	arg1.Thread_run = 1;
	arg1.pDlg = this;

	CStringList* newlist2 = new CStringList;
	arg2.pList = newlist2;
	arg2.Thread_run = 1;
	arg2.pDlg = this;
	
	arg3.pDlg = this;
	arg3.frame_seq = -1;
	

	pThread1 = AfxBeginThread(TXThread, (LPVOID)&arg1);
	pThread2 = AfxBeginThread(RXThread, (LPVOID)&arg2);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CUDPClient_thdDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CUDPClient_thdDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CUDPClient_thdDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void CUDPClient_thdDlg::OnBnClickedSend()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_pDataSocket == NULL)
	{
		MessageBox(_T("서버에 접속 안 함!"), _T("알림"), MB_ICONERROR);

	}
	else
	{
		m_ipaddr.GetWindowTextW( peerIp );
		peerPort = 6789;

		CString tx_message;
		
		m_tx_edit_short.GetWindowText(tx_message);

		if (tx_message == "") {            //입력창에 메시지가 없으면 에러메시지 발생

			AfxMessageBox(_T("내용을 입력하세요!"));
			return;
		}
		//tx_message += _T("\r\n");
		tx_cs.Lock();
		arg1.pList->AddTail(tx_message);
		tx_cs.Unlock();
		m_tx_edit_short.SetWindowTextW(_T(""));
		m_tx_edit_short.SetFocus();

		int len = m_tx_edit.GetWindowTextLengthW();
		m_tx_edit.SetSel(len, len);
		m_tx_edit.ReplaceSel(tx_message);
		
		
	}
}


void CUDPClient_thdDlg::OnBnClickedClose()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

template<class T> // 퀵정렬 템플릿
void QSortCArray(T& t, int(__cdecl *compare)(const void *elem1, const void *elem2))
{
	if (t.GetSize() <= 0)
	{
		return;
	}
	qsort(t.GetData(), t.GetSize(), sizeof(t[0]), compare);
}

// CPacket 자료형 비교 함수, 퀵정렬용
int ComparePacket(const void *elem1, const void *elem2)
{
	Packet* p1 = (Packet*)elem1;
	Packet* p2 = (Packet*)elem2;
	if (p1->seq > p2->seq) return +1;
	if (p1->seq < p2->seq) return -1;
	return 0;
}
void CUDPClient_thdDlg::ProcessReceive(CDataSocket* pSocket, int nErrorCode)
{
	TCHAR pBuf[16 + 1]; // 16byte Packet수신용
	int nbytes;
	memset(pBuf, 0, sizeof(pBuf));

	Packet* newPacket = new Packet();
	Packet AckPacket = Packet(); //Ack reply용입니다.

	nbytes = pSocket->Receive(pBuf, sizeof(Packet)); // 패킷이 char[]형으로 날아옴
	pBuf[nbytes] = NULL;

	newPacket = (Packet*)pBuf; // Packet형으로 만듦


	std::wcout << (const wchar_t*)peerIp << "로 부터 총 " << newPacket->total_sequence_number << "개 frame 수신중\n=> ";
	std::cout << "현재 " << newPacket->seq << "번째 frame도착\n";

	unsigned short* short_packet = (unsigned short*)newPacket;//Packet to unsigned short*
	unsigned short calculatedChecksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
	printf("받은 패킷의 체크섬 %x, 계산한 체크섬 값: %x \n", newPacket->checksum, calculatedChecksum);

	if (calculatedChecksum != 0) { // checksum 에러이면
		std::cout << "체크섬 에러입니다.\n";

		/*stop & wait 에러 컨트롤*/
		if (newPacket->response.ACK != 0) { // ACK로 추정되는 메세지가 checksum에러일 경우. 무시해서 expire되게한다.
			return;
		}
		else { // 에러인데 ACK메세지가 아니라고 추정되면, 데이터패킷이 에러인걸로 간주하고 NACK메세지를 보낸다.
			AckPacket.response.ACK = newPacket->seq; //받은 패킷번호
			AckPacket.response.more = true; // 더 수신가능
			AckPacket.response.no_error = false; // 에러없음이 거짓
			
			AckPacket.checksum = 0;
			unsigned short* short_packet = (unsigned short*)&AckPacket;
			AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
			printf("보내려는 nack 패킷의 체크섬 %x", AckPacket.checksum);

			Data_socket_cs.Lock();
			std::cout << newPacket->seq << " 번 frame에 대한 nack메세지를 보냅니다.\n";
			m_pDataSocket->SendToEx((char*)&AckPacket, sizeof(Packet), peerPort, peerIp, 0);
			Data_socket_cs.Unlock();

			return; // 에러이고, ACK메세지도 아닌것으로 추정. 데이터 에러이므로 NACK보내고, 버림.
		}

	}
	else { // checksum에러가 없음

		   /*Stop & Wait는 piggy back안할 것이므로 ack 또는 data용으로 구별*/

		   /*Stop & Wait ACK메세지 수신하는 경우*/
		if (newPacket->response.ACK != 0) {
			std::cout << "ACK메세지도착: 에러여부: " << newPacket->response.no_error << " 해당frame 번호: " << newPacket->response.ACK << " 계속수신가능여부: " << newPacket->response.no_error << "\n";

			// ack수신할때마다 send부분에 알려줘야함. 1개의 frame씩 보내기 때문에.
			if (newPacket->response.no_error == true) { // ACK 
				ack_receive_buffer.Add(newPacket->response.ACK); // 정상적으로 받은 frame넘버를 저장.
			}
			else { // NACK
				ack_receive_buffer.Add(-1 * newPacket->response.ACK); // 오류로 받은 frame넘버를 저장. 음수로
			}
			return; //piggy back아니므로 ack만 확인하고 버림
		}

		else { // ack메세지 아니므로 data 수신처리 


			if (newPacket->seq != 0) { // 데이터를 받았으면, 확인메세지를 보냅니다.
				AckPacket.response.ACK = newPacket->seq; //받은 패킷번호
				AckPacket.response.more = true; // 더 수신가능
				AckPacket.response.no_error = true; // 에러없음

				AckPacket.checksum = 0;
				unsigned short* short_packet = (unsigned short*)&AckPacket;
				AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
				printf("보내려는 ack 패킷의 체크섬 %x", AckPacket.checksum);

				Data_socket_cs.Lock();
				std::cout << newPacket->seq << " 번 frame에 대한 ack메세지를 보냅니다.\n";
				m_pDataSocket->SendToEx((char*)&AckPacket, sizeof(Packet), peerPort, peerIp, 0);
				Data_socket_cs.Unlock();
			}

			//받은 packet에 대하여 48bit data추출

			std::string data_temp = "";

			for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 10
				std::bitset<8> bits(newPacket->data[i]);
				data_temp += bits.to_string(); // bitset to string
			}
			cout << "이진화데이터 48bit :\n" << data_temp << "\n를 받았습니다.\n\n";


			packet_receive_buffer.Add(*newPacket); // 받은 패킷버퍼에 추가.
			if (packet_receive_buffer.GetSize() == newPacket->total_sequence_number) { // 1:1이므로 받은거에 들어있는거 바로체크해도됨
																					   // 최대 frame수랑 buffer에 저장된 frame수랑 같으면

																					   //Packet에 대하여 오름차순 퀵정렬
				QSortCArray(packet_receive_buffer, ComparePacket);
				//받은패킷들에 대해 모든 data추출 48*total_sequence_number bit
				std::string data_temp = "";
				for (int k = 0; k < newPacket->total_sequence_number; ++k) {
					for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 6
						std::bitset<8> bits(packet_receive_buffer.GetAt(k).data[i]);
						std::cout << bits << "\n";
						data_temp += bits.to_string(); // bitset to string
					}
				}

				rx_cs.Lock();
				arg2.pList->AddTail(CString(data_temp.c_str())); // string to CString // 추출한 data값 을 버퍼에 추가
				rx_cs.Unlock();

				packet_receive_buffer.RemoveAll(); // 버퍼비우기

			}
		}
	}
}



void CUDPClient_thdDlg::OnTimer(UINT_PTR nIDEvent) // 클래스 뷰에서 WM_Timer를 추가 시켜줬습니다.
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	
	//타이머동작

	/* Stop & Wait 동작 */
	timeout = true;

	CDialogEx::OnTimer(nIDEvent);
}

void CUDPClient_thdDlg::StartTimer(unsigned int timer_id, unsigned int n) // 클래스 뷰에서 WM_Timer를 추가 시켜줬습니다.
{ 
	SetTimer(timer_id, 1000, 0); // SetTimer(id, 주기ms, )이고 주기마다 OnTimer를 실행합니다.
}

void CUDPClient_thdDlg::StopTimer(unsigned int timer_id) // 클래스 뷰에서 WM_Timer를 추가 시켜줬습니다.
{
	KillTimer(timer_id); // 해당아이디의 timer를 종료합니다.
}