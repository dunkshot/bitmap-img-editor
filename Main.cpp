#define _USE_MATH_DEFINES //math.h�� ������ִ� pi���� ��������
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "Editor_Basic.h"
#include "InsertImage.h"
#include "lettering.h"

#define DEG2RAD(n)      (M_PI * (n) / 180)     //������ �������� �ٲ�

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildProc_button(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildProc_filter(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LPCTSTR lpszClass = TEXT("�ú���");
HINSTANCE g_hInst; //��ư�� ���̴� hInstance
WNDCLASS WndClass;
BOOL bEllipse = TRUE;
HWND g_hWnd; //�۷ι��ڵ�

#define wndWidth 1152
#define wndHeight 648
#define originImg 0
#define historySize 5	//���� ��Ҵ� 5�ܰ������ ����

FILE *file = NULL;
int width = 0, height = 0;
int hstryCnt = 1;		//history Count : ������� ���� ī��Ʈ ���� // �۾� �ϳ� �� ������ ī��Ʈ ++
int UndoCnt = 0;		//������� �Լ� ȣ�� Ƚ���� ī��Ʈ�Ͽ� 5�� �ʰ��� return
int RedoCnt = 0;
int isFilterSelectOn = FALSE;
int isFilterSave = FALSE;
BOOL DoPaint = FALSE;
unsigned char R, G, B;
unsigned char Gray;
RECT rt;
static DOUBLE nAngle = 0;
static int widthOrigin = 0, heightOrigin = 0;
BOOL Rotateflag = FALSE;
BOOL WorkingNow = FALSE;
HDC Original_hdc; // ����� �ϱ����� ���� hdc

typedef struct _BmpFileHeader {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} BmpFileHeader;

typedef struct _BmpInfoHeader {
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
}  BmpInfoHeader;

typedef struct _RgbTriple {
	BYTE   rgbtBlue;
	BYTE   rgbtGreen;
	BYTE   rgbtRed;
} RgbTriple;

BmpFileHeader bmpfh;
BmpInfoHeader bmpih;
RgbTriple *pixelData;
RgbTriple **pixelDataArr_Origin;	//������ ����� �� 2����
RgbTriple **pixelDataArr_Temp;		//���� ���� �� �� �� �� 2����
RgbTriple **pixelDataArr_TempPrint;	//������ ����Ұ� �� 2����
RgbTriple ***pixelDataArr;			//3���� �迭 ���� �Ҵ�

COLORREF **color;

void SetIndex(TCHAR *FilePath);
void saveFile(TCHAR *FilePath);
void DoCreateMain(HWND hWnd, TCHAR *FilePath);
void DoPaintMain0(HWND hWnd, TCHAR *FilePath);
void DoPaintMain(HWND hWnd);
void DoTest(HWND hWdn);
void Undo(HWND hWnd);
void DoReset(HWND hWnd);
void Redo(HWND hWnd);
void BeginFilterSelect(HWND hWnd);
void CancelFilter(HWND hWnd);
void ApplyFilter(HWND hWnd);
void FilterReset(HWND hWnd);

void Brightness_Up_Filter(HWND hWnd);
void Brightness_Down_Filter(HWND hWnd);
void Contrast_Up_Filter(HWND hWnd);
void Contrast_Down_Filter(HWND hWnd);
void Chroma_Up_Filter(HWND hWnd);
void Chroma_Down_Filter(HWND hWnd);
void Gray_Filter(HWND hWnd);
void Sepia_Filter(HWND hWnd);
void Reverse_Filter(HWND hWnd);
void Median_Filter(HWND hWnd);
void Blur_Filter(HWND hWnd);
void Gaussian_Filter(HWND hWnd);
void Sharpening_Filter(HWND hWnd);
void Edge_Filter(HWND hWnd);
void Sobel_Filter(HWND hWnd);
void Woodcut_Filter(HWND hWnd);

void RotateImage(HWND hWnd, static DOUBLE nAngle);
void Draw(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
void Crop(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
void CaptureNow(HWND hWnd);

void DownScale(HWND hWnd, TCHAR *FilePath);

/**************************************************/
/* �����Ҵ� �� �迭�� ���� free������ߵ� */
void MemInit(void) {
	if (pixelDataArr != NULL) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < heightOrigin; j++) {
				free(pixelDataArr[i][j]);
			}
			free(pixelDataArr[i]);
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpszCmdParam, int nCmdShow) {
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;						//���� �޸�
	WndClass.cbWndExtra = 0;						//���� �޸�
	WndClass.hbrBackground = CreateSolidBrush(RGB(226, 226, 226)); //��� ��
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW); //���콺 Ŀ�� ������
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//������ ������
	WndClass.hInstance = hInstance;					//�ڵ� �ν��Ͻ�
	WndClass.lpfnWndProc = WndProc;					//���ν��� �Լ�
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1); //@@@@@@@@@@@@�޴�
	WndClass.lpszClassName = lpszClass;	//Ŭ�����̸�
	WndClass.style = CS_HREDRAW | CS_VREDRAW;		//������ ��Ÿ��
	RegisterClass(&WndClass);			//���� ����� Ŭ������ ��� (�ü���� Ŀ�ο� ��ϵ�)
	//��ġ �⺻�� : CW_USEDEFAULT
	
	// �ʿ��� �κ��� �����Ͽ� ���ϵ� Ŭ������ ��� - ��ư ǥ���� ���ϵ� ������
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.lpfnWndProc = ChildProc_button;
	WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	WndClass.lpszClassName = TEXT("ChildCls_button");
	RegisterClass(&WndClass);

	// �ʿ��� �κ��� �����Ͽ� ���ϵ� Ŭ������ ��� - ���ͼ���â ǥ���� ���ϵ� ������
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.lpfnWndProc = ChildProc_filter;
	WndClass.lpszClassName = TEXT("ChildCls_filter");
	WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	RegisterClass(&WndClass);

	//���������� ���� - ��ġ 10*10, ũ�� 1152*648
	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10, wndWidth, wndHeight, 
		NULL, (HMENU)NULL, hInstance, NULL); 
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Global Handle
	g_hWnd = hWnd;

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message); //Ű���� �ؼ�
		DispatchMessage(&Message); //�޽��� ����
	}
	return (int)Message.wParam;
}


//��ư�� �̹��� ������ ���� �ڵ鼱��
HBITMAP hBitmap;
HWND hButton;
BOOL DrawPen = FALSE;
BOOL DoCrop = FALSE;
BOOL NowWorking = FALSE;

// ���� ������ Ŭ����
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{	
	PAINTSTRUCT     ps;
	HDC             hDc;
	HBITMAP hBit, hOldBitmap;
	HDC hMemDc;
	BITMAP bmp;

	switch (iMessage)
	{
	case WM_CREATE:
		hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP_quit)); // quit.bmp ��Ʈ���� �о�´�

		//��ưǥ�� ���� ���ϵ� ������ ����
		CreateWindow(TEXT("ChildCls_button"), TEXT("��ưǥ�ÿ���"), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, 292, 800, hwnd, (HMENU)NULL, g_hInst, NULL);

	case WM_COMMAND: //�޴��� ���� �޽��� ó��
		switch (wParam)
		{
		case ID_Quit: //�����ϱ�
			PostQuitMessage(0);
			break;

		case ID_Info:
			//MessageBox(hwnd, TEXT("iiiiiiiiiiiiiiiiinfo\n ���⿣ ũ����\n����"), L"����", NULL);
			break;
		}
		break;
	case WM_KEYDOWN:
		if (Rotateflag) {
			if (WorkingNow) {
				InvalidateRect(g_hWnd, &rt, TRUE);
				switch (wParam) {
					// ȭ��ǥŰ
				case VK_UP: nAngle -= 180;
					break;
				case VK_DOWN: nAngle += 180;
					break;
				case VK_LEFT: nAngle -= 90;
					break;
				case VK_RIGHT: nAngle += 90;
					break;
					// ����Ű
				case VK_NUMPAD0: nAngle = 0;
					break;
				case VK_NUMPAD1: nAngle -= 90;
					break;
				case VK_NUMPAD2: nAngle += 1;
					break;
				case VK_NUMPAD3: nAngle += 90;
					break;
				case VK_NUMPAD4: nAngle -= 5;
					break;
				case VK_NUMPAD5: nAngle += 1;
					break;
				case VK_NUMPAD6: nAngle += 5;
					break;
				case VK_NUMPAD7: nAngle -= 45;
					break;
				case VK_NUMPAD8: nAngle -= 1;
					break;
				case VK_NUMPAD9: nAngle += 45;
					break;
					//DEFAULT:            return 0;
				}
			}
		}
		break;
	case VK_INSERT: // Clipboard
		if (!IsClipboardFormatAvailable(CF_BITMAP)) //Ŭ����������
			return 0;
		if (OpenClipboard(hwnd)) { //�� �����찡 Ŭ�����带 �����ϰ��Ѵ�.
			hBit = (HBITMAP)GetClipboardData(CF_BITMAP); //CF_BITMAP�� Ŭ�����忡 ������ ������ ���� �޸��ڵ��� hmem�� �޾Ƽ� ����
			CloseClipboard(); //Ŭ�����忡 �����͸� ���� �� Ŭ�����带 �ݴ´�.
			hDc = GetDC(hwnd); //�����쿡 ��Ʈ���� �׸��غ� �Ѵ�.
			hMemDc = CreateCompatibleDC(hDc);
			hOldBitmap = (HBITMAP)SelectObject(hMemDc, hBit);
			GetObject(hBit, sizeof(BITMAP), &bmp);
			BitBlt(hDc, 292, 0, bmp.bmWidth, bmp.bmHeight, hMemDc, 0, 0, SRCCOPY);//Ŭ�����忡 ����� ��Ʈ���̹����� ���
			SelectObject(hMemDc, hOldBitmap);
			DeleteDC(hMemDc);
			ReleaseDC(hwnd, hDc);
		}
		break;
	case WM_PAINT: {
		if (Rotateflag) {
			hDc = BeginPaint(hwnd, &ps);
			RotateImage(hwnd, nAngle);
			EndPaint(hwnd, &ps);
			//CaptureNow(g_hWnd);
		}
		lettering_Main(hwnd);
		InsertImageMain(hwnd);

		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		break;
	}
	}

	//�׸��� ��ư�� ������ draw�Լ��� ������
	//Ű���� �޽����� �޾ƿ��� ���� ���� �����쿡 ��ġ��Ŵ
	if (DrawPen == TRUE) {
						 Draw(g_hWnd, iMessage, wParam, lParam);
					 }

	//�ڸ��� ��ư�� ������ crop�Լ��� ������
	if (DoCrop == TRUE) {
						 Crop(g_hWnd, iMessage, wParam, lParam);
					 }

	/****************************���ڻ��� �۾��� �ʿ��� ó��************************************/
	if (iMessage == WM_CTLCOLOREDIT && (HWND)lParam == FontEdit && SelectedFont == TRUE) {

						 // ���ɼ�(����ڽ�)
						 // Pettern Brush�� ������ִ°�?

						 SetTextColor((HDC)wParam, CFT.rgbColors);
						 SetBkMode((HDC)wParam, OPAQUE);

						 return (LRESULT)GetStockObject(WHITE_BRUSH);

					 }

	DragAndMoveText(hwnd, iMessage, wParam, lParam);
	/********************************************************************************************/
	DragAndMoveImage(hwnd, iMessage, wParam, lParam);
	
	return DefWindowProc(hwnd, iMessage, wParam, lParam);   //ó������ ���� �޽��� ��ȯ
}



//������ ��ư ��Ȱ��ȭ�� ���� ��ư�� �ĺ��� �ڵ� ����
//Handle of Main Button, Handle of Filter Button (0,0)���� (3,2)����
HWND hMB00, hMB01, hMB02, hMB10, hMB11, hMB12, hMB20, hMB21, hMB22, hMB30, hMB31, hMB32,
		hFB00, hFB01, hFB02, hFB10, hFB11, hFB12, hFB20, hFB21, hFB22, hFB30, hFB31, hFB32;
HWND f_hwnd;
HWND b_hwnd;
HWND p_hwnd;
TCHAR sFilePathName[MAX_FILENAME_SIZE];		//���ڰ����� �������


// ���ϵ� Ŭ������ �Լ� - ��ư ǥ�� ����
LRESULT CALLBACK ChildProc_button(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	// ������ ���� ���� ���̾�α�
	OPENFILENAME Ofn;		//���������� ����ü
	//TCHAR sFilePathName[MAX_FILENAME_SIZE];		//���ڰ����� �������
	static TCHAR sFilter[] = L"��Ʈ�� ���� \0*.bmp"; //���Ͱ��� ���� : bmp�� �ɷ�����
	
	//��ư �̹����� ��������� �ڵ� ����
	BITMAP bmp;
	HDC hdc;

	//�̹����� �׷��� ����ü ����
	static LPDRAWITEMSTRUCT lpdis;
	static HDC memDC;
	
	switch (iMessage)
	{
	case WM_CREATE:
	{
		// 1�� ��ư����
		hMB00 = CreateWindow(L"button", L"���� child", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 9, 85, 85, hWnd, (HMENU)ID_LoadFile, g_hInst, NULL);
		hMB01 = CreateWindow(L"button", L"���� child", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 10, 85, 85, hWnd, (HMENU)ID_SaveFile, g_hInst, NULL);
		hMB02 = CreateWindow(L"button", L"����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 10, 85, 85, hWnd, (HMENU)ID_Quit, g_hInst, NULL);

		// 2��
		hMB10 = CreateWindow(L"button", L"���� ���", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 103, 85, 85, hWnd, (HMENU)ID_Undo, g_hInst, NULL);
		hMB11 = CreateWindow(L"button", L"���� ����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 103, 85, 85, hWnd, (HMENU)ID_Reset, g_hInst, NULL);
		hMB12 = CreateWindow(L"button", L"�ٽ� ����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 103, 85, 85, hWnd, (HMENU)ID_Redo, g_hInst, NULL);

		// 3��
		hMB20 = CreateWindow(L"button", L"�׸���", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 197, 85, 85, hWnd, (HMENU)ID_Pen, g_hInst, NULL);
		hMB21 = CreateWindow(L"button", L"��������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 197, 85, 85, hWnd, (HMENU)ID_Shape, g_hInst, NULL);
		hMB22 = CreateWindow(L"button", L"�ؽ�Ʈ����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 197, 85, 85, hWnd, (HMENU)ID_Text, g_hInst, NULL);

		// 4��
		hMB30 = CreateWindow(L"button", L"ȸ��", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 291, 85, 85, hWnd, (HMENU)ID_Rotate, g_hInst, NULL);
		hMB31 = CreateWindow(L"button", L"�ڸ���", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 291, 85, 85, hWnd, (HMENU)ID_Crop, g_hInst, NULL);
		hMB32 = CreateWindow(L"button", L"ȿ��", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 291, 85, 85, hWnd, (HMENU)ID_FilterSelect, g_hInst, NULL);

		break;
	}
	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		//memDC = CreateCompatibleDC(lpdis->hDC);
		//SelectObject(memDC, hBitmap);
		switch (wParam)
		{
		case ID_LoadFile:
			if (lpdis->itemState & ODS_SELECTED) // ��ư�� ���� ����(BUTTONDOWN)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else // ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_SaveFile:
			if (lpdis->itemState & ODS_SELECTED) // ��ư�� ���� ����(BUTTONDOWN)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else // ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Quit:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Undo:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Reset:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Redo:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Pen:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Shape:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Text:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Rotate:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Crop:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_FilterSelect:
			if (lpdis->itemState & ODS_SELECTED) { // ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;
		}
		break;

	case WM_COMMAND: //��ư�� ���� �޽��� ó��
		switch (wParam){
		case ID_LoadFile: 	// �ҷ�����
			memset(&Ofn, 0, sizeof(OPENFILENAME));		// ������ ���� �������� ����ü������ 0->�ʱ�ȭ
			Ofn.lStructSize = sizeof(OPENFILENAME);		// ���籸��ü�� ũ�⸦ OPENFILE����ü��ŭ�� ������� ��
			Ofn.hwndOwner = hWnd;						// ���� ���̾�α��� �θ��ڵ鰪�� ����
			Ofn.lpstrFilter = sFilter;					// ������̾�α��� �������İ��� sFilter������ ����
			Ofn.lpstrFile = sFilePathName;				// ������ ������ ��ġ�� �̸����� ������ ���ڿ�����(�ּҰ�)�� ��������
			Ofn.nMaxFile = MAX_FILENAME_SIZE;			// �����ϴ� ������ ��ο� �̸��� �ִ� ũ�⸦ ����
			Ofn.lpstrInitialDir = L"C:\\";				//���̾�α��� �ʱⰪ ��ġ�� C:\�� ����. ���ڿ����� \���� \\�� \.�� ���

			if (GetOpenFileName(&Ofn) != 0)				//���̾�α׿��� ������� ���� ���� ��� 
			{
				wsprintf(sFilePathName, L"%s", Ofn.lpstrFile);
				//������ ������ ���,���ϸ��� sFilePathName�� ����

				hstryCnt = 1; //�����丮 ī��Ʈ �ʱ�ȭ : ���� ���� �ҷ��� ���� ����� ����ȭ�� ������ ���� ����

				SetIndex(Ofn.lpstrFile);
				DoCreateMain(g_hWnd, Ofn.lpstrFile);

				DoPaint = FALSE;
				DoPaintMain(g_hWnd);

				DownScale(g_hWnd, Ofn.lpstrFile); //��� �̹��� ���
			}
			break;

		case ID_SaveFile: 	// ����
			memset(&Ofn, 0, sizeof(OPENFILENAME));		// ������ ���� �������� ����ü������ 0->�ʱ�ȭ
			Ofn.lStructSize = sizeof(OPENFILENAME);		// ���籸��ü�� ũ�⸦ OPENFILE����ü��ŭ�� ������� ��
			Ofn.hwndOwner = hWnd;						// ���� ���̾�α��� �θ��ڵ鰪�� ����
			Ofn.lpstrFilter = sFilter;					// ������̾�α��� �������İ��� sFilter������ ����
			Ofn.lpstrFile = sFilePathName;				// ������ ������ ��ġ�� �̸����� ������ ���ڿ�����(�ּҰ�)�� ��������
			Ofn.nMaxFile = MAX_FILENAME_SIZE;			// �����ϴ� ������ ��ο� �̸��� �ִ� ũ�⸦ ����
			Ofn.lpstrInitialDir = L"C:\\";				//���̾�α��� �ʱⰪ ��ġ�� C:\�� ����. ���ڿ����� \���� \\�� \.�� ���
			if (GetSaveFileName(&Ofn) != 0)				//���̾�α׿��� ������� ���� ���� ��� 
			{
				//������
				wsprintf(sFilePathName, L"%s", Ofn.lpstrFile);
				//���� ������ ������ ��ο� �̸��� sFilePathName�� �־���. ������ �̹� �������־���
				saveFile(Ofn.lpstrFile);

				MessageBox(hWnd, sFilePathName, L"���� ����", MB_OKCANCEL);
				//�޼����ڽ��� ���� ���� ������ ������ ��ο� �̸��� ���
			}
			break;

		case ID_Undo:		// �������
			Undo(g_hWnd);
			break;

		case ID_Reset:		// ó������ �ǵ�����
			DoReset(g_hWnd);
			break;

		case ID_Redo:		// �ٽý���
			Redo(g_hWnd);
			break;
		case ID_Rotate: 	// ȸ�����
			SetFocus(g_hWnd);
			Rotateflag = TRUE;
			WorkingNow = ~(WorkingNow);
			InvalidateRect(g_hWnd, &rt, TRUE);
			break;
		case ID_FilterSelect:	// ����
			if (file == NULL)
				break;

			BeginFilterSelect(g_hWnd);

			//��Ȱ��ȭ
			EnableWindow(hMB00, FALSE); EnableWindow(hMB01, FALSE); EnableWindow(hMB02, FALSE);
			EnableWindow(hMB10, FALSE); EnableWindow(hMB11, FALSE); EnableWindow(hMB12, FALSE);
			EnableWindow(hMB20, FALSE); EnableWindow(hMB21, FALSE); EnableWindow(hMB22, FALSE);
			EnableWindow(hMB30, FALSE); EnableWindow(hMB31, FALSE); EnableWindow(hMB32, FALSE);

			EnableWindow(b_hwnd, FALSE);

			//���ͼ���â ���� ���ϵ� ������ ����
			f_hwnd = CreateWindow(TEXT("ChildCls_filter"), TEXT("���ͼ���â"), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
				0, 0, 292, 800, hWnd, (HMENU)NULL, g_hInst, NULL);
			break;

		case ID_Pen:		// �׸���

			if (NowWorking == FALSE)
			{
				SetFocus(g_hWnd); //Ű�����Է��� �ޱ� ���� ���� �ڵ�� ��Ŀ������
				DrawPen = TRUE;
				NowWorking = TRUE;
			}
			break;

		case ID_Crop:		// �ڸ���
			if (NowWorking == FALSE)
			{
				SetFocus(g_hWnd); //Ű�����Է��� �ޱ� ���� ���� �ڵ�� ��Ŀ������ - ���ص��ɰŰ�����, ���ο���
				DoCrop = TRUE;
				NowWorking = TRUE;
			}
			break;
		case ID_Shape:		// ���� ����
			SetFocus(g_hWnd);
			InsertImageStart = TRUE;
			InvalidateRect(g_hWnd, NULL, FALSE);
			break;
		case ID_Text:		// �ؽ�Ʈ ����
			SetFocus(g_hWnd);
			ShowFontSelect(g_hWnd);
			break;
		case ID_Quit:		// ����
			PostQuitMessage(0);
			break;
		}
						break;

						break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_PAINT:
			break;
		}
	
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

S
// ���ϵ� Ŭ������ �Լ� - ���ͼ���â
LRESULT CALLBACK ChildProc_filter(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	BITMAP bmp; //��ư �̹����� ��������� �ڵ� ����

	//�̹����� �׷��� ����ü ����
	static LPDRAWITEMSTRUCT lpdis;
	static HDC memDC;

	switch (iMessage)
	{
	case WM_CREATE:
	{
		//hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP_quit)); // ��Ʈ�� �о�´�
		isFilterSelectOn = TRUE; //���ͼ���â�� ���ȳ�? ��. �̰� �������� �׸����� ���ϵ���

		// 1�� ��ư����
		hFB00 = CreateWindow(L"button", L"�ڷ�", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 9, 85, 85, hWnd, (HMENU)ID_CancelFilter, g_hInst, NULL);
		hFB01 = CreateWindow(L"button", L"���͸���", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 10, 85, 85, hWnd, (HMENU)ID_ResetFilter, g_hInst, NULL);
		hFB01 = CreateWindow(L"button", L"����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 10, 85, 85, hWnd, (HMENU)ID_ApplyFilter, g_hInst, NULL);

		// 2��
		hFB10 = CreateWindow(L"button", L"�������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 103, 85, 85, hWnd, (HMENU)ID_F_BrightUp, g_hInst, NULL);
		hFB11 = CreateWindow(L"button", L"�������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 103, 85, 85, hWnd, (HMENU)ID_F_ContrastUp, g_hInst, NULL);
		hFB12 = CreateWindow(L"button", L"ä������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 103, 85, 85, hWnd, (HMENU)ID_F_ChromaUp, g_hInst, NULL);

		// 3��
		hFB20 = CreateWindow(L"button", L"��Ⱘ��", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			10, 197, 85, 85, hWnd, (HMENU)ID_F_BrightDown, g_hInst, NULL);
		hFB21 = CreateWindow(L"button", L"��񰨼�", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 197, 85, 85, hWnd, (HMENU)ID_F_ContrastDown, g_hInst, NULL);
		hFB22 = CreateWindow(L"button", L"ä������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 197, 85, 85, hWnd, (HMENU)ID_F_ChromaDown, g_hInst, NULL);

		// 4��
		hFB30 = CreateWindow(L"button", L"�׷���", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			10, 291, 85, 85, hWnd, (HMENU)ID_F_Gray, g_hInst, NULL);
		hFB31 = CreateWindow(L"button", L"���Ǿ�", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 291, 85, 85, hWnd, (HMENU)ID_F_Sepia, g_hInst, NULL);
		hFB32 = CreateWindow(L"button", L"����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 291, 85, 85, hWnd, (HMENU)ID_F_Reverse, g_hInst, NULL);

		// 5��
		CreateWindow(L"button", L"�޵��", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 385, 85, 85, hWnd, (HMENU)ID_F_Median, g_hInst, NULL);
		CreateWindow(L"button", L"��", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 385, 85, 85, hWnd, (HMENU)ID_F_Blur, g_hInst, NULL);
		CreateWindow(L"button", L"����þ�", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 385, 85, 85, hWnd, (HMENU)ID_F_Gaussian, g_hInst, NULL);

		// 6��
		CreateWindow(L"button", L"������", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 479, 85, 85, hWnd, (HMENU)ID_F_Sharpening, g_hInst, NULL);
		CreateWindow(L"button", L"����", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 479, 85, 85, hWnd, (HMENU)ID_F_Edge, g_hInst, NULL);
		CreateWindow(L"button", L"�Һ�", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 479, 85, 85, hWnd, (HMENU)ID_F_Sobel, g_hInst, NULL);

		break;
	}

	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		switch (wParam)
		{
		case ID_CancelFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_ResetFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_ApplyFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_BrightUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ContrastUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ChromaUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_BrightDown:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ContrastDown:
		{
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ChromaDown:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 510, SRCCOPY); 
				DeleteDC(memDC);
			}
		break;

		case ID_F_Gray:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		break;

		case ID_F_Sepia:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 510, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Reverse:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư���� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 510, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Median:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Blur:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Gaussian:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Sharpening:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Edge:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 345, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 345, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Sobel:
			if (lpdis->itemState & ODS_SELECTED) {// ��ư�� ���� ����(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// ��ư�� UP�Ǿ� �ִ� ����(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;


		}

		break;

	case WM_COMMAND: //��ư�� ���� �޽��� ó��
		switch (wParam)
		{
		case ID_CancelFilter:

			//���͸� �����ߴٸ� �׳� ����
			if (isFilterSave == TRUE) {
				CancelFilter(g_hWnd);		//���� ���
				isFilterSelectOn = FALSE;	//���ͼ���â �����ϴ�

				//����â ������ ��Ȱ��ȭ�� ��ư�� �ٽ� Ȱ��ȭ
				EnableWindow(hMB00, TRUE); EnableWindow(hMB01, TRUE); EnableWindow(hMB02, TRUE);
				EnableWindow(hMB10, TRUE); EnableWindow(hMB11, TRUE); EnableWindow(hMB12, TRUE);
				EnableWindow(hMB20, TRUE); EnableWindow(hMB21, TRUE); EnableWindow(hMB22, TRUE);
				EnableWindow(hMB30, TRUE); EnableWindow(hMB31, TRUE); EnableWindow(hMB32, TRUE);

				EnableWindow(hFB00, FALSE); EnableWindow(hFB01, FALSE); EnableWindow(hFB02, FALSE);
				EnableWindow(hFB10, FALSE); EnableWindow(hFB11, FALSE); EnableWindow(hFB12, FALSE);
				EnableWindow(hFB20, FALSE); EnableWindow(hFB21, FALSE); EnableWindow(hFB22, FALSE);
				EnableWindow(hFB30, FALSE); EnableWindow(hFB31, FALSE); EnableWindow(hFB32, FALSE);

				DestroyWindow(f_hwnd);
				DownScale(g_hWnd, sFilePathName); //����â ���ư� �� �����̹��� �ٽ� ���
				break;
			}

			//���͸� �������� �ʾҴٸ� �����
			else { 
				if (MessageBox(hWnd, TEXT("��������� �������� �ʰ� ���� ���·� ���ư��ϴ�.\n���� ȭ���� �ݿ��Ϸ��� �����ư�� �����ּ���.\n���ư��ðڽ��ϱ�?"),
					TEXT("�˸�"), MB_YESNO) == IDYES)
				{
					CancelFilter(g_hWnd);		//���� ���
					isFilterSelectOn = FALSE;	//���ͼ���â �����ϴ�

					//����â ������ ��Ȱ��ȭ�� ��ư�� �ٽ� Ȱ��ȭ
					EnableWindow(hMB00, TRUE); EnableWindow(hMB01, TRUE); EnableWindow(hMB02, TRUE);
					EnableWindow(hMB10, TRUE); EnableWindow(hMB11, TRUE); EnableWindow(hMB12, TRUE);
					EnableWindow(hMB20, TRUE); EnableWindow(hMB21, TRUE); EnableWindow(hMB22, TRUE);
					EnableWindow(hMB30, TRUE); EnableWindow(hMB31, TRUE); EnableWindow(hMB32, TRUE);

					EnableWindow(hFB00, FALSE); EnableWindow(hFB01, FALSE); EnableWindow(hFB02, FALSE);
					EnableWindow(hFB10, FALSE); EnableWindow(hFB11, FALSE); EnableWindow(hFB12, FALSE);
					EnableWindow(hFB20, FALSE); EnableWindow(hFB21, FALSE); EnableWindow(hFB22, FALSE);
					EnableWindow(hFB30, FALSE); EnableWindow(hFB31, FALSE); EnableWindow(hFB32, FALSE);

					DestroyWindow(f_hwnd);
					DownScale(g_hWnd, sFilePathName); //����â ���ư� �� �����̹��� �ٽ� ���
					break;
				}
				else
					break;
			}
			

		case ID_ResetFilter:
			FilterReset(g_hWnd);
			break;

		case ID_ApplyFilter:
			//ApplyFilter(g_hWnd);		//���� ����
			CaptureNow(g_hWnd);
			DC_Copy(hWnd, Copy_hdc, &Original_hdc);
			isFilterSave = TRUE; //���� ���� �Ϸ�
			isFilterSelectOn = FALSE;	//���ͼ���â �����ϴ�
			break;

		case ID_F_BrightUp:
			Brightness_Up_Filter(g_hWnd);
			break;

		case ID_F_BrightDown:
			Brightness_Down_Filter(g_hWnd);
			break;

		case ID_F_ContrastUp:
			Contrast_Up_Filter(g_hWnd);
			break;

		case ID_F_ContrastDown:
			Contrast_Down_Filter(g_hWnd);
			break;

		case ID_F_ChromaUp:
			Chroma_Up_Filter(g_hWnd);
			break;

		case ID_F_ChromaDown:
			Chroma_Down_Filter(g_hWnd);
			break;

		case ID_F_Gray:
			Gray_Filter(g_hWnd);
			break;

		case ID_F_Sepia:
			Sepia_Filter(g_hWnd);
			break;

		case ID_F_Reverse:
			Reverse_Filter(g_hWnd);
			break;

		case ID_F_Median:
			Median_Filter(g_hWnd);
			break;

		case ID_F_Blur:
			Blur_Filter(g_hWnd);
			break;

		case ID_F_Gaussian:
			Gaussian_Filter(g_hWnd);
			break;

		case ID_F_Sharpening:
			Sharpening_Filter(g_hWnd);
			break;

		case ID_F_Edge:
			Edge_Filter(g_hWnd);
			break;

		case ID_F_Sobel:
			Sobel_Filter(g_hWnd);
			break;

		case ID_F_Woodcut:
			Woodcut_Filter(g_hWnd);
			break;


		case ID_Quit:
			break;
		}

	case WM_DESTROY:
		//PostQuitMessage(0);
		break;

	case WM_CLOSE:
		ShowWindow(f_hwnd, SW_HIDE);
		break;

	case WM_PAINT:
		break;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void RotateImage(HWND hWnd, static DOUBLE nAagle)
{
	int x = 0, y = 0;
	int bit = 1;
	int length = 0;
	int cx = 0, cy = 0;
	int sx = 0, sy = 0;

	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		//MessageBox(hWnd, TEXT("�̹����� �ε���� �ʾҽ��ϴ�. \n������ ���� ���ּ���"), TEXT("����"), MB_OK);
		return;
	}
	length = sqrt(pow(width, 2) / 4 + pow(height, 2) / 4);  //bitmap�� ȸ�� �ݰ� ������

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			x = ((length - width / 2) + i) - length;  //bitmap�� �߰� ��ǥ�� �������� �Ҷ��� x�� 
			y = (length - height / 2) + (height - j) - length;   //bitmap�� �߰� ��ǥ�� �������� �Ҷ��� y��
			cx = x*cos(DEG2RAD(nAagle)) - y*sin(DEG2RAD(nAagle)) + length;  //ȸ���� ��Ų x���� ��ǥ
			cy = x*sin(DEG2RAD(nAagle)) + y*cos(DEG2RAD(nAagle)) + length;  //ȸ���� ��Ų y���� ��ǥ
			sx = (x + 0.5)*cos(DEG2RAD(nAagle)) - (y + 0.5)*sin(DEG2RAD(nAagle)) + length;  //ȸ���ҋ� ��������� ���۵��� �����ϱ� ���� ���� ����ϱ����� ���� �ٸ����� x��
			sy = (x + 0.5)*sin(DEG2RAD(nAagle)) + (y + 0.5)*cos(DEG2RAD(nAagle)) + length;   //ȸ���ҋ� ��������� ���۵��� �����ϱ� ���� ���� ����ϱ����� ���� �ٸ����� y��
			SetPixel_F(hWnd, hdc, cx + 186, cy - 196,
				RGB(pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtRed, pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtGreen, pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtBlue));
			SetPixel_F(hWnd, hdc, sx + 186, sy - 196,
				RGB(pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtRed, pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtGreen, pixelDataArr[(hstryCnt - 1)%5][j][i].rgbtBlue));

		}
	}


	PrintOut(hdc, width + 292, height, SET);
	CaptureNow(hWnd);
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	
	ReleaseDC(hWnd, hdc);
	return;
}

//���ͼ����� �����Ҷ� ȣ�� : �ӽù迭�� ����ȭ�� ���� ������ ȭ���� ����
void BeginFilterSelect(HWND hWnd) 
{
	//5��° ���࿡�� ���ߴ� ���� ���� : hstryCnt%5-1 �� ������ �Ǵ� ����̹Ƿ� ����ó��
	if (hstryCnt % 5 == 0) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < width; k++) {
				pixelDataArr_Temp[j][k].rgbtBlue = pixelDataArr[4][j][k].rgbtBlue;
				pixelDataArr_Temp[j][k].rgbtGreen = pixelDataArr[4][j][k].rgbtGreen;
				pixelDataArr_Temp[j][k].rgbtRed = pixelDataArr[4][j][k].rgbtRed;
			}
		}
	}

	else {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < width; k++) {
				pixelDataArr_Temp[j][k].rgbtBlue = pixelDataArr[hstryCnt % 5 - 1][j][k].rgbtBlue;
				pixelDataArr_Temp[j][k].rgbtGreen = pixelDataArr[hstryCnt % 5 - 1][j][k].rgbtGreen;
				pixelDataArr_Temp[j][k].rgbtRed = pixelDataArr[hstryCnt % 5 - 1][j][k].rgbtRed;
			}
		}
	}

}

//���͸��� : ���� ����â ���� ������ ȭ������ ���ư�
void FilterReset(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	//���ͼ��� ���� ���� �̹��� ���
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_Temp[i][j].rgbtRed, pixelDataArr_Temp[i][j].rgbtGreen, pixelDataArr_Temp[i][j].rgbtBlue));
		}
	}

	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;

	ReleaseDC(hWnd, hdc);
}

//��������
void ApplyFilter(HWND hWnd)
{
	//���� ȭ�鿡 �ִ� �� ���� ����� pixelDataArr�� ����
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr[hstryCnt%5][i][j].rgbtBlue = pixelDataArr_TempPrint[i][j].rgbtBlue;
			pixelDataArr[hstryCnt%5][i][j].rgbtGreen = pixelDataArr_TempPrint[i][j].rgbtGreen;
			pixelDataArr[hstryCnt%5][i][j].rgbtRed = pixelDataArr_TempPrint[i][j].rgbtRed;
		}
	}
	hstryCnt++; //���� ������ �迭�� ���������� ī��Ʈ++ - �����������
	BeginFilterSelect(g_hWnd); //���� ���� ������ �ݿ��ϱ� ���� temp�� �ٽ� ���� (���� ���� �����)

}

//����ϰ� ������
void CancelFilter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	//���� ��Ҵ��� �ʱ�ȭ
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr_Temp[i][j].rgbtBlue = 0;
			pixelDataArr_Temp[i][j].rgbtGreen = 0;
			pixelDataArr_Temp[i][j].rgbtRed = 0;

			pixelDataArr_TempPrint[i][j].rgbtBlue = 0;
			pixelDataArr_TempPrint[i][j].rgbtGreen = 0;
			pixelDataArr_TempPrint[i][j].rgbtRed = 0;
		}
	}

	//���ͼ��� ���� ���� �̹��� ���
	//hstryCnt%5-1�� ������ �Ǿ� �׼��� ���� �Ǵ� ��� ����ó��
	if (hstryCnt % 5 == 0) {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[4][i][j].rgbtRed, pixelDataArr[4][i][j].rgbtGreen, pixelDataArr[4][i][j].rgbtBlue));
			}
		}
	}

	else {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[hstryCnt % 5 - 1][i][j].rgbtRed, pixelDataArr[hstryCnt % 5 - 1][i][j].rgbtGreen, pixelDataArr[hstryCnt % 5 - 1][i][j].rgbtBlue));
			}
		}
	}

	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;
	ReleaseDC(hWnd, hdc);
}


//�������, �������, �������� ����
void SetIndex(TCHAR *FilePath)
{
	FILE *fr = _wfopen(FilePath, L"rb,ccs=UNICODE"); //�����ڵ���� fopen
	if (fr == NULL)
	{
		printf("xx\n");
	}

	// ������� �о ����
	fread(&bmpfh.bfType, 1, sizeof(bmpfh.bfType), fr);
	fread(&bmpfh.bfSize, 1, sizeof(bmpfh.bfSize), fr);
	fread(&bmpfh.bfReserved1, 1, sizeof(bmpfh.bfReserved1), fr);
	fread(&bmpfh.bfReserved2, 1, sizeof(bmpfh.bfReserved2), fr);
	fread(&bmpfh.bfOffBits, 1, sizeof(unsigned long), fr);

	// ������� �о ����
	fread(&bmpih.biSize, 1, sizeof(bmpih.biSize), fr);
	fread(&bmpih.biWidth, 1, sizeof(bmpih.biWidth), fr);
	fread(&bmpih.biHeight, 1, sizeof(bmpih.biHeight), fr);
	fread(&bmpih.biPlanes, 1, sizeof(bmpih.biPlanes), fr);
	fread(&bmpih.biBitCount, 1, sizeof(bmpih.biBitCount), fr);
	fread(&bmpih.biCompression, 1, sizeof(bmpih.biCompression), fr);
	fread(&bmpih.biSizeImage, 1, sizeof(bmpih.biSizeImage), fr);
	fread(&bmpih.biXPelsPerMeter, 1, sizeof(bmpih.biXPelsPerMeter), fr);
	fread(&bmpih.biYPelsPerMeter, 1, sizeof(bmpih.biYPelsPerMeter), fr);
	fread(&bmpih.biClrUsed, 1, sizeof(bmpih.biClrUsed), fr);
	fread(&bmpih.biClrImportant, 1, sizeof(bmpih.biClrImportant), fr);

	pixelData = (RgbTriple*)malloc(bmpih.biSizeImage);     // �̹��� ���� �Ҵ�
	fread(pixelData, 1, bmpih.biSizeImage, fr);            // �̹��� ������ �о����

	fclose(fr);

}

void saveFile(TCHAR *FilePath)
{
	FILE *fw = _wfopen(FilePath, L"wb,ccs=UNICODE"); //�����ڵ���� fopen
	if (file == NULL) {
		return;
	}

	//������ �״�� ���
	// ������� ���� -- ������ �з��� ����Ǿ ����� ��� �и����� ��������
	fwrite(&bmpfh.bfType, sizeof(bmpfh.bfType), 1, fw);
	fwrite(&bmpfh.bfSize, sizeof(bmpfh.bfSize), 1, fw);
	fwrite(&bmpfh.bfReserved1, sizeof(bmpfh.bfReserved1), 1, fw);
	fwrite(&bmpfh.bfReserved2, sizeof(bmpfh.bfReserved2), 1, fw);
	fwrite(&bmpfh.bfOffBits, sizeof(bmpfh.bfOffBits), 1, fw);

	fwrite(&bmpih, sizeof(BITMAPINFOHEADER), 1, fw); // ������� ����
	
	//�ȼ����� - 3�����迭 ���� @@@@@@@@@@@@@@@@@@@@@���� �պ����� hstryCnt���� ������ �ٽú�
	if (hstryCnt == 0)
		hstryCnt = 5;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			fwrite(&pixelDataArr[hstryCnt % 5 - 1][i][j], 1, sizeof(unsigned char) * 3, fw);
		}
	}

	fclose(fw);
}

void DoCreateMain(HWND hWnd, TCHAR *FilePath)
{
	file = _wfopen(FilePath, L"rb,ccs=UNICODE"); //�����ڵ���� fopen

	if (file != NULL) {
		char buff[2] = { 0,0 };
		fread(buff, 2, 1, file);
		if (!(buff[0] == 'B' && buff[1] == 'M')) {
			MessageBeep(MB_OKCANCEL);
			return;
		}

		fseek(file, 18, SEEK_SET);
		fread(&width, 4, 1, file);
		fread(&height, 4, 1, file);

		widthOrigin = width;
		heightOrigin = height;

		//�������� ���縦 ���� �ݺ��� - width,height���� �˰� �����ؾ� �ϱ� ������ ���⿡ ��ġ��
		//history �迭 3���� ���� �Ҵ�
		pixelDataArr = (RgbTriple***)malloc(sizeof(RgbTriple**)*historySize);
		pixelDataArr_Origin = (RgbTriple**)malloc(sizeof(RgbTriple*)*height);
		pixelDataArr_Temp = (RgbTriple**)malloc(sizeof(RgbTriple*)*height);
		pixelDataArr_TempPrint = (RgbTriple**)malloc(sizeof(RgbTriple*)*height);
		for (int k = 0; k < historySize; k++)
		{
			pixelDataArr[k] = (RgbTriple**)malloc(sizeof(RgbTriple*)*height);
			for (int i = 0; i < height; i++)
			{
				pixelDataArr[k][i] = (RgbTriple*)malloc(sizeof(RgbTriple)*width);
				pixelDataArr_Origin[i] = (RgbTriple*)malloc(sizeof(RgbTriple)*width);
				pixelDataArr_Temp[i] = (RgbTriple*)malloc(sizeof(RgbTriple)*width);
				pixelDataArr_TempPrint[i] = (RgbTriple*)malloc(sizeof(RgbTriple)*width);
			}
		}

		//�ʱ�ȭ
		for (int i = 0; i < historySize; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < width; k++) {
					pixelDataArr[i][j][k].rgbtBlue = 0;
					pixelDataArr[i][j][k].rgbtGreen = 0;
					pixelDataArr[i][j][k].rgbtRed = 0;
				}
			}
		}

		//3�����迭 ù ū ����� Origin�迭�� ���� ���� (x,y ��ǥ ���� ����)
		//pixelDataArr[0][x][y]�� ���� �̹����� ��
		//��������� ���� �� ���� 1���� ����(pixelDataArr[A][x][y]���� [A])�� +1 �����ϸ� x,y ������ ���
		//���� ������ �� ���� �迭�� �����ϴ� ������ ������������ �ս��� ��������
		int l = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				pixelDataArr_Origin[j][k].rgbtBlue = pixelData[l].rgbtBlue;
				pixelDataArr_Origin[j][k].rgbtGreen = pixelData[l].rgbtGreen;
				pixelDataArr_Origin[j][k].rgbtRed = pixelData[l].rgbtRed;

				pixelDataArr[0][j][k].rgbtRed = pixelData[l].rgbtRed;
				pixelDataArr[0][j][k].rgbtBlue = pixelData[l].rgbtBlue;
				pixelDataArr[0][j][k].rgbtGreen = pixelData[l].rgbtGreen;

				l++;
			}
		}

		//������ ũ�� ����
		SetWindowPos(hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, width+292+17, height+60, SWP_NOMOVE | SWP_SHOWWINDOW);
		SetRect(&rt, 292, 0, width + 292 + 17, height + 60);
		return; //wndWidth, wndHeight
	}
	else {
		MessageBeep(MB_OKCANCEL);
	}
}

//��ư���� ����̹��� ��� (�̸�����)
void DownScale(HWND hWnd, TCHAR *FilePath)
{
	PAINTSTRUCT ps;
	HBITMAP hImage, hOldBitmap;
	//HDC hdc = BeginPaint(hWnd, &ps); //@@@@@@@@@@@@@@@@@@@@@@@�̰� �ΰ� ���̰� �����?
	HDC hdc = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hdc);

	hImage = (HBITMAP)LoadImage(NULL, FilePath, IMAGE_BITMAP
		, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);
	
	SetStretchBltMode(hdc, HALFTONE); //����ս� ����
	StretchBlt(hdc, 9, 385, 270, 185, hMemDC, 0, 0, width, height, SRCCOPY); 
	//(9,385)��ġ�� 270*185�� ũ��� ������ (0,0)~(width,height) ������ ����

	SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hImage);
	DeleteDC(hMemDC);
	ReleaseDC(hWnd, hdc);
}

//�Ѻ��� DoPaint �ν���
void DoPaintMain0(HWND hWnd, TCHAR *FilePath)
{
	HBITMAP hImage, hOldBitmap;
	PAINTSTRUCT ps;
	//HDC hdc = BeginPaint(hWnd, &ps);
	HDC hdc = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hdc); // For the created Memory DC

	// ù��° �μ� NULL : �������ҽ� ���
	hImage = (HBITMAP)LoadImage(NULL, FilePath, IMAGE_BITMAP
		, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	// hndle ����
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);

	StretchBlt(hdc, 400, 100, 273, 179, hMemDC, 0, 0, width, height, SRCCOPY);
	//BitBlt(hdc, 292, 0, width, height, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOldBitmap); // hImage ������ �����ϱ� ���� hOldBitmap�� �����Ѵ�
	DeleteObject(hImage); // ���� ������ ��Ʈ���� �����Ѵ�
	DeleteDC(hMemDC); // �޸� DC�� �����Ѵ�

	//EndPaint(hWnd, &ps);
	ReleaseDC(hWnd, hdc);
}

// �����տ� �׽�Ʈ
void DoPaintMain(HWND hWnd)
{
	HDC hdc;
	PAINTSTRUCT ps;

	if (DoPaint == FALSE) {

		PAINTSTRUCT ps;
		HDC hdc = GetDC(hWnd);
		COLORREF color;

		int i, j;

		fseek(file, 54, SEEK_SET);

		//��ȭ�� ���
		for (int j = 0; j < height; j++)
		{
			for (int i = 0; i < width; i++)
			{
				fread(&B, sizeof(unsigned char), 1, file);
				fread(&G, sizeof(unsigned char), 1, file);
				fread(&R, sizeof(unsigned char), 1, file);
				SetPixel_F(hWnd, hdc, 292 + i, height - j, RGB(R, G, B));
			}
		}

		PrintOut(hdc, width, height, SET);
		DC_Copy(hWnd, hdc, &Original_hdc);
		Reset_SetPixel_F();
		ReleaseDC(hWnd, hdc);

		DoPaint = TRUE;

	}
	else {

		hdc = BeginPaint(hWnd, &ps);
		PrintOut(hdc, width, height, ORIGINAL);
		EndPaint(hWnd, &ps);

	}

}

//�̰� �׽�Ʈ����
void DoTest(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		//MessageBox(hWnd, TEXT("�̹����� �ε���� �ʾҽ��ϴ�. \n������ ���� ���ּ���"), TEXT("����"), MB_OK);
		return;
	}

	//3�����迭 ����ȭ
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr_TempPrint[i][j].rgbtRed = 255 - (pixelDataArr_Temp[i][j].rgbtRed);
			pixelDataArr_TempPrint[i][j].rgbtGreen = 255 - (pixelDataArr_Temp[i][j].rgbtGreen);
			pixelDataArr_TempPrint[i][j].rgbtBlue = 255 - (pixelDataArr_Temp[i][j].rgbtBlue);

			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_TempPrint[i][j].rgbtRed, pixelDataArr_TempPrint[i][j].rgbtGreen, pixelDataArr_TempPrint[i][j].rgbtBlue));
			//x, y ��ǥ �ٲ�� �˾Ƶμ�
		}
	}
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;

	ReleaseDC(hWnd, hdc);
}

void Brightness_Up_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb;
	COLORREF color;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, 292+i, j);

			tr = GetRValue(color);
			tg = GetGValue(color);
			tb = GetBValue(color);

			tr = tr + 10;
			tg = tg + 10;
			tb = tb + 10;

			if (tr >= 255) // �����÷ο� ó��
				tr = 255;
			if (tg >= 255)
				tg = 255;
			if (tb >= 255)
				tb = 255;

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb));

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

void Brightness_Down_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb;
	COLORREF color;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i+292, j);

			tr = GetRValue(color);
			tg = GetGValue(color);
			tb = GetBValue(color);

			tr = tr - 10;
			tg = tg - 10;
			tb = tb - 10;

			if (tr < 0) // ����÷ο� ó��
				tr = 0;
			if (tg < 0)
				tg = 0;
			if (tb < 0)
				tb = 0;

			SetPixel_F(hWnd, hdc, i+292, j, RGB(tr, tg, tb));

			pixelDataArr_TempPrint[height-1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height-1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height-1 - j][i].rgbtBlue = tb;
		}
	}
	PrintOut(hdc, width, height, SET);
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

void Contrast_Up_Filter(HWND hWnd) // ��ϴ�� ����
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue�� ���� ���� 8��Ʈ �ڷḦ ���� 
	COLORREF color; // GetPixel�� ���� ���� 32��Ʈ �ڷḦ ����

	for (int j = 0; j < height; j++) // ��Ʈ�� ��ü �ȼ��� �����͸� �ް� ����ϱ� ���� ����
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // �� �ȼ��� �����͸� �޾ƿ´�.

			tr = GetRValue(color); // �ȼ��� Red��
			tg = GetGValue(color); // �ȼ��� Green��
			tb = GetBValue(color); // �ȼ��� Blue��

			tr = tr * 1.1; // ���� ���� R, G, B�� �����ϰ� ���Ѵ�.
			tg = tg * 1.1;
			tb = tb * 1.1;

			if (tr >= 255) // �����÷ο� ó��
				tr = 255;
			if (tg >= 255)
				tg = 255;
			if (tb >= 255)
				tb = 255;

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // �� �ȼ��� ������� ����Ѵ�

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC������ �ִ� ȭ���� �ѹ��� ����Ѵ�.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}
void Contrast_Down_Filter(HWND hWnd) // ��ϴ�� ����
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue�� ���� ���� 8��Ʈ �ڷḦ ���� 
	COLORREF color; // GetPixel�� ���� ���� 32��Ʈ �ڷḦ ����

	for (int j = 0; j < height; j++) // ��Ʈ�� ��ü �ȼ��� �����͸� �ް� ����ϱ� ���� ����
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // �� �ȼ��� �����͸� �޾ƿ´�.

			tr = GetRValue(color); // �ȼ��� Red��
			tg = GetGValue(color); // �ȼ��� Green��
			tb = GetBValue(color); // �ȼ��� Blue��

			tr = tr * 0.9; // ���� ���� R, G, B�� �����ϰ� ������.
			tg = tg * 0.9;
			tb = tb * 0.9;

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // �� �ȼ��� ������� ����Ѵ�

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC������ �ִ� ȭ���� �ѹ��� ����Ѵ�.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}


void Chroma_Up_Filter(HWND hWnd) // ä�� ����
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue�� ���� ���� 8��Ʈ �ڷḦ ���� 
	double max_color; // R,G,B�� ���� ū ������ �����ϴ� ���� 
	COLORREF color; // GetPixel�� ���� ���� 32��Ʈ �ڷḦ ����

	for (int j = 0; j < height; j++) // ��Ʈ�� ��ü �ȼ��� �����͸� �ް� ����ϱ� ���� ����
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // �� �ȼ��� �����͸� �޾ƿ´�.

			tr = GetRValue(color); // �ȼ��� Red��
			tg = GetGValue(color); // �ȼ��� Green��
			tb = GetBValue(color); // �ȼ��� Blue��

			max_color = tr; // ��  R,G,B�� ���� ū ������ ����ȴ�.
			if (tg > max_color)
				max_color = tg;
			else if (tb > max_color)
				max_color = tb;

			if (max_color == tr) // �ش� �ȼ����� ���� ū ������ �������
								 // ���� ������Ų��.
			{
				tr = (tr * 1.03);
				if (tr >= 255) // �����÷ο� ó��
					tr = 255;
			}

			else if (max_color == tg)

			{
				tg = (tg * 1.03);
				if (tg >= 255)
					tg = 255;
			}

			else if (max_color == tb)

			{
				tb = (tb * 1.03);
				if (tb >= 255)
					tb = 255;
			}

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // �� �ȼ��� ������� ����Ѵ�

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC������ �ִ� ȭ���� �ѹ��� ����Ѵ�.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}
void Chroma_Down_Filter(HWND hWnd) // ä�� ����
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue�� ���� ���� 8��Ʈ �ڷḦ ���� 
	double max_color; // R,G,B�� ���� ū ������ �����ϴ� ���� 
	COLORREF color; // GetPixel�� ���� ���� 32��Ʈ �ڷḦ ����

	for (int j = 0; j < height; j++) // ��Ʈ�� ��ü �ȼ��� �����͸� �ް� ����ϱ� ���� ����
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // �� �ȼ��� �����͸� �޾ƿ´�.

			tr = GetRValue(color); // �ȼ��� Red��
			tg = GetGValue(color); // �ȼ��� Green��
			tb = GetBValue(color); // �ȼ��� Blue��

			max_color = tr; // ��  R,G,B�� ���� ū ������ ����ȴ�.
			if (tg > max_color)
				max_color = tg;
			else if (tb > max_color)
				max_color = tb;

			if (max_color == tr) // �ش� �ȼ����� ���� ū ������ ������� ���� ���ҽ�Ų��.
				tr = tr * 0.9;
			else if (max_color == tg)
				tg = tg * 0.9;
			else if (max_color == tb)
				tb = tb * 0.9;

			SetPixel_F(hWnd, hdc, i+292, j, RGB(tr, tg, tb)); // �� �ȼ��� ������� ����Ѵ�

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC������ �ִ� ȭ���� �ѹ��� ����Ѵ�.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

void Gray_Filter(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	//3�����迭 ����ȭ
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr_TempPrint[i][j].rgbtRed = 
				(pixelDataArr_Temp[i][j].rgbtRed)*0.3 + (pixelDataArr_Temp[i][j].rgbtGreen)*0.59 + (pixelDataArr_Temp[i][j].rgbtBlue)*0.11;
			pixelDataArr_TempPrint[i][j].rgbtGreen =
				(pixelDataArr_Temp[i][j].rgbtRed)*0.3 + (pixelDataArr_Temp[i][j].rgbtGreen)*0.59 + (pixelDataArr_Temp[i][j].rgbtBlue)*0.11;
			pixelDataArr_TempPrint[i][j].rgbtBlue =
				(pixelDataArr_Temp[i][j].rgbtRed)*0.3 + (pixelDataArr_Temp[i][j].rgbtGreen)*0.59 + (pixelDataArr_Temp[i][j].rgbtBlue)*0.11;
			
			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_TempPrint[i][j].rgbtRed, pixelDataArr_TempPrint[i][j].rgbtGreen, pixelDataArr_TempPrint[i][j].rgbtBlue));

		}
	}
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F();

	ReleaseDC(hWnd, hdc);
}

void Sepia_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	unsigned char gray;

	//3�����迭 ����ȭ
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			Gray = (0.3 * pixelDataArr_Temp[i][j].rgbtRed + 0.59 * pixelDataArr_Temp[i][j].rgbtGreen + 0.11 * pixelDataArr_Temp[i][j].rgbtBlue);

			pixelDataArr_TempPrint[i][j].rgbtRed = ((Gray > 206) ? 255 : Gray + 49);
			pixelDataArr_TempPrint[i][j].rgbtGreen = ((Gray < 14) ? 0 : Gray - 14);
			pixelDataArr_TempPrint[i][j].rgbtBlue = ((Gray < 56) ? 0 : Gray - 56);

			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_TempPrint[i][j].rgbtRed, pixelDataArr_TempPrint[i][j].rgbtGreen, pixelDataArr_TempPrint[i][j].rgbtBlue));

		}
	}
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F();

	ReleaseDC(hWnd, hdc);
}

void Reverse_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr_TempPrint[i][j].rgbtRed = 255 - (pixelDataArr_Temp[i][j].rgbtRed);
			pixelDataArr_TempPrint[i][j].rgbtGreen = 255 - (pixelDataArr_Temp[i][j].rgbtGreen);
			pixelDataArr_TempPrint[i][j].rgbtBlue = 255 - (pixelDataArr_Temp[i][j].rgbtBlue);

			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_TempPrint[i][j].rgbtRed, pixelDataArr_TempPrint[i][j].rgbtGreen, pixelDataArr_TempPrint[i][j].rgbtBlue));
		}
	}
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;

	ReleaseDC(hWnd, hdc);
}

#define Mask_Size 3
unsigned int Median_Sort(unsigned int N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size�ȼ��� �߾Ӱ��� ��ȯ
{
	int i, j, k, l;
	unsigned int temp = 0; // ���Ŀ� ���� �ӽú���

	for (i = 0; i < Mask_Size; i++) // 2���� �迭�� ���� ����
	{
		for (j = 0; j < Mask_Size; j++)
		{
			for (k = 0; k < Mask_Size; k++)
			{
				for (l = 0; l < Mask_Size; l++)
				{
					if (N[i][j] < N[k][l])
					{
						temp = N[i][j];
						N[i][j] = N[k][l];
						N[k][l] = temp;
					}
				}
			}
		}
	}
	return N[Mask_Size / 2][Mask_Size / 2];   // �߾Ӱ��� ��ȯ
}
void Median_Filter(HWND hWnd) // 
{
	HDC hdc = GetDC(hWnd);
	
	COLORREF color;
	unsigned int N[Mask_Size][Mask_Size]; // ����ũ�� ����� ������ ����

	for (int j = 0; j < (height - 2); j++) // sort�� ���� ��µǴ� �ȼ��� ��üȭ�鿡 ����ϱ� ���� ����
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++) // ����ũ�� �ԷµǴ� �ȼ��� ���� ����
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + 292 + k, j + l); // 32��Ʈ ���������� ��ȯ�Ǵ� GetPixel_F
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i +292 + 1, j + 1, Median_Sort(N)); // Median_Sort(N)�� ��ȯ ���� GetPixel�� ���� ���� ���� �ϳ��̴�.
																 // �߰����� 2���� �迭�� �ε����� ���� ��ȯ�ȴ�.
			
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Median_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Median_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Median_Sort(N));

		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}


COLORREF Blur_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size�ȼ��� ��հ��� ��ȯ
{
	int i, j;
	COLORREF sum = 0;
	unsigned char nR, nG, nB; // GetRValue�� ���� R,G,B ���� ����
	int Rsum = 0, Gsum = 0, Bsum = 0; // nR, nG, nB�� ����� ���ϱ� ���� ����
									  // ���ϴ� �������� 255�� �Ѿ�� ������ int���� �ʿ�

	for (i = 0; i < Mask_Size; i++) // ��������� ���� ����
	{
		for (j = 0; j < Mask_Size; j++)
		{
			nR = GetRValue(N[i][j]);
			nG = GetGValue(N[i][j]);
			nB = GetBValue(N[i][j]);

			Rsum = Rsum + nR;
			Gsum = Gsum + nG;
			Bsum = Bsum + nB;
		}
	}

	Rsum = Rsum / (Mask_Size * Mask_Size); // ����ȭ�� ���� ����
	Gsum = Gsum / (Mask_Size * Mask_Size);
	Bsum = Bsum / (Mask_Size * Mask_Size);

	nR = Rsum; // ��հ��� 0 ~ 255 ���������� �°� ����
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // ��� ����ũ�� ������� ���� ��ȯ
}
void Blur_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k + 292, j + l);
					N[l][k] = color;
				}
			}
			
			SetPixel_F(hWnd, hdc, i + 292 + 1, j + 1, Blur_Sort(N));

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Blur_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Blur_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Blur_Sort(N));

		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

COLORREF Gaussian_Sort(COLORREF N[Mask_Size][Mask_Size])
{
	int i, j;
	COLORREF sum = 0;
	unsigned char nR, nG, nB;
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if (i == 1 && j == 1)
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR / 4);
				Gsum = Gsum + (nG / 4);
				Bsum = Bsum + (nB / 4);
			}

			else if ((i + j) % 2 == 0)
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR / 16);
				Gsum = Gsum + (nG / 16);
				Bsum = Bsum + (nB / 16);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR / 8);
				Gsum = Gsum + (nG / 8);
				Bsum = Bsum + (nB / 8);
			}
		}
	}

	nR = Rsum;
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
void Gaussian_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k + 292, j + l);
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i + 292 + 1, j + 1, Gaussian_Sort(N));

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Gaussian_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Gaussian_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Gaussian_Sort(N));
		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

COLORREF Sharpening_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size�ȼ��� ����ġ ��հ��� ��ȯ
{
	int i, j;
	COLORREF sum = 0;
	int nR, nG, nB;
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if ((i == 1) && (j == 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 9);
				Gsum = Gsum + (nG * 9);
				Bsum = Bsum + (nB * 9);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * (-1));
				Gsum = Gsum + (nG * (-1));
				Bsum = Bsum + (nB * (-1));
			}
		}
	}


	nR = Rsum;
	nG = Gsum;
	nB = Bsum;

	if (nR >= 255) // �����÷ο� ó��
		nR = 255;
	else if (nR < 0)
		nR = 0;

	if (nG >= 255)
		nG = 255;
	else if (nG < 0)
		nG = 0;

	if (nB >= 255)
		nB = 255;
	else if (nB < 0)
		nB = 0;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
void Sharpening_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k +292, j + l);
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i + 292 + 1, j + 1, Sharpening_Sort(N));

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Sharpening_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Sharpening_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Sharpening_Sort(N));
		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

COLORREF Edge_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size�ȼ��� ����ġ ��հ��� ��ȯ
{
	int i, j;
	COLORREF sum = 0;
	int nR, nG, nB;
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if ((i == 1) && (j == 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 8);
				Gsum = Gsum + (nG * 8);
				Bsum = Bsum + (nB * 8);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * (-1));
				Gsum = Gsum + (nG * (-1));
				Bsum = Bsum + (nB * (-1));
			}
		}
	}


	nR = Rsum;
	nG = Gsum;
	nB = Bsum;

	if (nR >= 255) // �����÷ο� ó��
		nR = 255;
	else if (nR < 0)
		nR = 0;

	if (nG >= 255)
		nG = 255;
	else if (nG < 0)
		nG = 0;

	if (nB >= 255)
		nB = 255;
	else if (nB < 0)
		nB = 0;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
void Edge_Filter(HWND hWnd)
{
	Gray_Filter(hWnd);
	HDC hdc = GetDC(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k + 292, j + l);
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i + 292 + 1, j + 1, Edge_Sort(N));

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Edge_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Edge_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Edge_Sort(N));
		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

COLORREF Horizon_Sobel_Sort(COLORREF N[Mask_Size][Mask_Size])
{
	int i, j;
	int nR, nG, nB; // ����Ʈ unsigned char (0.255) 
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if ((i == 0) && (j != 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * -1);
				Gsum = Gsum + (nG * -1);
				Bsum = Bsum + (nB * -1);
			}

			else if ((i == 2) && (j != 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + nR;
				Gsum = Gsum + nG;
				Bsum = Bsum + nB;
			}

			else if ((i == 0) && (j == 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * -2);
				Gsum = Gsum + (nG * -2);
				Bsum = Bsum + (nB * -2);
			}

			else if ((i == 2) && (j == 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 2);
				Gsum = Gsum + (nG * 2);
				Bsum = Bsum + (nB * 2);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 0);
				Gsum = Gsum + (nG * 0);
				Bsum = Bsum + (nB * 0);
			}
		}
	}

	if (Rsum >= 255)
		Rsum = 255;
	else if (Rsum < 0)
		Rsum = 0;

	if (Gsum >= 255)
		Gsum = 255;
	else if (Gsum < 0)
		Gsum = 0;

	if (Bsum >= 255)
		Bsum = 255;
	else if (Bsum < 0)
		Bsum = 0;

	nR = Rsum; // ����ȭ : ����ġ �� 1
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
COLORREF Vertic_Sobel_Sort(COLORREF N[Mask_Size][Mask_Size])
{
	int i, j;
	int nR, nG, nB; // ����Ʈ unsigned char (0.255) 
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if ((i == 0) && (j != 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * -1);
				Gsum = Gsum + (nG * -1);
				Bsum = Bsum + (nB * -1);
			}

			else if ((i == 2) && (j != 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + nR;
				Gsum = Gsum + nG;
				Bsum = Bsum + nB;
			}

			else if ((i == 1) && (j == 0))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * -2);
				Gsum = Gsum + (nG * -2);
				Bsum = Bsum + (nB * -2);
			}

			else if ((i == 1) && (j == 2))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 2);
				Gsum = Gsum + (nG * 2);
				Bsum = Bsum + (nB * 2);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 0);
				Gsum = Gsum + (nG * 0);
				Bsum = Bsum + (nB * 0);
			}
		}
	}

	if (Rsum >= 255)
		Rsum = 255;
	else if (Rsum < 0)
		Rsum = 0;

	if (Gsum >= 255)
		Gsum = 255;
	else if (Gsum < 0)
		Gsum = 0;

	if (Bsum >= 255)
		Bsum = 255;
	else if (Bsum < 0)
		Bsum = 0;

	nR = Rsum; // ����ȭ : ����ġ �� 1
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
void Sobel_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	Gray_Filter(hWnd);

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k +292, j + l);
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i + 1 +292, j + 1, Horizon_Sobel_Sort(N));
			SetPixel_F(hWnd, hdc, i + 1+292, j + 1, Vertic_Sobel_Sort(N));

		}
	}

	Reset_GetPixel_F();

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j);

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(color);
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(color);
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(color);
		}
	}

	PrintOut(hdc, width, height, SET);
	
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

COLORREF Woodcut_Sort(COLORREF N[Mask_Size][Mask_Size])
{
	int i, j;
	int nR, nG, nB; // ����Ʈ unsigned char (0.255) 
	int Rsum = 0, Gsum = 0, Bsum = 0;

	for (i = 0; i < Mask_Size; i++)
	{
		for (j = 0; j < Mask_Size; j++)
		{
			if ((i == 1) && (j == 1))
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * 9);
				Gsum = Gsum + (nG * 9);
				Bsum = Bsum + (nB * 9);
			}

			else
			{
				nR = GetRValue(N[i][j]);
				nG = GetGValue(N[i][j]);
				nB = GetBValue(N[i][j]);

				Rsum = Rsum + (nR * (-1)); // �����÷ο� 
				Gsum = Gsum + (nG * (-1)); // 
				Bsum = Bsum + (nB * (-1)); // 
			}
		}
	}

	nR = Rsum; // ����ȭ : ����ġ �� 1
	nG = Gsum;
	nB = Bsum;

	if (Rsum >= 255)
		Rsum = 255;
	else if (Rsum < 0)
		Rsum = 0;

	if (Gsum >= 255)
		Gsum = 255;
	else if (Gsum < 0)
		Gsum = 0;

	if (Bsum >= 255)
		Bsum = 255;
	else if (Bsum < 0)
		Bsum = 0;

	return RGB(nR, nG, nB); // ����ũ�� ����ġ ������� ���� ��ȯ
}
void Woodcut_Filter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	Blur_Filter(hWnd);
	COLORREF color;
	COLORREF N[Mask_Size][Mask_Size];

	for (int j = 0; j < (height - 2); j++)
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++)
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + k + 292, j + l);
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i + 292 + 1, j + 1, Woodcut_Sort(N) - N[1][1]);

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Woodcut_Sort(N) - N[1][1]);
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Woodcut_Sort(N) - N[1][1]);
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Woodcut_Sort(N) - N[1][1]);

		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

//�׸���
void Draw(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static int sx, sy, oldx, oldy;
	int ex, ey;
	static BOOL bNowDraw = FALSE;
	HDC hdc, hMemDC;
	HBITMAP OldBitmap;
	PAINTSTRUCT ps;
	switch (iMessage)
	{
	case WM_LBUTTONDOWN:
		sx = LOWORD(lParam);
		sy = HIWORD(lParam);
		bNowDraw = TRUE;
		return;
	case WM_LBUTTONUP:
		bNowDraw = FALSE;
		return;
	case WM_MOUSEMOVE:
		if (bNowDraw)
		{
			hdc = GetDC(hWnd);
			MoveToEx(hdc, sx, sy, NULL);
			sx = LOWORD(lParam); //�߰�	
			sy = HIWORD(lParam); //�߰�
			LineTo(hdc, sx, sy);
			ReleaseDC(hWnd, hdc);
		}
		break;

	case WM_RBUTTONDOWN:
		NowWorking = FALSE;
		DrawPen = FALSE;
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			NowWorking = FALSE;
			DrawPen = FALSE;

			CaptureNow(g_hWnd);

			return;
		}
		break;
	}
	

	return;
}

//���� ȭ���� arr�� �����Ѵ�
void CaptureNow(HWND hWnd) 
{
	HDC hdc = GetDC(hWnd);
	COLORREF color;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			color = GetPixel_F(hWnd, hdc, 292 + j, i);

			pixelDataArr[hstryCnt%5][height -1 - i][j].rgbtRed = color & 0xFF;
			pixelDataArr[hstryCnt%5][height -1 - i][j].rgbtGreen = (color >> 8) & 0xFF;
			pixelDataArr[hstryCnt%5][height -1 - i][j].rgbtBlue = (color >> 16) & 0xFF;
		}
	}

	hstryCnt++; //�������, �ٽý���, ��������
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

void DrawBoxOutline(HWND hwnd, POINT ptBeg, POINT ptEnd)
{
	HDC hdc = GetDC(hwnd);

	SetROP2(hdc, R2_NOT);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, ptBeg.x, ptBeg.y, ptEnd.x, ptEnd.y);

	ReleaseDC(hwnd, hdc);
}

void Crop(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static BOOL  fBlocking, fValidBox;
	static POINT ptBeg, ptEnd, ptBoxBeg, ptBoxEnd;
	HDC hdc = GetDC(hWnd);
	PAINTSTRUCT  ps;
	int i = 0;
	switch (iMessage)
	{
		
		// Inside the capture window procedure.
	case WM_LBUTTONDOWN:
		ptBeg.x = ptEnd.x = LOWORD(lParam);
		ptBeg.y = ptEnd.y = HIWORD(lParam);

		DrawBoxOutline(hWnd, ptBeg, ptEnd);

		SetCapture(hWnd);
		SetCursor(LoadCursor(NULL, IDC_CROSS));

		fBlocking = TRUE;
		return;

	case WM_MOUSEMOVE:
		if (fBlocking)
		{
			SetCursor(LoadCursor(NULL, IDC_CROSS));

			DrawBoxOutline(hWnd, ptBeg, ptEnd);

			ptEnd.x = (short)LOWORD(lParam);
			ptEnd.y = (short)HIWORD(lParam);

			if (ptEnd.x <= 0)
			{
				ptEnd.x = 1;
			}
			if (ptEnd.x > width+292)
			{
				ptEnd.x = width+292;
			}
			if (ptEnd.y <= 0)
			{
				ptEnd.y = 1;
			}
			if (ptEnd.y > height)
			{
				ptEnd.y = height;
			}
			DrawBoxOutline(hWnd, ptBeg, ptEnd);
		}
		return;

	case WM_LBUTTONUP:

		if (fBlocking)
		{
			DrawBoxOutline(hWnd, ptBeg, ptEnd);

			ptBoxBeg = ptBeg;
			ptBoxEnd.x = (short)LOWORD(lParam);
			ptBoxEnd.y = (short)HIWORD(lParam);

			if (ptBoxEnd.x <= 0)
			{
				ptBoxEnd.x = 1;
			}
			if (ptBoxEnd.x > width+292)
			{
				ptBoxEnd.x = width+292;
			}
			if (ptBoxEnd.y <= 0)
			{
				ptBoxEnd.y = 1;
			}
			if (ptBoxEnd.y > height)
			{
				ptBoxEnd.y = height;
			}
			ReleaseCapture();
			SetCursor(LoadCursor(NULL, IDC_ARROW));

			fBlocking = FALSE;
			fValidBox = TRUE;

		}


		if (fValidBox)
		{
			hdc = GetDC(hWnd);
			SelectObject(hdc, GetStockObject(BLACK_BRUSH));

			COLORREF **color;
			color = (COLORREF**)malloc(sizeof(COLORREF*) * height);
			for (int i = 0; i<height; i++) {
				color[i] = (COLORREF*)malloc(sizeof(COLORREF) * width);
			}

			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					color[i][j] = RGB(255, 255, 255);

			if (ptBoxEnd.y < ptBoxBeg.y)
			{
				long temp = ptBoxBeg.y;
				ptBoxBeg.y = ptBoxEnd.y;
				ptBoxEnd.y = temp;
			}
			if (ptBoxEnd.x < ptBoxBeg.x)
			{
				long temp = ptBoxBeg.x;
				ptBoxBeg.x = ptBoxEnd.x;
				ptBoxEnd.x = temp;
			}
			for (int i = 0; i < ptBoxEnd.y - ptBoxBeg.y; i++)
			{
				for (int j = 0; j < ptBoxEnd.x - ptBoxBeg.x; j++)
				{
					color[i][j] = GetPixel_F(hWnd, hdc, ptBoxBeg.x + j, ptBoxBeg.y + i);
				}
			}

			for (int i = 0; i < height; i++)
				for (int j = 0; j < width; j++)
					SetPixel_F(hWnd, hdc, j+292, i, color[i][j]);

			PrintOut(hdc, width, height, SET);
			Reset_SetPixel_F();
			Reset_GetPixel_F();

			ReleaseDC(hWnd, hdc);

		}


		if (MessageBox(hWnd, TEXT("���� ������ �����Ͻðڽ��ϱ�?"),TEXT("�˸�"), MB_YESNO) == IDYES)
		{

			width = abs(ptEnd.x - ptBeg.x);
			height = abs(ptEnd.y - ptBeg.y);
			bmpih.biWidth = width;
			bmpih.biHeight = height;
			//bmpih.biSize = width*height * 3;
			bmpih.biSizeImage = width*height * 3;
			bmpfh.bfSize = width*height * 3 + 54;

			CaptureNow(g_hWnd);

			//NowWorking = FALSE;
			//DoCrop = FALSE;

			//pixelDataArr����, �����丮ī��Ʈ
		}
		else
		{
			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					SetPixel_F(hWnd, hdc, 292 + j, height - i,
						RGB(pixelDataArr[hstryCnt-1][i][j].rgbtRed, pixelDataArr[hstryCnt - 1][i][j].rgbtGreen, pixelDataArr[hstryCnt - 1][i][j].rgbtBlue));
				}
			}
			PrintOut(hdc, width + 292, height, SET);
			Reset_SetPixel_F;
			Reset_GetPixel_F;
			//NowWorking = false;
			//DoCrop = FALSE;
		}

		ReleaseDC(hWnd, hdc);
		return;

	case WM_CHAR:
		if (fBlocking & (wParam == '\x1B'))     // i.e., Escape
		{
			DrawBoxOutline(hWnd, ptBeg, ptEnd);

			ReleaseCapture();
			SetCursor(LoadCursor(NULL, IDC_ARROW));

			fBlocking = FALSE;
		}
		return;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			NowWorking = FALSE;
			DoCrop = FALSE;
			return;
		}
		break;

		break;
	}
}

//�������
void Undo(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		MessageBox(hWnd, TEXT("�̹����� �ε���� �ʾҽ��ϴ�. \n������ ���� ���ּ���"), TEXT("���"), MB_OK);
		return;
	}

	//�����丮 ī��Ʈ�� 5���� �ö��� �ʾ��� �� ������Ҹ� ��� �������
	if (hstryCnt - 2 < 0) {
		MessageBox(hWnd, TEXT("���̻� �ڷ� �� �� �����ϴ�."), TEXT("�˸�"), MB_OK);
		return;
	}
	
	//�����丮 ī��Ʈ�� 5�� �Ѿ��� �� ������Ҹ� ��� ���� ���
	if (UndoCnt < historySize) { //������� �Լ��� ȣ�� Ƚ���� �ټ��� �����϶�

			//���� �̹��� ���
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtRed, pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtGreen, pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtBlue));
				//x, y ��ǥ �ٲ�� �˾Ƶμ�
			}
		}
		hstryCnt--;
		UndoCnt++;
		RedoCnt--;
		PrintOut(hdc, width + 292, height, SET);
		Reset_SetPixel_F;
		ReleaseDC(hWnd, hdc);
	}
	else { //������� �Լ��� ȣ�� Ƚ���� �ټ��� �ʰ��϶�
		MessageBox(hWnd, TEXT("���� ��Ҵ� �ִ� �ټ� �ܰ���� �����մϴ�."), TEXT("�˸�"), MB_OK);
		return;
	}

}

//���󺹱�
void DoReset(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		return;
	}

	//origin�迭�κ��� pixelDatArr[0][][]�� �����̹��� ���� & ���
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			pixelDataArr[0][j][k].rgbtRed = pixelDataArr_Origin[j][k].rgbtRed;
			pixelDataArr[0][j][k].rgbtGreen = pixelDataArr_Origin[j][k].rgbtGreen;
			pixelDataArr[0][j][k].rgbtBlue = pixelDataArr_Origin[j][k].rgbtBlue;

			SetPixel_F(hWnd, hdc, 292 + k, height - j,
				RGB(pixelDataArr[0][j][k].rgbtRed, pixelDataArr[0][j][k].rgbtGreen, pixelDataArr[0][j][k].rgbtBlue));
			//x, y ��ǥ �ٲ�� �˾Ƶμ�
		}
	}

	hstryCnt = 1;
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;
	ReleaseDC(hWnd, hdc);
}

//�ٽý���
void Redo(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		MessageBox(hWnd, TEXT("�̹����� �ε���� �ʾҽ��ϴ�. \n������ ���� ���ּ���"), TEXT("����"), MB_OK);
		return;
	}

	//�ٽ� ���� Ƚ���� ������� Ƚ���� �ѱ� �� ����
	if (RedoCnt >= UndoCnt) {
		MessageBox(hWnd, TEXT("���̻� ������ �� �� �����ϴ�."), TEXT("�˸�"), MB_OK);
		return;
	}

	else {
		//�� ������ ���� �̹��� ���
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{

				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[(hstryCnt) % 5][i][j].rgbtRed, pixelDataArr[(hstryCnt) % 5][i][j].rgbtGreen, pixelDataArr[(hstryCnt) % 5][i][j].rgbtBlue));
				//x, y ��ǥ �ٲ�� �˾Ƶμ�
			}
		}
		hstryCnt++;
		RedoCnt++;
		UndoCnt--;
		PrintOut(hdc, width + 292, height, SET);
		Reset_SetPixel_F;

		ReleaseDC(hWnd, hdc);
	}
}