// DataSocket.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "TCPClient_thd.h"
#include "DataSocket.h"
#include "TCPClient_thdDlg.h"


// CDataSocket

CDataSocket::CDataSocket(CTCPClient_thdDlg *pDlg)
{
	m_pDlg = pDlg;
}

CDataSocket::~CDataSocket()
{
}


// CDataSocket ��� �Լ�


void CDataSocket::OnReceive(int nErrorCode)
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CSocket::OnReceive(nErrorCode);
	m_pDlg->ProcessReceive(this, nErrorCode);
}
