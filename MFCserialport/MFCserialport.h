
// MFCserialport.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CMFCserialportApp:
// �� Ŭ������ ������ ���ؼ��� MFCserialport.cpp�� �����Ͻʽÿ�.
//

class CMFCserialportApp : public CWinApp
{
public:
	CMFCserialportApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CMFCserialportApp theApp;