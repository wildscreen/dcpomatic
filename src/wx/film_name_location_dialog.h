/*
    Copyright (C) 2012-2014 Carl Hetherington <cth@carlh.net>

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

#include <wx/wx.h>
#include <wx/filepicker.h>
#include "wx_util.h"
#include "table_dialog.h"

class DirPickerCtrl;

class FilmNameLocationDialog : public TableDialog
{
public:
	FilmNameLocationDialog (wxWindow *, wxString title, bool offer_templates);
	~FilmNameLocationDialog ();

	boost::filesystem::path path () const;
	bool check_path ();
	boost::optional<std::string> template_name () const;

private:
	void use_template_clicked ();

	wxTextCtrl* _name;
#ifdef DCPOMATIC_USE_OWN_PICKER
	DirPickerCtrl* _folder;
#else
	wxDirPickerCtrl* _folder;
#endif
	wxCheckBox* _use_template;
	wxChoice* _template_name;
	static boost::optional<boost::filesystem::path> _directory;
};
