#include "stdafx.h"
#include "ControlCenter.h"
#include "StuInfo.h"



CControlCenter::CControlCenter(STUINFODATA stuInfo)
	: m_port(0)
	, m_socketMsg(INVALID_SOCKET)
//	, m_isCreateScreenMonitor(true)
	, m_pScreenMonitor(NULL)
	, m_stuInfo(stuInfo)
	, m_multicastDlg(NULL)
{
//	m_pSocketCenter = new CSocketCenter();
}


CControlCenter::~CControlCenter()
{
	if (m_socketMsg != INVALID_SOCKET)
	{
		closesocket(m_socketMsg);
		m_socketMsg = INVALID_SOCKET;
	}
	if (m_pScreenMonitor != NULL)
	{
		delete m_pScreenMonitor;
		m_pScreenMonitor = NULL;
	}

	if (m_socketMulticast != INVALID_SOCKET)
	{
		closesocket(m_socketMulticast);
		m_socketMulticast = NULL;
	}

	// m_multicastDlg �Ѿ���CMulticast �����ͷ�
}
/*
	����IP ��ַ�Ͷ˿ڵ���Ϣ
	Input:
	IP--IP��ַ
	port--�˿ں�
*/
void CControlCenter::SetConnectInfo(char* IP, int port)
{
	strcpy_s(m_IP, IP);
	m_port = port;
}

/*
	���ӽ�ʦ��SOCKET
*/
void CControlCenter::ConnectClient()
{
	while (true)
	{
//		CSocketCenter socketCenter;
		m_socketMsg = m_socketCenter.InitSocket(m_IP, m_port);
		if (m_socketMsg == SOCKET_ERROR)
		{
			Sleep(3000);
//			printf("Sleep\n");
			continue;
		}
		else
		{
			break;
		}
	}
	RecvRequest();
}

void CControlCenter::RecvRequest()
{
	m_socketMulticast = m_socketCenter.InitMulticastSocket(MULTICAST_TRANS_PORT,
		MULTICAST_IP);

	MSGTYPE msgType;
//	CSocketCenter socketCenter;
	while (true)
	{
		memset(&msgType, 0, sizeof(MSGTYPE));
		// ���Ͻ������Կͻ��˵�����
		int result = m_socketCenter.RecvDataTCP(m_socketMsg,
			(char*)&msgType, sizeof(MSGTYPE));
//		closesocket(m_socketMsg);
		if (result == 0)
		{
			AfxMessageBox(_T("result = 0"));
			break;
		}
		this->ExecuteRequest(msgType);
	}
}



void CControlCenter::ExecuteRequest(MSGTYPE msgType)
{
//  	// ֪ͨ�ͻ��������Ѿ����յ��ˣ����Կ�ʼ����������
//  	m_pSocketCenter->SendDataToClient(m_socketConn, (char*)&msgType, sizeof(MSGTYPE));
	switch (msgType.msgID)
	{
	// ���ͱ�����ص�ϵͳ����Ϣ
	case SYSINFO:
	{
//		printf("GetSystemInfo\n");
		CStuInfo StuInfo;
		StuInfo.SendStuInfo(m_socketMsg, m_stuInfo);
	}
		break;
	// ���������ͼ���ݵ���Ϣͷ����
	case SCREENINFO:
//		printf("��ͼͷ��Ϣ\n");
		
	// ���������ͼ������
	case SCREENDATA:
//	{
		if (m_pScreenMonitor != NULL)
		{
			delete m_pScreenMonitor;
			m_pScreenMonitor = NULL;
		}
		m_pScreenMonitor = new CScreenMonitor();
		m_pScreenMonitor->SetSocket(m_socketMsg);
		m_pScreenMonitor->SetSendFlag(true);
		m_hScreenDlg = (::CreateThread(0, 0, OnSendScreenData,
			(LPVOID)m_pScreenMonitor, 0, NULL));

// 		m_screenMonitor.SetSocket(m_socketMsg);
// 		m_screenMonitor.SetSendFlag(true);
// 		CloseHandle(::CreateThread(0, 0, SendScreenData, 
// 			(LPVOID)&m_screenMonitor, 0, NULL));

//	}
		break;
	case SCREENTRANEND:
 	{
		m_pScreenMonitor->SetSendFlag(false);
		DWORD exitCode = 0;
		::GetExitCodeThread(m_hScreenDlg, &exitCode);
		while (exitCode == STILL_ACTIVE)
		{
			Sleep(10);
			::GetExitCodeThread(m_hScreenDlg, &exitCode);
		}
		CloseHandle(m_hScreenDlg);
		delete m_pScreenMonitor;
		m_pScreenMonitor = NULL;
// 		delete m_pScreenMonitor;;
// 		m_pScreenMonitor = NULL;
	}
		break;
	case MULTICASTBEGIN:
		// m_multicastDlg �ڴ���CMulticast �����ͷ���
		m_multicastDlg = new CMulticastDlg();
		m_multicastDlg->SetMulticastSocket(m_socketMulticast);
		m_multicastDlg->SetIsStop(false);
		CloseHandle(::CreateThread(0, 0, OnShowDlg,(LPVOID)m_multicastDlg, 0, NULL));
		m_hMulticast = ::CreateThread(0, 0, OnMulticastDlg,
			(LPVOID)m_multicastDlg, 0, NULL);
		break;
	case ENDMULTICAST:
		EndMulticast();
		break;
	default:
	{
//		printf("UnKnow Command\n");
		return;
	}
	}
}


void CControlCenter::EndMulticast()
{
	m_multicastDlg->SetIsStop(true);
	DWORD exitCode = 0;
	m_multicastDlg->CloseDlg();

	GetExitCodeThread(m_hMulticast, &exitCode);
	while (STILL_ACTIVE == exitCode)
	{
		Sleep(100);
		GetExitCodeThread(m_hMulticast, &exitCode);
	}
//	TerminateThread(m_hMulticast, exitCode);

	CloseHandle(m_hMulticast);
	delete m_multicastDlg;
	m_multicastDlg = NULL;
}

DWORD WINAPI CControlCenter::OnSendScreenData(LPVOID self)
{
	CScreenMonitor* screenThread = (CScreenMonitor*)self;
	screenThread->SendScreenData();
	return 0;
}

DWORD WINAPI CControlCenter::OnShowDlg(LPVOID self)
{
	CMulticastDlg* multicastDlg = (CMulticastDlg*)self;
 	multicastDlg->DoModal();
// 	multicastDlg->Create(IDD_MULTICAST);
// 	multicastDlg->ShowWindow(SW_NORMAL);
	return 0;
}

DWORD WINAPI CControlCenter::OnMulticastDlg(LPVOID self)
{
	CMulticastDlg* multicastDlg = (CMulticastDlg*)self;
	multicastDlg->SetScreenData();
//	multicastDlg->DoModal();
	return 0;
}

void CControlCenter::MulticastDlg()
{
// 	m_socketMulticast = m_pSocketCenter->InitMulticastSocket(MULTICAST_TRANS_PORT,
// 		MULTICAST_IP);
// 
// 	m_multicastDlg = new CMulticastDlg(m_addr, m_socketMulticast/*, m_socketMsg*/);
// 	m_multicastDlg->DoModal();
// // 	m_multicastDlg->Create(IDD_MULTICAST);
// // 	m_multicastDlg->ShowWindow(SW_NORMAL);
// 
// 	CloseHandle(::CreateThread(0, 0, OnSetScreenData,
// 		(LPVOID)m_multicastDlg, 0, NULL));

//	m_multicastDlg->SetScreenData();
}

