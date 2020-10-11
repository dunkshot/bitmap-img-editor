#define _USE_MATH_DEFINES //math.h에 저장되있는 pi값을 쓰기위함
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "Editor_Basic.h"
#include "InsertImage.h"
#include "lettering.h"

#define DEG2RAD(n)      (M_PI * (n) / 180)     //각도를 라디안으로 바꿈

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildProc_button(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildProc_filter(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LPCTSTR lpszClass = TEXT("시빌워");
HINSTANCE g_hInst; //버튼에 쓰이는 hInstance
WNDCLASS WndClass;
BOOL bEllipse = TRUE;
HWND g_hWnd; //글로벌핸들

#define wndWidth 1152
#define wndHeight 648
#define originImg 0
#define historySize 5	//실행 취소는 5단계까지만 가능

FILE *file = NULL;
int width = 0, height = 0;
int hstryCnt = 1;		//history Count : 실행취소 위한 카운트 선언 // 작업 하나 할 때마다 카운트 ++
int UndoCnt = 0;		//실행취소 함수 호출 횟수를 카운트하여 5번 초과시 return
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
HDC Original_hdc; // 명령을 하기전의 원본 hdc

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
RgbTriple **pixelDataArr_Origin;	//원본이 저장될 곳 2차원
RgbTriple **pixelDataArr_Temp;		//필터 적용 시 막 쓸 거 2차원
RgbTriple **pixelDataArr_TempPrint;	//막쓴거 출력할거 거 2차원
RgbTriple ***pixelDataArr;			//3차원 배열 동적 할당

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
/* 동적할당 된 배열을 전부 free시켜줘야됨 */
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

	WndClass.cbClsExtra = 0;						//여분 메모리
	WndClass.cbWndExtra = 0;						//여분 메모리
	WndClass.hbrBackground = CreateSolidBrush(RGB(226, 226, 226)); //배경 색
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW); //마우스 커서 아이콘
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//윈도우 아이콘
	WndClass.hInstance = hInstance;					//핸들 인스턴스
	WndClass.lpfnWndProc = WndProc;					//프로시저 함수
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1); //@@@@@@@@@@@@메뉴
	WndClass.lpszClassName = lpszClass;	//클래스이름
	WndClass.style = CS_HREDRAW | CS_VREDRAW;		//윈도우 스타일
	RegisterClass(&WndClass);			//내가 사용할 클래스를 등록 (운영체제의 커널에 등록됨)
	//위치 기본값 : CW_USEDEFAULT
	
	// 필요한 부분을 변경하여 차일드 클래스를 등록 - 버튼 표시할 차일드 윈도우
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.lpfnWndProc = ChildProc_button;
	WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	WndClass.lpszClassName = TEXT("ChildCls_button");
	RegisterClass(&WndClass);

	// 필요한 부분을 변경하여 차일드 클래스를 등록 - 필터선택창 표시할 차일드 윈도우
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.lpfnWndProc = ChildProc_filter;
	WndClass.lpszClassName = TEXT("ChildCls_filter");
	WndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	RegisterClass(&WndClass);

	//메인윈도우 생성 - 위치 10*10, 크기 1152*648
	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10, wndWidth, wndHeight, 
		NULL, (HMENU)NULL, hInstance, NULL); 
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Global Handle
	g_hWnd = hWnd;

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message); //키보드 해석
		DispatchMessage(&Message); //메시지 전달
	}
	return (int)Message.wParam;
}


//버튼에 이미지 입히기 위한 핸들선언
HBITMAP hBitmap;
HWND hButton;
BOOL DrawPen = FALSE;
BOOL DoCrop = FALSE;
BOOL NowWorking = FALSE;

// 메인 윈도우 클래스
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
		hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP_quit)); // quit.bmp 비트맵을 읽어온다

		//버튼표시 영역 차일드 윈도우 생성
		CreateWindow(TEXT("ChildCls_button"), TEXT("버튼표시영역"), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, 292, 800, hwnd, (HMENU)NULL, g_hInst, NULL);

	case WM_COMMAND: //메뉴를 통한 메시지 처리
		switch (wParam)
		{
		case ID_Quit: //종료하기
			PostQuitMessage(0);
			break;

		case ID_Info:
			//MessageBox(hwnd, TEXT("iiiiiiiiiiiiiiiiinfo\n 여기엔 크레딧\n헤헤"), L"정보", NULL);
			break;
		}
		break;
	case WM_KEYDOWN:
		if (Rotateflag) {
			if (WorkingNow) {
				InvalidateRect(g_hWnd, &rt, TRUE);
				switch (wParam) {
					// 화살표키
				case VK_UP: nAngle -= 180;
					break;
				case VK_DOWN: nAngle += 180;
					break;
				case VK_LEFT: nAngle -= 90;
					break;
				case VK_RIGHT: nAngle += 90;
					break;
					// 숫자키
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
		if (!IsClipboardFormatAvailable(CF_BITMAP)) //클립보드조사
			return 0;
		if (OpenClipboard(hwnd)) { //이 윈도우가 클립보드를 독점하게한다.
			hBit = (HBITMAP)GetClipboardData(CF_BITMAP); //CF_BITMAP는 클립보드에 저장할 데이터 유형 메모리핸들을 hmem에 받아서 저장
			CloseClipboard(); //클립보드에 데이터를 넣은 후 클립보드를 닫는다.
			hDc = GetDC(hwnd); //윈도우에 비트맵을 그릴준비를 한다.
			hMemDc = CreateCompatibleDC(hDc);
			hOldBitmap = (HBITMAP)SelectObject(hMemDc, hBit);
			GetObject(hBit, sizeof(BITMAP), &bmp);
			BitBlt(hDc, 292, 0, bmp.bmWidth, bmp.bmHeight, hMemDc, 0, 0, SRCCOPY);//클립보드에 저장된 비트맵이미지를 출력
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

	//그리기 버튼을 누르면 draw함수를 실행함
	//키보드 메시지를 받아오기 위해 메인 윈도우에 위치시킴
	if (DrawPen == TRUE) {
						 Draw(g_hWnd, iMessage, wParam, lParam);
					 }

	//자르기 버튼을 누르면 crop함수를 실행함
	if (DoCrop == TRUE) {
						 Crop(g_hWnd, iMessage, wParam, lParam);
					 }

	/****************************문자삽입 작업에 필요한 처리************************************/
	if (iMessage == WM_CTLCOLOREDIT && (HWND)lParam == FontEdit && SelectedFont == TRUE) {

						 // 가능성(투명박스)
						 // Pettern Brush를 만들수있는가?

						 SetTextColor((HDC)wParam, CFT.rgbColors);
						 SetBkMode((HDC)wParam, OPAQUE);

						 return (LRESULT)GetStockObject(WHITE_BRUSH);

					 }

	DragAndMoveText(hwnd, iMessage, wParam, lParam);
	/********************************************************************************************/
	DragAndMoveImage(hwnd, iMessage, wParam, lParam);
	
	return DefWindowProc(hwnd, iMessage, wParam, lParam);   //처리되지 않은 메시지 반환
}



//각각의 버튼 비활성화를 위해 버튼을 식별할 핸들 선언
//Handle of Main Button, Handle of Filter Button (0,0)부터 (3,2)까지
HWND hMB00, hMB01, hMB02, hMB10, hMB11, hMB12, hMB20, hMB21, hMB22, hMB30, hMB31, hMB32,
		hFB00, hFB01, hFB02, hFB10, hFB11, hFB12, hFB20, hFB21, hFB22, hFB30, hFB31, hFB32;
HWND f_hwnd;
HWND b_hwnd;
HWND p_hwnd;
TCHAR sFilePathName[MAX_FILENAME_SIZE];		//문자공간을 만들어줌


// 차일드 클래스의 함수 - 버튼 표시 영역
LRESULT CALLBACK ChildProc_button(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	// 파일을 열기 위한 다이얼로그
	OPENFILENAME Ofn;		//오픈파일의 구조체
	//TCHAR sFilePathName[MAX_FILENAME_SIZE];		//문자공간을 만들어줌
	static TCHAR sFilter[] = L"비트맵 파일 \0*.bmp"; //필터값을 지정 : bmp만 걸러낸다
	
	//버튼 이미지를 씌우기위한 핸들 선언
	BITMAP bmp;
	HDC hdc;

	//이미지를 그려줄 구조체 선언
	static LPDRAWITEMSTRUCT lpdis;
	static HDC memDC;
	
	switch (iMessage)
	{
	case WM_CREATE:
	{
		// 1열 버튼생성
		hMB00 = CreateWindow(L"button", L"열기 child", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 9, 85, 85, hWnd, (HMENU)ID_LoadFile, g_hInst, NULL);
		hMB01 = CreateWindow(L"button", L"저장 child", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 10, 85, 85, hWnd, (HMENU)ID_SaveFile, g_hInst, NULL);
		hMB02 = CreateWindow(L"button", L"종료", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 10, 85, 85, hWnd, (HMENU)ID_Quit, g_hInst, NULL);

		// 2열
		hMB10 = CreateWindow(L"button", L"실행 취소", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 103, 85, 85, hWnd, (HMENU)ID_Undo, g_hInst, NULL);
		hMB11 = CreateWindow(L"button", L"원상 복구", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 103, 85, 85, hWnd, (HMENU)ID_Reset, g_hInst, NULL);
		hMB12 = CreateWindow(L"button", L"다시 실행", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 103, 85, 85, hWnd, (HMENU)ID_Redo, g_hInst, NULL);

		// 3열
		hMB20 = CreateWindow(L"button", L"그리기", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 197, 85, 85, hWnd, (HMENU)ID_Pen, g_hInst, NULL);
		hMB21 = CreateWindow(L"button", L"도형삽입", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 197, 85, 85, hWnd, (HMENU)ID_Shape, g_hInst, NULL);
		hMB22 = CreateWindow(L"button", L"텍스트삽입", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 197, 85, 85, hWnd, (HMENU)ID_Text, g_hInst, NULL);

		// 4열
		hMB30 = CreateWindow(L"button", L"회전", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 291, 85, 85, hWnd, (HMENU)ID_Rotate, g_hInst, NULL);
		hMB31 = CreateWindow(L"button", L"자르기", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 291, 85, 85, hWnd, (HMENU)ID_Crop, g_hInst, NULL);
		hMB32 = CreateWindow(L"button", L"효과", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
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
			if (lpdis->itemState & ODS_SELECTED) // 버튼이 눌린 상태(BUTTONDOWN)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else // 버튼이 UP되어 있는 상태(BUTTONUP)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_SaveFile:
			if (lpdis->itemState & ODS_SELECTED) // 버튼이 눌린 상태(BUTTONDOWN)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else // 버튼이 UP되어 있는 상태(BUTTONUP)
			{
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Quit:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Undo:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Reset:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Redo:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 85, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 0, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Pen:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Shape:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Text:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Rotate:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_Crop:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_FilterSelect:
			if (lpdis->itemState & ODS_SELECTED) { // 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 255, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 170, SRCCOPY);
				DeleteDC(memDC);
			}
			break;
		}
		break;

	case WM_COMMAND: //버튼을 통한 메시지 처리
		switch (wParam){
		case ID_LoadFile: 	// 불러오기
			memset(&Ofn, 0, sizeof(OPENFILENAME));		// 위에서 만든 오픈파일 구조체변수에 0->초기화
			Ofn.lStructSize = sizeof(OPENFILENAME);		// 현재구조체의 크기를 OPENFILE구조체만큼의 사이즈로 줌
			Ofn.hwndOwner = hWnd;						// 현재 다이얼로그의 부모핸들값을 지정
			Ofn.lpstrFilter = sFilter;					// 현재다이얼로그의 파일형식값을 sFilter값으로 지정
			Ofn.lpstrFile = sFilePathName;				// 선택한 파일의 위치와 이름값을 저장할 문자열변수(주소값)을 지정해줌
			Ofn.nMaxFile = MAX_FILENAME_SIZE;			// 선택하는 파일의 경로와 이름의 최대 크기를 지정
			Ofn.lpstrInitialDir = L"C:\\";				//다이얼로그의 초기값 위치를 C:\로 지정. 문자열에서 \사용시 \\나 \.을 사용

			if (GetOpenFileName(&Ofn) != 0)				//다이얼로그에서 열기취소 하지 않을 경우 
			{
				wsprintf(sFilePathName, L"%s", Ofn.lpstrFile);
				//선택한 파일의 경로,파일명을 sFilePathName에 저장

				hstryCnt = 1; //히스토리 카운트 초기화 : 다음 파일 불러와 필터 적용시 검은화면 나오는 버그 대응

				SetIndex(Ofn.lpstrFile);
				DoCreateMain(g_hWnd, Ofn.lpstrFile);

				DoPaint = FALSE;
				DoPaintMain(g_hWnd);

				DownScale(g_hWnd, Ofn.lpstrFile); //축소 이미지 출력
			}
			break;

		case ID_SaveFile: 	// 저장
			memset(&Ofn, 0, sizeof(OPENFILENAME));		// 위에서 만든 오픈파일 구조체변수에 0->초기화
			Ofn.lStructSize = sizeof(OPENFILENAME);		// 현재구조체의 크기를 OPENFILE구조체만큼의 사이즈로 줌
			Ofn.hwndOwner = hWnd;						// 현재 다이얼로그의 부모핸들값을 지정
			Ofn.lpstrFilter = sFilter;					// 현재다이얼로그의 파일형식값을 sFilter값으로 지정
			Ofn.lpstrFile = sFilePathName;				// 선택한 파일의 위치와 이름값을 저장할 문자열변수(주소값)을 지정해줌
			Ofn.nMaxFile = MAX_FILENAME_SIZE;			// 선택하는 파일의 경로와 이름의 최대 크기를 지정
			Ofn.lpstrInitialDir = L"C:\\";				//다이얼로그의 초기값 위치를 C:\로 지정. 문자열에서 \사용시 \\나 \.을 사용
			if (GetSaveFileName(&Ofn) != 0)				//다이얼로그에서 저장취소 하지 않을 경우 
			{
				//저장경로
				wsprintf(sFilePathName, L"%s", Ofn.lpstrFile);
				//저장 선택한 파일의 경로와 이름을 sFilePathName에 넣어줌. 위에서 이미 지정해주엇음
				saveFile(Ofn.lpstrFile);

				MessageBox(hWnd, sFilePathName, L"저장 성공", MB_OKCANCEL);
				//메세지박스를 통해 내가 선택한 파일의 경로와 이름을 출력
			}
			break;

		case ID_Undo:		// 실행취소
			Undo(g_hWnd);
			break;

		case ID_Reset:		// 처음으로 되돌리기
			DoReset(g_hWnd);
			break;

		case ID_Redo:		// 다시실행
			Redo(g_hWnd);
			break;
		case ID_Rotate: 	// 회전기능
			SetFocus(g_hWnd);
			Rotateflag = TRUE;
			WorkingNow = ~(WorkingNow);
			InvalidateRect(g_hWnd, &rt, TRUE);
			break;
		case ID_FilterSelect:	// 필터
			if (file == NULL)
				break;

			BeginFilterSelect(g_hWnd);

			//비활성화
			EnableWindow(hMB00, FALSE); EnableWindow(hMB01, FALSE); EnableWindow(hMB02, FALSE);
			EnableWindow(hMB10, FALSE); EnableWindow(hMB11, FALSE); EnableWindow(hMB12, FALSE);
			EnableWindow(hMB20, FALSE); EnableWindow(hMB21, FALSE); EnableWindow(hMB22, FALSE);
			EnableWindow(hMB30, FALSE); EnableWindow(hMB31, FALSE); EnableWindow(hMB32, FALSE);

			EnableWindow(b_hwnd, FALSE);

			//필터선택창 영역 차일드 윈도우 생성
			f_hwnd = CreateWindow(TEXT("ChildCls_filter"), TEXT("필터선택창"), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
				0, 0, 292, 800, hWnd, (HMENU)NULL, g_hInst, NULL);
			break;

		case ID_Pen:		// 그리기

			if (NowWorking == FALSE)
			{
				SetFocus(g_hWnd); //키보드입력을 받기 위해 메인 핸들로 포커스변경
				DrawPen = TRUE;
				NowWorking = TRUE;
			}
			break;

		case ID_Crop:		// 자르기
			if (NowWorking == FALSE)
			{
				SetFocus(g_hWnd); //키보드입력을 받기 위해 메인 핸들로 포커스변경 - 안해도될거같은데, 메인에도
				DoCrop = TRUE;
				NowWorking = TRUE;
			}
			break;
		case ID_Shape:		// 도형 삽입
			SetFocus(g_hWnd);
			InsertImageStart = TRUE;
			InvalidateRect(g_hWnd, NULL, FALSE);
			break;
		case ID_Text:		// 텍스트 삽입
			SetFocus(g_hWnd);
			ShowFontSelect(g_hWnd);
			break;
		case ID_Quit:		// 종료
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
// 차일드 클래스의 함수 - 필터선택창
LRESULT CALLBACK ChildProc_filter(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	BITMAP bmp; //버튼 이미지를 씌우기위한 핸들 선언

	//이미지를 그려줄 구조체 선언
	static LPDRAWITEMSTRUCT lpdis;
	static HDC memDC;

	switch (iMessage)
	{
	case WM_CREATE:
	{
		//hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP_quit)); // 비트맵 읽어온다
		isFilterSelectOn = TRUE; //필터선택창이 열렸냐? 응. 이거 열린동안 테마변경 못하도록

		// 1열 버튼생성
		hFB00 = CreateWindow(L"button", L"뒤로", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 9, 85, 85, hWnd, (HMENU)ID_CancelFilter, g_hInst, NULL);
		hFB01 = CreateWindow(L"button", L"필터리셋", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 10, 85, 85, hWnd, (HMENU)ID_ResetFilter, g_hInst, NULL);
		hFB01 = CreateWindow(L"button", L"적용", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 10, 85, 85, hWnd, (HMENU)ID_ApplyFilter, g_hInst, NULL);

		// 2열
		hFB10 = CreateWindow(L"button", L"밝기증가", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 103, 85, 85, hWnd, (HMENU)ID_F_BrightUp, g_hInst, NULL);
		hFB11 = CreateWindow(L"button", L"대비증가", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 103, 85, 85, hWnd, (HMENU)ID_F_ContrastUp, g_hInst, NULL);
		hFB12 = CreateWindow(L"button", L"채도증가", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 103, 85, 85, hWnd, (HMENU)ID_F_ChromaUp, g_hInst, NULL);

		// 3열
		hFB20 = CreateWindow(L"button", L"밝기감소", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			10, 197, 85, 85, hWnd, (HMENU)ID_F_BrightDown, g_hInst, NULL);
		hFB21 = CreateWindow(L"button", L"대비감소", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 197, 85, 85, hWnd, (HMENU)ID_F_ContrastDown, g_hInst, NULL);
		hFB22 = CreateWindow(L"button", L"채도감소", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 197, 85, 85, hWnd, (HMENU)ID_F_ChromaDown, g_hInst, NULL);

		// 4열
		hFB30 = CreateWindow(L"button", L"그레이", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			10, 291, 85, 85, hWnd, (HMENU)ID_F_Gray, g_hInst, NULL);
		hFB31 = CreateWindow(L"button", L"세피아", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 291, 85, 85, hWnd, (HMENU)ID_F_Sepia, g_hInst, NULL);
		hFB32 = CreateWindow(L"button", L"반전", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 291, 85, 85, hWnd, (HMENU)ID_F_Reverse, g_hInst, NULL);

		// 5열
		CreateWindow(L"button", L"메디안", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 385, 85, 85, hWnd, (HMENU)ID_F_Median, g_hInst, NULL);
		CreateWindow(L"button", L"블러", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 385, 85, 85, hWnd, (HMENU)ID_F_Blur, g_hInst, NULL);
		CreateWindow(L"button", L"가우시안", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 385, 85, 85, hWnd, (HMENU)ID_F_Gaussian, g_hInst, NULL);

		// 6열
		CreateWindow(L"button", L"샤프닝", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			9, 479, 85, 85, hWnd, (HMENU)ID_F_Sharpening, g_hInst, NULL);
		CreateWindow(L"button", L"엣지", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			103, 479, 85, 85, hWnd, (HMENU)ID_F_Edge, g_hInst, NULL);
		CreateWindow(L"button", L"소벨", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			195, 479, 85, 85, hWnd, (HMENU)ID_F_Sobel, g_hInst, NULL);

		break;
	}

	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		switch (wParam)
		{
		case ID_CancelFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_ResetFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_ApplyFilter:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_BrightUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ContrastUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ChromaUp:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 425, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 340, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_BrightDown:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ContrastDown:
		{
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		}
		break;

		case ID_F_ChromaDown:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 510, SRCCOPY); 
				DeleteDC(memDC);
			}
		break;

		case ID_F_Gray:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 510, SRCCOPY);
				DeleteDC(memDC);
			}
		break;

		case ID_F_Sepia:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 340, 510, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Reverse:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼ㄹ이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 595, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 510, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Median:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 0, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Blur:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 85, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Gaussian:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 170, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Sharpening:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 255, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Edge:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 345, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 345, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;

		case ID_F_Sobel:
			if (lpdis->itemState & ODS_SELECTED) {// 버튼이 눌린 상태(BUTTONDOWN)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 765, SRCCOPY);
				DeleteDC(memDC);
			}
			else {// 버튼이 UP되어 있는 상태(BUTTONUP)
				memDC = CreateCompatibleDC(lpdis->hDC);
				SelectObject(memDC, hBitmap);
				BitBlt(lpdis->hDC, 0, 0, 85, 85, memDC, 425, 680, SRCCOPY);
				DeleteDC(memDC);
			}
			break;


		}

		break;

	case WM_COMMAND: //버튼을 통한 메시지 처리
		switch (wParam)
		{
		case ID_CancelFilter:

			//필터를 저장했다면 그냥 끄고
			if (isFilterSave == TRUE) {
				CancelFilter(g_hWnd);		//필터 취소
				isFilterSelectOn = FALSE;	//필터선택창 꺼집니다

				//선택창 닫으면 비활성화된 버튼들 다시 활성화
				EnableWindow(hMB00, TRUE); EnableWindow(hMB01, TRUE); EnableWindow(hMB02, TRUE);
				EnableWindow(hMB10, TRUE); EnableWindow(hMB11, TRUE); EnableWindow(hMB12, TRUE);
				EnableWindow(hMB20, TRUE); EnableWindow(hMB21, TRUE); EnableWindow(hMB22, TRUE);
				EnableWindow(hMB30, TRUE); EnableWindow(hMB31, TRUE); EnableWindow(hMB32, TRUE);

				EnableWindow(hFB00, FALSE); EnableWindow(hFB01, FALSE); EnableWindow(hFB02, FALSE);
				EnableWindow(hFB10, FALSE); EnableWindow(hFB11, FALSE); EnableWindow(hFB12, FALSE);
				EnableWindow(hFB20, FALSE); EnableWindow(hFB21, FALSE); EnableWindow(hFB22, FALSE);
				EnableWindow(hFB30, FALSE); EnableWindow(hFB31, FALSE); EnableWindow(hFB32, FALSE);

				DestroyWindow(f_hwnd);
				DownScale(g_hWnd, sFilePathName); //메인창 돌아갈 때 원본이미지 다시 띄움
				break;
			}

			//필터를 저장하지 않았다면 물어봐
			else { 
				if (MessageBox(hWnd, TEXT("변경사항을 적용하지 않고 이전 상태로 돌아갑니다.\n현재 화면을 반영하려면 적용버튼을 눌러주세요.\n돌아가시겠습니까?"),
					TEXT("알림"), MB_YESNO) == IDYES)
				{
					CancelFilter(g_hWnd);		//필터 취소
					isFilterSelectOn = FALSE;	//필터선택창 꺼집니다

					//선택창 닫으면 비활성화된 버튼들 다시 활성화
					EnableWindow(hMB00, TRUE); EnableWindow(hMB01, TRUE); EnableWindow(hMB02, TRUE);
					EnableWindow(hMB10, TRUE); EnableWindow(hMB11, TRUE); EnableWindow(hMB12, TRUE);
					EnableWindow(hMB20, TRUE); EnableWindow(hMB21, TRUE); EnableWindow(hMB22, TRUE);
					EnableWindow(hMB30, TRUE); EnableWindow(hMB31, TRUE); EnableWindow(hMB32, TRUE);

					EnableWindow(hFB00, FALSE); EnableWindow(hFB01, FALSE); EnableWindow(hFB02, FALSE);
					EnableWindow(hFB10, FALSE); EnableWindow(hFB11, FALSE); EnableWindow(hFB12, FALSE);
					EnableWindow(hFB20, FALSE); EnableWindow(hFB21, FALSE); EnableWindow(hFB22, FALSE);
					EnableWindow(hFB30, FALSE); EnableWindow(hFB31, FALSE); EnableWindow(hFB32, FALSE);

					DestroyWindow(f_hwnd);
					DownScale(g_hWnd, sFilePathName); //메인창 돌아갈 때 원본이미지 다시 띄움
					break;
				}
				else
					break;
			}
			

		case ID_ResetFilter:
			FilterReset(g_hWnd);
			break;

		case ID_ApplyFilter:
			//ApplyFilter(g_hWnd);		//필터 적용
			CaptureNow(g_hWnd);
			DC_Copy(hWnd, Copy_hdc, &Original_hdc);
			isFilterSave = TRUE; //필터 저장 완료
			isFilterSelectOn = FALSE;	//필터선택창 꺼집니다
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
		//MessageBox(hWnd, TEXT("이미지가 로드되지 않았습니다. \n파일을 먼저 열주세요"), TEXT("오류"), MB_OK);
		return;
	}
	length = sqrt(pow(width, 2) / 4 + pow(height, 2) / 4);  //bitmap의 회전 반경 반지름

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			x = ((length - width / 2) + i) - length;  //bitmap의 중간 좌표를 원점으로 할때의 x축 
			y = (length - height / 2) + (height - j) - length;   //bitmap의 중간 좌표를 원점으로 할때의 y축
			cx = x*cos(DEG2RAD(nAagle)) - y*sin(DEG2RAD(nAagle)) + length;  //회전을 시킨 x축의 좌표
			cy = x*sin(DEG2RAD(nAagle)) + y*cos(DEG2RAD(nAagle)) + length;  //회전을 시킨 y축의 좌표
			sx = (x + 0.5)*cos(DEG2RAD(nAagle)) - (y + 0.5)*sin(DEG2RAD(nAagle)) + length;  //회전할떄 만들어지는 구멍들을 보강하기 위해 새로 출력하기위해 값을 다르게한 x축
			sy = (x + 0.5)*sin(DEG2RAD(nAagle)) + (y + 0.5)*cos(DEG2RAD(nAagle)) + length;   //회전할떄 만들어지는 구멍들을 보강하기 위해 새로 출력하기위해 값을 다르게한 y축
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

//필터선택을 시작할때 호출 : 임시배열에 선택화면 진입 직전의 화면을 복사
void BeginFilterSelect(HWND hWnd) 
{
	//5번째 실행에서 멈추는 버그 대응 : hstryCnt%5-1 이 음수가 되는 경우이므로 예외처리
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

//필터리셋 : 필터 선택창 진입 직전의 화면으로 돌아감
void FilterReset(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	//필터선택 진입 직전 이미지 띄움
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

//필터적용
void ApplyFilter(HWND hWnd)
{
	//지금 화면에 있는 걸 메인 저장소 pixelDataArr에 저장
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr[hstryCnt%5][i][j].rgbtBlue = pixelDataArr_TempPrint[i][j].rgbtBlue;
			pixelDataArr[hstryCnt%5][i][j].rgbtGreen = pixelDataArr_TempPrint[i][j].rgbtGreen;
			pixelDataArr[hstryCnt%5][i][j].rgbtRed = pixelDataArr_TempPrint[i][j].rgbtRed;
		}
	}
	hstryCnt++; //메인 데이터 배열에 저장했으니 카운트++ - 실행취소위해
	BeginFilterSelect(g_hWnd); //필터 적용 사항을 반영하기 위해 temp에 다시 복사 (다중 필터 적용시)

}

//취소하고 나가기
void CancelFilter(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);

	//갖고 놀았던거 초기화
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

	//필터선택 진입 직전 이미지 띄움
	//hstryCnt%5-1이 음수가 되어 액세스 위반 되는 경우 예외처리
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


//파일헤더, 인포헤더, 색상정보 복사
void SetIndex(TCHAR *FilePath)
{
	FILE *fr = _wfopen(FilePath, L"rb,ccs=UNICODE"); //유니코드버전 fopen
	if (fr == NULL)
	{
		printf("xx\n");
	}

	// 파일헤더 읽어서 저장
	fread(&bmpfh.bfType, 1, sizeof(bmpfh.bfType), fr);
	fread(&bmpfh.bfSize, 1, sizeof(bmpfh.bfSize), fr);
	fread(&bmpfh.bfReserved1, 1, sizeof(bmpfh.bfReserved1), fr);
	fread(&bmpfh.bfReserved2, 1, sizeof(bmpfh.bfReserved2), fr);
	fread(&bmpfh.bfOffBits, 1, sizeof(unsigned long), fr);

	// 인포헤더 읽어서 저장
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

	pixelData = (RgbTriple*)malloc(bmpih.biSizeImage);     // 이미지 공간 할당
	fread(pixelData, 1, bmpih.biSizeImage, fr);            // 이미지 데이터 읽어들임

	fclose(fr);

}

void saveFile(TCHAR *FilePath)
{
	FILE *fw = _wfopen(FilePath, L"wb,ccs=UNICODE"); //유니코드버전 fopen
	if (file == NULL) {
		return;
	}

	//읽은걸 그대로 출력
	// 파일헤더 저장 -- 정보가 밀려서 저장되어서 헤더는 멤버 분리시켜 저장했음
	fwrite(&bmpfh.bfType, sizeof(bmpfh.bfType), 1, fw);
	fwrite(&bmpfh.bfSize, sizeof(bmpfh.bfSize), 1, fw);
	fwrite(&bmpfh.bfReserved1, sizeof(bmpfh.bfReserved1), 1, fw);
	fwrite(&bmpfh.bfReserved2, sizeof(bmpfh.bfReserved2), 1, fw);
	fwrite(&bmpfh.bfOffBits, sizeof(bmpfh.bfOffBits), 1, fw);

	fwrite(&bmpih, sizeof(BITMAPINFOHEADER), 1, fw); // 인포헤더 저장
	
	//픽셀저장 - 3차원배열 버전 @@@@@@@@@@@@@@@@@@@@@좀더 손봐야함 hstryCnt개념 정립후 다시봐
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
	file = _wfopen(FilePath, L"rb,ccs=UNICODE"); //유니코드버전 fopen

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

		//색상정보 복사를 위한 반복문 - width,height값을 알고 실행해야 하기 때문에 여기에 위치함
		//history 배열 3차원 동적 할당
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

		//초기화
		for (int i = 0; i < historySize; i++) {
			for (int j = 0; j < height; j++) {
				for (int k = 0; k < width; k++) {
					pixelDataArr[i][j][k].rgbtBlue = 0;
					pixelDataArr[i][j][k].rgbtGreen = 0;
					pixelDataArr[i][j][k].rgbtRed = 0;
				}
			}
		}

		//3차원배열 첫 큰 덩어리와 Origin배열에 원본 복사 (x,y 좌표 구분 위해)
		//pixelDataArr[0][x][y]에 원본 이미지가 들어감
		//변경사항이 생길 때 마다 1차원 변수(pixelDataArr[A][x][y]에서 [A])가 +1 증가하며 x,y 데이터 기록
		//같은 정보를 두 가지 배열에 저장하는 이유는 원본데이터의 손실을 막기위함
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

		//윈도우 크기 변경
		SetWindowPos(hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, width+292+17, height+60, SWP_NOMOVE | SWP_SHOWWINDOW);
		SetRect(&rt, 292, 0, width + 292 + 17, height + 60);
		return; //wndWidth, wndHeight
	}
	else {
		MessageBeep(MB_OKCANCEL);
	}
}

//버튼영역 축소이미지 출력 (미리보기)
void DownScale(HWND hWnd, TCHAR *FilePath)
{
	PAINTSTRUCT ps;
	HBITMAP hImage, hOldBitmap;
	//HDC hdc = BeginPaint(hWnd, &ps); //@@@@@@@@@@@@@@@@@@@@@@@이거 두개 차이가 뭐라고?
	HDC hdc = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hdc);

	hImage = (HBITMAP)LoadImage(NULL, FilePath, IMAGE_BITMAP
		, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);
	
	SetStretchBltMode(hdc, HALFTONE); //색상손실 보정
	StretchBlt(hdc, 9, 385, 270, 185, hMemDC, 0, 0, width, height, SRCCOPY); 
	//(9,385)위치에 270*185의 크기로 원본의 (0,0)~(width,height) 영역을 복사

	SelectObject(hMemDC, hOldBitmap);
	DeleteObject(hImage);
	DeleteDC(hMemDC);
	ReleaseDC(hWnd, hdc);
}

//한별이 DoPaint 부스터
void DoPaintMain0(HWND hWnd, TCHAR *FilePath)
{
	HBITMAP hImage, hOldBitmap;
	PAINTSTRUCT ps;
	//HDC hdc = BeginPaint(hWnd, &ps);
	HDC hdc = GetDC(hWnd);
	HDC hMemDC = CreateCompatibleDC(hdc); // For the created Memory DC

	// 첫번째 인수 NULL : 독립리소스 사용
	hImage = (HBITMAP)LoadImage(NULL, FilePath, IMAGE_BITMAP
		, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	// hndle 저장
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hImage);

	StretchBlt(hdc, 400, 100, 273, 179, hMemDC, 0, 0, width, height, SRCCOPY);
	//BitBlt(hdc, 292, 0, width, height, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOldBitmap); // hImage 선택을 해제하기 위해 hOldBitmap을 선택한다
	DeleteObject(hImage); // 선택 해제된 비트맵을 제거한다
	DeleteDC(hMemDC); // 메모리 DC를 제거한다

	//EndPaint(hWnd, &ps);
	ReleaseDC(hWnd, hdc);
}

// 모듈결합용 테스트
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

		//원화면 출력
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

//이게 테스트필터
void DoTest(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		//MessageBox(hWnd, TEXT("이미지가 로드되지 않았습니다. \n파일을 먼저 열주세요"), TEXT("오류"), MB_OK);
		return;
	}

	//3차원배열 색상변화
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixelDataArr_TempPrint[i][j].rgbtRed = 255 - (pixelDataArr_Temp[i][j].rgbtRed);
			pixelDataArr_TempPrint[i][j].rgbtGreen = 255 - (pixelDataArr_Temp[i][j].rgbtGreen);
			pixelDataArr_TempPrint[i][j].rgbtBlue = 255 - (pixelDataArr_Temp[i][j].rgbtBlue);

			SetPixel_F(hWnd, hdc, 292 + j, height - i,
				RGB(pixelDataArr_TempPrint[i][j].rgbtRed, pixelDataArr_TempPrint[i][j].rgbtGreen, pixelDataArr_TempPrint[i][j].rgbtBlue));
			//x, y 좌표 바뀐거 알아두셈
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

			if (tr >= 255) // 오버플로우 처리
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

			if (tr < 0) // 언더플로우 처리
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

void Contrast_Up_Filter(HWND hWnd) // 명암대비 증가
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue를 통해 받은 8비트 자료를 저장 
	COLORREF color; // GetPixel을 통해 받은 32비트 자료를 저장

	for (int j = 0; j < height; j++) // 비트맵 전체 픽셀의 데이터를 받고 출력하기 위한 루프
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // 한 픽셀씩 데이터를 받아온다.

			tr = GetRValue(color); // 픽셀의 Red값
			tg = GetGValue(color); // 픽셀의 Green값
			tb = GetBValue(color); // 픽셀의 Blue값

			tr = tr * 1.1; // 같은 값을 R, G, B에 동일하게 곱한다.
			tg = tg * 1.1;
			tb = tb * 1.1;

			if (tr >= 255) // 오버플로우 처리
				tr = 255;
			if (tg >= 255)
				tg = 255;
			if (tb >= 255)
				tb = 255;

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // 한 픽셀의 결과값을 출력한다

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC영역에 있는 화면을 한번에 출력한다.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}
void Contrast_Down_Filter(HWND hWnd) // 명암대비 감소
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue를 통해 받은 8비트 자료를 저장 
	COLORREF color; // GetPixel을 통해 받은 32비트 자료를 저장

	for (int j = 0; j < height; j++) // 비트맵 전체 픽셀의 데이터를 받고 출력하기 위한 루프
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // 한 픽셀씩 데이터를 받아온다.

			tr = GetRValue(color); // 픽셀의 Red값
			tg = GetGValue(color); // 픽셀의 Green값
			tb = GetBValue(color); // 픽셀의 Blue값

			tr = tr * 0.9; // 같은 값을 R, G, B에 동일하게 나눈다.
			tg = tg * 0.9;
			tb = tb * 0.9;

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // 한 픽셀의 결과값을 출력한다

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC영역에 있는 화면을 한번에 출력한다.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}


void Chroma_Up_Filter(HWND hWnd) // 채도 증가
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue를 통해 받은 8비트 자료를 저장 
	double max_color; // R,G,B중 가장 큰 색상을 저장하는 변수 
	COLORREF color; // GetPixel을 통해 받은 32비트 자료를 저장

	for (int j = 0; j < height; j++) // 비트맵 전체 픽셀의 데이터를 받고 출력하기 위한 루프
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // 한 픽셀씩 데이터를 받아온다.

			tr = GetRValue(color); // 픽셀의 Red값
			tg = GetGValue(color); // 픽셀의 Green값
			tb = GetBValue(color); // 픽셀의 Blue값

			max_color = tr; // 각  R,G,B중 가장 큰 색상이 저장된다.
			if (tg > max_color)
				max_color = tg;
			else if (tb > max_color)
				max_color = tb;

			if (max_color == tr) // 해당 픽셀에서 가장 큰 색상을 대상으로
								 // 값을 증가시킨다.
			{
				tr = (tr * 1.03);
				if (tr >= 255) // 오버플로우 처리
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

			SetPixel_F(hWnd, hdc, i + 292, j, RGB(tr, tg, tb)); // 한 픽셀의 결과값을 출력한다

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC영역에 있는 화면을 한번에 출력한다.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}
void Chroma_Down_Filter(HWND hWnd) // 채도 감소
{
	HDC hdc = GetDC(hWnd);
	double tr, tg, tb; // GetValue를 통해 받은 8비트 자료를 저장 
	double max_color; // R,G,B중 가장 큰 색상을 저장하는 변수 
	COLORREF color; // GetPixel을 통해 받은 32비트 자료를 저장

	for (int j = 0; j < height; j++) // 비트맵 전체 픽셀의 데이터를 받고 출력하기 위한 루프
	{
		for (int i = 0; i < width; i++)
		{
			color = GetPixel_F(hWnd, hdc, i + 292, j); // 한 픽셀씩 데이터를 받아온다.

			tr = GetRValue(color); // 픽셀의 Red값
			tg = GetGValue(color); // 픽셀의 Green값
			tb = GetBValue(color); // 픽셀의 Blue값

			max_color = tr; // 각  R,G,B중 가장 큰 색상이 저장된다.
			if (tg > max_color)
				max_color = tg;
			else if (tb > max_color)
				max_color = tb;

			if (max_color == tr) // 해당 픽셀에서 가장 큰 색상을 대상으로 값을 감소시킨다.
				tr = tr * 0.9;
			else if (max_color == tg)
				tg = tg * 0.9;
			else if (max_color == tb)
				tb = tb * 0.9;

			SetPixel_F(hWnd, hdc, i+292, j, RGB(tr, tg, tb)); // 한 픽셀의 결과값을 출력한다

			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = tr;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = tg;
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = tb;
		}
	}

	PrintOut(hdc, width, height, SET); // DC영역에 있는 화면을 한번에 출력한다.
	Reset_SetPixel_F();
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}

void Gray_Filter(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	//3차원배열 색상변화
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

	//3차원배열 색상변화
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
unsigned int Median_Sort(unsigned int N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size픽셀의 중앙값을 반환
{
	int i, j, k, l;
	unsigned int temp = 0; // 정렬에 쓰일 임시변수

	for (i = 0; i < Mask_Size; i++) // 2차원 배열의 버블 정렬
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
	return N[Mask_Size / 2][Mask_Size / 2];   // 중앙값을 반환
}
void Median_Filter(HWND hWnd) // 
{
	HDC hdc = GetDC(hWnd);
	
	COLORREF color;
	unsigned int N[Mask_Size][Mask_Size]; // 마스크의 사이즈를 결정과 선언

	for (int j = 0; j < (height - 2); j++) // sort를 통해 출력되는 픽셀을 전체화면에 출력하기 위한 루프
	{
		for (int i = 0; i < (width - 2); i++)
		{
			for (int l = 0; l <= 2; l++) // 마스크에 입력되는 픽셀을 위한 루프
			{
				for (int k = 0; k <= 2; k++)
				{
					color = GetPixel_F(hWnd, hdc, i + 292 + k, j + l); // 32비트 정수형으로 반환되는 GetPixel_F
					N[l][k] = color;
				}
			}

			SetPixel_F(hWnd, hdc, i +292 + 1, j + 1, Median_Sort(N)); // Median_Sort(N)의 반환 값은 GetPixel을 통해 얻은 값중 하나이다.
																 // 중간값이 2차원 배열의 인덱스를 통해 반환된다.
			
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtRed = GetRValue(Median_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtGreen = GetGValue(Median_Sort(N));
			pixelDataArr_TempPrint[height - 1 - j][i].rgbtBlue = GetBValue(Median_Sort(N));

		}
	}

	PrintOut(hdc, width, height, SET);
	Reset_GetPixel_F();
	ReleaseDC(hWnd, hdc);
}


COLORREF Blur_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size픽셀의 평균값을 반환
{
	int i, j;
	COLORREF sum = 0;
	unsigned char nR, nG, nB; // GetRValue를 통한 R,G,B 값의 저장
	int Rsum = 0, Gsum = 0, Bsum = 0; // nR, nG, nB의 평균을 구하기 위한 변수
									  // 더하는 과정에서 255을 넘어서기 때문에 int형이 필요

	for (i = 0; i < Mask_Size; i++) // 컨벌루션을 위한 루프
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

	Rsum = Rsum / (Mask_Size * Mask_Size); // 정규화를 위한 연산
	Gsum = Gsum / (Mask_Size * Mask_Size);
	Bsum = Bsum / (Mask_Size * Mask_Size);

	nR = Rsum; // 평균값을 0 ~ 255 색상정보에 맞게 저장
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // 평균 마스크의 컨벌루션 값을 반환
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

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
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

COLORREF Sharpening_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size픽셀의 가중치 평균값을 반환
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

	if (nR >= 255) // 오버플로우 처리
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

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
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

COLORREF Edge_Sort(COLORREF N[Mask_Size][Mask_Size]) // Mask_Size * Mask_Size픽셀의 가중치 평균값을 반환
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

	if (nR >= 255) // 오버플로우 처리
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

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
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
	int nR, nG, nB; // 디폴트 unsigned char (0.255) 
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

	nR = Rsum; // 정규화 : 가중치 합 1
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
}
COLORREF Vertic_Sobel_Sort(COLORREF N[Mask_Size][Mask_Size])
{
	int i, j;
	int nR, nG, nB; // 디폴트 unsigned char (0.255) 
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

	nR = Rsum; // 정규화 : 가중치 합 1
	nG = Gsum;
	nB = Bsum;

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
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
	int nR, nG, nB; // 디폴트 unsigned char (0.255) 
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

				Rsum = Rsum + (nR * (-1)); // 오버플로우 
				Gsum = Gsum + (nG * (-1)); // 
				Bsum = Bsum + (nB * (-1)); // 
			}
		}
	}

	nR = Rsum; // 정규화 : 가중치 합 1
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

	return RGB(nR, nG, nB); // 마스크의 가중치 컨벌루션 값을 반환
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

//그리기
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
			sx = LOWORD(lParam); //추가	
			sy = HIWORD(lParam); //추가
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

//현재 화면을 arr에 저장한다
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

	hstryCnt++; //실행취소, 다시실행, 필터적용
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


		if (MessageBox(hWnd, TEXT("변경 사항을 적용하시겠습니까?"),TEXT("알림"), MB_YESNO) == IDYES)
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

			//pixelDataArr저장, 히스토리카운트
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

//실행취소
void Undo(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		MessageBox(hWnd, TEXT("이미지가 로드되지 않았습니다. \n파일을 먼저 열주세요"), TEXT("경고"), MB_OK);
		return;
	}

	//히스토리 카운트가 5까지 올라가지 않았을 때 실행취소를 계속 누를경우
	if (hstryCnt - 2 < 0) {
		MessageBox(hWnd, TEXT("더이상 뒤로 갈 수 없습니다."), TEXT("알림"), MB_OK);
		return;
	}
	
	//히스토리 카운트가 5를 넘었을 때 실행취소를 계속 누를 경우
	if (UndoCnt < historySize) { //실행취소 함수의 호출 횟수가 다섯번 이하일때

			//직전 이미지 출력
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtRed, pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtGreen, pixelDataArr[(hstryCnt - 2) % 5][i][j].rgbtBlue));
				//x, y 좌표 바뀐거 알아두셈
			}
		}
		hstryCnt--;
		UndoCnt++;
		RedoCnt--;
		PrintOut(hdc, width + 292, height, SET);
		Reset_SetPixel_F;
		ReleaseDC(hWnd, hdc);
	}
	else { //실행취소 함수의 호출 횟수가 다섯번 초과일때
		MessageBox(hWnd, TEXT("실행 취소는 최대 다섯 단계까지 가능합니다."), TEXT("알림"), MB_OK);
		return;
	}

}

//원상복구
void DoReset(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		return;
	}

	//origin배열로부터 pixelDatArr[0][][]에 원본이미지 복사 & 출력
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			pixelDataArr[0][j][k].rgbtRed = pixelDataArr_Origin[j][k].rgbtRed;
			pixelDataArr[0][j][k].rgbtGreen = pixelDataArr_Origin[j][k].rgbtGreen;
			pixelDataArr[0][j][k].rgbtBlue = pixelDataArr_Origin[j][k].rgbtBlue;

			SetPixel_F(hWnd, hdc, 292 + k, height - j,
				RGB(pixelDataArr[0][j][k].rgbtRed, pixelDataArr[0][j][k].rgbtGreen, pixelDataArr[0][j][k].rgbtBlue));
			//x, y 좌표 바뀐거 알아두셈
		}
	}

	hstryCnt = 1;
	PrintOut(hdc, width + 292, height, SET);
	Reset_SetPixel_F;
	ReleaseDC(hWnd, hdc);
}

//다시실행
void Redo(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);

	if (file == NULL) {
		MessageBox(hWnd, TEXT("이미지가 로드되지 않았습니다. \n파일을 먼저 열주세요"), TEXT("오류"), MB_OK);
		return;
	}

	//다시 실행 횟수는 실행취소 횟수를 넘길 수 없다
	if (RedoCnt >= UndoCnt) {
		MessageBox(hWnd, TEXT("더이상 앞으로 갈 수 없습니다."), TEXT("알림"), MB_OK);
		return;
	}

	else {
		//현 상태의 다음 이미지 출력
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{

				SetPixel_F(hWnd, hdc, 292 + j, height - i,
					RGB(pixelDataArr[(hstryCnt) % 5][i][j].rgbtRed, pixelDataArr[(hstryCnt) % 5][i][j].rgbtGreen, pixelDataArr[(hstryCnt) % 5][i][j].rgbtBlue));
				//x, y 좌표 바뀐거 알아두셈
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