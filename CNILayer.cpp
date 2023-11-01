#include "pch.h"
#include "CNILayer.h"
#include <pcap.h>
#include <Packet32.h>
#pragma comment(lib, "packet.lib")
#define BPF_MAJOR_VERSION

CNILayer::CNILayer(char* pName)
    : CBaseLayer(pName), devicesList{}
{
    LoadNpcapDlls();
    PopulateDeviceList();
}

CNILayer::~CNILayer()
{
    pcap_freealldevs(pointerToDeviceList);
}

BOOL CNILayer::LoadNpcapDlls()
{
    TCHAR npcap_dir[512];
    UINT len;
    len = GetSystemDirectory(npcap_dir, 480);
    if (!len) {
        fprintf(stderr, "Error in GetSystemDirectory: %x", GetLastError());
        return FALSE;
    }
    strcat_s(npcap_dir, 512, TEXT("\\Npcap"));
    if (SetDllDirectory(npcap_dir) == 0) {
        fprintf(stderr, "Error in SetDllDirectory: %x", GetLastError());
        return FALSE;
    }
    return TRUE;
}

void CNILayer::PopulateDeviceList()
{
    char error[PCAP_ERRBUF_SIZE];
    pcap_if_t* allDevices;
    if (!pcap_findalldevs(&allDevices, error))
    {
        this->pointerToDeviceList = allDevices;
        for (pcap_if_t* currentDevice = allDevices; currentDevice != nullptr; currentDevice = currentDevice->next)
        {
            NetworkDevice device{};
            device.name = currentDevice->name;
            device.description = currentDevice->description;
            devicesList.push_back(device);
        }
    }
}

std::vector<CNILayer::NetworkDevice>* CNILayer::GetDevicesList()
{
    return &this->devicesList;
}

BOOL CNILayer::GetMacAddress(char* deviceName, CNILayer::PhysicalAddress* outAddress)
{
    PPACKET_OID_DATA oidData = (PPACKET_OID_DATA)malloc(sizeof(PACKET_OID_DATA) + 6);
    if (oidData == nullptr)
    {

        return false;
    }

    oidData->Oid = OID_802_3_PERMANENT_ADDRESS;
    oidData->Length = 6;
    ZeroMemory(oidData->Data, 6);
    if (deviceName == nullptr)
    {
        return false;
    }
    LPADAPTER adapter = PacketOpenAdapter(deviceName);
    if (adapter == nullptr)
    {
        return false;
    }

    if (!PacketRequest(adapter, false, oidData))
    {
        return false;
    }

    for (int i = 0; i < 6; i++)
    {
        ((char*)outAddress)[i] = oidData->Data[i];
    }

    PacketCloseAdapter(adapter);
    return true;
}

BOOL CNILayer::Send(unsigned char* payload, int length)
{
    if (pcap_sendpacket(_adapter, payload, length))
    {
        AfxMessageBox(_T("패킷 전송 실패"));
        return FALSE;
    }
    return TRUE;
}
void CNILayer::StartReceive(char* adapterName)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_create(adapterName, errbuf);
    if (!handle)
    {
        AfxMessageBox("Failed to open device handle");
#ifdef DEBUG
        DebugBreak();
#endif
        return;
    }
    pcap_set_timeout(handle, 0);
    pcap_set_promisc(handle, TRUE);
    //pcap_set_immediate_mode(handle, TRUE); WinPCAP    ̺귯            ȵ 
    pcap_set_buffer_size(handle, 1024 * 1024);
    pcap_set_snaplen(handle, 65535);

    pcap_activate(handle);

    _adapter = handle;

}

BOOL CNILayer::Receive(unsigned char* packet)
{
    return this->GetUpperLayer(0)->Receive(packet);
}


UINT CNILayer::ReadingThread(LPVOID pParam)
{
    pcap_pkthdr* packetHeader;
    u_char* packet;
    CNILayer* NIlayer = (CNILayer*)pParam;

    int code = 1;
    while (NIlayer->m_isWorkingThread)
    {
        code = pcap_next_ex(NIlayer->_adapter, &packetHeader, (const u_char**)&packet);
        switch (code)
        {
        case 1:
            break;
        case PCAP_ERROR:
            AfxMessageBox(pcap_geterr(NIlayer->_adapter));
#ifdef DEBUG
            DebugBreak();
#endif
        case 0:
        default:
            return false;
        }
        NIlayer->Receive(packet);
        Sleep(30);

        //Do something...
    }
    return 0;
}

void CNILayer::SetThreadloop() {
    m_isWorkingThread = true;
}