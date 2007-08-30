// preinclude.h --- Patch includes
//
// Copyright (C) 2002, 2003, 2004 Raymond Penners <raymond@dotsphinx.com>
// All rights reserved.
//
// Time-stamp: <2007-08-30 16:03:01 robc>
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
//

#ifndef PREINCLUDE_HH
#define PREINCLUDE_HH

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef HAVE_GTKMM24
#define MEMBER_SLOT sigc::mem_fun
#else
#define MEMBER_SLOT SigC::slot
#endif


#undef THREAD_PRIORITY_NORMAL
#undef THREAD_PRIORITY_LOW
#undef THREAD_PRIORITY_HIGH
#undef THREAD_PRIORITY_URGENT
#undef DELETE
#undef ERROR
#undef OK


#endif
