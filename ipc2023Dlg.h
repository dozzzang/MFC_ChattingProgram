
// ipc2023Dlg.h: 헤더 파일
//

#pragma once

#include "LayerManager.h"	// Added by ClassView
#include "CChatAppLayer.h"	// Added by ClassView
#include "CEthernetLayer.h"	// Added by ClassView
#include "CNILayer.h"
#include "CFileAppLayer.h"
// Cipc2023Dlg 대화 상자
class Cipc2023Dlg : public CDialogEx, public CBaseLayer
{
	// 생성입니다.
public:
	Cipc2023Dlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.



	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IPC2023_DIALOG };
#endif

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


	// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//	UINT m_unDstAddr;
	//	UINT unSrcAddr;
	//	CString m_stMessage;
	//	CListBox m_ListChat;

	afx_msg void OnTimer(UINT_PTR nIDEvent);

public:
	BOOL			Receive(unsigned char* ppayload);
	inline void		SendData();

private:
	CLayerManager	m_LayerMgr;
	int				m_nAckReady;

	enum {
		IPC_INITIALIZING,
		IPC_READYTOSEND,
		IPC_WAITFORACK,
		IPC_ERROR,
		IPC_BROADCASTMODE,
		IPC_UNICASTMODE,
		IPC_ADDR_SET,
		IPC_ADDR_RESET
	};

	void			SetDlgState(int state);
	inline void		EndofProcess();
	inline void     SetRegstryMessage();
	LRESULT         OnRegSendMsg(WPARAM wParam, LPARAM lParam);
	LRESULT         OnRegAckMsg(WPARAM wParam, LPARAM lParam);
	BOOL			m_bSendReady;

	// Object App
	CChatAppLayer* m_ChatApp;
	CEthernetLayer* m_Ethernet;
	CNILayer* m_Link;
	CFileAppLayer* m_File;
	// Implementation
	UINT			m_wParam;
	DWORD			m_lParam;
public:
	afx_msg void OnBnClickedButtonAddr();
	afx_msg void OnBnClickedButtonSend();
	CString m_unSrcAddr;
	CString m_unDstAddr;
	CString m_stMessage;
	CListBox m_ListChat;
	CString	m_Filepath;
	CProgressCtrl m_Progress;//progress bar
	afx_msg void OnBnClickedCheckToall();
	// The combobox containing available device list
	CComboBox deviceComboBox;
	
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButtonSend2();
	afx_msg void OnEnChangeEditSrc();
	afx_msg void OnEnChangeEditDst();
	afx_msg void OnClickedJpg();
	afx_msg void OnClickedTxt();
	afx_msg void OnClickedButtonSendfile();
	afx_msg void OnEnChangeEditFilepath();
	afx_msg void OnBnClickedButtonAddfile();	
};