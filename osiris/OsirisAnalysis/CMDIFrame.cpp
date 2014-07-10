/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE                          
*               National Center for Biotechnology Information
*                                                                          
*  This software/database is a "United States Government Work" under the   
*  terms of the United States Copyright Act.  It was written as part of    
*  the author's official duties as a United States Government employee and 
*  thus cannot be copyrighted.  This software/database is freely available 
*  to the public for use. The National Library of Medicine and the U.S.    
*  Government have not placed any restriction on its use or reproduction.  
*                                                                          
*  Although all reasonable efforts have been taken to ensure the accuracy  
*  and reliability of the software and data, the NLM and the U.S.          
*  Government do not and cannot warrant the performance or results that    
*  may be obtained by using this software or data. The NLM and the U.S.    
*  Government disclaim all warranties, express or implied, including       
*  warranties of performance, merchantability or fitness for any particular
*  purpose.                                                                
*                                                                          
*  Please cite the author in any work or product based on this material.   
*
* ===========================================================================
*
*  FileName: CMDIFrame.cpp
*  Author:   Douglas Hoffman
*
*/
#include "mainFrame.h"
#include <wx/msgdlg.h>
#include "CMDIFrame.h"
#include "wxIDS.h"
#include "nwx/CIncrementer.h"
#include "Platform.h"


DEFINE_EVENT_TYPE(CEventKillWindow)

CMDIFrame::~CMDIFrame()
{
  m_pParent->RemoveWindow(this);
}

CMDIFrame::CMDIFrame(
    mainFrame* parent, 
    wxWindowID id, 
    const wxString& title, 
    const wxPoint& pos,
    const wxSize& size,
    long style) : CMDIFrameSuper(
      CMDIFramePARENT,
			id, title, 
      pos
      , size, style),
      m_pParent(parent)
{
  m_nFocusRecursive = 0;
  m_bNoPromptReload = false;
  m_bAutoReload = false;
  m_pLastMenuShown = NULL;
  parent->InsertWindow(this);
#ifdef MANUALLY_PLACE_FRAMES
  parent->PlaceFrame(this);
#endif
}

bool CMDIFrame::MenuEvent(wxCommandEvent &) { return false;}
wxMenu *CMDIFrame::GetMenu() { return NULL; }
wxMenu *CMDIFrame::GetGraphMenu() { return NULL; }
wxMenu *CMDIFrame::GetTableMenu() { return NULL; }


void CMDIFrame::UpdateHistory() {;}
void CMDIFrame::UpdateLadderLabels() {;}
void CMDIFrame::UpdateFileMenu() {;}

wxString CMDIFrame::GetFileName()
{
  // base class returns empty string
  wxString s;
  return s;
}
bool CMDIFrame::SetToolbarMenuLabel(bool bShow, bool bPlural)
{
  wxMenu *pMenu = GetMenu();
  bool bRtn = false;
  if(pMenu != NULL)
  {
    wxMenuItem *pItem = pMenu->FindItem(IDmenuShowHideToolbar);
    if(pItem != NULL)
    {
      const wxString *psLabel(NULL);
      switch((bPlural ? 1 : 0) + (bShow ? 2 : 0))
      {
      case 0: // !plural && !show
        psLabel = &HIDE_TOOLBAR; break;
      case 1: // plural && !show
        psLabel = &HIDE_TOOLBARS; break;
      case 2: // !plural && show
        psLabel = &SHOW_TOOLBAR; break;
      case 3: // plural && show
        psLabel = &SHOW_TOOLBARS; break;
      }
      pItem->SetText(*psLabel);
      bRtn = true;
    }
  }
  return bRtn;
}
bool CMDIFrame::Destroy()
{
  CIncrementer xxx(m_nFocusRecursive);
  wxBusyCursor xx;
  return CMDIFrameSuper::Destroy();
}

bool CMDIFrame::Show(bool show)
{
  bool bRtn = CMDIFrameSuper::Show(show);
  if(show)
  {
    Raise();
    _NotifyParent();
  }
  return bRtn;
}
void CMDIFrame::CheckFileModification() {}
bool CMDIFrame::IsEdited() { return false; }

bool CMDIFrame::PromptReload(const wxString &sFileName, const wxString &sFileName2)
{
  bool bRtn = false;
  if(m_bAutoReload)
  {
    bRtn = true;
  }
  else if(!m_bNoPromptReload)
  {
    wxString sPrompt;
    if(sFileName.IsEmpty())
    {
      sPrompt =
        "This file has been modified,\n"
        "would you like to reload it?";
    }
    else if(!sFileName2.IsEmpty())
    {
      wxFileName fn(sFileName);
      wxFileName fn2(sFileName2);
      sPrompt = "These files:\n  ";
      sPrompt.Append(fn.GetFullName());
      sPrompt.Append("\n  ");
      sPrompt.Append(fn2.GetFullName());
      sPrompt.Append("\nhave been modified,\n"
        "would you like to reload them?");
    }
    else
    {
      wxFileName fn(sFileName);
      sPrompt = "This file, ";
      sPrompt.Append(fn.GetFullName());
      sPrompt.Append(", has been modified,\n"
        "would you like to reload it?");
    }
    wxMessageDialog dlg(
      this,sPrompt,"File Update",
      wxYES_NO | wxICON_QUESTION);
    int n = dlg.ShowModal();
    bRtn = (n == wxID_YES);
    m_bNoPromptReload = !bRtn; // user said no, do not ask again
  }
  return bRtn;
}

void CMDIFrame::UpdateStatusBar()
{
#if HAS_STATUS_BAR
  m_pParent->SetStatusText("");
#endif
}

void CMDIFrame::OnActivate(wxActivateEvent &e)
{
  if(!e.GetActive())
  {
    m_pParent->KillActiveFrame(this);
  }
  else if(!m_nFocusRecursive)
  {
    CIncrementer xxx(m_nFocusRecursive);
    m_pParent->SetActiveFrame(this);
    CheckFileModification();
  }
  e.Skip();
}
void CMDIFrame::OnFocusSet(wxFocusEvent &e)
{
  if(!m_nFocusRecursive)
  {
    CIncrementer xxx(m_nFocusRecursive);
    m_pParent->SetActiveFrame(this);
    CheckFileModification();
  }
  e.Skip(); // not sure why
}
void CMDIFrame::OnFocusKill(wxFocusEvent &e)
{
  m_pParent->KillActiveFrame(this);
  e.Skip(); // not sure why
}
void CMDIFrame::OnMenuOpen(wxMenuEvent &e)
{
  SetLastMenuShown(e.GetMenu());
}
void CMDIFrame::OnMenuClose(wxMenuEvent &)
{
#ifndef __NO_MDI__
  CMDIFrame *pFrame = (CMDIFrame *)GetActiveChild();
  if(pFrame != NULL)
  {
    pFrame->UpdateStatusBar();
  }
#endif
}
void CMDIFrame::OnDoClose(wxCommandEvent &)
{
  wxBusyCursor xxx;
  if( this->Close() ) this->Destroy();
}

void CMDIFrame::RaiseWindow()
{
  if(IsIconized())
  {
    Iconize(false);
  }
  Activate();
}
void CMDIFrame::_NotifyParent()
{
  m_pParent->CheckActiveFrame();
}

bool CMDIFrame::PopupMenu_(wxMenu *menu, const wxPoint &pos)
{
  bool bRtn = PopupMenu(menu, pos );
  UpdateStatusBar();
  return bRtn;
}
bool CMDIFrame::PopupMenu_(wxMenu *menu, int x, int y)
{
  bool bRtn = PopupMenu(menu,x,y);
  UpdateStatusBar();
  return bRtn;
}
void CMDIFrame::_OnMenuEvent(wxCommandEvent &e)
{
  if(!MenuEvent(e))
  {
    e.Skip();
  }
}
void CMDIFrame::_Kill()
{
  wxCommandEvent ee(CEventKillWindow,GetId());
  ee.SetEventObject(this);
  AddPendingEvent(ee);
}

bool CMDIFrame::DialogIsShowingOrNoFocus()
{
  wxWindow *pW = wxWindow::FindFocus();
  bool bRtn = (pW == NULL);
  if(!bRtn)
  {
    wxClassInfo *pClass(CLASSINFO(wxTopLevelWindow));
    while((pW != NULL) && (!pW->IsKindOf(pClass)))
    {
      pW = pW->GetParent();
    }
    bRtn = (pW != NULL) &&
      pW->IsKindOf(CLASSINFO(wxDialog));
  }
  return bRtn;
}

const wxString CMDIFrame::HIDE_TOOLBARS("Hide Toolbars\tCtrl+T");
const wxString CMDIFrame::SHOW_TOOLBARS("Show Toolbars\tCtrl+T");
const wxString CMDIFrame::HIDE_TOOLBAR("Hide Toolbar\tCtrl+T");
const wxString CMDIFrame::SHOW_TOOLBAR("Show Toolbar\tCtrl+T");

BEGIN_EVENT_TABLE(CMDIFrame,CMDIFrameSuper)
EVT_MENU(wxID_CLOSE,  CMDIFrame::OnDoClose)
EVT_MENU_OPEN(CMDIFrame::OnMenuOpen)
EVT_MENU_CLOSE(CMDIFrame::OnMenuClose)
EVT_SET_FOCUS(CMDIFrame::OnFocusSet)
EVT_KILL_FOCUS(CMDIFrame::OnFocusKill)
EVT_ACTIVATE(CMDIFrame::OnActivate)
EVT_MENU_RANGE(IDmenu_START, IDmenu_END, CMDIFrame::_OnMenuEvent)
EVT_COMMAND(wxID_ANY,CEventKillWindow,CMDIFrame::OnDoClose)
END_EVENT_TABLE()

IMPLEMENT_ABSTRACT_CLASS(CMDIFrame,CMDIFrameSuper)
