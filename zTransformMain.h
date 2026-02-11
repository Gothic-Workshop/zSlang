/***************************************************************
 * Name:      zTransformMain.h
 * Purpose:   Defines Application Frame
 * Author:    Sekti ()
 * Created:   2011-03-11
 * Copyright: Sekti (http://forum.worldofplayers.de/forum/member.php?u=11242)
 * License:
 **************************************************************/

#error This file is obsolete.

#ifndef ZTRANSFORMMAIN_H
#define ZTRANSFORMMAIN_H

class zTransformApp;

class zTransformFrame: public wxFrame
{
    public:

        zTransformFrame(zTransformApp* APP, wxWindow* parent,wxWindowID id = -1);
        virtual ~zTransformFrame();

    private:

        //(*Handlers(zTransformFrame)
        void OnQuit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnLogCtrlText(wxCommandEvent& event);
        void OnPanel1Paint(wxPaintEvent& event);
        void BT_ChooseDirectory(wxCommandEvent& event);
        void BT_Run(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnReadme(wxCommandEvent& event);
        void ToggleInformations(wxCommandEvent& event);
        void ToggleWarnings(wxCommandEvent& event);
        void ToggleErrors(wxCommandEvent& event);
        //*)

        //(*Identifiers(zTransformFrame)
        static const long ID_TEXTCTRL2;
        static const long ID_BUTTON1;
        static const long ID_BUTTON2;
        static const long ID_PANEL2;
        static const long ID_TEXTCTRL1;
        static const long ID_PANEL3;
        static const long ID_PANEL1;
        static const long idMenuQuit;
        static const long ID_MENUITEM2;
        static const long ID_MENUITEM3;
        static const long ID_MENUITEM4;
        static const long ID_MENUITEM5;
        static const long ID_MENUITEM1;
        static const long idMenuAbout;
        static const long ID_STATUSBAR1;
        //*)

        //(*Declarations(zTransformFrame)
        wxMenuItem* LogErrors;
        wxFileDialog* FileChooser;
        wxMenuItem* LogWarnings;
        wxButton* Button1;
        wxTextCtrl* FileName;
        wxPanel* Panel1;
        wxButton* Button2;
        wxPanel* Panel3;
        wxTextCtrl* LogCtrl;
        wxStatusBar* StatusBar1;
        wxPanel* Panel2;
        wxMenuItem* LogInfos;
        wxMenuItem* clearBeforeRun;
        //*)

        zTransformApp *APP;

        DECLARE_EVENT_TABLE()

    friend class zTransformApp;
};

#endif // ZTRANSFORMMAIN_H
