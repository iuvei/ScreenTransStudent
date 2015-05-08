#pragma once
#include <math.h>
//////////////////////////////////////////////////////////////////////////
// ZLIB��
#include "zlib.h"
#include "zconf.h"
#pragma comment(lib,"zdll.lib")
//////////////////////////////////////////////////////////////////////////
#include <afxwin.h>
#include "Comment.h"
#include <iostream>
#include "SocketCenter.h"
using namespace std;
class CScreenMonitor
{
public:
	CScreenMonitor();
	~CScreenMonitor();
	void GetDeskScreeData();

	void InitBITMAPINFO(BITMAP &bitmap, int height, int width);

	void CompressBmpData(BYTE* pBmpOriginalData);
//	void CompressBmpData(BYTE* pBmpOriginalData, UINT sizePerBlock);
//	void SendBmpHeaderInfo();
	void SendBmpData();
	void SendScreenData();
	void SetSocket(SOCKET socketMsg);
	void SetSendFlag(bool isSendFlag);
	void CleanData();
private:
//public:
	int m_bmpHeadTotalSize;
	BITMAPINFO* m_pBMPINFO;
	uLongf m_imgTotalSize;
	BYTE* m_pBmpTransData;
	CSocketCenter m_pSocketCenter;
	bool m_isSendFlag;
	SOCKET m_socketMsg;
//	SOCKET m_socketScreen;
	uLongf  m_compressBmpDataLen;
	bool m_isInitBITMAPINFO;
//	bool m_isSocketConn;
// 	// ��һ������ͼ������
// 	BYTE* m_pBmpOriDataFirst;
// 	// �ڶ�������ͼ������
// 	BYTE* m_pBmpOriDataSecond;
// 	// ÿһ���ж��ٵ�ɨ����
// 	uLongf m_heightPerBlock;
// 	// ���һ���ɨ���ߵ�����
// 	uLongf m_heightLastBlock;
// 	// ��Ļ��ȵ�λ������
// 	uLongf m_widthPerBlock;
// 	// ÿ�����������
// 	uLongf m_sizePerBlcok;
};

