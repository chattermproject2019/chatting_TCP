#pragma once
class CTCPServer_thdDlg;

// CListenSocket 명령 대상입니다.

class CListenSocket : public CSocket
{
public:
	CListenSocket(CTCPServer_thdDlg *pDlg);
	virtual ~CListenSocket();
	CTCPServer_thdDlg *m_pDlg;
	virtual void OnAccept(int nErrorCode);
};


