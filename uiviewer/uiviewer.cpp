// dui-demo.cpp : main source file
//

#include "stdafx.h"
#include "PreviewHost.h"
#include <Shellapi.h>
#include "PreviewContainer.h"
#include "../ExtendCtrls/SCtrlsRegister.h"
#include <SouiFactory.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#pragma comment(lib,"imm32.lib")


#ifdef _DEBUG
#define SYS_NAMED_RESOURCE _T("soui-sys-resourced.dll")
#else
#define SYS_NAMED_RESOURCE _T("soui-sys-resource.dll")
#endif

class SApp : public SApplication
{
public:
	SApp(IRenderFactory *pRendFactory,HINSTANCE hInst):SApplication(pRendFactory,hInst){}
protected:
	virtual IObject * OnCreateUnknownObject(const SObjectInfo & objInfo) const
	{
		if(objInfo.mType == SWindow::GetClassType())
		{
			return CreateWindowByName(SWindow::GetClassName());
		}else
		{
			return NULL;
		}
	}

    STDMETHOD_(IBitmapS *,LoadImage)(THIS_ LPCTSTR pszType,LPCTSTR pszResName)
	{
		int nBufSize = GetRawBufferSize(pszType,pszResName);
		char *pBuf = (char*)malloc(nBufSize);
		BOOL bLoad = GetRawBuffer(pszType,pszResName,pBuf,nBufSize);
		if(bLoad && nBufSize>6)
		{
			if(_tcscmp(pszType,L"svg")!=0)
			{
				return SApplication::LoadImage(pszType,pszResName);
			}
			const unsigned char bom16[2]={0xff,0xfe};
			const unsigned char bom8[3]={0xef,0xbb,0xbf};
			SStringA strBuf;
			if(memcmp(pBuf,bom16,2)==0)
			{
				strBuf = S_CW2A(SStringW((WCHAR*)(pBuf+2),(nBufSize-2)/2),CP_UTF8);
			}else if(memcmp(pBuf,bom8,3)==0)
			{
				strBuf = SStringA(pBuf+3,nBufSize-3);
			}else
			{
				strBuf = S_CA2A(SStringA(pBuf,nBufSize),CP_ACP,CP_UTF8);
			}

            const char* svg_buffer = NULL;

			if(strBuf.Left(4)=="<svg")
			{
                svg_buffer = strBuf.c_str();
			}
            else if (strBuf.Left(5) == "<?xml")
            {
                int pos = strBuf.Find("<svg", 5);
                if (pos != -1)
                {
                    svg_buffer = strBuf.c_str() + pos;
                }
            }

            if (svg_buffer)
            {
                NSVGimage *image = nsvgParse((char*)svg_buffer,"px", 96.0f);
				IBitmapS *Ret=NULL;
				if(image)
				{
					int w = (int)image->width;
					int h = (int)image->height;

					NSVGrasterizer* rast = nsvgCreateRasterizer();

					unsigned char *img = (unsigned char*)malloc(w*h*4);
					nsvgRasterize(rast, image, 0,0,1, img, w, h, w*4);
					GETRENDERFACTORY->CreateBitmap(&Ret);
					Ret->Init(w,h,img);
					free(img);

					nsvgDeleteRasterizer(rast);
					nsvgDelete(image);

				}
				return Ret;
            }
		}
		return SResLoadFromMemory::LoadImage(pBuf,nBufSize);
	}
};

class SUiViewer
{
private:
	SComMgr m_ComMgr;
	bool m_bInitSucessed;
	SApplication * m_theApp;
	SouiFactory m_souiFac;
	HWND m_hDesignParent;

public:
	SUiViewer(HINSTANCE hInstance):m_theApp(NULL), m_bInitSucessed(false){
		
		CAutoRefPtr<SOUI::IRenderFactory> pRenderFactory;
		BOOL bLoaded = FALSE;
		//使用GDI渲染界面
		bLoaded = m_ComMgr.CreateRender_Skia((IObjRef * *)& pRenderFactory);
		SASSERT_FMT(bLoaded, _T("load interface [render] failed!"));
		//设置图像解码引擎。默认为GDIP。基本主流图片都能解码。系统自带，无需其它库
		CAutoRefPtr<SOUI::IImgDecoderFactory> pImgDecoderFactory;
		bLoaded = m_ComMgr.CreateImgDecoder((IObjRef * *)& pImgDecoderFactory);
		SASSERT_FMT(bLoaded, _T("load interface [%s] failed!"), _T("imgdecoder"));

		pRenderFactory->SetImgDecoderFactory(pImgDecoderFactory);
		m_theApp = new SApp(pRenderFactory, hInstance);	
		m_bInitSucessed = (TRUE==bLoaded);
	};

	operator bool()const
	{
		return m_bInitSucessed;
	}
	//加载系统资源
	BOOL LoadSystemRes()
	{
		BOOL bLoaded = FALSE;

		//从DLL加载系统资源
		{
			HMODULE hModSysResource = LoadLibrary(SYS_NAMED_RESOURCE);
			if (hModSysResource)
			{
				CAutoRefPtr<IResProvider> sysResProvider;
				m_souiFac.CreateResProvider(RES_PE, (IObjRef * *)& sysResProvider);
				sysResProvider->Init((WPARAM)hModSysResource, 0);
				m_theApp->LoadSystemNamedResource(sysResProvider);
				FreeLibrary(hModSysResource);
			}
			else
			{
				SASSERT(0);
			}
		}

		return bLoaded;
	}
	//加载用户资源
	BOOL LoadUserRes(LPCTSTR pszDir)
	{
		SAutoRefPtr<IResProvider>   pResProvider;
		m_souiFac.CreateResProvider(RES_FILE, (IObjRef * *)& pResProvider);
		BOOL bLoaded = pResProvider->Init((LPARAM)pszDir, 0);
		if(bLoaded)
		{
			m_theApp->AddResProvider(pResProvider);
		}
		return bLoaded;
	}
	
	void RegCustomCtrls()
	{
		SCtrlsRegister::RegisterCtrls(m_theApp);
	}

	~SUiViewer()
	{
		if (m_theApp)
		{
			delete m_theApp;
			m_theApp = NULL;
		}
	}

	int Run(SNativeWnd *pHostWnd, HWND hParent)
	{
		m_hDesignParent = hParent;
		if (hParent)
		{
			CRect rc;
			::GetClientRect(hParent,&rc);
			pHostWnd->CreateNative(L"SouiPreview",WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,0,0,0,rc.Width(),rc.Height(),m_hDesignParent,0,NULL);
		}else
		{
			pHostWnd->CreateNative(L"SouiPreview",WS_HSCROLL|WS_VSCROLL|WS_MAXIMIZEBOX|WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION|WS_THICKFRAME|WS_CLIPCHILDREN,0,0,0,500,500,GetActiveWindow(),0,NULL);
			pHostWnd->ShowWindow(SW_SHOW);
		}
		return m_theApp->Run(pHostWnd->m_hWnd);
	}
};


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int /*nCmdShow*/)
{
	HRESULT hRes = OleInitialize(NULL);
	SASSERT(SUCCEEDED(hRes));
	int nRet = 0;
	{
		SUiViewer souiEngine(hInstance);
		if (souiEngine)
		{
			souiEngine.LoadSystemRes();
			souiEngine.RegCustomCtrls();
			int nArgs = 0;
			LPWSTR pszCmds = GetCommandLineW();
			LPWSTR * args = CommandLineToArgvW(pszCmds,&nArgs);
			if(nArgs >= 3)
			{
				HWND hParent = NULL, hEditor = NULL;
				if (nArgs >= 4)
					hEditor = (HWND)_wtol(args[3]);
				if (nArgs >= 5)
					hParent = (HWND)_wtol(args[4]);
				
				SStringT resFolder = S_CW2T(args[1]);
				resFolder.Trim('"');
				souiEngine.LoadUserRes(resFolder);

				CPreviewContainer prevWnd(S_CW2T(args[2]), hEditor);
				nRet = souiEngine.Run(&prevWnd, hParent);
			}
			LocalFree(args);
		}
	}
	OleUninitialize();
	return nRet;
}
