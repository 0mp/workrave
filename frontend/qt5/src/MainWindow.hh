// MainWindow.hh
//
// Copyright (C) 2006, 2007, 2013 Raymond Penners & Rob Caelers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef MAINWINDOW_HH
#define MAINWINDOW_HH

#include <QDialog>

#include <QDialog>
#include <QVBoxLayout>
#include "TimerBoxView.hh"
#include "TimerBoxControl.hh"

// namespace Ui {
//   class MainWindow;
// }

class MainWindow : public QDialog
{
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

  void on_heartbeat();
  
private:
  //Ui::MainWindow *ui;

  TimerBoxView *timer_box_view;
  TimerBoxControl *timer_box_control;
  QVBoxLayout *layout;
};

#endif // MAINWINDOW_HH
