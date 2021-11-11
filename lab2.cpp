#include <windows.h>;
#include <stdio.h>
#include <fstream>
#include <list>
#include "strsafe.h"
using namespace std;


#define SCROLL_SPEED 30
#define BOLD_MENU_ID 1
#define COURSIVE_MENU_ID 2
#define UNDERLINE_MENU_ID 3

    boolean InitStringMatrix();
    void DrawTextBlock(HDC hdc, int left, int top, int width, int height, int raw, int column);
    bool ChangeColor(HWND hWnd, bool parametr, int MenuId);
    string getLongestStringInRow(int currentLine);
    void DrawTable(HDC hdc, int widht, int height, int borderSize);
    int GetBlockHeight(HDC hdc, int currentLine, int width, int borderSize);
    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    byte currentColorWindow = COLOR_3DSHADOW;
    HWND hWnd;
    HDC hdc;
    RECT client_rect;
    SCROLLINFO scrInfo;
    int TableHeight;
    const int rows = 10, columns = 5;
    int top = 0, bottom = 0; // для скролла
    string stringMatrix[rows][columns];
    int width = 640;
    int height = 480;
    int isBold, isCoursive, isUnderline;
    HMENU menu;
    PLOGFONT plf;
    size_t pcch;
    TCHAR lpszRotate[35] = TEXT("String to be rotated. String to be");

    int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
    {
        if (!InitStringMatrix()) {
            return -1;
        }
        WNDCLASSEX wcex;
        MSG msg;

        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_DBLCLKS;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(currentColorWindow);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = L"TextTable";
        wcex.hIconSm = wcex.hIcon;
        RegisterClassEx(&wcex);

        RECT screen_rect;
        GetWindowRect(GetDesktopWindow(), &screen_rect); // разрешение экрана
        int leftTopX = screen_rect.right / 2 - width / 2;
        int leftTopY = screen_rect.bottom / 2 - height / 2;

        hWnd = CreateWindow(L"TextTable", L"TextTable",
            WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_VISIBLE | WS_BORDER, leftTopX, leftTopY,
            width, height, NULL, NULL, hInstance, NULL);
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        menu = CreateMenu();
        HMENU hStyle = CreateMenu();

        AppendMenu(menu, MF_BYCOMMAND | MF_UNCHECKED, BOLD_MENU_ID, L"BOLD");
       
        SetMenu(hWnd, menu);


        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return (int)msg.wParam;
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        int wheelDelta;
        int currentPos;
        int step = 0;

        switch (message)
        {

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {            
            case BOLD_MENU_ID:
                isBold = ChangeColor(hWnd, isBold, BOLD_MENU_ID);
                InvalidateRect(hWnd, NULL, TRUE);
                PAINTSTRUCT ps;
                hdc = BeginPaint(hWnd, &ps);
                DrawTable(hdc, width, height, 3);
                EndPaint(hWnd, &ps);
                break;
            }
            break;
        case WM_SIZE:
            width = LOWORD(lParam);
            height = HIWORD(lParam);
            InvalidateRect(hWnd, NULL, TRUE);
            top = 0;

            scrInfo.cbSize = sizeof(SCROLLINFO);
            scrInfo.nPage = HIWORD(lParam); //размер страницы устанавливаем равным высоте окна
            scrInfo.nMin = 0; //диапазон прокрутки устанавливаем по размеру содержимого
            if (height > TableHeight) {
                scrInfo.nMax = 0;
            }
            else {
                scrInfo.nMax = TableHeight;
            }
            scrInfo.fMask = SIF_RANGE | SIF_PAGE; //применяем новые параметры
            SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);

            break;
        case WM_MOUSEWHEEL:
            wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (wheelDelta > 0)
                SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, NULL);
            if (wheelDelta < 0)
                SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, NULL);
            break;
        case WM_VSCROLL:

            scrInfo.cbSize = sizeof(SCROLLINFO);
            scrInfo.fMask = SIF_ALL; //получаем текущие параметры scrollbar-а
            GetScrollInfo(hWnd, SB_VERT, &scrInfo);
            currentPos = scrInfo.nPos; //запоминаем текущее положение содержимого
            switch (LOWORD(wParam)) { //определяем действие пользователя и изменяем положение
            case SB_LINEUP: //клик на стрелку вверх
                scrInfo.nPos -= SCROLL_SPEED;
                step = SCROLL_SPEED;
                break;
            case SB_LINEDOWN: //клик на стрелку вниз 
                scrInfo.nPos += SCROLL_SPEED;
                step = -SCROLL_SPEED;
                break;
            case SB_THUMBTRACK: //перетаскивание ползунка
                scrInfo.nPos = scrInfo.nTrackPos;
                step = currentPos - scrInfo.nPos;
                break;
            default: return 0; //все прочие действия (например нажатие PageUp/PageDown) игнорируем
            }

            scrInfo.fMask = SIF_POS; //пробуем применить новое положение
            SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
            GetScrollInfo(hWnd, SB_VERT, &scrInfo);

            top += step;
            if (((top != SCROLL_SPEED) && (step > 0)) || ((step < 0) && (bottom >= height)))
            {
                InvalidateRect(hWnd, NULL, TRUE);
                PAINTSTRUCT ps;
                hdc = BeginPaint(hWnd, &ps);
                DrawTable(hdc, width, height, 3);
                EndPaint(hWnd, &ps);
            }
            else {
                top -= step;
            }
            break;
        case WM_PAINT:
            PAINTSTRUCT ps;
            hdc = BeginPaint(hWnd, &ps);
            DrawTable(hdc, width, height, 3);
            EndPaint(hWnd, &ps);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    boolean InitStringMatrix() {
        string str;
        char letter;
        int countWord = 0;
        ifstream file("text.txt");
        if (file.is_open()) {
            while (!file.eof())
            {
                letter = file.get();
                if (letter == ' ') {
                    countWord++;
                }
                str += letter;
            }
            str = str.substr(0, str.length() - 1);
            file.close();
        }
        else {
            return false;
        }

        string* words;
        words = new string[countWord];
        countWord = 0;

        string currentWord;
        for (int i = 0; i < str.length(); i++) {
            if (str[i] == ' ') {
                if (!currentWord.empty()) {
                    words[countWord] = currentWord;
                    countWord++;
                }
                currentWord = "";
            }
            else {
                currentWord += str[i];
            }
        }

        int countWordInCell = floor ((float)countWord / (float)(rows * columns));
        countWord = 0;

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < columns; j++) {
                for (int k = 0; k < countWordInCell; k++) {
                    stringMatrix[i][j] += words[countWord] + ' ';
                    countWord++;
                }
                
            }
        }
       
        return true;
    }

    void DrawLine(HDC hdc, int x1, int y1, int x2, int y2)
    {
        MoveToEx(hdc, x1, y1, NULL);
        LineTo(hdc, x2, y2);
    }

    bool ChangeColor(HWND hWnd, bool parametr, int MenuId) {
        parametr =! parametr;
        CheckMenuItem(menu, MenuId, MF_BYCOMMAND | (parametr ? MF_CHECKED : MF_UNCHECKED));
        return parametr;
    }

    HFONT generateFont()
    {
        int fnWeight = FW_NORMAL;
        DWORD fdwItalic = FALSE;
        DWORD fdwUnderline = FALSE;

        if (isBold) fnWeight += FW_BOLD;
        if (isCoursive) fdwItalic = TRUE;
        if (isUnderline) fdwUnderline = TRUE;
        HFONT hFont = CreateFont(20, 7, 0, 0, fnWeight, fdwItalic, fdwUnderline, FALSE, RUSSIAN_CHARSET, OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Times New Roman"));

        return hFont;
    }

    void DrawTable(HDC hdc, int widht, int height, int borderSize)
    {
          int posX = 0, posY = top;
          int widthCell = (float)width / columns;
          int heightCell = 0;
          HFONT hFont = generateFont();
          SelectObject(hdc, hFont);
          for (int i = 0; i < rows; i++)
          {
             
              heightCell = GetBlockHeight(hdc, i, widthCell,borderSize);
              if (heightCell <= 100) {
                  int x = 0;
              }
              posX = 0;
              for (int j = 0; j < columns; j++)
              {
                  DrawTextBlock(hdc, posX, posY, widthCell, heightCell, i, j);
                  DrawLine(hdc, posX, posY, posX, posY + heightCell);
                  posX += widthCell;

              }
              DrawLine(hdc, posX, posY, posX, posY + heightCell);
              DrawLine(hdc, 0, (int)posY, posX, (int)posY);
              posY += heightCell;
          }
          DrawLine(hdc, 0, (int)posY, posX, (int)posY);
          bottom = posY;
          TableHeight = posY;
          DeleteObject(hFont);
    }

    string getLongestStringInRow(int currentLine)
    {
        string longestSrting = stringMatrix[currentLine][0];
        for (int j = 1; j < columns; j++)
        {
            if (stringMatrix[currentLine][j].length() > longestSrting.length())
                longestSrting = stringMatrix[currentLine][j];
        }

        return longestSrting;
    }

    int GetBlockHeight(HDC hdc, int currentLine, int width, int borderSize)
    {
        string longestString = getLongestStringInRow(currentLine);
        wstring widestr = wstring(longestString.begin(), longestString.end());
        const wchar_t* widecstr = widestr.c_str();
        RECT rect;
        rect.top = (long)(0);
        rect.left = (long)(0);
        rect.right = (long)(width - borderSize);
        rect.bottom = (long)(1);

        int height = (int)DrawText(hdc, widecstr, widestr.length(), &rect,
            DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_CENTER);
        return  height;
    }

    void DrawTextBlock(HDC hdc, int left, int top, int width, int height, int raw, int column)
    {
        string str = stringMatrix[raw][column].c_str();
        wstring widestr = wstring(str.begin(), str.end());
        const wchar_t* widecstr = widestr.c_str();
        RECT rect;
        rect.top = (long)(top);
        rect.left = (long)(left);
        rect.right = (long)(left + width);
        rect.bottom = (long)(top + height);

        DrawText(hdc, widecstr, widestr.length(), &rect,
            DT_WORDBREAK | DT_EDITCONTROL | DT_CENTER);
    }

   