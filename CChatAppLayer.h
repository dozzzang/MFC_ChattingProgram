#pragma once
// CChatAppLayer.h: interface for the CChatAppLayer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_)
#define AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseLayer.h"
#include "pch.h"
#include <vector>
class CChatAppLayer
	: public CBaseLayer
{
private:
	inline void		ResetHeader();
	CObject* mp_Dlg;
	std::vector<unsigned char> m_assembledPayload; //totlen만큼 동적할당
	int m_receivedLength = 0;
	int m_TotalLength = 0;

public:
	BOOL			Receive(unsigned char* ppayload);
	BOOL			Send(unsigned char* ppayload, int nlength);
	BOOL            DoFragmentation_c(unsigned char* ppayload, int nlength);

	CChatAppLayer(char* pName);
	virtual ~CChatAppLayer();

	typedef struct _CHAT_APP_HEADER {
		unsigned short	totlen; // total length of the data
		unsigned char	type; 
		unsigned char   unused; // Unused
		unsigned char	data[APP_DATA_SIZE]; // application data

	} CHAT_APP_HEADER, * PCHAT_APP_HEADER;

	// Used for indicating message type.
	enum : unsigned short {
		CHAT_MESSAGE_NORMAL,
		CHAT_MESSAGE_BROADCAST
	};

protected:
	CHAT_APP_HEADER		m_sHeader;
};

#endif // !defined(AFX_CHATAPPLAYER_H__E78615DE_0F23_41A9_B814_34E2B3697EF2__INCLUDED_)










