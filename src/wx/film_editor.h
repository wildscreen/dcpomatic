/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

    This file is part of DCP-o-matic.

    DCP-o-matic is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DCP-o-matic is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DCP-o-matic.  If not, see <http://www.gnu.org/licenses/>.

*/

/** @file src/wx/film_editor.h
 *  @brief FilmEditor class.
 */

#include "lib/film.h"
#include <wx/wx.h>
#include <boost/signals2.hpp>

class wxNotebook;
class Film;
class ContentPanel;
class DCPPanel;
class FilmViewer;

/** @class FilmEditor
 *  @brief A wx widget to edit a film's metadata, and perform various functions.
 */
class FilmEditor : public wxPanel
{
public:
	FilmEditor (wxWindow *, FilmViewer* viewer);

	void set_film (boost::shared_ptr<Film>);

	boost::signals2::signal<void (boost::filesystem::path)> FileChanged;

	/* Stuff for panels */

	ContentPanel* content_panel () const {
		return _content_panel;
	}

	boost::shared_ptr<Film> film () const {
		return _film;
	}

	/* Handle changes to the model */
	void film_changed (Film::Property);
	void film_content_changed (int);

	void set_general_sensitivity (bool);
	void active_jobs_changed (boost::optional<std::string>);

	wxNotebook* _main_notebook;
	ContentPanel* _content_panel;
	DCPPanel* _dcp_panel;

	/** The film we are editing */
	boost::shared_ptr<Film> _film;
};
