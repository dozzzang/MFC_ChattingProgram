#pragma once
#if !defined(AFX_FILELAYER_H__D67222B3_1B00_4C77_84A4_CEF6D572E181__INCLUDED_)
#define AFX_FILELAYER_H__D67222B3_1B00_4C77_84A4_CEF6D572E181__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "BaseLayer.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <Windows.h>

class CFileAppLayer 
    : public CBaseLayer
{   
    CString FilePath;
    CFile WriteFile;
    CProgressCtrl* p_Progress;

private:
    inline void		ResetHeader();
    BOOL     bSEND;
    CString GetFilepath();
    

public:
    CFileAppLayer(char* pName);
    virtual ~CFileAppLayer();
    unsigned char filename[512];

    typedef struct _FILE_APP
    {
        unsigned long fapp_totlen; // 총 길이
        unsigned short fapp_type; // 데이터 타입
        unsigned char fapp_msg_type; // 메시지 종류
        unsigned char unused; // 사용 안함
        unsigned long fapp_seq_num; // fragmentation 순서
        unsigned char fapp_data[FAPP_DATA_SIZE];
    } FILE_APP, * LPFILE_APP;


    BOOL Send(unsigned char* frame, int size);
    BOOL Receive(unsigned char* frame);
    void SetFilepath(CString Path);
    static UINT     F_Sendthr(LPVOID Filepath);
    void            SetProgressCtrl(CProgressCtrl* p);
    static BOOL DoFragmentation_f(CFileAppLayer* FileApplayer,HANDLE hfile,DWORD Filesize);
    static UINT		FileThread(LPVOID pParam);
    // Used for indicating message type.
    enum : unsigned short {
        CHAT_MESSAGE_NORMAL,
        CHAT_MESSAGE_BROADCAST
    };
protected:
    FILE_APP		m_sHeader;
    enum {
        DATA_TYPE_BEGIN = 0x00,
        DATA_TYPE_CONT = 0x01,
        DATA_TYPE_END = 0x02
    };

};
#endif // !defined(AFX_FILELAYER_H__D67222B3_1B00_4C77_84A4_CEF6D572E181__INCLUDED_)