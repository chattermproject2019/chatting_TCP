#pragma once
class CTCPServer_thdDlg;

// CDataSocket ��� ����Դϴ�.

class CDataSocket : public CSocket
{
public:
	CDataSocket(CTCPServer_thdDlg *pDlg);
	virtual ~CDataSocket();
	CTCPServer_thdDlg *m_pDlg;
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);
};


