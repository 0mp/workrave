// PreferencesParser.cc --- Preferences parser
//
// Copyright (C) 2002, 2003 Raymond Penners <raymond@dotsphinx.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_EXERCISES

#include "ExercisesParser.hh"
#include "Util.hh"

#include "nls.h"
#include "debug.hh"

#include <unistd.h>
#include <assert.h>

ExercisesParser::ExercisesParser(std::list<Exercise> &ex)
  : exercises(&ex), exercise(NULL)
{
}

void
ExercisesParser::on_start_element (Glib::Markup::ParseContext& context,
                                   const Glib::ustring& element_name,
                                   const AttributeMap& attributes)
{
  TRACE_ENTER_MSG("ExercisesParser::on_start_element", element_name);
  if (element_name == "exercise")
    {
      exercises->push_back(Exercise());
      exercise = &(*(exercises->end()));
    }
  AttributeMap::const_iterator it = attributes.find("xml:lang");
  if (it != attributes.end())
    {
      lang = (*it).second;
    }
  else
    {
      lang = "";
    }
  cdata = "";
  TRACE_EXIT();
}

void
ExercisesParser::on_end_element (Glib::Markup::ParseContext& context,
                                 const Glib::ustring& element_name)
{
  TRACE_ENTER_MSG("ExercisesParser::on_end_element", element_name);
  if (element_name == "title")
    {
      TRACE_MSG("title=" << cdata);
    }
  else if (element_name == "description")
    {
      TRACE_MSG("desc=" << cdata);
    }
  TRACE_MSG("lang=" << lang);
  TRACE_EXIT();
}

void
ExercisesParser::on_text (Glib::Markup::ParseContext& context,
                          const Glib::ustring& text)
{
  TRACE_ENTER_MSG("ExercisesParser::on_text", text);
  cdata.append(text);
  TRACE_EXIT();
}

void
ExercisesParser::on_passthrough (Glib::Markup::ParseContext& context,
                                 const Glib::ustring& passthrough_text)
{
  TRACE_ENTER_MSG("ExercisesParser::on_passthrough", passthrough_text);
  TRACE_EXIT();
}
   

void
ExercisesParser::parse_exercises(Glib::ustring file_name,
                                 std::list<Exercise> &exe)
{
  TRACE_ENTER_MSG("ExercisesParser::get_exercises", file_name);
  
  // I hate C++ streams.
  FILE *stream = fopen(file_name.c_str(), "rb");
  if (stream)
    {
      ExercisesParser parser(exe);
      Glib::Markup::ParseContext context(parser);

      char buf[1024];
      while (true)
        {
          int n = fread(buf, 1, sizeof(buf), stream);
          if (ferror(stream))
            break;
          context.parse(buf, buf + n);
          if (feof(stream))
            break;
        }
      fclose(stream);
      context.end_parse();
    }
  TRACE_EXIT();
}


void
ExercisesParser::parse_exercises(std::list<Exercise> &exercises)
{
  Glib::ustring file_name = Util::complete_directory
    ("exercises.xml", Util::SEARCH_PATH_EXERCISES);
  return parse_exercises(file_name, exercises);
}

#endif // HAVE_EXERCISES
