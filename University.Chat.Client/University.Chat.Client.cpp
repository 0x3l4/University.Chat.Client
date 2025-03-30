#pragma comment(lib, "Comctl32.lib")

#include "framework.h"
#include "University.Chat.Client.h"

DWORD WINAPI ClientSendThread(LPVOID lpParam)
{
    LPCWSTR lpszPipename = L"\\\\.\\pipe\\MyNamedPipe";
    HANDLE hPipe = NULL;

    while (!stopClientThread)
    {
        if (hPipe == NULL)
            hPipe = CreateFile(lpszPipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            DWORD tmp = GetLastError();
            LogStringCreator(L"Не удалось подключиться к серверу. GLE=%d\n", (const wchar_t*)tmp);

            // Если сервер завершился - выходим
            if (tmp == ERROR_FILE_NOT_FOUND || tmp == ERROR_PIPE_BUSY)
            {
                Sleep(5000);
                continue;
            }
            else
                return 0; // Неизвестная ошибка - завершаем поток
        }

        //hEvent = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, L"Global\\ExclusiveModeEvent");
        hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"Global\\ExclusiveAccessMutex");
        if (hMutex == NULL) {
            LogStringCreator(L"Ошибка при открытии мьютекса: GLE=%d\n", (const wchar_t*)GetLastError());
            Sleep(TIME_QUANT);
            continue;
        }

        // Устанавливаем режим передачи сообщений (важно для работы в MESSAGE mode)
        DWORD dwMode = PIPE_READMODE_MESSAGE;
        if (!SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
        {
            DWORD tmp = GetLastError();
            LogStringCreator(L"SetNamedPipeHandleState failed. GLE=%d\n", (const wchar_t*)tmp);
            CloseHandle(hPipe);
            Sleep(5000);
            continue;
        }

        while (!stopClientThread) {
            MessageCicleElement(hPipe);
        }

        CloseHandle(hMutex);
        CloseHandle(hPipe); // Закрываем соединение после выхода из внутреннего цикла
    }
    return 0;
}

void MessageCicleElement(HANDLE hPipe) {

    Sleep(TIME_QUANT);
    wchar_t message[BUFSIZE] = { 0 }, chBuf[BUFSIZE] = { 0 };
    DWORD cbWritten, cbRead;

    GetWindowTextW(hEditMsg, message, BUFSIZE);
    LogStringCreator(L"Отправка сообщения: \"%s\"\n", message);

    if (isThisClientWantExclusive) {
        const wchar_t* prefix = L"EXCLUSIVE";
        wcscpy_s(message, prefix);
    }

    DWORD cbToWrite = (wcslen(message) + 1) * sizeof(WCHAR);

    // Отправка сообщения
    if (!WriteFile(hPipe, message, cbToWrite, &cbWritten, NULL))
    {
        DWORD tmp = GetLastError();
        LogStringCreator(L"Ошибка WriteFile. GLE=%d\n", (const wchar_t*)tmp);
        if (tmp == ERROR_BROKEN_PIPE)
        {
            LogStringCreator(L"Сервер отключился. GLE=%d\n", (const wchar_t*)tmp);
        }
        Sleep(TIME_QUANT);
        stopClientThread = true;
        return;
    }
    FlushFileBuffers(hPipe); // Гарантируем, что данные отправлены
    // Читаем ответ сервера
    BOOL fSuccess = ReadFile(hPipe, chBuf, BUFSIZE * sizeof(WCHAR), &cbRead, NULL);

    if (!fSuccess)
    {
        DWORD tmp = GetLastError();
        LogStringCreator(L"Ошибка ReadFile. GLE=%d\n", (const wchar_t*)tmp);
        if (tmp == ERROR_BROKEN_PIPE)
        {
            LogStringCreator(L"Сервер отключился. GLE=%d\n", (const wchar_t*)tmp);
        }
        Sleep(TIME_QUANT);
        stopClientThread = true;
        return;
    }

    chBuf[cbRead / sizeof(WCHAR)] = L'\0'; // Гарантируем корректное окончание строки

    if (wcscmp(chBuf, L"NOACCESS") == 0) {
        // Сервер занят монопольным клиентом
        PrintMessage(L"Сервер занят. Ожидание...\n");
        WaitForSingleObject(hMutex, INFINITE); // Ждём сигнала от сервера

        ReleaseMutex(hMutex);
        return;
    }
    else
        LogStringCreator(L"Ответ сервера: %s\n", chBuf);

    return;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_UNIVERSITYCHATCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UNIVERSITYCHATCLIENT));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UNIVERSITYCHATCLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_UNIVERSITYCHATCLIENT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

    HWND hWnd = CreateWindowEx(0, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_X, WINDOW_Y, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont;

    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Разобрать выбор в меню:
        switch (wmId)
        {
        case IDC_MNPLY_CHECK: {
            LRESULT state = SendMessage(hChkMonopoly, BM_GETCHECK, 0, 0);
            if (state == BST_UNCHECKED)
            {
                if (!isThisClientWantExclusive) {
                    PrintMessage(L"Монопольный режим включен.\r\n");
                    isThisClientWantExclusive = true;
                    isThisClientBlocked = false;
                    SendMessage(hChkMonopoly, BM_SETCHECK, BST_CHECKED, 0);
                }
                else {
                    DWORD dwWaitResult = WaitForSingleObject(hMutex, 1000);
                    if (dwWaitResult != WAIT_OBJECT_0) {
                        PrintMessage(L"Сервер занят. Отказано.\r\n");
                        isThisClientBlocked = true;
                    }
                }
            }
            else // state == BST_UNCHECKED
            {
                PrintMessage(L"Монопольный режим выключен.\r\n");
                isThisClientWantExclusive = false;
                SendMessage(hChkMonopoly, BM_SETCHECK, BST_UNCHECKED, 0);
                ReleaseMutex(hMutex);
            }
            break;
        }
        case IDC_BTN_CONNECT:
        {
            OnConnect();
            break;
        }
        case IDC_BTN_DISCONNECT:
        {
            OnDisconnect();
            break;
        }
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDC_BTN_EXIT:
        case IDM_EXIT:
            if (MessageBoxExW(hWnd, L"Вы уверены, что хотите выйти?", L"Подтвердите", MB_YESNO | MB_ICONQUESTION, NULL) == IDYES)
            {
                DestroyWindow(hWnd);
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_CREATE: {
        OnCreate(hWnd, lParam, hFont);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
        pmmi->ptMinTrackSize.x = WINDOW_X;
        pmmi->ptMinTrackSize.y = WINDOW_Y;
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Добавьте сюда любой код прорисовки, использующий HDC...
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void OnCreate(HWND hWnd, LPARAM lParam, HFONT hFont) {
    InitCommonControls();

    hButtonConnect = CreateWindow(L"BUTTON", L"Подключиться к серверу",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        10, 10, 150, 40, hWnd, (HMENU)IDC_BTN_CONNECT, hInst, NULL);

    hButtonDisconnect = CreateWindow(L"BUTTON", L"Отключиться от сервера",
        WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_DEFPUSHBUTTON | BS_MULTILINE,
        170, 10, 150, 40, hWnd, (HMENU)IDC_BTN_DISCONNECT, hInst, NULL);

    HWND hLabel = CreateWindowEx(NULL, L"STATIC", L"Отправляемое сообщение", WS_CHILD | WS_VISIBLE,
        10, 50 + BASIC_MARGIN, 200, 20, hWnd, (HMENU)301, hInst, NULL);

    hEditMsg = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE,
        10, 80 + BASIC_MARGIN, 300, 25, hWnd, (HMENU)IDC_EDIT_MSG, hInst, NULL);

    hChkMonopoly = CreateWindow(L"BUTTON", L"Монопольный режим доступа",
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_MULTILINE,
        10, 120, 200, 30, hWnd, (HMENU)IDC_MNPLY_CHECK, hInst, NULL);

    hButtonExit = CreateWindow(L"BUTTON", L"Выход",
        WS_CHILD | WS_VISIBLE,
        240, 120, 100, 30, hWnd, (HMENU)IDC_BTN_EXIT, hInst, NULL);

    RECT rc;
    GetClientRect(hWnd, &rc);
    int clientWidth = rc.right;
    int clientHeight = rc.bottom;

    hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
        10, 160, 370, clientWidth - BASIC_MARGIN * 5 - 160, hWnd, (HMENU)IDC_EDIT, hInst, NULL);

    if (hEdit == NULL)
        MessageBox(hWnd, L"Не удалось создать элемент управления.", L"Ошибка", MB_ICONERROR);
    else {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    }
}

void OnConnect() {
    if (hClientThread == NULL || hClientThread == INVALID_HANDLE_VALUE) {
        stopClientThread = false;
        std::wstring customMessage = L"1";
        hClientThread = CreateThread(NULL, 0, ClientSendThread, NULL, 0, NULL);

        if (hClientThread) {
            PrintMessage(L"Клиент запущен.\r\n");
            EnableWindow(hButtonConnect, FALSE);
            EnableWindow(hButtonDisconnect, TRUE);
        }
        else PrintMessage(L"Не удалось запустить клиент.\r\n");
    }
}

void OnDisconnect() {
    if (hClientThread != NULL) {
        stopClientThread = true;
        WaitForSingleObject(hClientThread, INFINITE);

        EnableWindow(hButtonConnect, TRUE);
        EnableWindow(hButtonDisconnect, FALSE);
        if (CloseHandle(hClientThread)) {
            hClientThread = NULL;
            PrintMessage(L"Клиент остановлен.\r\n");
        }
        else
            PrintMessage(L"Не удалось остановить клиент.\r\n");
    }
}

void LogStringCreator(const wchar_t* msg, const wchar_t* payload) {
    wchar_t buff[256];
    swprintf_s(buff, msg, payload);
    PrintMessage(buff);
}

void PrintMessage(const wchar_t* msg) {
    if (hEdit != NULL) {
        int length = GetWindowTextLengthW(hEdit);
        SendMessageW(hEdit, EM_SETSEL, (WPARAM)length, (LPARAM)length);
        SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)(GetCurrentTimeString().append(msg).c_str()));
    }
}

std::wstring GetCurrentTimeString() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buffer[64];
    swprintf_s(buffer, 64, L"[%02d:%02d:%02d]: ", st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buffer);
}