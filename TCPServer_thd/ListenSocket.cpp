// ListenSocket.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "TCPServer_thd.h"
#include "ListenSocket.h"
#include "TCPServer_thdDlg.h"

// CListenSocket

CListenSocket::CListenSocket(CTCPServer_thdDlg *pDlg)
{
	m_pDlg = pDlg;
}

CListenSocket::~CListenSocket()
{
}


// CListenSocket ��� �Լ�


void CListenSocket::OnAccept(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CSocket::OnAccept(nErrorCode);
	m_pDlg->ProcessAccept(nErrorCode);

}
