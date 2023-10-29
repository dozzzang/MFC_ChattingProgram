// CChatAppLayer.cpp: implementation of the CChatAppLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch.h"
#include "CChatAppLayer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CChatAppLayer::CChatAppLayer(char* pName)
    : CBaseLayer(pName),
    mp_Dlg(NULL)
{
    ResetHeader();
}

CChatAppLayer::~CChatAppLayer()
{

}

void CChatAppLayer::ResetHeader()
{
    m_sHeader.totlen = 0x0000;
    m_sHeader.type = 0x00;
    m_sHeader.unused = 0x00;
    memset(m_sHeader.data, 0, APP_DATA_SIZE);
}

BOOL CChatAppLayer::Send(unsigned char* ppayload, int nlength)
{
    if (nlength <= 1496) {
        unsigned short length = min(APP_DATA_SIZE, nlength);
        m_sHeader.totlen = length;

        BOOL bSuccess = FALSE;

        memcpy(m_sHeader.data, ppayload, length);
        bSuccess = ((CEthernetLayer*)(mp_UnderLayer))->Send((unsigned char*)&m_sHeader, length + APP_HEADER_SIZE, CHAT_TYPE);
        return bSuccess;
    }
    else 
        return DoFragmentation_c(ppayload, nlength);
}
BOOL CChatAppLayer::DoFragmentation_c(unsigned char* ppayload, int nlength)
/* Returns whether it is framented or not
    @param1 - User Message
    @param2 - sizeof(ppayload)
    @return - BOOL
*/
{
    int remaining_length = nlength;
    int sent_length = 0;
    int total_fragments = (nlength / 1496) + 1;

    m_sHeader.totlen = (unsigned short)nlength; // 수신 대비

    while ((int)m_sHeader.type < total_fragments) {
        int current_send_length = (remaining_length > 1496) ? 1496 : remaining_length;
        
        // 첫 번째 프래그먼트가 아닌 경우, 현재 프래그먼트의 길이를 설정
        if (m_sHeader.type > 0)
            m_sHeader.totlen = (unsigned short)current_send_length;

        memcpy(m_sHeader.data, ppayload + sent_length, current_send_length);

        BOOL bSuccess = ((CEthernetLayer*)(mp_UnderLayer))->Send((unsigned char*)&m_sHeader, current_send_length + APP_HEADER_SIZE, CHAT_TYPE);
        if (!bSuccess) {
            return FALSE; // 실패 시 바로 FALSE를 반환
        }

        sent_length += current_send_length;
        remaining_length -= current_send_length;

        m_sHeader.type++;
    }
    return TRUE;
}


BOOL CChatAppLayer::Receive(unsigned char* ppayload)
{
    PCHAT_APP_HEADER app_hdr = (PCHAT_APP_HEADER)ppayload;

    if (app_hdr->type == 0x00) {
        m_TotalLength = app_hdr->totlen;
        m_assembledPayload.clear();
        m_assembledPayload.reserve(m_TotalLength); // 필요한 메모리를 미리 동적 할당
    }
    //  데이터를 버퍼에 추가
    m_assembledPayload.insert(m_assembledPayload.end(), app_hdr->data, app_hdr->data + app_hdr->totlen);
    m_receivedLength += app_hdr->totlen;

    // 모든 조각이 수신된지 확인 후 최종 수신 처리
    if (m_receivedLength == m_TotalLength) {
        CString Msg;

        // 여기서 데이터를 처리하고 상위 레이어로 전송
        if (app_hdr->type == CHAT_MESSAGE_BROADCAST)
            Msg.Format(_T("[BROADCAST] %s"), &m_assembledPayload[0]);
        else
            Msg.Format(_T("[Recipient] %s"), &m_assembledPayload[0]);

        mp_aUpperLayer[0]->Receive((unsigned char*)Msg.GetBuffer(0));

        m_receivedLength = 0;
        m_TotalLength = 0;
    }
    return TRUE;
}


