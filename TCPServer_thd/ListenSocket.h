#pragma once
class CTCPServer_thdDlg;

// CListenSocket ��� ����Դϴ�.

class CListenSocket : public CSocket
{
public:
	CListenSocket(CTCPServer_thdDlg *pDlg);
	virtual ~CListenSocket();
	CTCPServer_thdDlg *m_pDlg;
	virtual void OnAccept(int nErrorCode);
};


