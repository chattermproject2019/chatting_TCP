// DataSocket.cpp : 구현 파일입니다.
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


// CDataSocket 멤버 함수


void CDataSocket::OnReceive(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CSocket::OnReceive(nErrorCode);
	m_pDlg->ProcessReceive(this, nErrorCode);
}
