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
    
    DWORD dwFileSize = GetFileSize(hFile, NULL);    //송신과정 1

    if (FApplayer->p_Progress) {
        FApplayer->p_Progress->SetRange(0, dwFileSize / FAPP_DATA_SIZE); //오류발생~~! 근데 실행은 잘 되는데?
    }
    //가장 처음은 파일명 전송 (receive) 송신과정 3
    FApplayer->bSEND = FALSE;
    FApplayer->m_sHeader.fapp_totlen = (unsigned long)dwFileSize;
    FApplayer->m_sHeader.fapp_type = DATA_TYPE_BEGIN;
    FApplayer->m_sHeader.fapp_seq_num = 0;
    //memset(FApplayer->m_sHeader.fapp_data, 0, copyLength + 1);
    memcpy(FApplayer->m_sHeader.fapp_data,FApplayer->filename, copyLength + 1); //파일명 못받는 오류해결
    FApplayer->bSEND = FApplayer->Send((unsigned char*)&(FApplayer->m_sHeader), 13 + copyLength);
    Sleep(30);
    FApplayer->p_Progress->SetPos(dwFileSize / FAPP_DATA_SIZE); // 송신과정 6

    if (dwFileSize <= FAPP_DATA_SIZE) { //송신과정 2
        FApplayer->m_sHeader.fapp_type = DATA_TYPE_END; //modify
        FApplayer->m_sHeader.fapp_seq_num = 1;
        ReadFile(hFile, FApplayer->m_sHeader.fapp_data, dwFileSize, &dwWrite, NULL);
        if (dwWrite != FAPP_DATA_SIZE)
            return -1;
       //real send
       FApplayer->bSEND = FALSE;
       FApplayer->m_sHeader.fapp_totlen = dwFileSize; //modify
       FApplayer->bSEND = FApplayer->Send((unsigned char*)&(FApplayer->m_sHeader), 12 + (dwWrite > FAPP_DATA_SIZE ? FAPP_DATA_SIZE : dwWrite));
       Sleep(30);
    }
    else {
        DoFragmentation_f(FApplayer, hFile, dwFileSize); //송신과정 5
    }

    CloseHandle(hFile);
    return FApplayer->bSEND;
}
BOOL CFileAppLayer::DoFragmentation_f(CFileAppLayer* FileApplayer, HANDLE hfile, DWORD Filesize) {
    DWORD dwWrite = 0, dwRead;
    DWORD total_size = Filesize;
    DWORD sent_size = 0;
    unsigned long seq = 1;
    int cnt = 0;
    int remainder = Filesize % FAPP_DATA_SIZE; // 나머지
    int quot = Filesize / FAPP_DATA_SIZE + (remainder ? 1 : 0); // 몫, 나머지가 있으면 +1
    unsigned char buffer[99999];
    ReadFile(hfile, buffer, total_size, &dwRead, NULL); // dwRead는 실제로 읽은 바이트 수

    do {
        DWORD bytesToSend = (cnt == quot - 1) ? remainder : FAPP_DATA_SIZE; // 마지막 패킷이면 나머지, 아니면 정해진 크기
        FileApplayer->m_sHeader.fapp_type = (cnt == quot - 1) ? DATA_TYPE_END : DATA_TYPE_CONT; // 마지막이면 DATA_TYPE_END, 아니면 DATA_TYPE_CONT
        FileApplayer->m_sHeader.fapp_seq_num = seq++;
        memcpy(FileApplayer->m_sHeader.fapp_data, &buffer[cnt * FAPP_DATA_SIZE], bytesToSend); // 현재 조각 복사
        FileApplayer->m_sHeader.fapp_totlen = bytesToSend; // 현재 보낼 크기 설정

        // 실제 전송
        FileApplayer->bSEND = FileApplayer->Send((unsigned char*)&(FileApplayer->m_sHeader), 12 + bytesToSend);
        Sleep(30);

        // 진행률 업데이트
        FileApplayer->p_Progress->SetPos(seq);

        sent_size += bytesToSend; // 전송된 크기 갱신
        cnt++; // 다음 패킷을 위해 카운트 증가

    } while (cnt < quot);

    return FileApplayer->bSEND;
}





BOOL CFileAppLayer::Send(unsigned char* frame, int size) {
     bSEND= FALSE;
    bSEND = ((CEthernetLayer*)(mp_UnderLayer))->Send(frame, size, FILE_TYPE);

    return bSEND;
}

BOOL CFileAppLayer::Receive(unsigned char* frame) {
    LPFILE_APP payload = (LPFILE_APP)frame;

    if (payload->fapp_type == DATA_TYPE_BEGIN) {
        CString file_name;
        file_name.Format(_T("%s"), payload->fapp_data);
        WriteFile.Open(file_name, CFile::modeCreate | CFile::modeWrite);
        WriteFile.SetLength(payload->fapp_totlen);
    }
    else {
        WriteFile.Seek((payload->fapp_seq_num - 1) * FAPP_DATA_SIZE, CFile::begin);
        WriteFile.Write(payload->fapp_data, payload->fapp_totlen);
        if (payload->fapp_type == DATA_TYPE_END) {
            WriteFile.Close();
            AfxMessageBox(_T("Success"));
        }
    }
    return TRUE;
}


