
// UDPServer_thd.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CUDPServer_thdApp:
// �� Ŭ������ ������ ���ؼ��� UDPServer_thd.cpp�� �����Ͻʽÿ�.
//

class CUDPServer_thdApp : public CWinApp
{
public:
	CUDPServer_thdApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CUDPServer_thdApp theApp;