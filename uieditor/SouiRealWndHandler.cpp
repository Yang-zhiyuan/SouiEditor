﻿#include "StdAfx.h"
#include "SouiRealWndHandler.h"
#include "ScintillaWnd.h"
#include "DesignWnd.h"

namespace SOUI
{
	CSouiRealWndHandler::CSouiRealWndHandler(void)
	{
	}

	CSouiRealWndHandler::~CSouiRealWndHandler(void)
	{
	}

	HWND CSouiRealWndHandler::OnRealWndCreate(IWindow *pRealWnd1)
	{
		SRealWnd *pRealWnd = sobj_cast<SRealWnd>(pRealWnd1);
		if (pRealWnd->GetRealWndParam().m_strClassName == _T("scintilla"))
		{
			CScintillaWnd *pWnd = new CScintillaWnd;
			BOOL bOK = pWnd->Create(pRealWnd->GetRealWndParam().m_strWindowName, WS_CHILD, CRect(), pRealWnd->GetContainer()->GetHostHwnd(), pRealWnd->GetID(), SApplication::getSingleton().GetModule());
			if (!bOK)
			{
				SASSERT(FALSE);
				delete pWnd;
				return 0;
			}
			pRealWnd->SetUserData((ULONG_PTR)pWnd);
			return pWnd->m_hWnd;
		}
		else if (pRealWnd->GetRealWndParam().m_strClassName == _T("designer_wnd"))
		{
			CDesignWnd *pWnd = new CDesignWnd;
			HWND hWnd = pWnd->CreateEx(pRealWnd->GetContainer()->GetHostHwnd(),WS_CHILD|WS_CLIPCHILDREN,0,0,0,0,0);
			if (!hWnd)
			{
				SASSERT(FALSE);
				delete pWnd;
				return 0;
			}
			pWnd->InitFromXml(&pRealWnd->GetRealWndParam().m_xmlParams.root().child(L"params"));
			pRealWnd->SetUserData((ULONG_PTR)pWnd);
			return hWnd;
		}else
		{
			return 0;
		}
	}

	void CSouiRealWndHandler::OnRealWndDestroy(IWindow *pRealWnd1)
	{
		SRealWnd *pRealWnd = sobj_cast<SRealWnd>(pRealWnd1);
		if (pRealWnd->GetRealWndParam().m_strClassName == _T("scintilla"))
		{
			CScintillaWnd *pWnd = (CScintillaWnd *)pRealWnd->GetUserData();
			if (pWnd)
			{
				pWnd->DestroyWindow();
				delete pWnd;
			}
		}
	}

	//不处理，返回FALSE
	BOOL CSouiRealWndHandler::OnRealWndSize(IWindow *pRealWnd)
	{
		return FALSE;
	}

	//不处理，返回FALSE
	BOOL CSouiRealWndHandler::OnRealWndInit(IWindow *pRealWnd)
	{
		return FALSE;
	}

	BOOL CSouiRealWndHandler::OnRealWndPosition(THIS_ IWindow * pRealWnd, RECT rcWnd)
	{
		return FALSE;
	}

}
