// TimeBar.cpp --- Time bar
//
// Copyright (C) 2004 Raymond Penners <raymond@dotsphinx.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$

#include <stdio.h>

#include "TimeBar.h"

const int MARGINX = 4;
const int MARGINY = 3;
const int MIN_HORIZONTAL_BAR_HEIGHT = 20; // stolen from gtk's progress bar

#define TIME_BAR_CLASS_NAME "WorkraveTimeBar"
#define GDK_TO_COLORREF(r, g, b) (((r)>>8) | (((g)>>8) <<8) | (((b)>>8) << 16))

HBRUSH TimeBar::bar_colors[TimeBarInterface::COLOR_ID_SIZEOF];
HFONT TimeBar::bar_font = NULL;

TimeBar::TimeBar(HWND parent, HINSTANCE hinst)
{
  init(hinst);

  bar_text[0] = 0;
  bar_max_value = 100;
  bar_value = 0;
  secondary_bar_max_value = 0;
  secondary_bar_value = 100;
  secondary_bar_color = TimeBarInterface::COLOR_ID_INACTIVE;
  bar_color = TimeBarInterface::COLOR_ID_ACTIVE;

  hwnd = CreateWindowEx(0, TIME_BAR_CLASS_NAME, "",
                        WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 56, 16, parent, NULL, hinst, NULL);
  SetWindowLong(hwnd, GWL_USERDATA, (LONG)this);

  compute_size(width, height);
  SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
}

TimeBar::~TimeBar()
{
  DestroyWindow(hwnd);
}

LRESULT CALLBACK
TimeBar::wnd_proc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
  TimeBar  *pThis = (TimeBar*)GetWindowLong(hWnd, GWL_USERDATA);

  switch (uMessage)
    {
    case WM_PAINT:
      return pThis->on_paint();
    }
  return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void
TimeBar::get_size(int &w, int &h)
{
  w = width;
  h = height;
}


void
TimeBar::compute_size(int &w, int &h)
{
  HDC dc = GetDC(hwnd);
  SelectObject(dc, (HGDIOBJ) bar_font);
  char buf[80];

  time_to_string(-(59+59*60+9*60*60), buf, sizeof(buf));

  RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.bottom = 0;
  rect.right = 0;
  h= DrawText(dc, buf, strlen(buf), &rect, DT_CALCRECT);
  if (! h)
    {
      w = 32;
      h = 16;
    }
  else
    {
      w= rect.right;
    }

  w += 2*MARGINX;
  h += 2*MARGINY;
  if (h < MIN_HORIZONTAL_BAR_HEIGHT) 
    {
      h = MIN_HORIZONTAL_BAR_HEIGHT;
    }
  ReleaseDC(hwnd, dc);
}

LRESULT
TimeBar::on_paint(void)
{
  PAINTSTRUCT ps;
  RECT        rc;

  BeginPaint(hwnd, &ps);
  HDC dc = GetDC(hwnd);

  GetClientRect(hwnd, &rc);


  RECT r;

  int winx = rc.left, winy = rc.top, winw = rc.right-rc.left, winh = rc.bottom-rc.top;

  SelectObject(dc, (HGDIOBJ) bar_font);
  r.left = r.top = r.bottom = r.right = 0;

  // Bar
  int bar_width = 0;
  int border_size = 2;
  if (bar_max_value > 0)
    {
      bar_width = (bar_value * (winw - 2 * border_size)) / bar_max_value;
    }

  // Secondary bar
  int sbar_width = 0;
  if (secondary_bar_max_value >  0)
    {
      sbar_width = (secondary_bar_value * (winw - 2 * border_size)) / secondary_bar_max_value;
    }

  int bar_h = winh - 2 * border_size;

  if (sbar_width > 0)
    {
      // Overlap
      //assert(secondary_bar_color == COLOR_ID_INACTIVE);
      TimeBarInterface::ColorId overlap_color;
      switch (bar_color)
        {
        case TimeBarInterface::COLOR_ID_ACTIVE:
          overlap_color = TimeBarInterface::COLOR_ID_INACTIVE_OVER_ACTIVE;
          break;
        case TimeBarInterface::COLOR_ID_OVERDUE:
          overlap_color = TimeBarInterface::COLOR_ID_INACTIVE_OVER_OVERDUE;
          break;
        default:
          abort();
        }

      if (sbar_width >= bar_width)
        {
          if (bar_width)
            {
              r.left = winx+ border_size ;
              r.top = winy+ border_size ;
              r.right = r.left + bar_width;
              r.bottom = r.top + bar_h;
              FillRect(dc, &r, bar_colors[overlap_color]);               }
          if (sbar_width > bar_width)
            {
              r.left = winx + bar_width+ border_size ;
              r.top = winy+ border_size ;
              r.right = r.left + sbar_width - bar_width;
              r.bottom = r.top + bar_h;
              FillRect(dc, &r, bar_colors[secondary_bar_color]);               }
        }
      else
        {
          if (sbar_width)
            {
              r.left = winx+ border_size ;
              r.top = winy+ border_size ;
              r.right = r.left + sbar_width;
              r.bottom = r.top + bar_h;
              FillRect(dc, &r, bar_colors[overlap_color]);   
            }
          r.left = winx + border_size + sbar_width;
          r.top = winy + border_size ;
          r.right = r.left + bar_width - sbar_width;
          r.bottom = r.top + bar_h;
          FillRect(dc, &r, bar_colors[bar_color]);   
        }
    }
  else
    {
      // No overlap
      r.left = winx + border_size;
      r.top = winy + border_size;
      r.right = r.left + bar_width;
      r.bottom = r.top + bar_h;
      FillRect(dc, &r, bar_colors[bar_color]);       }
    
  r.left = winx + border_size + __max(bar_width, sbar_width);
  r.top = winy + border_size;
  r.right = winx + winw - border_size;
  r.bottom = r.top + bar_h;
  FillRect(dc, &r, bar_colors[TimeBarInterface::COLOR_ID_BG]);     


  r.left = winx;
  r.top = winy;
  r.bottom = r.top + winh;
  r.right = r.left + winw;
  DrawEdge(dc, &r, BF_ADJUST|EDGE_SUNKEN,BF_RECT);

  SetBkMode(dc, TRANSPARENT);
  DrawText(dc, bar_text, strlen(bar_text), &r, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);

  ReleaseDC(hwnd, dc);
  EndPaint(hwnd, &ps);

  return 0;
}

void
TimeBar::init(HINSTANCE hinst)
{
  //If the window class has not been registered, then do so.
  WNDCLASS wc;
  if(!GetClassInfo(hinst, TIME_BAR_CLASS_NAME, &wc))
    {
      ZeroMemory(&wc, sizeof(wc));
      wc.style          = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
      wc.lpfnWndProc    = (WNDPROC)wnd_proc;
      wc.cbClsExtra     = 0;
      wc.cbWndExtra     = 0;
      wc.hInstance      = hinst;
      wc.hIcon          = NULL;
      wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground  = (HBRUSH)CreateSolidBrush(RGB(192, 0, 0));
      wc.lpszMenuName   = NULL;
      wc.lpszClassName  = TIME_BAR_CLASS_NAME;
      
      RegisterClass(&wc);

      HBRUSH green_mix_blue = CreateSolidBrush(0x00b2d400);
      HBRUSH light_green = CreateSolidBrush(GDK_TO_COLORREF(37008,61166,37008));
      HBRUSH orange = CreateSolidBrush(GDK_TO_COLORREF(65535, 42405, 0));
      HBRUSH light_blue = CreateSolidBrush(GDK_TO_COLORREF(44461, 55512, 59110));
      HBRUSH bg = CreateSolidBrush(GetSysColor(COLOR_3DLIGHT));

      bar_colors[TimeBarInterface::COLOR_ID_ACTIVE] = light_blue;
      bar_colors[TimeBarInterface::COLOR_ID_INACTIVE] = light_green;
      bar_colors[TimeBarInterface::COLOR_ID_OVERDUE] = orange;
      bar_colors[TimeBarInterface::COLOR_ID_INACTIVE_OVER_ACTIVE] = green_mix_blue;
      bar_colors[TimeBarInterface::COLOR_ID_INACTIVE_OVER_OVERDUE] = light_green;
      bar_colors[TimeBarInterface::COLOR_ID_BG] = bg;

      NONCLIENTMETRICS ncm;
      ncm.cbSize = sizeof(NONCLIENTMETRICS);

      SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
      bar_font = CreateFontIndirect(&ncm.lfStatusFont);

    }
}





//! Converts the specified time to a string
void
TimeBar::time_to_string(time_t time, char *buf, int len)
{
  // FIXME: Should validate len.
  char t[2];
  
  if (time < 0)
    {
      t[0] = '-';
      t[1] = 0;
      time = -time;
    }
  else
    {
      t[0] = 0;
    }
  int hrs = time/3600;
  int min = (time / 60) % 60;
  int sec = time % 60;
  
  if (hrs > 0)
    {
      sprintf(buf, "%s%d:%02d:%02d", t, hrs, min, sec);
    }
  else
    {
      sprintf(buf, "%s%d:%02d", t, min, sec);
    }
}

void
TimeBar::set_position(BOOL visible, int x, int y)
{
  if (! visible)
    {
      ShowWindow(hwnd, SW_HIDE);
    }
  else
    {
      SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
    }
}



void 
TimeBar::set_progress(int value, int max_value)
{
  bar_value = value;
  bar_max_value = max_value;
}

void 
TimeBar::set_secondary_progress(int value, int max_value)
{
  secondary_bar_value = value;
  secondary_bar_max_value = max_value;
}
  
void 
TimeBar::set_text(const char *text)
{
  strncpy(bar_text, text, APPLET_BAR_TEXT_MAX_LENGTH-1);
  bar_text[APPLET_BAR_TEXT_MAX_LENGTH-1] = '\0';

}

void 
TimeBar::update()
{
  RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
}

void 
TimeBar::set_bar_color(TimeBarInterface::ColorId color)
{
  bar_color = color;
}

void 
TimeBar::set_secondary_bar_color(TimeBarInterface::ColorId color)
{
  secondary_bar_color = color;
}
