/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/userprofile.c
 * PURPOSE:     Computer settings for networking
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"
#include <sddl.h>


typedef struct _PROFILEDATA
{
    BOOL bMyProfile;
    PWSTR pszFullName;
} PROFILEDATA, *PPROFILEDATA;


static VOID
SetListViewColumns(HWND hwndListView)
{
    LV_COLUMN column;
    RECT rect;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    memset(&column, 0x00, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.40);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_USERPROFILE_NAME, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.fmt = LVCFMT_RIGHT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_USERPROFILE_SIZE, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 2;
    LoadString(hApplet, IDS_USERPROFILE_TYPE, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 2, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15);
    column.iSubItem = 3;
    LoadString(hApplet, IDS_USERPROFILE_STATUS, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 3, &column);

    column.fmt = LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.15) - GetSystemMetrics(SM_CYHSCROLL);
    column.iSubItem = 4;
    LoadString(hApplet, IDS_USERPROFILE_MODIFIED, szStr, ARRAYSIZE(szStr));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 4, &column);
}


static VOID
AddUserProfile(
    _In_ HWND hwndListView,
    _In_ LPTSTR lpProfileSid,
    _In_ PSID pMySid)
{
    PPROFILEDATA pProfileData = NULL;
    PWSTR pszAccountName = NULL;
    PWSTR pszDomainName = NULL;
    SID_NAME_USE Use;
    DWORD dwAccountNameSize, dwDomainNameSize;
    DWORD dwProfileData;
    PWSTR ptr;
    PSID pSid = NULL;
    LV_ITEM lvi;

    if (!ConvertStringSidToSid(lpProfileSid,
                               &pSid))
        return;

    dwAccountNameSize = 0;
    dwDomainNameSize = 0;
    LookupAccountSidW(NULL,
                      pSid,
                      NULL,
                      &dwAccountNameSize,
                      NULL,
                      &dwDomainNameSize,
                      &Use);

    pszDomainName = HeapAlloc(GetProcessHeap(),
                              0,
                              dwDomainNameSize * sizeof(WCHAR));
    if (pszDomainName == NULL)
        goto done;

    pszAccountName = HeapAlloc(GetProcessHeap(),
                               0,
                               dwAccountNameSize * sizeof(WCHAR));
    if (pszAccountName == NULL)
        goto done;

    if (!LookupAccountSidW(NULL,
                           pSid,
                           pszAccountName,
                           &dwAccountNameSize,
                           pszDomainName,
                           &dwDomainNameSize,
                           &Use))
        goto done;

    /* Show only the user accounts */
    if (Use != SidTypeUser)
        goto done;

    dwProfileData = sizeof(PROFILEDATA) +
                    ((wcslen(pszDomainName) + wcslen(pszAccountName) + 2) * sizeof(WCHAR));
    pProfileData = HeapAlloc(GetProcessHeap(),
                             0,
                             dwProfileData);
    if (pProfileData == NULL)
        goto done;

    pProfileData->bMyProfile = EqualSid(pMySid, pSid);

    ptr = (PWSTR)((ULONG_PTR)pProfileData + sizeof(PROFILEDATA));
    pProfileData->pszFullName = ptr;

    wsprintf(pProfileData->pszFullName, L"%s\\%s", pszDomainName, pszAccountName);

    memset(&lvi, 0x00, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvi.pszText = pProfileData->pszFullName;
    lvi.state = 0;
    lvi.lParam = (LPARAM)pProfileData;
    ListView_InsertItem(hwndListView, &lvi);

done:
    if (pszDomainName != NULL)
        HeapFree(GetProcessHeap(), 0, pszDomainName);

    if (pszAccountName != NULL)
        HeapFree(GetProcessHeap(), 0, pszAccountName);

    if (pSid != NULL)
        LocalFree(pSid);
}


static VOID
UpdateButtonState(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView)
{
    LVITEM Item;
    INT iSelected;
    BOOL bMyProfile;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected != -1)
    {
        Item.mask = LVIF_PARAM;
        Item.iItem = iSelected;
        Item.iSubItem = 0;
        if (ListView_GetItem(hwndListView, &Item))
        {
            if (Item.lParam != 0)
            {
                bMyProfile = ((PPROFILEDATA)Item.lParam)->bMyProfile;
                if (/*IsUserAnAdmin() &&*/ !bMyProfile)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), TRUE);
                }
            }
        }
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_CHANGE), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_CHANGE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_DELETE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERPROFILE_COPY), FALSE);
    }
}


static VOID
AddUserProfiles(
    _In_ HWND hwndDlg,
    _In_ HWND hwndListView)
{
    HKEY hKeyUserProfiles = INVALID_HANDLE_VALUE;
    DWORD dwIndex;
    WCHAR szProfileSid[64];
    DWORD dwSidLength;
    FILETIME ftLastWrite;
    DWORD dwSize;
    HANDLE hToken = NULL;
    PTOKEN_USER pTokenUser = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize == 0)
        goto done;

    pTokenUser = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (pTokenUser == NULL)
        goto done;

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize))
        goto done;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                      0,
                      KEY_READ,
                      &hKeyUserProfiles))
        goto done;

    for (dwIndex = 0; ; dwIndex++)
    {
        dwSidLength = ARRAYSIZE(szProfileSid);
        if (RegEnumKeyExW(hKeyUserProfiles,
                          dwIndex,
                          szProfileSid,
                          &dwSidLength,
                          NULL,
                          NULL,
                          NULL,
                          &ftLastWrite))
            break;

        AddUserProfile(hwndListView, szProfileSid, pTokenUser->User.Sid);
    }

    if (ListView_GetItemCount(hwndListView) != 0)
        ListView_SetItemState(hwndListView, 0, LVIS_SELECTED, LVIS_SELECTED);

    UpdateButtonState(hwndDlg, hwndListView);

done:
    if (hKeyUserProfiles != INVALID_HANDLE_VALUE)
        RegCloseKey(hKeyUserProfiles);

    if (pTokenUser != NULL)
        HeapFree(GetProcessHeap(), 0, pTokenUser);

    if (hToken != NULL)
        CloseHandle(hToken);
}


static VOID
OnInitUserProfileDialog(HWND hwndDlg)
{
    /* Initialize the list view control */
    SetListViewColumns(GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));

    AddUserProfiles(hwndDlg, GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST));
}


static
VOID
OnDestroy(
    _In_ HWND hwndDlg)
{
    HWND hwndList;
    INT nItems, i;
    LVITEM Item;

    hwndList = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);

    nItems = ListView_GetItemCount(hwndList);
    for (i = 0; i < nItems; i++)
    {
        Item.iItem = i;
        Item.iSubItem = 0;
        if (ListView_GetItem(hwndList, &Item))
        {
            if (Item.lParam != 0)
                HeapFree(GetProcessHeap(), 0, (PVOID)Item.lParam);
        }
    }
}


static
VOID
OnNotify(
    _In_ HWND hwndDlg,
    _In_ NMHDR *nmhdr)
{
    if (nmhdr->idFrom == IDC_USERACCOUNT_LINK && nmhdr->code == NM_CLICK)
    {
        ShellExecuteW(hwndDlg, NULL, L"usrmgr.cpl", NULL, NULL, 0);
    }
    else if (nmhdr->idFrom == IDC_USERPROFILE_LIST && nmhdr->code == LVN_ITEMCHANGED)
    {
        UpdateButtonState(hwndDlg, nmhdr->hwndFrom);
    }
}


static
BOOL
DeleteUserProfile(
    HWND hwndDlg)
{
    WCHAR szTitle[64], szRawText[128], szCookedText[256];
    HWND hwndListView;
    LVITEM Item;
    INT iSelected;
    PPROFILEDATA pProfileData;

    hwndListView = GetDlgItem(hwndDlg, IDC_USERPROFILE_LIST);
    if (hwndListView == NULL)
        return FALSE;

    iSelected = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (iSelected == -1)
        return FALSE;

    ZeroMemory(&Item, sizeof(LVITEM));
    Item.mask = LVIF_PARAM;
    Item.iItem = iSelected;
    Item.iSubItem = 0;
    if (!ListView_GetItem(hwndListView, &Item))
        return FALSE;

    if (Item.lParam == 0)
        return FALSE;

    pProfileData = (PPROFILEDATA)Item.lParam;
    if (pProfileData->bMyProfile)
        return FALSE;

    LoadStringW(hApplet, IDS_USERPROFILE_CONFIRM_DELETE_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadStringW(hApplet, IDS_USERPROFILE_CONFIRM_DELETE, szRawText, ARRAYSIZE(szRawText));
    swprintf(szCookedText, szRawText, pProfileData->pszFullName);

    if (MessageBoxW(hwndDlg,
                    szCookedText,
                    szTitle,
                    MB_ICONQUESTION | MB_YESNO) == IDNO)
        return FALSE;

    /* FIXME: Delete the profile here! */

    return TRUE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
UserProfileDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitUserProfileDialog(hwndDlg);
            return TRUE;

        case WM_DESTROY:
            OnDestroy(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    return TRUE;

                case IDC_USERPROFILE_CHANGE:
                    break;

                case IDC_USERPROFILE_DELETE:
                    DeleteUserProfile(hwndDlg);
                    break;

                case IDC_USERPROFILE_COPY:
                    break;
            }
            break;

        case WM_NOTIFY:
            OnNotify(hwndDlg, (NMHDR *)lParam);
            break;
    }

    return FALSE;
}
