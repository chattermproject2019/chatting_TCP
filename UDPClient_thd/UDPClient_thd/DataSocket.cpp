// DataSocket.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "UDPClient_thd.h"
#include "DataSocket.h"
#include "UDPClient_thdDlg.h"

// CDataSocket

CDataSocket::CDataSocket(CUDPClient_thdDlg *pDlg)
{
	m_pDlg = pDlg;
}

CDataSocket::~CDataSocket()
{
}


// CDataSocket 멤버 함수
BOOL CDataSocket::OnMessagePending()
{
	MSG Message;
	if (::PeekMessage(&Message, NULL, WM_TIMER, WM_TIMER, PM_NOREMOVE))
	{
		if (Message.wParam == 10)
		{
			::PeekMessage(&Message, NULL, WM_TIMER, WM_TIMER, PM_REMOVE);
			CancelBlockingCall();
			Close();
		}
	}
	return CSocket::OnMessagePending();
}


void CDataSocket::OnReceive(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	CSocket::OnReceive(nErrorCode);
	m_pDlg->ProcessReceive(this, nErrorCode);
}
