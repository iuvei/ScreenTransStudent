#include "stdafx.h"
#include "ScreenMonitor.h"
#include <math.h>
//////////////////////////////////////////////////////////////////////////
// zlib�� ��Ҫʹ�����е�ѹ��ͼƬ����
#include "zlib.h"
#include "zconf.h"
#pragma comment(lib,"zdll.lib")
//////////////////////////////////////////////////////////////////////////
#include <afxwin.h>
#include <io.h>
#include <fcntl.h>

/*
//������ĺ����ӵ����ʼ���ĵط���Ȼ����Ϳ���ʹ��printf������
void InitConsoleWindow()
{
	int nCrt = 0;
	FILE* fp;
	AllocConsole();
	nCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	fp = _fdopen(nCrt, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);
}
*/


CScreenMonitor::CScreenMonitor()
	: m_pBmpTransData(NULL)
	, m_pBMPINFO(NULL)
	, m_isSendFlag(true)
	, m_isInitBITMAPINFO(true)
//	, m_socketScreen(INVALID_SOCKET)
//	, m_isSocketConn(true)
// 	, m_pBmpOriDataFirst(NULL)
// 	, m_pBmpOriDataSecond(NULL)
// 	, m_heightPerBlock(0)
// 	, m_heightLastBlock(0)
// 	, m_widthPerBlock(0)
// 	, m_sizePerBlcok(0)
{
}


CScreenMonitor::~CScreenMonitor()
{
	CleanData();
// 	if (m_socketMsg != INVALID_SOCKET)
// 	{
// 		closesocket(m_socketMsg);
// 		m_socketMsg = INVALID_SOCKET;
// 	}
}

void CScreenMonitor::CleanData()
{
	if (m_pBMPINFO != NULL)
	{
		LocalFree(m_pBMPINFO);
		m_pBMPINFO = NULL;
	}

	if (m_pBmpTransData != NULL)
	{
		delete[] m_pBmpTransData;
		m_pBmpTransData = NULL;
	}

// 	if (m_socketScreen != INVALID_SOCKET)
// 	{
// 		closesocket(m_socketScreen);
// 		m_socketScreen = NULL;
// 	}

}
void CScreenMonitor::SetSocket(SOCKET socketMsg)
{
	m_socketMsg = socketMsg;
// 	m_socketScreen = socketScreen;
}


/*
	�������Ľ�ͼ���ݲ����е���ѹ������
*/
void CScreenMonitor::GetDeskScreeData()
{
	CDC* pDeskDC = CWnd::GetDesktopWindow()->GetDC(); //��ȡ���滭������
	int width = GetSystemMetrics(SM_CXSCREEN); //��ȡ��Ļ�Ŀ��
	int height = GetSystemMetrics(SM_CYSCREEN); //��ȡ��Ļ�ĸ߶�
	CDC memDC; //����һ���ڴ滭��
	memDC.CreateCompatibleDC(pDeskDC); //����һ�����ݵĻ���
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(pDeskDC, width, height); //��������λͼ
	memDC.SelectObject(&bmp); //ѡ��λͼ����
	BITMAP bitmap;
	bmp.GetBitmap(&bitmap);

	memDC.BitBlt(0, 0, width, height, pDeskDC, 0, 0, SRCCOPY);
	m_imgTotalSize = bitmap.bmWidthBytes * bitmap.bmHeight;

//  	if (true == m_isInitBITMAPINFO)
//  	{
		// ��ʼ����ͼͼ��ͷ��Ϣ
		InitBITMAPINFO(bitmap, height, width);
//  		m_isInitBITMAPINFO = false;
//  	}
	//��ȡ��ǰ��꼰��λ��
	HCURSOR hCursor = GetCursor();
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	//��ȡ����ͼ������
	ICONINFO IconInfo;
	if (GetIconInfo(hCursor, &IconInfo))
	{
		ptCursor.x -= ((int)IconInfo.xHotspot);
		ptCursor.y -= ((int)IconInfo.yHotspot);
		if (IconInfo.hbmMask != NULL)
			DeleteObject(IconInfo.hbmMask);
		if (IconInfo.hbmColor != NULL)
			DeleteObject(IconInfo.hbmColor);
	}
	//�ڼ����豸�������ϻ����ù��
	DrawIconEx(
		memDC.m_hDC,         // handle to device context 
		ptCursor.x, ptCursor.y,
		hCursor,         // handle to icon to draw 
		0, 0,          // width of the icon 
		0,           // index of frame in animated cursor 
		NULL,          // handle to background brush 
		DI_NORMAL | DI_COMPAT      // icon-drawing flags 
		);
	// �����ͼ��ԭʼ��������
	BYTE* pBmpOriginalData = new BYTE[m_imgTotalSize];
	// ��ͼ������ݿ�����pBmpOriginalData ��
	if (::GetDIBits(memDC.m_hDC, bmp, 0, bitmap.bmHeight,
				pBmpOriginalData, m_pBMPINFO, DIB_RGB_COLORS) == 0)
	{
		AfxMessageBox(_T("GetDIBits Error"));
		delete[] pBmpOriginalData;
		pBmpOriginalData = NULL;
		LocalFree(m_pBMPINFO);
		m_pBMPINFO = NULL;
		return;
	}

	// ѹ����ͼ����
	CompressBmpData(pBmpOriginalData);

	delete[] pBmpOriginalData;
	pBmpOriginalData = NULL;
	pDeskDC->DeleteDC();
	DeleteDC(memDC);
	DeleteObject(bmp);
}

/*
	���ͽ�ͼ���ݵ��ͻ���
*/
void CScreenMonitor::SendBmpData()
{
	BMPDATA bmpData;
	memset(&bmpData, 0, sizeof(BMPDATA));
	// ����ͼ������
	bmpData.infoType = 1;
	int count = int(ceil(double(m_compressBmpDataLen) / SCREEN_TRANS_SIZE));

	memcpy(&bmpData.bmpHeadInfo, m_pBMPINFO, m_bmpHeadTotalSize);
	bmpData.bmpCompressSize = m_compressBmpDataLen;


	bmpData.isShow = false;
	UINT beginPos;
	for (int i = 0; i < count; i++)
	{
		// ֪ͨ�ͻ��˿��Կ�ʼ������Ļ������
		m_pSocketCenter.SendReadyInfo(m_socketMsg, SCREENDATA);


		memset(bmpData.transData, 0, SCREEN_TRANS_SIZE);
		beginPos = i * SCREEN_TRANS_SIZE;
		bmpData.beginPos = beginPos;
		if (i == count - 1) // ���һ�η�������
		{
			bmpData.isShow = true;
			bmpData.infoType = 2;
			uLongf last = m_compressBmpDataLen - beginPos;
			memcpy_s(bmpData.transData, last,
				m_pBmpTransData + beginPos, last);
		}
		else
		{
			memcpy_s(bmpData.transData, SCREEN_TRANS_SIZE,
				m_pBmpTransData + beginPos, SCREEN_TRANS_SIZE);
		}
		//��������
		m_pSocketCenter.SendDataTCP(m_socketMsg, (char*)&bmpData, sizeof(BMPDATA));
	}
}

/*
	���ͽ�ͼ���ݵ��ͻ��˵��߳��ܷ�ֹͣ�ı�ʾλ
	input:
		isSendFlag--true�����̣߳�falseֹͣ����
*/
void CScreenMonitor::SetSendFlag(bool isSendFlag)
{
//	m_isInitBITMAPINFO = true;
	m_isSendFlag = isSendFlag;
}

/*
	���Ϸ������ݵ��ͻ��˵��̵߳����庯��
*/
void CScreenMonitor::SendScreenData()
{
	while (true == m_isSendFlag)
	{
		this->GetDeskScreeData();
		SendBmpData();
		Sleep(300);
	}
//	delete this;
	CleanData();
	
}

/*
	ѹ����ͼ����
	input:
		pBmpOriginalData--ͼ���������Ϣ
*/
void CScreenMonitor::CompressBmpData(BYTE* pBmpOriginalData)
{
	BYTE* pCompressData = NULL;
	m_compressBmpDataLen = 0;
	// ��Ҫһ���㹻��Ŀռ�
	m_compressBmpDataLen = (uLongf)((m_imgTotalSize + 12)*(100.1 / 100)) + 1;

	pCompressData = new BYTE[m_compressBmpDataLen];
	// �����ݽ���ѹ�������浽pCompressData ��
	int err = compress(pCompressData, &m_compressBmpDataLen, pBmpOriginalData, m_imgTotalSize);

	if (err != Z_OK) {
// 		InitConsoleWindow();
// 		printf("compess error: %d", err);
		exit(1);
	}

// 	InitConsoleWindow();
// 	printf("\r\norignal size: %d, compressed size : %d\r\n", 
// 			m_imgTotalSize, m_compressBmpDataLen);
// 	cout << "orignal size: " << m_imgTotalSize
// 		<< " , compressed size : " << m_compressBmpDataLen << '\n';
	if (m_pBmpTransData != NULL)
	{
		delete[] m_pBmpTransData;
		m_pBmpTransData = NULL;
	}
	// ��ѹ��������ݱ��浽m_pBmpTransData ��
	m_pBmpTransData = new BYTE[m_compressBmpDataLen];
	memcpy(m_pBmpTransData, pCompressData, m_compressBmpDataLen);
	delete[] pCompressData;
	pCompressData = NULL;
}

/*
	��ʼ����ͼ����Ϣͷ�ṹ�� BITMAPINFO
	input:
		bitmap--��ͼ�Ĵ�С����Ϣ
		height--����ĸ�
		width--����Ŀ�
*/
void CScreenMonitor::InitBITMAPINFO(BITMAP &bitmap, int height, int width)
{
//	m_imgTotalSize = bitmap.bmWidthBytes * bitmap.bmHeight;
	// ÿһ���ɨ���ߵ�����
// 	m_heightPerBlock = height / BLOCKNUM;
// 	// ���һ���ɨ���ߵ�����
// 	m_heightLastBlock = height - m_heightPerBlock * (BLOCKNUM - 1);
// 	m_widthPerBlock = width;
// 	m_sizePerBlcok = m_heightPerBlock * m_widthPerBlock;
// 	InitConsoleWindow();
// 	printf("\nm_heightPerBlock = %d, m_heightLastBlock = %d, height = %d\n",
// 		m_heightPerBlock, m_heightLastBlock, height);
	double paletteSize = 0; //��¼��ɫ���С
	if (bitmap.bmBitsPixel < 16) //�ж��Ƿ�Ϊ���ɫλͼ
	{
		//paletteSize = pow(2.0, (double)bitmap.bmBitsPixel*sizeof(RGBQUAD));
		paletteSize = (1 << bitmap.bmBitsPixel)*sizeof(RGBQUAD);
	}
	m_bmpHeadTotalSize = (int)paletteSize + sizeof(BITMAPINFO);

	m_pBMPINFO = (BITMAPINFO*)LocalAlloc(LPTR, m_bmpHeadTotalSize);
	m_pBMPINFO->bmiHeader.biBitCount = bitmap.bmBitsPixel;
	m_pBMPINFO->bmiHeader.biClrImportant = 0;
	m_pBMPINFO->bmiHeader.biCompression = 0;
	m_pBMPINFO->bmiHeader.biHeight = height;
	m_pBMPINFO->bmiHeader.biPlanes = bitmap.bmPlanes;
	m_pBMPINFO->bmiHeader.biSize = m_bmpHeadTotalSize;//sizeof(BITMAPINFO);
	m_pBMPINFO->bmiHeader.biSizeImage = m_imgTotalSize;
	m_pBMPINFO->bmiHeader.biWidth = width;
	m_pBMPINFO->bmiHeader.biXPelsPerMeter = 0;
	m_pBMPINFO->bmiHeader.biYPelsPerMeter = 0;
}
