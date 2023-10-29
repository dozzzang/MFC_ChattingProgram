// CFileAppLayer.cpp: implementation of the CFileAppLayer class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "pch.h"
#include "CFileAppLayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


CFileAppLayer::CFileAppLayer(char* pName)
    : CBaseLayer(pName)
{
    bSEND = TRUE;
    ResetHeader();
    FilePath = "";
}

CFileAppLayer::~CFileAppLayer()
{

}

void CFileAppLayer::ResetHeader()
{
    m_sHeader.fapp_totlen = 0x00000000;
    m_sHeader.fapp_type = 0x00;
    m_sHeader.fapp_msg_type = 0x00;
    m_sHeader.unused = 0x00;
    m_sHeader.fapp_seq_num = 0x00;

    memset(m_sHeader.fapp_data, 0, FAPP_DATA_SIZE);
}
CString CFileAppLayer::GetFilepath() {
    return FilePath;
}

void CFileAppLayer::SetFilepath(CString Path) {
    FilePath = Path;
}
void CFileAppLayer::SetProgressCtrl(CProgressCtrl* p) {
    p_Progress = p;
}
UINT CFileAppLayer::F_Send(LPVOID Fileobj) {
    /*Perform file preprocessing before being passed to the layer
      @param Fileobj
      @return 1 (for successful quit)*/
    CFileAppLayer* FApplayer = (CFileAppLayer*)Fileobj;
    HANDLE hFile;
    DWORD dwWrite = 0, dwRead;

    hFile = CreateFile(TEXT(FApplayer->FilePath), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return 1;
    
    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (FApplayer->p_Progress) {
        FApplayer->p_Progress->SetRange(0, dwFileSize);
    }

    if (dwFileSize <= FAPP_DATA_SIZE) {
        FApplayer->m_sHeader.fapp_totlen = (unsigned long)dwFileSize;
        FApplayer->m_sHeader.fapp_type = DATA_TYPE_BEGIN;
        FApplayer->m_sHeader.fapp_seq_num = 0;
        ReadFile(hFile, FApplayer->m_sHeader.fapp_data, dwFileSize, &dwWrite, NULL);
        if (dwWrite != FAPP_DATA_SIZE)
            return -1;
        FApplayer->p_Progress->SetPos(dwFileSize);
       //real send
       FApplayer->bSEND = FALSE;
       FApplayer->bSEND = FApplayer->Send((unsigned char*)&(FApplayer->m_sHeader), 12 + (dwWrite > FAPP_DATA_SIZE ? FAPP_DATA_SIZE : dwWrite));
    }
    else {
        DoFragmentation_f(FApplayer, hFile, dwFileSize);
    }

    CloseHandle(hFile);
    return FApplayer->bSEND;
}
BOOL CFileAppLayer::DoFragmentation_f(CFileAppLayer* FileApplayer,HANDLE hfile, DWORD Filesize) {   //Limited to 0x00 0x01 0x02 unlike DoFragmentation_c
    DWORD dwWrite = 0, dwRead;
    DWORD total_size = Filesize;
    DWORD sent_size = 0;
    unsigned long seq = 0; 
    unsigned char buffer[FAPP_DATA_SIZE];

    while (sent_size < total_size) {

        DWORD dwToRead = min(FAPP_DATA_SIZE, total_size - sent_size);

            if (ReadFile(hfile, buffer, dwToRead, &dwWrite, NULL) && dwWrite > 0) {
                FileApplayer->m_sHeader.fapp_totlen = (unsigned long)dwWrite;

                if (sent_size == 0)
                    FileApplayer->m_sHeader.fapp_type = DATA_TYPE_BEGIN;    //set 0x00
                else if (sent_size + dwWrite == Filesize)
                    FileApplayer->m_sHeader.fapp_type = DATA_TYPE_END;      //set 0x01
                else
                    FileApplayer->m_sHeader.fapp_type = DATA_TYPE_CONT;     //set 0x02

                FileApplayer->m_sHeader.fapp_seq_num = seq++;               //set seq_num
                memcpy(FileApplayer->m_sHeader.fapp_data, buffer, dwWrite); //set data
                //real send
                FileApplayer->bSEND = FALSE;
                FileApplayer->bSEND = FileApplayer->Send((unsigned char*)&(FileApplayer->m_sHeader), 12 + (dwWrite > FAPP_DATA_SIZE ? FAPP_DATA_SIZE : dwWrite));
                //continue to work
                sent_size += dwWrite;
                FileApplayer->p_Progress->SetPos(sent_size);
            }
        }
    return FileApplayer->bSEND;
}




BOOL CFileAppLayer::Send(unsigned char* frame, int size) {
     bSEND= FALSE;
    bSEND = ((CEthernetLayer*)(mp_UnderLayer))->Send((unsigned char*)&m_sHeader, size + 12, FILE_TYPE);

    return bSEND;
}

BOOL CFileAppLayer::Receive(unsigned char* ppayload) {
        LPFILE_APP payload = (LPFILE_APP)ppayload;

        static HANDLE hFile = INVALID_HANDLE_VALUE;

        if (payload->fapp_type == DATA_TYPE_BEGIN) {
            CString file_name;
            file_name.Format(_T("%s"), payload->fapp_data);
            hFile = CreateFile(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                // 파일을 열 수 없다면 에러 메시지를 출력하고 함수를 종료한다.
                AfxMessageBox(_T("File can't be opend"));
                return FALSE;
            }
            //파일 크기 설정
            //LARGE_INTEGER liSize;
            //liSize.QuadPart = payload->fapp_totlen;
            //::SetFilePointerEx(hFile, liSize, NULL, FILE_BEGIN);
            //::SetEndOfFile(hFile);
            //liSize.QuadPart = 0;
            //::SetFilePointerEx(hFile, liSize, NULL, FILE_BEGIN);
        }

            else if (payload->fapp_type == DATA_TYPE_CONT) {
                LARGE_INTEGER liPos;
                liPos.QuadPart = payload->fapp_seq_num * FAPP_DATA_SIZE;
                SetFilePointerEx(hFile, liPos, NULL, FILE_BEGIN);
                DWORD dwWritten;
                ::WriteFile(hFile, payload->fapp_data, payload->fapp_totlen, &dwWritten, NULL);
            }
            else {
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
                AfxMessageBox(_T("Success!"));
            }

            return TRUE;
        }
