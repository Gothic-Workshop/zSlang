/***************************************************************
 * Name:      zTransformMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Sekti ()
 * Created:   2011-03-11
 * Copyright: Sekti (http://forum.worldofplayers.de/forum/member.php?u=11242)
 * License:
 **************************************************************/

#include "wx_pch.h"
#include "zTransformMain.h"
#include <wx/msgdlg.h>
#include <zTransformApp.h>
#include <cLogger.h>
#include <cMain.h>
#include <Windows.h>

//(*InternalHeaders(zTransformFrame)
#include <wx/settings.h>
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/fontenum.h>
//*)

//(*IdInit(zTransformFrame)
const long zTransformFrame::ID_TEXTCTRL2 = wxNewId();
const long zTransformFrame::ID_BUTTON1 = wxNewId();
const long zTransformFrame::ID_BUTTON2 = wxNewId();
const long zTransformFrame::ID_PANEL2 = wxNewId();
const long zTransformFrame::ID_TEXTCTRL1 = wxNewId();
const long zTransformFrame::ID_PANEL3 = wxNewId();
const long zTransformFrame::ID_PANEL1 = wxNewId();
const long zTransformFrame::idMenuQuit = wxNewId();
const long zTransformFrame::ID_MENUITEM2 = wxNewId();
const long zTransformFrame::ID_MENUITEM3 = wxNewId();
const long zTransformFrame::ID_MENUITEM4 = wxNewId();
const long zTransformFrame::ID_MENUITEM5 = wxNewId();
const long zTransformFrame::ID_MENUITEM1 = wxNewId();
const long zTransformFrame::idMenuAbout = wxNewId();
const long zTransformFrame::ID_STATUSBAR1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(zTransformFrame,wxFrame)
    //(*EventTable(zTransformFrame)
    //*)
END_EVENT_TABLE()

zTransformFrame::zTransformFrame(zTransformApp* APP, wxWindow* parent,wxWindowID id) : APP(APP)
{
    //(*Initialize(zTransformFrame)
    wxMenuItem* MenuItem2;
    wxMenu* Menu3;
    wxMenuItem* MenuItem1;
    wxBoxSizer* BoxSizer2;
    wxMenu* Menu1;
    wxMenuItem* MenuItem3;
    wxMenuBar* Menu;
    wxBoxSizer* BoxSizer1;
    wxStaticBoxSizer* StaticBoxSizer1;
    wxMenu* Menu2;
    
    Create(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("wxID_ANY"));
    SetClientSize(wxSize(630,290));
    Panel1 = new wxPanel(this, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL1"));
    BoxSizer1 = new wxBoxSizer(wxVERTICAL);
    Panel2 = new wxPanel(Panel1, ID_PANEL2, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL2"));
    BoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    FileName = new wxTextCtrl(Panel2, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_TEXTCTRL2"));
    BoxSizer2->Add(FileName, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Button1 = new wxButton(Panel2, ID_BUTTON1, _("..."), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON1"));
    BoxSizer2->Add(Button1, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Button2 = new wxButton(Panel2, ID_BUTTON2, _("run!"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON2"));
    BoxSizer2->Add(Button2, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Panel2->SetSizer(BoxSizer2);
    BoxSizer2->Fit(Panel2);
    BoxSizer2->SetSizeHints(Panel2);
    BoxSizer1->Add(Panel2, 0, wxTOP|wxLEFT|wxRIGHT|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Panel3 = new wxPanel(Panel1, ID_PANEL3, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL3"));
    StaticBoxSizer1 = new wxStaticBoxSizer(wxHORIZONTAL, Panel3, _("Log"));
    LogCtrl = new wxTextCtrl(Panel3, ID_TEXTCTRL1, wxEmptyString, wxDefaultPosition, wxSize(390,100), wxTE_AUTO_SCROLL|wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|wxTE_RICH2, wxDefaultValidator, _T("ID_TEXTCTRL1"));
    wxFontEnumerator __FontEnumerator;
    __FontEnumerator.EnumerateFacenames();
    #if wxCHECK_VERSION(2, 8, 0)
    	const wxArrayString& __FontFaces = __FontEnumerator.GetFacenames();
    #else
    	const wxArrayString& __FontFaces = *__FontEnumerator.GetFacenames();
    #endif
    wxString __LogCtrlFontFace;
    if ( __FontFaces.Index(_T("DejaVu Sans Mono")) != wxNOT_FOUND )
    	__LogCtrlFontFace = _T("DejaVu Sans Mono");
    else if ( __FontFaces.Index(_T("Courier New")) != wxNOT_FOUND )
    	__LogCtrlFontFace = _T("Courier New");
    wxFont LogCtrlFont = wxSystemSettings::GetFont(wxSYS_OEM_FIXED_FONT);
    if ( !LogCtrlFont.Ok() ) LogCtrlFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    LogCtrlFont.SetPointSize((int)(LogCtrlFont.GetPointSize() * 1.200000));
    LogCtrlFont.SetFamily(wxSCRIPT);
    LogCtrlFont.SetFaceName(__LogCtrlFontFace);
    LogCtrl->SetFont(LogCtrlFont);
    StaticBoxSizer1->Add(LogCtrl, 1, wxEXPAND|wxALIGN_LEFT|wxALIGN_TOP, 0);
    Panel3->SetSizer(StaticBoxSizer1);
    StaticBoxSizer1->Fit(Panel3);
    StaticBoxSizer1->SetSizeHints(Panel3);
    BoxSizer1->Add(Panel3, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Panel1->SetSizer(BoxSizer1);
    BoxSizer1->Fit(Panel1);
    BoxSizer1->SetSizeHints(Panel1);
    Menu = new wxMenuBar();
    Menu1 = new wxMenu();
    MenuItem1 = new wxMenuItem(Menu1, idMenuQuit, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    Menu1->Append(MenuItem1);
    Menu->Append(Menu1, _("&File"));
    Menu3 = new wxMenu();
    LogInfos = new wxMenuItem(Menu3, ID_MENUITEM2, _("show informations"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(LogInfos);
    LogWarnings = new wxMenuItem(Menu3, ID_MENUITEM3, _("show warnings"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(LogWarnings);
    LogErrors = new wxMenuItem(Menu3, ID_MENUITEM4, _("show errors"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(LogErrors);
    clearBeforeRun = new wxMenuItem(Menu3, ID_MENUITEM5, _("clear before every run"), wxEmptyString, wxITEM_CHECK);
    Menu3->Append(clearBeforeRun);
    Menu->Append(Menu3, _("Logging"));
    Menu2 = new wxMenu();
    MenuItem3 = new wxMenuItem(Menu2, ID_MENUITEM1, _("Readme"), wxEmptyString, wxITEM_NORMAL);
    Menu2->Append(MenuItem3);
    MenuItem2 = new wxMenuItem(Menu2, idMenuAbout, _("About\tF1"), _("Show info about this application"), wxITEM_NORMAL);
    Menu2->Append(MenuItem2);
    Menu->Append(Menu2, _("Help"));
    SetMenuBar(Menu);
    StatusBar1 = new wxStatusBar(this, ID_STATUSBAR1, 0, _T("ID_STATUSBAR1"));
    int __wxStatusBarWidths_1[1] = { -1 };
    int __wxStatusBarStyles_1[1] = { wxSB_NORMAL };
    StatusBar1->SetFieldsCount(1,__wxStatusBarWidths_1);
    StatusBar1->SetStatusStyles(1,__wxStatusBarStyles_1);
    SetStatusBar(StatusBar1);
    FileChooser = new wxFileDialog(this, _("Select file"), wxEmptyString, wxEmptyString, _("ZSlang (*.zsl)|*.zsl|all files(*.*)|*.*"), wxFD_DEFAULT_STYLE|wxFD_OPEN|wxFD_FILE_MUST_EXIST, wxDefaultPosition, wxDefaultSize, _T("wxFileDialog"));
    
    Connect(ID_BUTTON1,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&zTransformFrame::BT_ChooseDirectory);
    Connect(ID_BUTTON2,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&zTransformFrame::BT_Run);
    Connect(idMenuQuit,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::OnQuit);
    Connect(ID_MENUITEM2,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::ToggleInformations);
    Connect(ID_MENUITEM3,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::ToggleWarnings);
    Connect(ID_MENUITEM4,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::ToggleErrors);
    Connect(ID_MENUITEM1,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::OnReadme);
    Connect(idMenuAbout,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&zTransformFrame::OnAbout);
    Connect(wxID_ANY,wxEVT_CLOSE_WINDOW,(wxObjectEventFunction)&zTransformFrame::OnClose);
    //*)
}

zTransformFrame::~zTransformFrame()
{
    //(*Destroy(zTransformFrame)
    //*)
}

void zTransformFrame::OnQuit(wxCommandEvent& event)
{
    Close();
}

void zTransformFrame::OnAbout(wxCommandEvent& event)
{
    wxString msg = "Program: " + cMain::getApplicationName() + "\n"
                 + "Version: " + cMain::getVersionString() + "\n"
                 + "Author:  " + "Sektenspinner";
    wxMessageBox(msg, _("Version Information"));
}

void zTransformFrame::BT_ChooseDirectory(wxCommandEvent& event)
{
    if (FileChooser->ShowModal() == wxID_OK) {
         FileName->ChangeValue(FileChooser->GetPath());
    }

}

void zTransformFrame::BT_Run(wxCommandEvent& event)
{
    if (APP->main) {
        if (this->clearBeforeRun->IsChecked()) {
            LogCtrl->Clear();
        }
        APP->main->runScript((const char*)FileName->GetValue().ToAscii());
        cLogger::getLogger()->commit();
    } else {
        FAULT("Main Component missing or already destructed?!");
    }
}

void zTransformFrame::OnClose(wxCloseEvent& event)
{
    APP->OnGUI_Shutdown();
    Destroy();
}

void zTransformFrame::OnReadme(wxCommandEvent& event)
{
    wxString readMePath = APP->getExecutablePath() + L"\\readme.txt";

    if (GetFileAttributes(readMePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wxMessageBox(string("Readme is missing."), _("Readme.txt not found."));
    } else {
        ShellExecute(0, L"open", readMePath.c_str(), 0, 0, SW_SHOWNORMAL) ;
    }
}

void zTransformFrame::ToggleInformations(wxCommandEvent& event)
{
    cLogger::setLogInfo(this->LogInfos->IsChecked());
}

void zTransformFrame::ToggleWarnings(wxCommandEvent& event)
{
    cLogger::setLogInfo(this->LogWarnings->IsChecked());
}

void zTransformFrame::ToggleErrors(wxCommandEvent& event)
{
    cLogger::setLogInfo(this->LogErrors->IsChecked());
}
