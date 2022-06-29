#include "pch.h"
#include "resource.h"
#include "FindAllDlg.h"
#include "SortHelper.h"
#include "Helpers.h"
#include <ListViewhelper.h>
#include <ThemeHelper.h>
#include <ClipboardHelper.h>

CFindAllDlg::CFindAllDlg(IMainFrame* frame) : m_pFrame(frame) {
}

LRESULT CFindAllDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
    if (m_Searcher.IsRunning()) {
        if (AtlMessageBox(m_hWnd, L"Search is still working. Closing this window will cancel the search. Close anyway?",
            IDS_APP_TITLE, MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
            return 0;

        m_Searcher.Cancel();
    }

	UpdateOptions();
    ShowWindow(SW_HIDE);
	return 0;
}

void CFindAllDlg::UpdateUI() {
    auto options = AppSettings::Get().Find();
    CheckButton(IDC_SEARCH_KEYS, options, FindOptions::SearchKeys);
    CheckButton(IDC_SEARCH_VALUES, options, FindOptions::SearchValues);
    CheckButton(IDC_SEARCH_DATA, options, FindOptions::SearchData);
    if (!CheckButton(IDC_SEARCH_SELECTED, options, FindOptions::SearchSelected))
        CheckDlgButton(IDC_SEARCH_ALL, BST_CHECKED);
    CheckButton(IDC_SEARCH_MATCHWHOLE, options, FindOptions::MatchWholeWords);
    CheckButton(IDC_SEARCH_CASE, options, FindOptions::MatchCase);
    CheckButton(IDC_SEARCH_STD, options, FindOptions::SearchStdRegistry);
    CheckButton(IDC_SEARCH_REAL, options, FindOptions::SearchRealRegistry);
}

void CFindAllDlg::Cancel() {
    m_Searcher.Cancel();
}

void CFindAllDlg::OnFinalMessage(HWND) {
    delete this;
}

CString CFindAllDlg::GetColumnText(HWND, int row, int col) const {
    auto& item = m_Items[row];

    switch (col) {
        case 0: return item.Path;
        case 1:
            if (item.Data.IsEmpty())
                return item.Name;
            return item.Name.IsEmpty() ? CString(Helpers::DefaultValueName) : item.Name;

        case 2: return item.Data;
    };
    ATLASSERT(false);
    return L"";
}

int CFindAllDlg::GetRowImage(HWND, int row, int) const {
    auto& item = m_Items[row];
    if (!item.Data.IsEmpty())
        return 2;
    if (!item.Name.IsEmpty())
        return 1;

    return 0;
}

void CFindAllDlg::DoSort(const SortInfo* si) {
    if (si == nullptr)
        return;

    auto compare = [&](auto& i1, auto& i2) {
        switch (si->SortColumn) {
            case 0: return SortHelper::Sort(i1.Path, i2.Path, si->SortAscending);
            case 1: return SortHelper::Sort(i1.Name, i2.Name, si->SortAscending);
            case 2: return SortHelper::Sort(i1.Data, i2.Data, si->SortAscending);
        }
        return false;
    };
    std::ranges::sort(m_Items, compare);
}

bool CFindAllDlg::OnDoubleClickList(HWND, int row, int, const POINT&) {
    if (row < 0)
        return false;

    auto& item = m_Items[row];
    CWaitCursor wait;
    if (m_pFrame->GoToItem(item.Path, item.Name, item.Data)) {
        // keep dialog alive
        return true;
    }
    else {
        AtlMessageBox(m_hWnd, L"Unable to locate item", IDS_APP_TITLE, MB_ICONERROR);
    }
    return 0;
}

bool CFindAllDlg::CheckButton(UINT id, FindOptions options, FindOptions value) {
    bool check = (options & value) == value;
    CheckDlgButton(id, check ? BST_CHECKED : BST_UNCHECKED);
    return check;
}

FindOptions CFindAllDlg::UpdateOptions() {
    auto options = FindOptions::None;
    if (IsDlgButtonChecked(IDC_SEARCH_KEYS))
        options |= FindOptions::SearchKeys;
    if (IsDlgButtonChecked(IDC_SEARCH_VALUES))
        options |= FindOptions::SearchValues;
    if (IsDlgButtonChecked(IDC_SEARCH_DATA))
        options |= FindOptions::SearchData;
    if (IsDlgButtonChecked(IDC_SEARCH_SELECTED))
        options |= FindOptions::SearchSelected;
    if (IsDlgButtonChecked(IDC_SEARCH_STD))
        options |= FindOptions::SearchStdRegistry;
    if (IsDlgButtonChecked(IDC_SEARCH_REAL))
        options |= FindOptions::SearchRealRegistry;
    if (IsDlgButtonChecked(IDC_SEARCH_MATCHWHOLE))
        options |= FindOptions::MatchWholeWords;
    if (IsDlgButtonChecked(IDC_SEARCH_CASE))
        options |= FindOptions::MatchCase;

    AppSettings::Get().Find(options);
    return options;
}

LRESULT CFindAllDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    InitDynamicLayout();
    CenterWindow(GetParent());

    SetDialogIcon(IDI_FINDALL);
    AddIconToButton(IDC_CANCEL, IDI_DELETE);
    AddIconToButton(IDC_FIND, IDI_FIND);

    m_Progress.Attach(GetDlgItem(IDC_PROGRESS));
    m_Progress.SetMarquee(TRUE);

    m_List.Attach(GetDlgItem(IDC_LIST));
    m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
    ::SetWindowTheme(m_List, L"Explorer", L"");

    CImageList images;
    images.Create(16, 16, ILC_COLOR32, 4, 3);
    std::vector<UINT> icons{ IDI_FOLDER, IDI_RENAME, IDI_TEXT };
    std::for_each(icons.begin(), icons.end(), [&](auto id) { images.AddIcon(AtlLoadIconImage(id, 0, 16, 16)); });
    m_List.SetImageList(images, LVSIL_SMALL);

    auto cm = GetColumnManager(m_List);
    cm->AddColumn(L"Key", 0, 350);
    cm->AddColumn(L"Value Name", 0, 150);
    cm->AddColumn(L"Data", 0, 250);
    cm->UpdateColumns();

    UpdateUI();

    Helpers::RestoreWindowPosition(m_hWnd, L"FindAllDlgRect");
    return 0;
}

LRESULT CFindAllDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL& handled) {
    Helpers::SaveWindowPosition(m_hWnd, L"FindAllDlgRect");
    handled = FALSE;
    return 0;
}

LRESULT CFindAllDlg::OnFind(WORD, WORD wID, HWND, BOOL&) {
    auto options = UpdateOptions();
    GetDlgItem(IDC_FIND).EnableWindow(FALSE);
    GetDlgItem(IDC_CANCEL).EnableWindow();
    m_Progress.ShowWindow(SW_SHOW);

    if (IsDlgButtonChecked(IDC_APPEND) == BST_UNCHECKED) {
        m_Items.clear();
        m_List.SetItemCount(0);
    }
    if (m_Items.empty())
        m_Items.reserve(64);

    CString text;
    GetDlgItemText(IDC_TEXT, text);
    m_Searcher.SetText(text);
    m_Searcher.SetOptions(options);
    if (m_Searcher.IsRunning()) {
        m_Searcher.Continue();
        return 0;
    }

    m_Searcher.SetStartKey(m_pFrame->GetCurrentKeyPath());
    m_Searcher.Find([&](auto path, auto name, auto data) {
        if (path == nullptr) {
            PostMessage(WM_SEARCH_COMPLETE, m_Searcher.IsCancelled());
        }
        else {
            ListItem item{ path, name, data };
            m_Items.push_back(item);
            m_List.SetItemCountEx(static_cast<int>(m_Items.size()), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
            m_Searcher.Continue();
        }
        });

    return 0;
}

LRESULT CFindAllDlg::OnCancel(WORD, WORD wID, HWND, BOOL&) {
    m_Searcher.Cancel();
    GetDlgItem(IDC_FIND).EnableWindow();
    GetDlgItem(IDC_CANCEL).EnableWindow(FALSE);
    m_Progress.ShowWindow(SW_HIDE);

    return 0;
}

LRESULT CFindAllDlg::OnTextChanged(WORD, WORD wID, HWND, BOOL&) {
    GetDlgItem(IDC_FIND).EnableWindow(GetDlgItem(IDC_TEXT).GetWindowTextLength() > 0);
    return 0;
}

LRESULT CFindAllDlg::OnClick(WORD, WORD wID, HWND, BOOL&) {
    if (GetDlgItem(IDC_FIND).IsWindowEnabled()) {
        if (IsDlgButtonChecked(IDC_SEARCH_STD) == BST_UNCHECKED && IsDlgButtonChecked(IDC_SEARCH_REAL) == BST_UNCHECKED)
            GetDlgItem(IDC_FIND).EnableWindow(FALSE);
    }
    return 0;
}

LRESULT CFindAllDlg::OnSearchComplete(UINT msg, WPARAM cancelled, LPARAM, BOOL&) {
    GetDlgItem(IDC_FIND).EnableWindow();
    GetDlgItem(IDC_CANCEL).EnableWindow(FALSE);
    m_Progress.ShowWindow(SW_HIDE);
    return 0;
}

LRESULT CFindAllDlg::OnSaveResults(WORD, WORD wID, HWND, BOOL&) {
    CSimpleFileDialog dlg(FALSE, L"txt", nullptr, OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT,
        L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0", m_hWnd);
    ThemeHelper::Suspend();
    if (dlg.DoModal() == IDOK) {
        CString text;
        for (int i = 0; i < (int)m_Items.size(); i++)
            text += GetColumnText(m_List, i, 0) + L"\t" + GetColumnText(m_List, i, 1) + L"\t" + GetColumnText(m_List, i, 2) + L"\r\n";
        if (!Helpers::WriteToFile(dlg.m_szFileName, text))
            m_pFrame->DisplayError(L"Error saving results");
    }
    ThemeHelper::Resume();
    return 0;
}

LRESULT CFindAllDlg::OnLoadResults(WORD, WORD wID, HWND, BOOL&) {
    CSimpleFileDialog dlg(TRUE, L"txt", nullptr, OFN_EXPLORER | OFN_ENABLESIZING,
        L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0", m_hWnd);
    ThemeHelper::Suspend();
    if (dlg.DoModal() == IDOK) {
        CString text;
        if(!Helpers::ReadFileText(dlg.m_szFileName, text))
            m_pFrame->DisplayError(L"Error loading results");
        else {
            if (IsDlgButtonChecked(IDC_APPEND) == BST_UNCHECKED) {
                m_Items.clear();
            }
            int cr = 0, start = 0;
            while ((cr = text.Find(L"\r\n", start)) > 0) {
                ListItem item;
                item.Path = text.Tokenize(L"\t", start);
                item.Name = text.Tokenize(L"\t", start);
                item.Data = text.Tokenize(L"\t", start);
                m_Items.push_back(std::move(item));
                start = cr + 2;
            }
            m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
            m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
        }
    }
    ThemeHelper::Resume();
    return 0;
}

LRESULT CFindAllDlg::OnCopy(WORD, WORD wID, HWND, BOOL&) {
    ClipboardHelper::CopyText(m_hWnd, m_List.GetSelectedCount() == 0 ? 
        ListViewHelper::GetAllRowsAsString(m_List) : ListViewHelper::GetSelectedRowsAsString(m_List));
    return 0;
}

LRESULT CFindAllDlg::OnDelete(WORD, WORD wID, HWND, BOOL&) {
    if (m_List.GetSelectedCount() == 0)
        m_Items.clear();
    else {
        int n = -1, offset = 0;
        while ((n = m_List.GetNextItem(n, LVIS_SELECTED)) >= 0) {
            m_Items.erase(m_Items.begin() + n - offset);
            offset++;
        }
    }
    m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
    return 0;
}
