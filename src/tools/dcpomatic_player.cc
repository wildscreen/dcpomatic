/*
    Copyright (C) 2017 Carl Hetherington <cth@carlh.net>

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

#include "lib/cross.h"
#include "lib/config.h"
#include "lib/util.h"
#include "lib/update_checker.h"
#include "lib/compose.hpp"
#include "lib/dcp_content.h"
#include "lib/job_manager.h"
#include "lib/job.h"
#include "lib/video_content.h"
#include "lib/subtitle_content.h"
#include "wx/wx_signal_manager.h"
#include "wx/wx_util.h"
#include "wx/about_dialog.h"
#include "wx/report_problem_dialog.h"
#include "wx/film_viewer.h"
#include "wx/player_information.h"
#include "wx/update_dialog.h"
#include "wx/player_config_dialog.h"
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/splash.h>
#include <wx/cmdline.h>
#include <wx/preferences.h>
#include <wx/progdlg.h>
#ifdef __WXOSX__
#include <ApplicationServices/ApplicationServices.h>
#endif
#include <boost/bind.hpp>
#include <iostream>

#ifdef check
#undef check
#endif

using std::string;
using std::cout;
using std::exception;
using boost::shared_ptr;
using boost::optional;

enum {
	ID_file_open = 1,
	ID_file_add_ov,
	ID_file_add_kdm,
	ID_view_scale_appropriate,
	ID_view_scale_full,
	ID_view_scale_half,
	ID_view_scale_quarter,
	ID_help_report_a_problem,
	ID_tools_check_for_updates,
};

class DOMFrame : public wxFrame
{
public:
	DOMFrame ()
		: wxFrame (0, -1, _("DCP-o-matic Player"))
		, _update_news_requested (false)
		, _info (0)
		, _config_dialog (0)
		, _viewer (0)
	{

#if defined(DCPOMATIC_WINDOWS)
		maybe_open_console ();
		cout << "DCP-o-matic Player is starting." << "\n";
#endif

		wxMenuBar* bar = new wxMenuBar;
		setup_menu (bar);
		SetMenuBar (bar);

#ifdef DCPOMATIC_WINDOWS
		SetIcon (wxIcon (std_to_wx ("id")));
#endif

		_config_changed_connection = Config::instance()->Changed.connect (boost::bind (&DOMFrame::config_changed, this));

		Bind (wxEVT_MENU, boost::bind (&DOMFrame::file_open, this), ID_file_open);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::file_add_ov, this), ID_file_add_ov);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::file_add_kdm, this), ID_file_add_kdm);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::file_exit, this), wxID_EXIT);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::edit_preferences, this), wxID_PREFERENCES);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::set_decode_reduction, this, optional<int>()), ID_view_scale_appropriate);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::set_decode_reduction, this, optional<int>(0)), ID_view_scale_full);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::set_decode_reduction, this, optional<int>(1)), ID_view_scale_half);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::set_decode_reduction, this, optional<int>(2)), ID_view_scale_quarter);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::help_about, this), wxID_ABOUT);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::help_report_a_problem, this), ID_help_report_a_problem);
		Bind (wxEVT_MENU, boost::bind (&DOMFrame::tools_check_for_updates, this), ID_tools_check_for_updates);

		/* Use a panel as the only child of the Frame so that we avoid
		   the dark-grey background on Windows.
		*/
		wxPanel* overall_panel = new wxPanel (this, wxID_ANY);

		_viewer = new FilmViewer (overall_panel, false, false);
		_info = new PlayerInformation (overall_panel, _viewer);
		wxSizer* main_sizer = new wxBoxSizer (wxVERTICAL);
		main_sizer->Add (_viewer, 1, wxEXPAND | wxALL, 6);
		main_sizer->Add (_info, 0, wxEXPAND | wxALL, 6);
		overall_panel->SetSizer (main_sizer);

		UpdateChecker::instance()->StateChanged.connect (boost::bind (&DOMFrame::update_checker_state_changed, this));
	}

	void set_decode_reduction (optional<int> reduction)
	{
		_viewer->set_dcp_decode_reduction (reduction);
	}

	void load_dcp (boost::filesystem::path dir)
	{
		_film.reset (new Film (optional<boost::filesystem::path>()));
		shared_ptr<DCPContent> dcp (new DCPContent (_film, dir));
		_film->examine_and_add_content (dcp, true);

		JobManager* jm = JobManager::instance ();

		wxProgressDialog* progress = new wxProgressDialog (_("DCP-o-matic Player"), _("Loading DCP"));

		while (jm->work_to_do() || signal_manager->ui_idle()) {
			dcpomatic_sleep (1);
			progress->Pulse ();
		}

		progress->Destroy ();

		DCPOMATIC_ASSERT (!jm->get().empty());

		shared_ptr<Job> last = jm->get().back();
		if (last->finished_in_error()) {
			error_dialog (this, std_to_wx (last->error_summary()) + ".\n");
			return;
		}

		if (dcp->subtitle) {
			dcp->subtitle->set_use (true);
		}

		_viewer->set_film (_film);
		_info->triggered_update ();
	}

private:

	void setup_menu (wxMenuBar* m)
	{
		wxMenu* file = new wxMenu;
		file->Append (ID_file_open, _("&Open...\tCtrl-O"));
		file->Append (ID_file_add_ov, _("&Add OV..."));
		file->Append (ID_file_add_kdm, _("&Add KDM..."));

#ifdef __WXOSX__
		file->Append (wxID_EXIT, _("&Exit"));
#else
		file->Append (wxID_EXIT, _("&Quit"));
#endif

#ifdef __WXOSX__
		file->Append (wxID_PREFERENCES, _("&Preferences...\tCtrl-P"));
#else
		wxMenu* edit = new wxMenu;
		edit->Append (wxID_PREFERENCES, _("&Preferences...\tCtrl-P"));
#endif

		wxMenu* view = new wxMenu;
		view->AppendRadioItem (ID_view_scale_appropriate, _("Set decode resolution to match display"));
		view->AppendRadioItem (ID_view_scale_full, _("Decode at full resolution"));
		view->AppendRadioItem (ID_view_scale_half, _("Decode at half resolution"));
		view->AppendRadioItem (ID_view_scale_quarter, _("Decode at quarter resolution"));

		wxMenu* tools = new wxMenu;
		tools->Append (ID_tools_check_for_updates, _("Check for updates"));

		wxMenu* help = new wxMenu;
#ifdef __WXOSX__
		help->Append (wxID_ABOUT, _("About DCP-o-matic"));
#else
		help->Append (wxID_ABOUT, _("About"));
#endif
		help->Append (ID_help_report_a_problem, _("Report a problem..."));

		m->Append (file, _("&File"));
#ifndef __WXOSX__
		m->Append (edit, _("&Edit"));
#endif
		m->Append (view, _("&View"));
		m->Append (tools, _("&Tools"));
		m->Append (help, _("&Help"));
	}

	void file_open ()
	{
		wxString d = wxStandardPaths::Get().GetDocumentsDir();
		if (Config::instance()->last_player_load_directory()) {
			d = std_to_wx (Config::instance()->last_player_load_directory()->string());
		}

		wxDirDialog* c = new wxDirDialog (this, _("Select DCP to open"), d, wxDEFAULT_DIALOG_STYLE | wxDD_DIR_MUST_EXIST);

		int r;
		while (true) {
			r = c->ShowModal ();
			if (r == wxID_OK && c->GetPath() == wxStandardPaths::Get().GetDocumentsDir()) {
				error_dialog (this, _("You did not select a folder.  Make sure that you select a folder before clicking Open."));
			} else {
				break;
			}
		}

		if (r == wxID_OK) {
			boost::filesystem::path const dcp (wx_to_std (c->GetPath ()));
			load_dcp (dcp);
			Config::instance()->set_last_player_load_directory (dcp.parent_path());
		}

		c->Destroy ();
	}

	void file_add_ov ()
	{
		wxDirDialog* c = new wxDirDialog (
			this,
			_("Select DCP to open as OV"),
			wxStandardPaths::Get().GetDocumentsDir(),
			wxDEFAULT_DIALOG_STYLE | wxDD_DIR_MUST_EXIST
			);

		int r;
		while (true) {
			r = c->ShowModal ();
			if (r == wxID_OK && c->GetPath() == wxStandardPaths::Get().GetDocumentsDir()) {
				error_dialog (this, _("You did not select a folder.  Make sure that you select a folder before clicking Open."));
			} else {
				break;
			}
		}

		if (r == wxID_OK) {
			shared_ptr<DCPContent> dcp = boost::dynamic_pointer_cast<DCPContent>(_film->content().front());
			DCPOMATIC_ASSERT (dcp);
			dcp->add_ov (wx_to_std(c->GetPath()));
			dcp->examine (shared_ptr<Job>());
		}

		c->Destroy ();
		_info->triggered_update ();
	}

	void file_add_kdm ()
	{
		wxFileDialog* d = new wxFileDialog (this, _("Select KDM"));

		if (d->ShowModal() == wxID_OK) {
			shared_ptr<DCPContent> dcp = boost::dynamic_pointer_cast<DCPContent>(_film->content().front());
			DCPOMATIC_ASSERT (dcp);
			try {
				dcp->add_kdm (dcp::EncryptedKDM (dcp::file_to_string (wx_to_std (d->GetPath ()), MAX_KDM_SIZE)));
			} catch (exception& e) {
				error_dialog (this, wxString::Format (_("Could not load KDM (%s)"), e.what ()));
				d->Destroy ();
				return;
			}

			dcp->examine (shared_ptr<Job>());
		}

		d->Destroy ();
		_info->triggered_update ();
	}

	void file_exit ()
	{
		Close ();
	}

	void edit_preferences ()
	{
		if (!_config_dialog) {
			_config_dialog = create_player_config_dialog ();
		}
		_config_dialog->Show (this);
	}

	void tools_check_for_updates ()
	{
		UpdateChecker::instance()->run ();
		_update_news_requested = true;
	}

	void help_about ()
	{
		AboutDialog* d = new AboutDialog (this);
		d->ShowModal ();
		d->Destroy ();
	}

	void help_report_a_problem ()
	{
		ReportProblemDialog* d = new ReportProblemDialog (this);
		if (d->ShowModal () == wxID_OK) {
			d->report ();
		}
		d->Destroy ();
	}

	void update_checker_state_changed ()
	{
		UpdateChecker* uc = UpdateChecker::instance ();

		bool const announce =
			_update_news_requested ||
			(uc->stable() && Config::instance()->check_for_updates()) ||
			(uc->test() && Config::instance()->check_for_updates() && Config::instance()->check_for_test_updates());

		_update_news_requested = false;

		if (!announce) {
			return;
		}

		if (uc->state() == UpdateChecker::YES) {
			UpdateDialog* dialog = new UpdateDialog (this, uc->stable (), uc->test ());
			dialog->ShowModal ();
			dialog->Destroy ();
		} else if (uc->state() == UpdateChecker::FAILED) {
			error_dialog (this, _("The DCP-o-matic download server could not be contacted."));
		} else {
			error_dialog (this, _("There are no new versions of DCP-o-matic available."));
		}

		_update_news_requested = false;
	}

	void config_changed ()
	{
		/* Instantly save any config changes when using the player GUI */
		try {
			Config::instance()->write_config();
		} catch (exception& e) {
			error_dialog (
				this,
				wxString::Format (
					_("Could not write to config file at %s.  Your changes have not been saved."),
					std_to_wx (Config::instance()->cinemas_file().string()).data()
					)
				);
		}
	}

	bool _update_news_requested;
	PlayerInformation* _info;
	wxPreferencesEditor* _config_dialog;
	FilmViewer* _viewer;
	boost::shared_ptr<Film> _film;
	boost::signals2::scoped_connection _config_changed_connection;
};

static const wxCmdLineEntryDesc command_line_description[] = {
	{ wxCMD_LINE_PARAM, 0, 0, "DCP to load or create", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_NONE, "", "", "", wxCmdLineParamType (0), 0 }
};

/** @class App
 *  @brief The magic App class for wxWidgets.
 */
class App : public wxApp
{
public:
	App ()
		: wxApp ()
		, _frame (0)
	{}

private:

	bool OnInit ()
	try
	{
		wxInitAllImageHandlers ();

		Config::FailedToLoad.connect (boost::bind (&App::config_failed_to_load, this));

		wxSplashScreen* splash = 0;
		try {
			if (!Config::have_existing ("config.xml")) {
				wxBitmap bitmap;
				boost::filesystem::path p = shared_path () / "splash.png";
				if (bitmap.LoadFile (std_to_wx (p.string ()), wxBITMAP_TYPE_PNG)) {
					splash = new wxSplashScreen (bitmap, wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT, 0, 0, -1);
					wxYield ();
				}
			}
		} catch (boost::filesystem::filesystem_error& e) {
			/* Maybe we couldn't find the splash image; never mind */
		}

		SetAppName (_("DCP-o-matic Player"));

		if (!wxApp::OnInit()) {
			return false;
		}

#ifdef DCPOMATIC_LINUX
		unsetenv ("UBUNTU_MENUPROXY");
#endif

#ifdef __WXOSX__
		ProcessSerialNumber serial;
		GetCurrentProcess (&serial);
		TransformProcessType (&serial, kProcessTransformToForegroundApplication);
#endif

		dcpomatic_setup_path_encoding ();

		/* Enable i18n; this will create a Config object
		   to look for a force-configured language.  This Config
		   object will be wrong, however, because dcpomatic_setup
		   hasn't yet been called and there aren't any filters etc.
		   set up yet.
		*/
		dcpomatic_setup_i18n ();

		/* Set things up, including filters etc.
		   which will now be internationalised correctly.
		*/
		dcpomatic_setup ();

		/* Force the configuration to be re-loaded correctly next
		   time it is needed.
		*/
		Config::drop ();

		_frame = new DOMFrame ();
		SetTopWindow (_frame);
		_frame->Maximize ();
		if (splash) {
			splash->Destroy ();
		}
		_frame->Show ();

		signal_manager = new wxSignalManager (this);

		if (!_dcp_to_load.empty() && boost::filesystem::is_directory (_dcp_to_load)) {
			try {
				_frame->load_dcp (_dcp_to_load);
			} catch (exception& e) {
				error_dialog (0, std_to_wx (String::compose (wx_to_std (_("Could not load DCP %1 (%2)")), _dcp_to_load, e.what())));
			}
		}

		Bind (wxEVT_IDLE, boost::bind (&App::idle, this));

		if (Config::instance()->check_for_updates ()) {
			UpdateChecker::instance()->run ();
		}

		return true;
	}
	catch (exception& e)
	{
		error_dialog (0, wxString::Format ("DCP-o-matic Player could not start: %s", e.what ()));
		return true;
	}

	void OnInitCmdLine (wxCmdLineParser& parser)
	{
		parser.SetDesc (command_line_description);
		parser.SetSwitchChars (wxT ("-"));
	}

	bool OnCmdLineParsed (wxCmdLineParser& parser)
	{
		if (parser.GetParamCount() > 0) {
			_dcp_to_load = wx_to_std (parser.GetParam (0));
		}

		return true;
	}

	void report_exception ()
	{
		try {
			throw;
		} catch (FileError& e) {
			error_dialog (
				0,
				wxString::Format (
					_("An exception occurred: %s (%s)\n\n") + REPORT_PROBLEM,
					std_to_wx (e.what()),
					std_to_wx (e.file().string().c_str ())
					)
				);
		} catch (exception& e) {
			error_dialog (
				0,
				wxString::Format (
					_("An exception occurred: %s.\n\n") + REPORT_PROBLEM,
					std_to_wx (e.what ())
					)
				);
		} catch (...) {
			error_dialog (0, _("An unknown exception occurred.") + "  " + REPORT_PROBLEM);
		}
	}

	/* An unhandled exception has occurred inside the main event loop */
	bool OnExceptionInMainLoop ()
	{
		report_exception ();
		/* This will terminate the program */
		return false;
	}

	void OnUnhandledException ()
	{
		report_exception ();
	}

	void idle ()
	{
		signal_manager->ui_idle ();
	}

	void config_failed_to_load ()
	{
		message_dialog (_frame, _("The existing configuration failed to load.  Default values will be used instead.  These may take a short time to create."));
	}

	DOMFrame* _frame;
	string _dcp_to_load;
};

IMPLEMENT_APP (App)
