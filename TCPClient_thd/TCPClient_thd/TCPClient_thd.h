
// TCPClient_thd.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CTCPClient_thdApp:
// �� Ŭ������ ������ ���ؼ��� TCPClient_thd.cpp�� �����Ͻʽÿ�.
//

class CTCPClient_thdApp : public CWinApp
{
public:
	CTCPClient_thdApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CTCPClient_thdApp theApp;