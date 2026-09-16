// DllTestorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DllTestor.h"
#include "DllTestorDlg.h"
#include "afxdialogex.h"
#include <fstream>
#include "TextMergeHelper.h"
#include "InPlaceConverter.h"

/*同时支持处理文件和目录*/
#if (defined ITEM_ONLY_DIR) && (defined ITEM_ONLY_FILE)
#define ITEM_DIR_FILE
#endif

/*防止同时未定义*/
#if (!defined ITEM_ONLY_DIR) && (!defined ITEM_ONLY_FILE)
#error "One of ITEM_ONLY_DIR or ITEM_ONLY_FILE must be defined!"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CDllTestorDlg 对话框

CDllTestorDlg::CDllTestorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDllTestorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDllTestorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_ITEMS, m_listItems);
	DDX_Control(pDX, IDC_EDIT_DIR, m_eDstDir);
	DDX_Control(pDX, IDC_CHECK_ISOLATE, m_chkIsolate);
}

_tstring CDllTestorDlg::GetIniPath(const TCHAR* szFileExt /*= _T(".ini")*/)
{
	TCHAR chpath[MAX_PATH];
	GetModuleFileName(NULL, chpath, sizeof(chpath));

	_tstring strModulePath = CMfcStrFile::CString2string(chpath);
	_tstring strIniPath = CStdStr::ReplaceSuffix(strModulePath, szFileExt);

	return strIniPath;
}

bool CDllTestorDlg::IsProperSuffix(const _tstring& strFilePath, const std::vector<_tstring>& vSuffix)
{
	//包含所有文件
	bool bGetAll = false;
	size_t nFSNum = vSuffix.size();

	for (size_t i = 0; i < nFSNum; ++i)
	{
		_tstring strCurSpe = vSuffix[i];
		if (strCurSpe == _T("*.*") || strCurSpe == _T("*"))
		{
			return true;
		}
	}

	//包含指定的suffix
	_tstring strCurSuffix = CStdStr::ToUpperLower(CStdStr::GetSuffixOfFile(strFilePath, false));
	for (size_t i = 0; i < nFSNum; ++i)
	{
		_tstring strCurSpe = CStdStr::ToUpperLower(CStdStr::GetSuffixOfFile(vSuffix[i], false));
		if (strCurSuffix == strCurSpe)
		{
			return true;
		}
	}

	return false;
}

int CDllTestorDlg::AddItemToList(_tstring stItemPath)
{
	std::vector<_tstring> vItems;
	int nItemNum = m_listItems.GetItemCount();
	for (int i = 0; i < nItemNum; ++i)
	{
		CString strCurItem = m_listItems.GetItemText(i, 0);
		_tstring stCurItem = CMfcStrFile::CString2string(strCurItem);
		vItems.push_back(stCurItem);
	}

	if (CStdTpl::VectorContains(vItems, stItemPath))
	{
		return nItemNum;
	}

#ifdef ITEM_ONLY_FILE
	if (CStdFile::IfAccessFile(stItemPath) && IsProperSuffix(stItemPath, m_cfg.vSuffixs))
	{
		int nPos = m_listItems.GetItemCount();
		m_listItems.InsertItem(nPos, stItemPath.c_str());
	}
#endif // ITEM_ONLY_FILE

#ifdef ITEM_ONLY_DIR
	if(PathIsDirectory(stItemPath.c_str()))
	{
		int nPos = m_listItems.GetItemCount();
		m_listItems.InsertItem(nPos, stItemPath.c_str());
	}
#endif // ITEM_ONLY_DIR

	return m_listItems.GetItemCount();
}

int CDllTestorDlg::ProcessFile(const _tstring& stSrcPath, std::ofstream& out, config_s& _cfg)
{
	//打开文件（只读、共享读取）
	HANDLE hFile = ::CreateFileW(stSrcPath.c_str(),
		GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		const std::wstring wOut = L"# 文件：" + stSrcPath + L"\r\n\r\n> [读取失败：无法打开文件]\r\n\r\n----------------------------------------\r\n\r\n";
		const std::string sOut = TextMerge::WideToUtf8(wOut);
		out.write(sOut.c_str(), (std::streamsize)sOut.size());
		return -1;
	}

	//获取文件大小
	LARGE_INTEGER liSize;
	if (!::GetFileSizeEx(hFile, &liSize))
	{
		::CloseHandle(hFile);
		const std::wstring wOut = L"# 文件：" + stSrcPath + L"\r\n\r\n> [读取失败：获取文件大小失败]\r\n\r\n----------------------------------------\r\n\r\n";
		const std::string sOut = TextMerge::WideToUtf8(wOut);
		out.write(sOut.c_str(), (std::streamsize)sOut.size());
		return -1;
	}

	const __int64 nFileSize = liSize.QuadPart;
	if (nFileSize > (__int64)_cfg.nMaxFileSizeMB * 1024 * 1024)
	{
		::CloseHandle(hFile);
		const std::wstring wOut = L"# 文件：" + stSrcPath
			+ L"\r\n\r\n> [文件过大，已跳过：" + std::to_wstring(nFileSize)
			+ L" 字节，限制 " + std::to_wstring((__int64)_cfg.nMaxFileSizeMB) + L" MB]\r\n\r\n"
			+ L"----------------------------------------\r\n\r\n";
		const std::string sOut = TextMerge::WideToUtf8(wOut);
		out.write(sOut.c_str(), (std::streamsize)sOut.size());
		return -1;
	}

	//一次性读取完整内容（循环读到fileSize或出错）
	std::string strData;
	strData.resize((size_t)nFileSize);

	DWORD dwTotalRead = 0;
	while (dwTotalRead < (DWORD)nFileSize)
	{
		DWORD dwRead = 0;
		if (!::ReadFile(hFile, &strData[dwTotalRead], (DWORD)nFileSize - dwTotalRead, &dwRead, NULL) || dwRead == 0)
		{
			break;
		}
		dwTotalRead += dwRead;
	}
	::CloseHandle(hFile);

	if (dwTotalRead < (DWORD)nFileSize)
	{
		const std::wstring wOut = L"# 文件：" + stSrcPath + L"\r\n\r\n> [读取失败：读取内容不完整]\r\n\r\n----------------------------------------\r\n\r\n";
		const std::string sOut = TextMerge::WideToUtf8(wOut);
		out.write(sOut.c_str(), (std::streamsize)sOut.size());
		return -1;
	}

	//检测编码并转换为UTF-8
	std::wstring wEncName;
	std::string strUtf8 = TextMerge::ConvertToUtf8(strData, wEncName);

	//写入段落
	const std::wstring wHeader = L"# 文件：" + stSrcPath
		+ L"\r\n\r\n> 编码：" + wEncName
		+ L"，大小：" + std::to_wstring(nFileSize) + L" 字节\r\n\r\n"
		+ L"----------------------------------------\r\n\r\n";
	const std::string sHeader = TextMerge::WideToUtf8(wHeader);

	const char szSeparator[] = "\r\n\r\n----------------------------------------\r\n\r\n";

	out.write(sHeader.c_str(), (std::streamsize)sHeader.size());
	out.write(strUtf8.c_str(), (std::streamsize)strUtf8.size());
	out.write(szSeparator, (std::streamsize)(sizeof(szSeparator) - 1));

	return 0;
}

BEGIN_EASYSIZE_MAP(CDllTestorDlg)
	EASYSIZE(IDOK,ES_BORDER,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,0)
	EASYSIZE(IDCANCEL,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
	EASYSIZE(IDC_STATIC_DSTFILE,ES_BORDER,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,0)
	EASYSIZE(IDC_BUTTON_OPEN,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
	EASYSIZE(IDC_EDIT_FILE,ES_BORDER,ES_KEEPSIZE,IDC_BUTTON_OPEN,ES_BORDER,0)
	EASYSIZE(IDC_STATIC_DSTDIR,ES_BORDER,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,0)
	EASYSIZE(IDC_BUTTON_BROWSE,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
	EASYSIZE(IDC_EDIT_DIR,ES_BORDER,ES_KEEPSIZE,IDC_BUTTON_BROWSE,ES_BORDER,0)
	EASYSIZE(IDC_LIST_ITEMS,ES_BORDER,ES_BORDER,ES_BORDER,ES_BORDER,0)
	EASYSIZE(IDC_BUTTON_ADD_ITEMS,ES_KEEPSIZE,ES_BORDER,ES_BORDER,ES_KEEPSIZE,0)
	EASYSIZE(IDC_BUTTON_DEL_ITEMS,ES_KEEPSIZE,ES_BORDER,ES_BORDER,ES_KEEPSIZE,0)
	EASYSIZE(IDC_BUTTON_CLEAR_ITEMS,ES_KEEPSIZE,ES_BORDER,ES_BORDER,ES_KEEPSIZE,0)
END_EASYSIZE_MAP

BEGIN_MESSAGE_MAP(CDllTestorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CDllTestorDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CDllTestorDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ITEMS, &CDllTestorDlg::OnBnClickedButtonAddItems)
	ON_BN_CLICKED(IDC_BUTTON_DEL_ITEMS, &CDllTestorDlg::OnBnClickedButtonDelItems)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_ITEMS, &CDllTestorDlg::OnBnClickedButtonClearItems)
	ON_BN_CLICKED(IDOK, &CDllTestorDlg::OnBnClickedOk)
	ON_WM_DROPFILES()
	ON_WM_ERASEBKGND()
	ON_WM_HELPINFO()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CDllTestorDlg 消息处理程序

BOOL CDllTestorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
#ifdef ITEM_DIR_FILE
	_tstring strItemName = _T("项目");
#else

#ifdef ITEM_ONLY_DIR
	_tstring strItemName = _T("目录");
#else
	_tstring strItemName = _T("文件");
#endif

#endif

	//窗口名称
	_tstring strWindowName = _T("代码合并展示工具");
	SetWindowText(strWindowName.c_str());

	//设置AfxMessageBox的标题
	//First free the string allocated by MFC at CWinApp startup.
	//The string is allocated before InitInstance is called.
	free((void*)theApp.m_pszAppName);
	//Change the name of the application file.
	//The CWinApp destructor will free the memory.
	theApp.m_pszAppName=_tcsdup(strWindowName.c_str());

	//按钮名称
	SetDlgItemText(IDC_BUTTON_ADD_ITEMS, (_T("添加") + strItemName).c_str());
	SetDlgItemText(IDC_BUTTON_DEL_ITEMS, (_T("删除") + strItemName).c_str());

	_tstring strAddItemTips = _T("待处理的") + strItemName +  _T("(可拖拽添加):");

	CRect rListRect;
	m_listItems.GetClientRect(rListRect);
	m_listItems.InsertColumn(0, strAddItemTips.c_str(), LVCFMT_LEFT, rListRect.Width());

	//读取配置信息
	_tstring strIniPath = GetIniPath();
	if (CStdFile::IfAccessFile(strIniPath))
	{
		ReadIniFile(strIniPath, m_cfg);
	}
	else
	{
		WriteIniFile(strIniPath, m_cfg);
	}

	//恢复“每个目录独立处理”勾选状态
	m_chkIsolate.SetCheck(m_cfg.bIsolatePerDir ? BST_CHECKED : BST_UNCHECKED);

	//只能拖拽单个目录
	m_eDstDir.SetFlag(EDIT_DIR_JUDGE | EDIT_SIG_JUDGE);

	if (m_cfg.bRemPath)
	{
		for (size_t i = 0; i < m_cfg.vItemPaths.size(); ++i)
		{
			AddItemToList(m_cfg.vItemPaths[i]);
		}

		if (m_cfg.vDstPaths.size() > 0)
		{
			SetDlgItemText(IDC_EDIT_DIR, VectorToString(m_cfg.vDstPaths).c_str());
		}
	}

	//如果是将文件夹拖拽到应用程序图标或者快捷方式
#ifdef CMD_INPUT
	int argc = 0;
#ifdef _UNICODE
	LPWSTR *argv=::CommandLineToArgvW(::GetCommandLine(),&argc);
#else
	LPWSTR *argv=::CommandLineToArgvW(CStdStr::s2ws(::GetCommandLine()).c_str(),&argc);
#endif

	CStringArray arrCmds;
	//去掉第一个程序自身的参数
	for (int i = 1; i < argc; ++i)
	{
#ifdef _UNICODE
		arrCmds.Add(argv[i]);
#else
		std::string sArc = CStdStr::ws2s(argv[i]);
		arrCmds.Add(sArc.c_str());
#endif
	}
	LocalFree(argv);

	if (arrCmds.GetCount() >= 1)
	{
		int nItems = (int)arrCmds.GetCount();
		CString strFiles, strDirs;
		int nItemCount = 0;
		for (int i = 0; i < nItems; ++i)
		{
			CString strCurItem = arrCmds[i];
			_tstring sItem = CMfcStrFile::CString2string(strCurItem);
			if(PathFileExists(sItem.c_str()))
			{
				nItemCount = AddItemToList(sItem);
			}
		}
		CString strTmp[2];
		strTmp[0].LoadString(IDS_PROCESS_NOW);
		strTmp[1].LoadString(IDS_TIPS);
		//文件和文件夹同时成立时才会执行，可根据需要修改
		if (nItemCount && MessageBox(strTmp[0], strTmp[1], MB_YESNO) == IDYES)
		{
			OnBnClickedOk();
		}
	}
#endif // CMD_INPUT

#ifdef CMD_OUTPUT
	SetCommandLine();
#endif // CMD_OUTPUT

	INIT_EASYSIZE;

	if (m_cfg.nWindowWidth < 640 || m_cfg.nWindowHeight < 480)
	{
		m_cfg.nWindowWidth = 640;
		m_cfg.nWindowHeight = 480;
	}
	SetWindowPos(&wndBottom,0,0,m_cfg.nWindowWidth,m_cfg.nWindowHeight, SWP_SHOWWINDOW);
	CenterWindow();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDllTestorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
// 来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
// 这将由框架自动完成。

void CDllTestorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDllTestorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDllTestorDlg::OnBnClickedButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strFilePath = CMfcStrFile::SaveSuffixFile(_T(".txt"));

	if (strFilePath.GetLength() > 0)
	{
		SetDlgItemText(IDC_EDIT_FILE, strFilePath);
		CString strTmp[2];
		strTmp[0].LoadString(IDS_PROCESS_NOW);
		strTmp[1].LoadString(IDS_TIPS);
		if (MessageBox(strTmp[0], strTmp[1], MB_YESNO) == IDYES)
		{
			OnBnClickedOk();
		}
	}
}

void CDllTestorDlg::OnBnClickedButtonBrowse()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strDirPath = CMfcStrFile::BrowseDir(true);

	if (strDirPath.GetLength() > 0)
	{
		SetDlgItemText(IDC_EDIT_DIR, strDirPath);
	}
}

void CDllTestorDlg::OnBnClickedButtonAddItems()
{
	// TODO: 在此添加控件通知处理程序代码
	int nPos = m_listItems.GetItemCount();
	CStringArray arrItems;
#if (defined ITEM_DIR_FILE) || (defined ITEM_ONLY_FILE)

	size_t nCount = CMfcStrFile::OpenMultiFiles(arrItems, 0);

#else
	CString strDir = CMfcStrFile::BrowseDir();
	arrItems.Add(strDir);
	size_t nCount = arrItems.GetCount();
#endif // 

	if (nCount > 0)
	{
		for (size_t i = 0; i < nCount; ++i)
		{
			_tstring stItem = CMfcStrFile::CString2string(arrItems[i]);
			AddItemToList(stItem);
		}
	}
}

void CDllTestorDlg::OnBnClickedButtonDelItems()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION nPos = m_listItems.GetFirstSelectedItemPosition();
	while (nPos)
	{
		int iSelItem = m_listItems.GetNextSelectedItem(nPos);
		m_listItems.DeleteItem(iSelItem);       
		nPos = m_listItems.GetFirstSelectedItemPosition();
	}
}


void CDllTestorDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	// 定义一个缓冲区来存放读取的文件名信息
	TCHAR* szItemPath = nullptr;
	const int nMaxPathLength = 2048;
	CStdTpl::NewSafely(szItemPath, nMaxPathLength, true);
	// 通过设置iFiles参数为0xFFFFFFFF,可以取得当前拖动的文件数量，
	// 当设置为0xFFFFFFFF,函数间忽略后面两个参数。
	UINT nNum = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	// 通过循环依次取得拖动文件的File Name信息，并把它添加到ListBox中
	for (UINT i = 0; i < nNum; ++i)
	{
		DragQueryFile(hDropInfo, i, szItemPath, nMaxPathLength);
		if (PathFileExists(szItemPath) == TRUE)
		{
			AddItemToList(CMfcStrFile::CString2string(szItemPath));
		}
	}

	// 结束此次拖拽操作，并释放分配的资源
	CStdTpl::DelPointerSafely(szItemPath, true);
	DragFinish(hDropInfo);

	CDialogEx::OnDropFiles(hDropInfo);
}

BOOL CDllTestorDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CDialogEx::OnEraseBkgnd(pDC);

#ifdef DLG_BACKGROUND
	HBITMAP hBitmap = nullptr;

	//读取同名bmp文件
	_tstring stBgPath = GetIniPath(_T(".bmp"));
	if (CStdFile::IfAccessFile(stBgPath))
	{
		hBitmap = (HBITMAP)LoadImage(AfxGetInstanceHandle(), stBgPath.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	else
	{
		hBitmap = ::LoadBitmap(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
	}

	//获取位图尺寸
	BITMAP bitmap;
	GetObject(hBitmap, sizeof (BITMAP), &bitmap);

	//获取对话框尺寸
	CRect rect;
	GetClientRect(&rect);

	//创建DC
	HDC m_hBkDC= ::CreateCompatibleDC(pDC->m_hDC);

	//绘图并清理
	if(hBitmap && m_hBkDC)
	{
		::SelectObject(m_hBkDC,hBitmap);
		::StretchBlt(pDC->m_hDC, 0, 0, rect.Width(), rect.Height(),m_hBkDC,0,0,bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
		::DeleteObject(hBitmap);
		::DeleteDC(m_hBkDC);
	}
#endif // DLG_BACKGROUND

	//这个很重要
	return TRUE;
}

BOOL CDllTestorDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (MessageBox(_T("如果遇到任何问题需要帮助，\n请通过邮箱autumoon@vip.qq.com联系我!\n\n立即复制“autumoon@vip.qq.com”到剪贴板吗？"), 
		_T("需要帮助"), MB_YESNO) == IDYES)
	{
		//已经复制到剪贴板
		if(OpenClipboard())   
		{   
			_tstring source(_T("autumoon@vip.qq.com"));
			HGLOBAL clipbuffer;   
			char* buffer;   
			EmptyClipboard();   
			clipbuffer = GlobalAlloc(GMEM_DDESHARE,   source.size() + 1);   
			buffer = (char*)GlobalLock(clipbuffer);   
#ifdef _UNICODE
			std::string sSrc = CStdStr::ws2s(source);
#else
			std::string sSrc(source);
#endif // _UNICODE
			strcpy_s(buffer, source.size() + 1, sSrc.c_str());
			GlobalUnlock(clipbuffer);   
			SetClipboardData(CF_TEXT,clipbuffer);   
			CloseClipboard();
			MessageBox(_T("“autumoon@vip.qq.com”\n  已经成功复制到剪贴板！"), _T("地址复制完成!"), MB_ICONINFORMATION);
		}   
	}

	return TRUE;
}

void CDllTestorDlg::OnBnClickedButtonClearItems()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_listItems.GetItemCount())
	{
		m_listItems.DeleteAllItems();
	}
}

bool CDllTestorDlg::MergeFilesToOutput(
	const std::vector<_tstring>& vFiles,
	const _tstring& stOutputFile,
	const std::string& stHeader,
	const _tstring& stDstDir,
	CProgressInterface* ppi,
	int nProgressStart,
	int nProgressTotal)
{
	std::ofstream out(stOutputFile.c_str(), std::ios::binary | std::ios::trunc);
	if (!out.is_open())
	{
		return false;
	}

	out.write(stHeader.c_str(), (std::streamsize)stHeader.size());

	_tstring stReportName = stDstDir + _T("InPlaceConvertReport.md");
	_tstring stCodeAll    = stDstDir + _T("codeAll.md");

	for (size_t i = 0; i < vFiles.size(); ++i)
	{
		const _tstring& stFile = vFiles[i];

		//排除输出文件自身
		if (_wcsicmp(stFile.c_str(), stOutputFile.c_str()) == 0)
		{
			continue;
		}
		//排除原地转码报告文件
		if (_wcsicmp(stFile.c_str(), stReportName.c_str()) == 0)
		{
			continue;
		}
		//排除 codeAll.md（独立模式下不需要被合并进来）
		if (_wcsicmp(stFile.c_str(), stCodeAll.c_str()) == 0)
		{
			continue;
		}

		ProcessFile(stFile, out, m_cfg);
		ppi->SetProgressValue(nProgressStart + (int)i + 1, nProgressTotal);
	}

	out.close();
	return true;
}

void CDllTestorDlg::OnBnClickedOk()
{
	// TODO:  在此添加命令处理程序代码
	CString strDstDir;
	GetDlgItemText(IDC_EDIT_DIR, strDstDir);

	if (!strDstDir.GetLength())
	{
		strDstDir = CMfcStrFile::BrowseDir(true);
		if (strDstDir.GetLength() > 0)
		{
			SetDlgItemText(IDC_EDIT_DIR, strDstDir);
		}
		else
		{
			return;
		}
	}

	std::vector<_tstring> vItems;
	int nItemCount = m_listItems.GetItemCount();
	for (int i = 0; i < nItemCount; ++i)
	{
		CString strCurItem = m_listItems.GetItemText(i, 0);
		_tstring stCurItem = CMfcStrFile::CString2string(strCurItem);
		vItems.push_back(stCurItem);
	}

	_tstring stDstDir = CMfcStrFile::CString2string(strDstDir);
	if (vItems.size() == 0 || !CStdDir::IfAccessDir(stDstDir) && !CStdDir::CreateDir(stDstDir))
	{
		return;
	}

	//保存配置文件
	m_cfg.vItemPaths = vItems;
	m_cfg.bIsolatePerDir = (m_chkIsolate.GetCheck() == BST_CHECKED);
	m_cfg.vDstPaths.clear();
	m_cfg.vDstPaths.push_back(stDstDir);
	WriteIniFile(GetIniPath(), m_cfg);

	//开始显示进度
	CTaskBarProgress tbp(m_hWnd);
	CProgressInterface* ppi = &tbp;
	ppi->Start();
	CElapsedTime et;

	//记录日志
	//CLOG::Out(_T("%s"), _T("start task!"));
	//记录耗时
	et.Begin();
	// ---- 原地转码（高风险，默认关闭） ----
	if (m_cfg.bInPlaceConvert)
	{
		CString strWarn;
		strWarn += _T("警告：你开启了【原地转码】功能。\r\n\r\n");
		strWarn += _T("此操作将把所有匹配后缀的源文件直接转换为 UTF-8");
		strWarn += (m_cfg.bInPlaceUtf8Bom ? _T("（带 BOM）") : _T("（不带 BOM）"));
		strWarn += _T("。\r\n\r\n");
		strWarn += _T("每个文件都会自动备份为 xxx.bak；若 .bak 已存在，");
		strWarn += _T("会依次尝试 .bak.1 ~ .bak.99。\r\n\r\n");
		strWarn += _T("强烈建议：\r\n");
		strWarn += _T("  - 操作前使用 Git / SVN 提交一次，或整体复制一份项目副本\r\n");
		strWarn += _T("  - 首次运行请把 InPlaceDryRun 设为 1，先预览报告\r\n\r\n");
		strWarn += _T("是否继续？");

		if (AfxMessageBox(strWarn, MB_YESNO | MB_ICONWARNING) != IDYES)
		{
			return;
		}

		_tstring stReportPath = CStdStr::AddSlashIfNeeded(stDstDir)
			+ _T("InPlaceConvertReport.md");
		InPlace::Result r = InPlace::DoInPlaceConvert(vItems, m_cfg, stReportPath);

		// 生成报告后，自动用系统默认程序打开
		HINSTANCE hInst = ::ShellExecuteW(
			m_hWnd,
			L"open",
			stReportPath.c_str(),
			NULL,
			NULL,
			SW_SHOWNORMAL);

		// 打开失败：静默处理，仅追加到完成提示里
		bool bReportOpened = ((INT_PTR)hInst > 32);

		CString strDone;
		strDone.Format(
			_T("原地转码完成。\r\n\r\n成功：%d\r\n跳过：%d\r\n失败：%d\r\n\r\n报告：\r\n%s%s"),
			r.nConverted, r.nSkipped, r.nFailed, stReportPath.c_str(),
			bReportOpened ? _T("") : _T("\r\n（注意：报告未能自动打开，请手动查看）"));
		AfxMessageBox(strDone, MB_OK | MB_ICONINFORMATION);
	}
	// ---- 原地转码结束 ----
	stDstDir = CStdStr::AddSlashIfNeeded(stDstDir);

	int nGlobalTotal = 0;	// 最终进度分母

	if (!m_cfg.bIsolatePerDir)
	{
		// ---------- 合并模式（原有行为） ----------
		_tstring stOutputFile = stDstDir + _T("codeAll.md");

		std::vector<_tstring> vAllFiles;
		for (size_t i = 0; i < vItems.size(); ++i)
		{
			if (!PathIsDirectory(vItems[i].c_str()))
			{
				continue;
			}

			std::vector<_tstring> vFound;
			getFiles(vItems[i], vFound, m_cfg.vSuffixs, true);

			for (size_t k = 0; k < vFound.size(); ++k)
			{
				//按名称排除
				if (TextMerge::IsPathExcluded(vFound[k],
						m_cfg.vExcludeDirNames, m_cfg.vExcludeFileNames))
				{
					continue;
				}

				vAllFiles.push_back(vFound[k]);
			}
		}

		nGlobalTotal = (int)vAllFiles.size();
		if (nGlobalTotal == 0)
		{
			nGlobalTotal = 1;
		}

		std::string header = TextMerge::WideToUtf8(L"# 文件清单\r\n\r\n");
		MergeFilesToOutput(vAllFiles, stOutputFile, header, stDstDir, ppi, 0, nGlobalTotal);
	}
	else
	{
		// ---------- 独立模式（每个目录一份） ----------
		struct Job
		{
			_tstring stDir;			//目录名（不含路径），用于标题
			_tstring stOutputFile;	//输出 .md 完整路径
			std::vector<_tstring> vFiles;
		};

		std::vector<Job> jobs;

		for (size_t i = 0; i < vItems.size(); ++i)
		{
			if (!PathIsDirectory(vItems[i].c_str()))
			{
				continue;
			}

			Job job;
			job.stDir = CStdStr::GetNameOfDir(vItems[i]);	//取目录名
			if (job.stDir.empty())
			{
				job.stDir = _T("unnamed");
			}

			//计算输出文件名：<目录名>.md；若已存在则 <目录名>_2.md ...
			_tstring stBaseName = job.stDir;
			_tstring stCandidate;
			for (int idx = 1; idx < 1000; ++idx)
			{
				if (idx == 1)
				{
					stCandidate = stDstDir + stBaseName + _T(".md");
				}
				else
				{
					stCandidate = stDstDir + stBaseName + _T("_")
						+ std::to_wstring(idx) + _T(".md");
				}

				DWORD dwAttr = ::GetFileAttributesW(stCandidate.c_str());
				if (dwAttr == INVALID_FILE_ATTRIBUTES)
				{
					break;
				}
			}
			job.stOutputFile = stCandidate;

			//递归收集该目录下的文件
			getFiles(vItems[i], job.vFiles, m_cfg.vSuffixs, true);

			//按名称排除，保证 nGlobalTotal 统计与实际处理数一致
			std::vector<_tstring> vFiltered;
			vFiltered.reserve(job.vFiles.size());
			for (size_t k = 0; k < job.vFiles.size(); ++k)
			{
				if (TextMerge::IsPathExcluded(job.vFiles[k],
						m_cfg.vExcludeDirNames, m_cfg.vExcludeFileNames))
				{
					continue;
				}
				vFiltered.push_back(job.vFiles[k]);
			}
			job.vFiles.swap(vFiltered);

			jobs.push_back(job);
		}

		//计算全局总文件数，用于进度条
		for (size_t j = 0; j < jobs.size(); ++j)
		{
			nGlobalTotal += (int)jobs[j].vFiles.size();
		}
		if (nGlobalTotal == 0)
		{
			nGlobalTotal = 1;
		}

		int nDone = 0;
		for (size_t j = 0; j < jobs.size(); ++j)
		{
			_tstring stHeader = L"# 目录：" + jobs[j].stDir + L"\r\n\r\n";
			std::string header = TextMerge::WideToUtf8(stHeader);

			bool ok = MergeFilesToOutput(jobs[j].vFiles,
				jobs[j].stOutputFile,
				header,
				stDstDir,
				ppi,
				nDone,
				nGlobalTotal);
			if (!ok)
			{
				CString strErr;
				strErr.Format(_T("无法创建输出文件：\r\n%s"),
					jobs[j].stOutputFile.c_str());
				AfxMessageBox(strErr, MB_OK | MB_ICONWARNING);
				//不中断，继续处理下一个目录
			}

			nDone += (int)jobs[j].vFiles.size();
		}
	}

	/*** 主程序结束 ***/

	//结束耗时
	int nMin = 0, nSecond = 0, nMilliSecond = 0;
	et.End(nMin, nSecond, nMilliSecond);
	//结束日志
	//CLOG::Out(_T("%s"),_T("end task!"));
	//CLOG::Out(_T("This task costs %d min %d second %d millisecond!"), nMin, nSecond, nMilliSecond);
	//CLOG::End();

	//结束进度显示
	ppi->End();
	FlashWindow(TRUE);

#ifdef DLG_ELAPSED_TIME
	CString strTips;
	strTips.Format(_T("本次耗时 %d分%d秒%d毫秒!"), nMin, nSecond, nMilliSecond);
	AfxMessageBox(strTips);
#else
	AfxMessageBox(IDS_PROCESS_OVER);
#endif // DLG_ELAPSED_TIME
}

void CDllTestorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	UPDATE_EASYSIZE;

	//改变ListCtrl表的宽度
	CRect rcList;
	CWnd* pWnd = GetDlgItem(IDC_LIST_ITEMS);
	if (pWnd)
	{
		pWnd->GetWindowRect(&rcList);
		m_listItems.SetColumnWidth(0, max(10, rcList.Width() - 6));
	}

	if (nType != SIZE_MINIMIZED)
	{
		CRect rcDlg;
		GetClientRect(rcDlg);
		InvalidateRect(rcDlg);
		m_cfg.nWindowWidth = cx;
		m_cfg.nWindowHeight = cy;
	}
}
