
// TCPServer_thd.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CTCPServer_thdApp:
// �� Ŭ������ ������ ���ؼ��� TCPServer_thd.cpp�� �����Ͻʽÿ�.
//

class CTCPServer_thdApp : public CWinApp
{
public:
	CTCPServer_thdApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CTCPServer_thdApp theApp;