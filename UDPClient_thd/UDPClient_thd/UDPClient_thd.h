
// UDPClient_thd.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CUDPClient_thdApp:
// �� Ŭ������ ������ ���ؼ��� UDPClient_thd.cpp�� �����Ͻʽÿ�.
//

class CUDPClient_thdApp : public CWinApp
{
public:
	CUDPClient_thdApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CUDPClient_thdApp theApp;