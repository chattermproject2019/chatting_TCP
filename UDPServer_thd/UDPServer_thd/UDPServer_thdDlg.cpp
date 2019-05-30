
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

#define STOP_AND_WAIT 0
#define GO_BACK_N 1
#define SELECTIVE_REJECT 2

CCriticalSection tx_cs;
CCriticalSection rx_cs;
CCriticalSection Data_socket_cs; //Data Socket tx�ε� ������, ACK���������ε� ���̱⶧���� �ߺ����ϱ� ����.
								 //CCriticalSection Packet_Send_Buffer;
CCriticalSection ACK_Receive_BUFFER_cs; // ACK�޼����� �����ߴ����� üũ�ϱ����� cs.
CCriticalSection ACK_Send_BUFFER_cs; // ACK�޼����� �������� �ִ����� üũ�ϱ����� cs.
									 //CCriticalSection sequence_cs; // ���� seq number, timer�����忡�� �ߺ��ؼ� �����ϰ� ���� �ʱ� ����.



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
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_Open, &CUDPServer_thdDlg::OnBnClickedOpen)
END_MESSAGE_MAP()


// CUDPServer_thdDlg �޽��� ó����


std::string CString_to_BinaryStr(CString message) { // CString ���� ��ȯ�ϸ� hex�� �Ǿ �����ٷο�
	std::string temp = CT2CA(message); // CString to string

	char *ptr = (char*)temp.c_str();//"GFG";

	std::string temp2 = "";

	int i;
	for (; *ptr != 0; ++ptr)
	{
		for (i = 7; i >= 0; --i) // 8bit
			temp2 += (*ptr & 1 << i) ? ("1") : ("0");
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

unsigned short checksum_packet(unsigned short* packet, int length) {
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

void CUDPServer_thdDlg::packetSegmentation(CString message) {

	std::string binaried = CString_to_BinaryStr(message); // ���ڿ��� 2�������ڿ��θ���ϴ�. ���̴� 1���ڸ� 8bit�� �����ϴ�.
														  // �� ���ڸ� 8bitũ��� ����ȭ�Ͽ����Ƿ�, Packet�� data���� 80bit�� 10���� ���ڸ� �����Ͽ� ���� �� �ֽ��ϴ�.
	std::wcout << "�����÷��� �޼��� :" << (const wchar_t*)message << " �� \n"; //CString�� wcout���� ����ؾ� 16������ �ȳ���
	std::cout << "�������� " << binaried << "�Դϴ�\n";
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
		temp += binaried.at(i); // 1bit�� �߰�.
		if (temp.length() == 8) { // 8��°���� 8bit�̹Ƿ� 1byte�� ���尡��
			std::bitset<8> bits(temp); // ex) "10101010" => ���� 170 == 0b10101010
			newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]�� ���ؼ� 8bit(1byte)�� ���ڷ� ����
			packet_data_count++;
			temp = "";
			if (packet_data_count == 6) { // �Ź� 48��° bit�� �߰��Ҷ����� �̶����� ������ packet�� packet buffer�� �����մϴ�.
				newPacket.seq = ++seq; // seq�ѹ��� �߰�
				newPacket.total_sequence_number = total_packet; // ���ڿ� ����ȭ�ѰŸ� 80bit�� ������ �� ���� frame��������
				
				newPacket.checksum = 0;
				unsigned short* short_packet = (unsigned short*)&newPacket;
				newPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
				printf("�������� ��Ŷ�� üũ�� %x\n", newPacket.checksum);

				packet_send_buffer.Add(newPacket); //���ۿ� ��Ŷ �߰�

				newPacket = Packet(); // �� ��Ŷ�Ҵ�
				packet_data_count = 0;
			}
		}
	}
	bool thereIsData = false; // �������ϴ� ���� �����Ͱ� �ִ��� üũ
	for (int data_checker = 0; data_checker < 6; data_checker++) {
		if (newPacket.data[data_checker] ^ 0 != 0) {
			thereIsData = true;
		}
	}
	if (thereIsData&&(temp.length() < 8)) { // 8bit���� �۰� ����� ��Ŷ�϶��� �ٷ� ����
		newPacket.seq = ++seq; // seq�ѹ��� �߰�
		std::bitset<8> bits(temp); // ex) "10101010" => ���� 170 == 0b10101010
		newPacket.data[packet_data_count % 10] = bits.to_ulong(); //data[0]~data[9]�� ���ؼ� 8bit(1byte)�� ���ڷ� ����
		newPacket.total_sequence_number = total_packet; // �̳༮�� ���� for������ packet�������Ƿ� �ִ� frame������ 1
		
		newPacket.checksum = 0;
		unsigned short* short_packet = (unsigned short*)&newPacket;
		newPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
		printf("�������� ��Ŷ�� üũ�� %x", newPacket.checksum);
		
		packet_send_buffer.Add(newPacket); //���ۿ� ��Ŷ �߰�
	
	}
	std::cout << "\n";
}

UINT timer_thread_func(LPVOID arg) { // Ÿ�̸� ������
	timerThreadArg *pArg = (timerThreadArg *)arg;
	CUDPServer_thdDlg *pDlg = (CUDPServer_thdDlg *)pArg->pDlg;

	pDlg->StartTimer(pDlg->arg3.timer_id, pDlg->arg3.deadline); // Ÿ�̸� ����, deadline�ֱ�� OnTime�Լ� ����

																//AfxEndThread(0, false);

	return 0;
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
		POSITION current_pos= NULL;

		//while (pos != NULL)
		if (pos != NULL) {
			tx_cs.Lock();
			current_pos = pos;
			
			CString str = plist->GetNext(pos); // Ŭ�������� ���ڿ��� plist�� ���, �װ� str�� ������
			pDlg->packetSegmentation(str); /// �� CString str�� ���� segmentation�Ͽ� pack���ۿ� ����
			

			// message�� �������� frame���� �ɰ��� ��Ȳ

			CString message;
			pDlg->m_tx_edit.GetWindowText(message);
			message += "\r\n";
			pDlg->m_tx_edit.SetWindowTextW(message);
			pDlg->m_tx_edit.LineScroll(pDlg->m_tx_edit.GetLineCount());
			plist->RemoveAt(current_pos);
			tx_cs.Unlock();

		}
		while (!pDlg->packet_send_buffer.IsEmpty() || !pDlg->ack_send_buffer.IsEmpty())
		{
	
			if (pDlg->mode == STOP_AND_WAIT) {
				//while(!pDlg->packet_send_buffer.IsEmpty()){ // ��Ŷ���ۿ� ���������� ����
				for (int i = 0; i < pDlg->packet_send_buffer.GetSize();) {



					//(char*)& �����ָ� ����ü ������. ���Ŵܵ� ������ �޾������
					pDlg->timeout = false;
					//std::cout << pDlg->packet_send_buffer.GetAt(i).seq << " �� frame�� �����ϴ�.\n";
					Data_socket_cs.Lock();
					pDlg->m_pDataSocket->SendToEx((char*)&pDlg->packet_send_buffer.GetAt(i), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
					Data_socket_cs.Unlock();

					/*sequence_cs.Lock();
					pDlg->next_sequnce = i;
					sequence_cs.Unlock();*/

					/* ack�޼��� ���ű�ٸ�*/
					std::cout << "Ack�޼����� ��ٸ��� �ֽ��ϴ�...\n";

					int timer_id = (int)(rand() % 1000); // �������� id ����, �ߺ����� �ʰ� �����ϱ�
					while (pDlg->timer_id_checker[timer_id]) { timer_id = (int)(rand() * 1000); }
					pDlg->timer_id_checker[timer_id] = true;

					pDlg->arg3.deadline = 1000; // 1�ʰ� deadline�Դϴ�.
					pDlg->arg3.timer_id = timer_id;
					//Ÿ�̸� ������ �ٷ� ����.
					pDlg->arg3.frame_seq = i;

					pDlg->timerThread = AfxBeginThread(timer_thread_func, (LPVOID)&pDlg->arg3, NULL);

					//pDlg->StartTimer(timer_id, pDlg->arg3.deadline); // Ÿ�̸� ����, deadline�ֱ�� OnTime�Լ� ����

					while (pDlg->ack_receive_buffer.IsEmpty()) { // ack���۰� �������.
						if (pDlg->timeout == true) { // ���ۿ� �ƹ��͵� ���� ���·�, �ð������� expire
							std::cout << "Expired!\n";
							break;
						}
					}

					//��while���� ���������� ���� ack���ۿ� ���� �߰� �Ǿ��ų�, timeout�Ǿ��ų� �����ϳ���
					pDlg->StopTimer(pDlg->arg3.timer_id); //timer���� 
					pDlg->timer_id_checker[timer_id] = false;

					if (!pDlg->ack_receive_buffer.IsEmpty()) { // ack ���ۿ� ���𰡰� ��������.
						if (pDlg->ack_receive_buffer.GetAt(0) > 0) { // ���� �޼����� ack����
							std::cout << "ack�޼����� �޾����Ƿ�, ���´� " << pDlg->ack_receive_buffer.GetAt(0) << "�� frame�� Ȯ�������ϴ�.\n";
							pDlg->ack_receive_buffer.RemoveAt(0); // ack ����Ȯ���Ѱ� clear
							++i;								  //pArg->frame_seq; // �״���־�� tx���� i++�Ǽ� ���� seq�� �Ѿ
						}
						else if ((pDlg->ack_receive_buffer.GetAt(0) < 0)) { // �����޼����� nack������ �Ȱ����� �ѹ� �� ����
							std::cout << "nack�޼����� �޾����Ƿ� ���´�" << -1 * pDlg->ack_receive_buffer.GetAt(0) << "�� frame�� �ٽ� �����ϴ�.\n";
							//--i;
							//pArg->frame_seq--; // �ѹ� �� ������ ����
							pDlg->ack_receive_buffer.RemoveAt(0); // ack ����Ȯ���Ѱ� clear
																  //break; // �Ȱ���(nack) frame���������� break;
						}
					}
					else if (pDlg->timeout == true) { // ��� ack�޼����� ���޾�����, timeout�� ��Ŷloss�̹Ƿ� �������������
						std::cout << "timeout�̹Ƿ� ���´� " << pDlg->packet_send_buffer.GetAt(i).seq << "�� frame�� �ٽ� �����ϴ�.\n";
						//pArg->frame_seq--; // �ѹ� �� ������ ����
						//i--;
					}

				}

			}
			else if (pDlg->mode == GO_BACK_N) {

				//while(!pDlg->packet_send_buffer.IsEmpty()){ // ��Ŷ���ۿ� ���������� ����
				int current_frame = 1;
				int before_frame = 0;// window�� �̵��ϱ� �� frame seq
				for (int i = 0; i < pDlg->packet_send_buffer.GetSize(); ++i) {

					pDlg->timeout = false;

					// �������ϴ� ack�޼����� ������ ���� ������ ��Ŷ�� ������ ���Խ�Ű��, �����մϴ�.
					ACK_Send_BUFFER_cs.Lock();
					if (!pDlg->ack_send_buffer.IsEmpty()) {
						pDlg->packet_send_buffer.GetAt(i).response = pDlg->ack_send_buffer.GetAt(0).response;
						pDlg->ack_send_buffer.RemoveAt(0);
						std::cout << pDlg->packet_send_buffer.GetAt(i).seq << " �� frame�� �����ϴ�.(PiggyBack)\n";
					}
					else {
						std::cout << pDlg->packet_send_buffer.GetAt(i).seq << " �� frame�� �����ϴ�.(Piggy Back �ƴ�!)\n";
					}
					ACK_Send_BUFFER_cs.Unlock();


					//pDlg->packet_send_buffer.GetAt(i)
					Data_socket_cs.Lock();
					pDlg->m_pDataSocket->SendToEx((char*)&pDlg->packet_send_buffer.GetAt(i), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
					Data_socket_cs.Unlock();


					if (current_frame < pDlg->window_size && //// window size��ŭ �����ϴ�.
						i + pDlg->window_size <= pDlg->packet_send_buffer.GetSize()) { // �׸��� �������� windowsize���� �۾ƾ��մϴ�.
						current_frame++;
						continue;
					}
					else if (!(i + pDlg->window_size <= pDlg->packet_send_buffer.GetSize())) {
						// window size����, ������ ��Ŷ���� ���� �������Ƿ� �� �����ϴ�. (���� continue�Ἥ window size��ŭ �����ְ� �־. �� ���ǹ� �����ָ� windowsize�� �� ����������� ������� ������ �Ϻ��ϰ� �ȵ�.)
						before_frame = i;
						for (int j = i + 1; j < pDlg->packet_send_buffer.GetSize(); ++j) {

							ACK_Send_BUFFER_cs.Lock();
							if (!pDlg->ack_send_buffer.IsEmpty()) {
								pDlg->packet_send_buffer.GetAt(j).response = pDlg->ack_send_buffer.GetAt(0).response;
								pDlg->ack_send_buffer.RemoveAt(0);
								std::cout << pDlg->packet_send_buffer.GetAt(j).seq << " �� frame�� �����ϴ�.(PiggyBack)\n";
							}
							else {
								std::cout << pDlg->packet_send_buffer.GetAt(j).seq << " �� frame�� �����ϴ�.(Piggy Back �ƴ�!)\n";
							}
							ACK_Send_BUFFER_cs.Unlock();

							Data_socket_cs.Lock();
							pDlg->m_pDataSocket->SendToEx((char*)&pDlg->packet_send_buffer.GetAt(j), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
							Data_socket_cs.Unlock();

							++i; // i�� ���������ݴϴ�.
						}

					}
					else {
						before_frame = i - current_frame;
						current_frame = 1;
					}

					/* ack�޼��� ���ű�ٸ�*/
					std::cout << "Ack�޼����� ��ٸ��� �ֽ��ϴ�...\n";

					int timer_id = (int)(rand() % 1000); // �������� id ����, �ߺ����� �ʰ� �����ϱ�
					while (pDlg->timer_id_checker[timer_id]) { timer_id = (int)(rand() * 30); }
					pDlg->timer_id_checker[timer_id] = true;

					pDlg->arg3.deadline = 1000; // 1�ʰ� deadline�Դϴ�.
					pDlg->arg3.timer_id = timer_id;
					//Ÿ�̸� ������ �ٷ� ����.
					pDlg->arg3.frame_seq = i;

					pDlg->timerThread = AfxBeginThread(timer_thread_func, (LPVOID)&pDlg->arg3, NULL);

					//pDlg->StartTimer(timer_id, pDlg->arg3.deadline); // Ÿ�̸� ����, deadline�ֱ�� OnTime�Լ� ����

					while (pDlg->ack_receive_buffer.IsEmpty()) { // ack���۰� �������.
						if (pDlg->timeout == true) { // ���ۿ� �ƹ��͵� ���� ���·�, �ð������� expire
							std::cout << "Expired!\n";
							break;
						}
					}

					//��while���� ���������� ���� ack���ۿ� ���� �߰� �Ǿ��ų�, timeout�Ǿ��ų� �����ϳ���
					pDlg->StopTimer(pDlg->arg3.timer_id); //timer���� 
					pDlg->timer_id_checker[timer_id] = false;

					if (!pDlg->ack_receive_buffer.IsEmpty()) { // ack ���ۿ� ���𰡰� ��������.
						ACK_Receive_BUFFER_cs.Lock();
						//std::cout << "ack�޼�������\n";
						if (pDlg->ack_receive_buffer.GetAt(0) > 0) { // ���� �޼����� ack����

							std::cout << "ack�޼����� �޾����Ƿ�, ���´� " << pDlg->ack_receive_buffer.GetAt(0) << "�� frame ���� Ȯ�������ϴ�.\n";
							pDlg->ack_receive_buffer.RemoveAt(0); // ack ����Ȯ���Ѱ� clear

																  //�״�� ����~
						}
						else if ((pDlg->ack_receive_buffer.GetAt(0) < 0)) { // �����޼����� nack������ �Ȱ����� �ѹ� �� ����

							std::cout << "nack�޼����� �޾����Ƿ� ���´�" << -1 * pDlg->ack_receive_buffer.GetAt(0) << "�� frame���� �ٽ� �����ϴ�.\n";
							//--i;
							//�ش� frame���� �ٽú����� �ϹǷ�, ������ frame seq�� i�� ����

							i = -1 * pDlg->ack_receive_buffer.GetAt(0);
							pDlg->ack_receive_buffer.RemoveAt(0); // ack ����Ȯ���Ѱ� clear
																  //break; // �Ȱ���(nack) frame���������� break;
						}
						ACK_Receive_BUFFER_cs.Unlock();
					}
					else if (pDlg->timeout == true) { // ��� ack�޼����� ���޾�����, timeout�� ��Ŷloss�̹Ƿ� �������������
						std::cout << "timeout�̹Ƿ� ���´� " << before_frame + 1 << "�� frame���� �ٽ� �����ϴ�.\n";
						//pArg->frame_seq--; // �ѹ� �� ������ ����
						//i--;

						if (i == 0) { //�ѹ��� ������
							i = before_frame - 1;
						}
						else {
							i = before_frame;
						}

						continue;
					}

				}

				while (!pDlg->ack_send_buffer.IsEmpty()) { // ������ piggy back���� �ʾҴٸ�, �׳� ack�޼��� �����ݴϴ�.

														   //if (!pDlg->packet_send_buffer.IsEmpty()) {// Ȥ���� ���߿� packet data������ ����� �� for���� �����ϰ�, ������ piggyback�ǵ����մϴ�.
														   //	break;
														   //}

					ACK_Send_BUFFER_cs.Lock();
					if (!pDlg->ack_send_buffer.IsEmpty()) {
						std::cout << pDlg->ack_send_buffer.GetAt(0).response.ACK << " �� ack�� �����ϴ�.(no PiggyBack)\n";
						Data_socket_cs.Lock();
						pDlg->m_pDataSocket->SendToEx((char*)&pDlg->ack_send_buffer.GetAt(0), sizeof(Packet), pDlg->peerPort, pDlg->peerIp, 0);
						Data_socket_cs.Unlock();
						pDlg->ack_send_buffer.RemoveAt(0);
					}
					ACK_Send_BUFFER_cs.Unlock();
				}
			}



			pDlg->packet_send_buffer.RemoveAll(); // for���̱� ������ �ٺ������� �� ����

		}

		//pDlg->m_pDataSocket->SendToEx(str, (str.GetLength() + 1) * sizeof(TCHAR), pDlg->peerPort, pDlg->peerIp, 0); ///UDP������ ���Ͽ� �ش� ��Ʈ�� ip�ּҷ� �޼����� �����մϴ�.
		
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

	window_size = 3;


	CString myIp = _T("127.0.0.1");
	int myPort = 6789;
	
	/*CString str123 = _T("123127787594735479749357479579479473");
	std::cout << sizeof(str123) << "\n";*/

	memset(timer_id_checker,false, sizeof(timer_id_checker));

	ACK_Send_BUFFER_cs.Lock();
	ack_send_buffer.RemoveAll();
	ACK_Send_BUFFER_cs.Unlock();

	packet_send_buffer.RemoveAll();

	ACK_Receive_BUFFER_cs.Lock();
	ack_receive_buffer.RemoveAll();
	ACK_Receive_BUFFER_cs.Unlock();

	mode = GO_BACK_N; // 0-STOP_AND_WAIT, 1-GO_BACK_N, 2-SELECTIVE_REJECT

	error_buffer.RemoveAll();
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

	arg3.pDlg = this;
	arg3.frame_seq = -1;

	pThread1 = AfxBeginThread(TXThread, (LPVOID)&arg1, THREAD_PRIORITY_ABOVE_NORMAL);
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
	//tx_message += _T("\r\n"); //�ٹٲ��� �߰�
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
	Packet AckPacket = Packet(); //Ack reply���Դϴ�.

	nbytes = pSocket->ReceiveFromEx(pBuf, sizeof(Packet), peerIp, peerPort, 0); // ��Ŷ�� char[]������ ���ƿ�
	pBuf[nbytes] = NULL;

	newPacket = (Packet*)pBuf; // Packet������ ����

	
	unsigned short* short_packet = (unsigned short*)newPacket;//Packet to unsigned short*
	unsigned short calculatedChecksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
	
	// control packet���� üũ
	if 	(calculatedChecksum==0 && newPacket->seq == 0xff && newPacket->total_sequence_number == 0xff && newPacket->data[2]== 0x7f && newPacket->data[3] == 0x7f && newPacket->data[4] == 0x7f && newPacket->data[5] == 0x7f) {
		
		//AfxMessageBox(_T("���� ����� �ʱ�ȭ�մϴ�.."));
		pThread1->SuspendThread(); //Tx stop
		pThread2->SuspendThread(); // tx stop
		packet_send_buffer.RemoveAll(); // �����ٲ��ٰŶ� �ǵ����̸� buffer�� ����ݴϴ�.
		packet_receive_buffer.RemoveAll();
		ack_receive_buffer.RemoveAll();
		ack_send_buffer.RemoveAll();
		pThread1->ResumeThread();// ������ �����
		pThread2->ResumeThread(); // ������ �����

		mode = newPacket->data[0];
		std::string mode_name = mode == 0 ? "STOP_AND_WAIT" : "GO_BACK_N";
		window_size = newPacket->data[1];
		std::string str_temp2 = ( std::string("mode: ") + mode_name + std::string("\nwindow size: ") + std::to_string(window_size).c_str() + std::string(("\n����ȭ�Ǿ����ϴ�.")));
		AfxMessageBox(CString(str_temp2.c_str()));
		
		return;
	}
	
	
	std::wcout << (const wchar_t*)peerIp << "�� ���� �� " << newPacket->total_sequence_number << "�� frame ������\n=> ";
	std::cout << "���� " << newPacket->seq << "��° frame����\n";
	printf("���� ��Ŷ�� üũ�� %x, ����� üũ�� ��: %x \n", newPacket->checksum, calculatedChecksum);


	
	if (mode == STOP_AND_WAIT) {
		
		if (calculatedChecksum != 0) { // checksum �����̸�
			std::cout << "üũ�� �����Դϴ�.\n";

			/*stop & wait ���� ��Ʈ��*/
			if (newPacket->response.ACK != 0) { // ACK�� �����Ǵ� �޼����� checksum������ ���. �����ؼ� expire�ǰ��Ѵ�.
				return;
			}
			if ( newPacket->seq!=0 ) { // �����ε� ACK�޼����� �ƴ϶�� �����Ǹ�, ��������Ŷ�� �����ΰɷ� �����ϰ� NACK�޼����� ������.

				cout << "������ ��Ŷ�� �����̹Ƿ�, �Ϻη� timer expire �ؼ� �ٽ� �����۽�Ŵ.\n";

				return; // �����̰�, ACK�޼����� �ƴѰ����� ����. ������ �����̹Ƿ� NACK������, ����.
			}

		}
		else { // checksum������ ����

			   /*Stop & Wait�� piggy back���� ���̹Ƿ� ack �Ǵ� data������ ����*/

			   /*Stop & Wait ACK�޼��� �����ϴ� ���*/
			if (newPacket->response.ACK != 0) {
				std::cout << "ACK�޼�������: ��������: " << newPacket->response.no_error << " �ش�frame ��ȣ: " << newPacket->response.ACK << " ��Ӽ��Ű��ɿ���: " << newPacket->response.no_error << "\n";

				// ack�����Ҷ����� send�κп� �˷������. 1���� frame�� ������ ������.
				if (newPacket->response.no_error == true) { // ACK 
					ack_receive_buffer.Add(newPacket->response.ACK); // ���������� ���� frame�ѹ��� ����.
				}
				else { // NACK
					ack_receive_buffer.Add(-1 * newPacket->response.ACK); // ������ ���� frame�ѹ��� ����. ������
				}
				return; //piggy back�ƴϹǷ� ack�� Ȯ���ϰ� ����
			}

			if(newPacket->seq != 0) { // ack�޼��� �ƴϹǷ� data ����ó�� 


				if (newPacket->seq != 0) { // �����͸� �޾�����, Ȯ�θ޼����� �����ϴ�.
					AckPacket.response.ACK = newPacket->seq; //���� ��Ŷ��ȣ
					AckPacket.response.more = true; // �� ���Ű���
					AckPacket.response.no_error = true; // ��������

					AckPacket.checksum = 0;
					unsigned short* short_packet = (unsigned short*)&AckPacket;
					AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
					printf("�������� ack ��Ŷ�� üũ�� %x\n", AckPacket.checksum);

					Data_socket_cs.Lock();
					std::cout << newPacket->seq << " �� frame�� ���� ack�޼����� �����ϴ�.\n";
					m_pDataSocket->SendToEx((char*)&AckPacket, sizeof(Packet), peerPort, peerIp, 0);
					Data_socket_cs.Unlock();
				}

				//���� packet�� ���Ͽ� 48bit data����

				std::string data_temp = "";

				for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 10
					std::bitset<8> bits(newPacket->data[i]);
					data_temp += bits.to_string(); // bitset to string
				}
				cout << "����ȭ������ 48bit :\n" << data_temp << "\n�� �޾ҽ��ϴ�.\n\n";


				packet_receive_buffer.Add(*newPacket); // ���� ��Ŷ���ۿ� �߰�.
				if (packet_receive_buffer.GetSize() == newPacket->total_sequence_number) { // 1:1�̹Ƿ� �����ſ� ����ִ°� �ٷ�üũ�ص���
																						   // �ִ� frame���� buffer�� ����� frame���� ������

																						   //Packet�� ���Ͽ� �������� ������
					QSortCArray(packet_receive_buffer, ComparePacket);
					//������Ŷ�鿡 ���� ��� data���� 48*total_sequence_number bit
					std::string data_temp = "";
					for (int k = 0; k < newPacket->total_sequence_number; ++k) {
						for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 6
							std::bitset<8> bits(packet_receive_buffer.GetAt(k).data[i]);
							std::cout << bits << "\n";
							data_temp += bits.to_string(); // bitset to string
						}
					}

					rx_cs.Lock();
					arg2.pList->AddTail(CString(data_temp.c_str())); // string to CString // ������ data�� �� ���ۿ� �߰�
					rx_cs.Unlock();

					packet_receive_buffer.RemoveAll(); // ���ۺ���

				}
			}
		}
	}
	else if (mode == GO_BACK_N ) {

		if (calculatedChecksum != 0) { // checksum �����̸�
			std::cout << "���� " << newPacket->seq << "��° frame����\n";
			std::cout << "üũ�� �����Դϴ�.\n";

			/*���� ��Ʈ��*/
			if (newPacket->response.ACK != 0) { // (n)ACK�� �����Ǵ� �޼����� checksum������ ���. �����ؼ� expire�ǰ��Ѵ�.
				std::cout << "checksum������ (N)ACK�� �����Ǵ� �޼����� �����Ͽ����Ƿ� �����մϴ�.\n";
				//���� => timeout��Ŵ (piggy back�ε�, �����͵� ���°�� �׳� ���õȴٴ� ��. )
			}
			if (newPacket->seq != 0) { // �����ε� ACK�޼����� �ƴ϶�� �����Ǹ�, �����ͺκ��� �����ΰɷ� �����ϰ� NACK�޼����� ������.

				std::cout << "checksum������ data�� �����Ǵ� �޼����� �߰ߵǾ� nack�� �����մϴ�.\n";

				current_error_frame = newPacket->seq;

				AckPacket.response.ACK = newPacket->seq; //���� ��Ŷ��ȣ
				AckPacket.response.more = true; // �� ���Ű���
				AckPacket.response.no_error = false; // ���������� ����
				AckPacket.total_sequence_number = 1; // ack�޼����� 1���� ���

				AckPacket.checksum = 0;
				unsigned short* short_packet = (unsigned short*)&AckPacket;
				AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
				printf("�������� nack ��Ŷ�� üũ�� %x\n", AckPacket.checksum);

				ACK_Send_BUFFER_cs.Lock();
				ack_send_buffer.Add(AckPacket); // ack��Ŷ�� ack send���ۿ� �߰��մϴ�.
				ACK_Send_BUFFER_cs.Unlock();
				Sleep(0); // ���� Ȥ�� ���� �켱������ �����忡�� �纸�մϴ�.
				/*ACK_Send_BUFFER_cs.Lock();
				Data_socket_cs.Lock();
				std::cout << ack_send_buffer.GetAt(0).seq << " �� frame�� ���� (n)ack�޼����� �����ϴ�. (Piggy Back�ƴ�!)\n";
				m_pDataSocket->SendToEx((char*)&ack_send_buffer.GetAt(0), sizeof(Packet), peerPort, peerIp, 0);
				Data_socket_cs.Unlock();
				ack_send_buffer.RemoveAt(0);
				ACK_Send_BUFFER_cs.Unlock();*/
				


				//return; // �����̰�, ACK�޼����� �ƴѰ����� ����. ������ �����̹Ƿ� NACK������, ����.
			}

		}
		else { // checksum������ ����


			   /*�������� (N)ACK�޼��� �����ϴ� ���*/
			if (newPacket->response.ACK != 0) {
				std::cout << "ACK�޼�������: ��������: " << newPacket->response.no_error << " �ش�frame ��ȣ: " << newPacket->response.ACK << " ��Ӽ��Ű��ɿ���: " << newPacket->response.no_error << "\n";

				// ack�����Ҷ����� send�κп� �˷������
				if (newPacket->response.no_error == true) { // ACK 
					ACK_Receive_BUFFER_cs.Lock();
					ack_receive_buffer.Add(newPacket->response.ACK); // ���������� ���� frame�ѹ��� ����.
					ACK_Receive_BUFFER_cs.Unlock();
				}
				else { // NACK
					ACK_Receive_BUFFER_cs.Lock();
					ack_receive_buffer.Add(-1 * newPacket->response.ACK); // ������ ���� frame�ѹ��� ����. ������
					ACK_Receive_BUFFER_cs.Unlock();
				}

			}

			/*�������� ������ ���� */
			if (newPacket->seq != 0) {
				std::cout << "���� " << newPacket->seq << "��° frame����\n";

				if (!packet_receive_buffer.IsEmpty()) { // ���� packet�� ���ۿ� ���� �ְ�
														// �������� ������ �޾Ҵµ� ���������� ���� frame seq�� ���������� ������
					if (newPacket->seq - 1 != packet_receive_buffer.GetAt(packet_receive_buffer.GetSize() - 1).seq) {
						std::cout << "���ӵ��� ���� frame�� �޾ҽ��ϴ�..\n"; // �ߺ����� ȿ���� ����
						std::cout << "���������� ���� seq�� " << packet_receive_buffer.GetAt(packet_receive_buffer.GetSize() - 1).seq << "�Դϴ�\n";
						if (current_error_frame != -1) { // ������ ������ �Ͼ�� �ʾ����Ƿ�, 
														 //�� ���ӵ��� ���� frame�� loss�Ǵ� routing delay�� ���� ������ ���ΰ� �Դϴ�.
														 // nack������ �ٽ� �޽��ϴ�.
							current_error_frame = packet_receive_buffer.GetAt(packet_receive_buffer.GetSize() - 1).seq + 1; // ���������� ���� seq���� �������� �޾ƾ��� ���� ������ �� ���Դϴ�.

							AckPacket.response.ACK = current_error_frame; //���� ��Ŷ��ȣ
							AckPacket.response.more = true; // �� ���Ű���
							AckPacket.response.no_error = false; // ��������
							AckPacket.total_sequence_number = 1; // ack�޼����� 1���� ���

							AckPacket.checksum = 0;
							unsigned short* short_packet = (unsigned short*)&AckPacket;
							AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
							printf("�������� nack ��Ŷ�� üũ�� %x\n", AckPacket.checksum);

							ACK_Send_BUFFER_cs.Lock();
							ack_send_buffer.Add(AckPacket); // ack��Ŷ�� ack send���ۿ� �߰��մϴ�.
							ACK_Send_BUFFER_cs.Unlock();
							
							Sleep(0); // ���� Ȥ�� ���� �켱������ �����忡�� �纸�մϴ�.
							/*ACK_Send_BUFFER_cs.Lock();
							Data_socket_cs.Lock();
							std::cout << ack_send_buffer.GetAt(0).seq << " �� frame�� ���� (n)ack�޼����� �����ϴ�. (Piggy Back�ƴ�!)\n";
							m_pDataSocket->SendToEx((char*)&ack_send_buffer.GetAt(0), sizeof(Packet), peerPort, peerIp, 0);
							Data_socket_cs.Unlock();
							ack_send_buffer.RemoveAt(0);
							ACK_Send_BUFFER_cs.Unlock();
*/
							return;
						}
						else { // ������ ������ �Ͼ�� nack������, ������� ������ ���̹Ƿ�, ���� �͵��� �� �����մϴ�.
							std::cout << "nack�� �������Ƿ�, �����޼��� �����Ҷ����� ���ӵ��� ���� frame ����\n";
							return;
						}

					}
				}
				else { // ������Ŷ ���۰� ��������� �޾Ҵٸ�
					   //���� ��Ŷ�� seq�� 1���� Ȯ���մϴ�.
					if (newPacket->seq != 1) { // ó�� �޴� frame�ε� seq�� 1�� �ƴϸ� �����Դϴ�.
						current_error_frame = 1;

						AckPacket.response.ACK = current_error_frame; //���� ��Ŷ��ȣ
						AckPacket.response.more = true; // �� ���Ű���
						AckPacket.response.no_error = false; // ����
						AckPacket.total_sequence_number = 1; // ack�޼����� 1���� ���

						AckPacket.checksum = 0;
						unsigned short* short_packet = (unsigned short*)&AckPacket;
						AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
						printf("�������� nack ��Ŷ�� üũ�� %x\n", AckPacket.checksum);

						ACK_Send_BUFFER_cs.Lock();
						ack_send_buffer.Add(AckPacket); // ack��Ŷ�� ack send���ۿ� �߰��մϴ�.
						ACK_Send_BUFFER_cs.Unlock();
						//Sleep(1); //�ǵ����̸� piggy back�ϵ��� ��¦ �������ݴϴ�.
						Sleep(0); // ���� Ȥ�� ���� �켱������ �����忡�� �纸�մϴ�.
						/*ACK_Send_BUFFER_cs.Lock();
						Data_socket_cs.Lock();
						std::cout << ack_send_buffer.GetAt(0).seq << " �� frame�� ���� (n)ack�޼����� �����ϴ�. (Piggy Back�ƴ�!)\n";
						m_pDataSocket->SendToEx((char*)&ack_send_buffer.GetAt(0), sizeof(Packet), peerPort, peerIp, 0);
						Data_socket_cs.Unlock();
						ack_send_buffer.RemoveAt(0);
						ACK_Send_BUFFER_cs.Unlock();*/

						return; // nack �������Ƿ� ���̻� �����Ͱ��� ó���� ���� �ʽ����Ӵ�
					}
					else { // ���������� �޾����Ƿ� �Ѿ�ϴ�.
						   // pass
					}
				}
				//Sleep(2000);

				current_error_frame = -1; // ����� �޾Ҵٸ�, ���������� clear

										  //���� packet�� ���Ͽ� 48bit data����
				std::string data_temp = "";

				for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 10
					std::bitset<8> bits(newPacket->data[i]);
					data_temp += bits.to_string(); // bitset to string
				}
				cout << "����ȭ������ 48bit :\n" << data_temp << "\n�� �޾ҽ��ϴ�.\n\n";

				packet_receive_buffer.Add(*newPacket); // ���� ��Ŷ���ۿ� �߰�.
				std::cout << "���� receive packet ���� ������� " << packet_receive_buffer.GetSize() << "�Դϴ�.\n";


				if (packet_receive_buffer.GetSize() % window_size == 0 ||
					newPacket->total_sequence_number == packet_receive_buffer.GetAt(packet_receive_buffer.GetSize() - 1).seq
					) { // window size��ŭ �������� ���������� Ȯ�θ޼����� �����ϴ�. 
						// �Ǵ� ������ frame�� �޾������� Ȯ�θ޼����� �����ϴ�.( �� frame���� windowsize�� ����� �ƴ� ���� �����Ƿ�   ) 

					AckPacket.seq = 0;

					AckPacket.response.ACK = newPacket->seq; //���� ��Ŷ��ȣ
					AckPacket.response.more = true; // �� ���Ű���
					AckPacket.response.no_error = true; // ��������
					AckPacket.total_sequence_number = 1;

					AckPacket.checksum = 0;
					unsigned short* short_packet = (unsigned short*)&AckPacket;
					AckPacket.checksum = checksum_packet(short_packet, sizeof(short_packet) / sizeof(short_packet[0]));
					printf("�������� ack ��Ŷ�� üũ�� %x\n", AckPacket.checksum);

					ACK_Send_BUFFER_cs.Lock();
					ack_send_buffer.Add(AckPacket); // ack��Ŷ�� ack send���ۿ� �߰��մϴ�.
					ACK_Send_BUFFER_cs.Unlock();

					Sleep(0); // ���� Ȥ�� ���� �켱������ �����忡�� �纸�մϴ�.
				/*ACK_Send_BUFFER_cs.Lock();
				Data_socket_cs.Lock();
				std::cout << ack_send_buffer.GetAt(0).seq << " �� frame�� ���� (n)ack�޼����� �����ϴ�. (Piggy Back�ƴ�!)\n";
				m_pDataSocket->SendToEx((char*)&ack_send_buffer.GetAt(0), sizeof(Packet), peerPort, peerIp, 0);
				Data_socket_cs.Unlock();
				ack_send_buffer.RemoveAt(0);
				ACK_Send_BUFFER_cs.Unlock();*/
					//Sleep(1); //�ǵ����̸� piggy back�ϵ��� ��¦ �������ݴϴ�.

				}

				if (packet_receive_buffer.GetSize() == newPacket->total_sequence_number) { // 1:1�̹Ƿ� �����ſ� ����ִ°� �ٷ�üũ�ص���
																						   // �ִ� frame���� buffer�� ����� frame���� ������																   //Packet�� ���Ͽ� �������� ������
					QSortCArray(packet_receive_buffer, ComparePacket);
					//������Ŷ�鿡 ���� ��� data���� 48*total_sequence_number bit
					std::string data_temp = "";
					for (int k = 0; k < packet_receive_buffer.GetSize(); ++k) {
						for (int i = 0; i < sizeof(newPacket->data); ++i) { // sizeof(newPacket->data) == 6
							std::bitset<8> bits(packet_receive_buffer.GetAt(k).data[i]);
							std::cout << bits << "\n";
							data_temp += bits.to_string(); // bitset to string
						}
					}
					rx_cs.Lock();
					arg2.pList->AddTail(CString(data_temp.c_str())); // string to CString // ������ data�� �� ���ۿ� �߰�
					rx_cs.Unlock();

					packet_receive_buffer.RemoveAll(); // ���ۺ���
					overlap_checker_Lock = false;
				}
			}


		}
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


void CUDPServer_thdDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	//Ÿ�̸ӵ���

	/* Stop & Wait ���� */
	timeout = true;

	CDialogEx::OnTimer(nIDEvent);
}

void CUDPServer_thdDlg::StartTimer(unsigned int timer_id, unsigned int n) // Ŭ���� �信�� WM_Timer�� �߰� ��������ϴ�.
{
	SetTimer(timer_id, 1000, 0); // SetTimer(id, �ֱ�ms, )�̰� �ֱ⸶�� OnTimer�� �����մϴ�.
}

void CUDPServer_thdDlg::StopTimer(unsigned int timer_id) // Ŭ���� �信�� WM_Timer�� �߰� ��������ϴ�.
{
	KillTimer(timer_id); // �ش���̵��� timer�� �����մϴ�.
}

void CUDPServer_thdDlg::OnBnClickedOpen()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	static TCHAR BASED_CODE szFilter[] = _T("�̹��� ����(*.BMP, *.GIF, *.JPG) | *.BMP; *.GIF; *.JPG; *.bmp; *.jpg; *.gif | �������(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.jpg"), _T("image"), OFN_HIDEREADONLY, szFilter);
	if (IDOK == dlg.DoModal())
	{
		CString pathName = dlg.GetPathName();
		MessageBox(pathName);
	}
}
