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
UINT CFileAppLayer::F_Sendthr(LPVOID Fileobj) {    
    /*Thread Begin
      @param Fileobj
      @return 1 (for successful quit)*/
    CFileAppLayer* FApplayer = (CFileAppLayer*)Fileobj;
    HANDLE hFile;
    DWORD dwWrite = 0, dwRead;

    const TCHAR* filePart = PathFindFileName(FApplayer->FilePath);
    int copyLength = min(strlen(filePart), sizeof(FApplayer->filename) - 1);
    // 멀티바이트 이므로 ANSI 환경
    strncpy_s((char*)FApplayer->filename, sizeof(FApplayer->filename),filePart, copyLength);
    FApplayer->filename[copyLength] = '\0';

    hFile = CreateFile(TEXT(FApplayer->FilePath), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return 1;
    
    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (FApplayer->p_Progress) {
        FApplayer->p_Progress->SetRange(0, dwFileSize / FAPP_DATA_SIZE); //오류발생~~! 근데 실행은 잘 되는데?
    }
    //가장 처음은 파일명 전송 (receive)
    FApplayer->bSEND = FALSE;
    FApplayer->m_sHeader.fapp_totlen = (unsigned long)dwFileSize;
    FApplayer->m_sHeader.fapp_type = DATA_TYPE_BEGIN;
    FApplayer->m_sHeader.fapp_seq_num = 0;
    FApplayer->bSEND = FApplayer->Send((unsigned char*)&(FApplayer->filename), 12 + (dwWrite > FAPP_DATA_SIZE ? FAPP_DATA_SIZE : dwWrite));
    FApplayer->p_Progress->SetPos(dwFileSize / FAPP_DATA_SIZE); // 송신 과정 6

    if (dwFileSize <= FAPP_DATA_SIZE) {
        FApplayer->m_sHeader.fapp_type = DATA_TYPE_CONT;
        FApplayer->m_sHeader.fapp_seq_num = 1;
        ReadFile(hFile, FApplayer->m_sHeader.fapp_data, dwFileSize, &dwWrite, NULL);
        if (dwWrite != FAPP_DATA_SIZE)
            return -1;
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
    unsigned long seq = 1; 
    unsigned char buffer[FAPP_DATA_SIZE];  

    while (sent_size < total_size) {

        DWORD dwToRead = min(FAPP_DATA_SIZE, total_size - sent_size);

            if (ReadFile(hfile, buffer, dwToRead, &dwWrite, NULL) && dwWrite > 0) {

                if (sent_size + dwWrite == Filesize)
                    FileApplayer->m_sHeader.fapp_type = DATA_TYPE_END;      //set 0x03
                else if(sent_size == 0)
                    FileApplayer->m_sHeader.fapp_type = DATA_TYPE_CONT;     //set 0x02

                FileApplayer->m_sHeader.fapp_seq_num = seq++;               //set seq_num
                memcpy(FileApplayer->m_sHeader.fapp_data, buffer, dwWrite); //set data
                //real send
                FileApplayer->bSEND = FALSE;
                FileApplayer->bSEND = FileApplayer->Send((unsigned char*)&(FileApplayer->m_sHeader), 12 + (dwWrite > FAPP_DATA_SIZE ? FAPP_DATA_SIZE : dwWrite));
                //continue to work
                sent_size += dwWrite;
                FileApplayer->p_Progress->SetPos(seq);  //송신 과정 6
            }
        }
    return FileApplayer->bSEND;
}




BOOL CFileAppLayer::Send(unsigned char* frame, int size) {
     bSEND= FALSE;
    bSEND = ((CEthernetLayer*)(mp_UnderLayer))->Send((unsigned char*)&m_sHeader, size, FILE_TYPE);

    return bSEND;
}

BOOL CFileAppLayer::Receive(unsigned char* frame) {
        LPFILE_APP payload = (LPFILE_APP)frame;

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

