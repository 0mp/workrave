// Copyright (C) 2013 Rob Caelers <robc@krandor.nl>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/make_shared.hpp>

#include "ToolkitMenu.hh"
#include "MenuModel.hh"

using namespace std;


ToolkitMenu::Ptr
ToolkitMenu::create(MenuModel::Ptr menu_model)
{
  return Ptr(new ToolkitMenu(menu_model));
}

ToolkitMenu::ToolkitMenu(MenuModel::Ptr menu_model)
{
  menu = boost::make_shared<detail::SubMenuEntry>(menu_model);
}

ToolkitMenu::~ToolkitMenu()
{
}

QMenu *
ToolkitMenu::get_menu() const
{
  return menu->get_menu();
}

namespace detail
{
MenuEntry::Ptr
MenuEntry::create(MenuModel::Ptr menu_model)
{
  MenuModelType type = menu_model->get_type();
  
  if (type == MenuModelType::MENU)
    {
      return Ptr(new SubMenuEntry(menu_model));
    }
  else
    {
      return Ptr(new ActionMenuEntry(menu_model));
    }
}
  
MenuEntry::MenuEntry(MenuModel::Ptr menu_model) : menu_model(menu_model)
{
}

MenuModel::Ptr 
MenuEntry::get_menu_model() const
{
  return menu_model;
}

SubMenuEntry::SubMenuEntry(MenuModel::Ptr menu_model)
  : MenuEntry(menu_model)
{
  menu = new QMenu(menu_model->get_text().c_str());

  for (const MenuModel::Ptr &item : menu_model->get_submenus())
    {
      MenuEntry::Ptr child = MenuEntry::create(item);
      children.push_back(child);
      
      menu->insertAction(0, child->get_action());
    }

  menu_model->signal_added().connect(boost::bind(&SubMenuEntry::on_menu_added, this, _1, _2)); 
  menu_model->signal_removed().connect(boost::bind(&SubMenuEntry::on_menu_removed, this, _1)); 
  menu_model->signal_changed().connect(boost::bind(&SubMenuEntry::on_menu_changed, this)); 
}

SubMenuEntry::~SubMenuEntry()
{
}

void
SubMenuEntry::on_menu_added(MenuModel::Ptr added, MenuModel::Ptr before)
{
  QAction *before_action = nullptr;
  MenuEntries::iterator pos = children.end();
  if (before)
    {
      pos = std::find_if(children.begin(), children.end(), [&] (MenuEntry::Ptr i) { return i->get_menu_model() == before; });
      if (pos != children.end())
        {
          before_action = (*pos)->get_action();
        }
    }

  MenuEntry::Ptr child = MenuEntry::create(added);
  children.insert(pos, child);

  menu->insertAction(before_action, child->get_action());
}

void
SubMenuEntry::on_menu_removed(MenuModel::Ptr removed)
{
  MenuEntries::iterator pos = std::find_if(children.begin(), children.end(), [&] (MenuEntry::Ptr i) { return i->get_menu_model() == removed; });
  if (pos != children.end())
    {
      children.erase(pos);
    }
}
  
void
SubMenuEntry::on_menu_changed()
{
  const char *text = menu_model->get_text().c_str();
  menu->setTitle(text);
}

QAction *
SubMenuEntry::get_action() const
{
  return menu->menuAction();
}

QMenu *
SubMenuEntry::get_menu() const
{
  return menu;
}
  

ActionMenuEntry::ActionMenuEntry(MenuModel::Ptr menu_model)
  : MenuEntry(menu_model)
{
  menu_model->signal_changed().connect(boost::bind(&ActionMenuEntry::on_menu_changed, this)); 

  action = new QAction(menu_model->get_text().c_str(), this);
  action->setCheckable(menu_model->get_type() == MenuModelType::RADIO || menu_model->get_type() == MenuModelType::CHECK);
  action->setChecked(menu_model->is_checked());
      
  connect(action, &QAction::triggered, this, &ActionMenuEntry::on_action);
}

ActionMenuEntry::~ActionMenuEntry()
{
}

QAction *
ActionMenuEntry::get_action() const
{
  return action;
}
  
void
ActionMenuEntry::on_menu_changed()
{
  if (action != NULL)
    {
      action->setText(menu_model->get_text().c_str());
      action->setCheckable(menu_model->get_type() == MenuModelType::RADIO || menu_model->get_type() == MenuModelType::CHECK);
      action->setChecked(menu_model->is_checked());
    }
}

void
ActionMenuEntry::on_action(bool checked)
{
  menu_model->activate();
}
  
}
