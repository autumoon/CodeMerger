// DllTestorDlg.h : 头文件
//

#pragma once
#include <afxcmn.h>
#include <fstream>
#include "Configure.h"		//配置文件
#include "ElapsedTime.h"	//输出耗时
#include "GetDirFiles.h"	//获取目录文件
#include "CLog.h"			//输出日志
#include "MfcStrFile.h"		//mfc字符串等
#include "ProgressInterface.h"
#include "MyEdit.h"			//支持拖拽
#include "EasySize.h"		//支持缩放

/*支持命令行方式输入输出*/
//#define CMD_INPUT
//#define CMD_OUTPUT

/*添加对话框背景*/
#define DLG_BACKGROUND

/*支持处理耗时输出*/
#define DLG_ELAPSED_TIME

/*支持处理文件或者目录，或者同时支持*/
#define ITEM_ONLY_DIR
//#define ITEM_ONLY_FILE

/*命令行输出头文件*/
#ifdef CMD_OUTPUT
#include <io.h>  
#include <fcntl.h>  
#include <iostream> //添加这3个头文件
#endif

// CDllTestorDlg 对话框
class CDllTestorDlg : public CDialogEx
{
	DECLARE_EASYSIZE
// 构造
public:
	CDllTestorDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DLLTESTOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;

	//配置文件参数
	config_s m_cfg;
	//获取配置文件路径
	_tstring GetIniPath(const TCHAR* szFileExt = _T(".ini"));

	//判断后缀是否符合条件
	bool IsProperSuffix(const _tstring& strFilePath, const std::vector<_tstring>& vSuffix);

	//添加到列表
	int AddItemToList(_tstring stItemPath);

	//处理单个文件
	int ProcessFile(const _tstring& stSrcPath, std::ofstream& out, config_s& _cfg);

	//合并一组文件到单个输出文件，返回是否成功创建输出文件
	bool MergeFilesToOutput(const std::vector<_tstring>& vFiles,
		const _tstring& stOutputFile,
		const std::string& stHeader,
		const _tstring& stDstDir,
		CProgressInterface* ppi,
		int nProgressStart,
		int nProgressTotal);
	
#ifdef CMD_OUTPUT
	int SetCommandLine()
	{
		AllocConsole();
		*stdin  = *( _fdopen(_open_osfhandle((intptr_t)::GetStdHandle(STD_INPUT_HANDLE), _O_TEXT), "r"));  
		*stdout = *( _fdopen(_open_osfhandle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT), "wt"));  
		std::ios_base::sync_with_stdio(); 

		return 0;
	}

	int ReleaseCommandLine()
	{
		FreeConsole();

		return 0;
	}
#endif

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CListCtrl m_listItems;
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonAddItems();
	afx_msg void OnBnClickedButtonClearItems();
	afx_msg void OnBnClickedButtonDelItems();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

	//主要实现
	CMyEdit m_eDstDir;
	CButton m_chkIsolate;
	afx_msg void OnBnClickedOk();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
