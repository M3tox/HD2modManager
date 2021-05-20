#include "framework.h"
#include "HD2modManagerW.h"

#define MAX_LOADSTRING 100

#define MODACTIVATE   500
#define MODDEACTIVATE 501
#define CHANGEMODPATH 502
#define TOGGLEREADME  503

// globals
HINSTANCE hInst;                               
WCHAR szTitle[MAX_LOADSTRING];                 
WCHAR szWindowClass[MAX_LOADSTRING];           
HWND modsDir, modsActive, hReadme, btnReadme, btnModPath, btnAddMod, btnRemMod, hwndPB;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void DrawWindows(HWND);
short CheckModSelection();
short CheckInstalledModSelection();
static std::string ModsPath(HWND);
MMM MM;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);


    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HD2MODMANAGERW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);


    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HD2MODMANAGERW));

    MSG msg;


    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// callback proc for browsing folder window
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED) {
		SendMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, (LPARAM)pData);
	}
	return 0;
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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HD2MODMANAGERW));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_HD2MODMANAGERW);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; 

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, 700, 449, nullptr, nullptr, hInstance, nullptr);

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
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case CHANGEMODPATH:
		{
			std::string selectedModPath = ModsPath(hWnd);
			// only assign mod path if user selected valid
			if (!selectedModPath.empty()) {
				MM.defaultModPath.assign(selectedModPath);
				MM.RefreshListBoxes(modsDir, modsActive);
			}
			MM.SaveChanges();
		}
		break;
		case MODACTIVATE:
		{
			std::string ModToAdd(MM.defaultModPath);
			ModToAdd.push_back('\\');

			short ModIndex = CheckInstalledModSelection();

			if (ModIndex != -1) {
				ModToAdd.append(MM.ModsInDir.at((ModIndex)));
			}
			else {
				MessageBoxA(NULL, "No mod selected! Please select one of the installed mods first!", "Info", MB_OK);
				break;
			}
			

			// show progress bar, hide buttons behind it
			// add mod via backend and hide progress bar, show buttons again
			ShowWindow(btnModPath, SW_HIDE);
			ShowWindow(btnReadme, SW_HIDE);
			EnableWindow(btnAddMod, false);
			EnableWindow(btnRemMod, false);

			ShowWindow(hwndPB, SW_SHOW);
			MM.AddMod(ModToAdd.c_str(), hwndPB);
			ShowWindow(hwndPB, SW_HIDE);

			ShowWindow(btnModPath, SW_SHOW);
			ShowWindow(btnReadme, SW_SHOW);
			EnableWindow(btnAddMod, true);
			EnableWindow(btnRemMod, true);

			// refresh listbox
			if (!MM.CheckForErrors()) {
				int32_t modIndex = MM.GetModsCount();
				if (modIndex != -1) {
					
					std::string activatedModName = MM.GetModName(modIndex);
					
					// cut out path to display only actually mod name
					std::string::size_type pos = activatedModName.rfind('\\');
					if (pos != std::string::npos)					
						activatedModName = activatedModName.substr(pos + 1, activatedModName.length());
					

					MM.RefreshListBoxes(modsDir, modsActive);
				}
				
			}
			MM.SaveChanges();
		}
		break;

		case MODDEACTIVATE:
		{
			MM.RemoveMod(CheckModSelection());
			MM.RefreshListBoxes(modsDir, modsActive);
			MM.SaveChanges();

		}
		break;
		case TOGGLEREADME:
		{
			if (IsWindowVisible(hReadme)) {

				// readme text box is active, so toggle it off
				::SetWindowTextA(btnReadme, "View README");
				ShowWindow(modsDir, SW_SHOW);
				ShowWindow(modsActive, SW_SHOW);
				ShowWindow(btnModPath, SW_SHOW);
				ShowWindow(btnRemMod, SW_SHOW);
				ShowWindow(btnAddMod, SW_SHOW);

				ShowWindow(hReadme, SW_HIDE);
			}
			else {
				// readme text bix is not active, toggle it on
				std::string readme = MM.ViewReadme(CheckModSelection());
				if (!MM.CheckForErrors()) {

					::SetWindowTextA(btnReadme, "View Mods");
					ShowWindow(modsDir, SW_HIDE);
					ShowWindow(modsActive, SW_HIDE);
					ShowWindow(btnModPath, SW_HIDE);
					ShowWindow(btnRemMod, SW_HIDE);
					ShowWindow(btnAddMod, SW_HIDE);

					::SetWindowTextA(hReadme, readme.c_str());
					ShowWindow(hReadme, SW_SHOW);
				}
			}

			
		}
		break;
		case ID_FILE_ALLOWMPMAPLISTMODIFICATION:
		{
			// toggle mpmaplist option on and off
			if (MM.automatedMPlist = !MM.automatedMPlist)
				CheckMenuItem(GetMenu(hWnd), ID_FILE_ALLOWMPMAPLISTMODIFICATION, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(GetMenu(hWnd), ID_FILE_ALLOWMPMAPLISTMODIFICATION, MF_BYCOMMAND | MF_UNCHECKED);

		}
		break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
	case WM_CREATE:
		DrawWindows(hWnd);
		MM.RefreshListBoxes(modsDir, modsActive);

		if (MM.automatedMPlist)
			CheckMenuItem(GetMenu(hWnd), ID_FILE_ALLOWMPMAPLISTMODIFICATION, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(GetMenu(hWnd), ID_FILE_ALLOWMPMAPLISTMODIFICATION, MF_BYCOMMAND | MF_UNCHECKED);

		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// about proc
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


void DrawWindows(HWND hWnd) {
	HFONT defaultFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("MS Shell Dlg"));

	HFONT titleFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("MS Shell Dlg"));

	// draw explanation titles above listboxes:
	SendMessage(CreateWindow(L"Static", L"Installed mods:", WS_VISIBLE | WS_CHILD | SS_CENTER, 5, 20, 300, 20, hWnd, NULL, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);
	SendMessage(CreateWindow(L"Static", L"Activated mods:", WS_VISIBLE | WS_CHILD | SS_CENTER, 379, 20, 300, 20, hWnd, NULL, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);
	
	// draw lists which show all mods and activated mods
	SendMessage(modsDir = CreateWindowA("LISTBOX", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 40, 300, 320, hWnd, NULL, NULL, NULL), WM_SETFONT, WPARAM(titleFont), TRUE);
	SendMessage(modsActive = CreateWindowA("LISTBOX", "", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER, 379, 40, 300, 320, hWnd, NULL, NULL, NULL), WM_SETFONT, WPARAM(titleFont), TRUE);



	// draw activate/deactivate buttons
	SendMessage(btnAddMod = CreateWindow(L"Button", L">", BS_FLAT | WS_VISIBLE | WS_CHILD, 312, 120, 60, 60, hWnd, (HMENU)MODACTIVATE, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);
	SendMessage(btnRemMod = CreateWindow(L"Button", L"<", BS_FLAT | WS_VISIBLE | WS_CHILD, 312, 220, 60, 60, hWnd, (HMENU)MODDEACTIVATE, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);

	// draw edit to display readme stuff
	SendMessage(hReadme = CreateWindowA("Edit", "", ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | WS_CHILD | WS_BORDER, 5, 20, 674, 326, hWnd, NULL, NULL, NULL), WM_SETFONT, WPARAM(titleFont), TRUE);

	// draw buttons below listboxes
	SendMessage(btnModPath = CreateWindow(L"Button", L"Change Mod files path", BS_FLAT | WS_VISIBLE | WS_CHILD, 5, 350, 300, 35, hWnd, (HMENU)CHANGEMODPATH, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);
	SendMessage(btnReadme = CreateWindow(L"Button", L"View README", BS_FLAT | WS_VISIBLE | WS_CHILD, 379, 350, 300, 35, hWnd, (HMENU)TOGGLEREADME, NULL, NULL), WM_SETFONT, WPARAM(defaultFont), TRUE);

	// prepare progress bar
	hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL, WS_CHILD | PBS_SMOOTH, 5, 350, 674, 35, hWnd, (HMENU)0, NULL, NULL);

}


static std::string ModsPath(HWND hWnd) {

	// preselect the folder where the program is executed from
	std::string bowseStart;
	bowseStart.resize(MAX_PATH);
	GetModuleFileNameA(NULL, (LPSTR)bowseStart.data(), MAX_PATH);
	uint32_t find = bowseStart.rfind('\\');
	if (find != std::string::npos) {
		bowseStart = bowseStart.substr(0, find + 1);
	}

	BROWSEINFOA bwi;
	ZeroMemory(&bwi, sizeof(BROWSEINFOA));
	bwi.lpfn = BrowseCallbackProc;
	bwi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bwi.hwndOwner = hWnd;
	bwi.lpszTitle = "Please select the folder you want to extract to.";
	bwi.lParam = (LPARAM)bowseStart.c_str();

	LPCITEMIDLIST pidl = NULL;
	if (pidl = SHBrowseForFolderA(&bwi)) {
		CHAR szFolder[MAX_PATH]{ 0 };
		if (SHGetPathFromIDListA(pidl, szFolder)) {
			return szFolder;
		}
	}

	return std::string();
}


short CheckModSelection() {
	short index = ((int)SendMessage(modsActive, LB_GETITEMDATA, (int)SendMessage(modsActive, LB_GETCURSEL, 0, 0), 0));
	
	if (index >= 0 && index <= MM.GetModsCount())
		return index;

	// tell user to select something first
	return -1;
}


short CheckInstalledModSelection() {
	short index = ((int)SendMessage(modsDir, LB_GETITEMDATA, (int)SendMessage(modsDir, LB_GETCURSEL, 0, 0), 0));

	if (index >= 0 && index <= MM.ModsInDir.size())
		return index;

	// tell user to select something first
	return -1;
}
