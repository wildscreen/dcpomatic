/*
    Copyright (C) 2013-2017 Carl Hetherington <cth@carlh.net>

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

/** @file  src/wx/about_dialog.cc
 *  @brief The "about DCP-o-matic" dialogue box.
 */

#include "about_dialog.h"
#include "wx_util.h"
#include "lib/version.h"
#include "lib/compose.hpp"
#include <wx/notebook.h>
#include <wx/hyperlink.h>

using std::vector;

AboutDialog::AboutDialog (wxWindow* parent)
	: wxDialog (parent, wxID_ANY, _("About DCP-o-matic"))
{
	wxBoxSizer* overall_sizer = new wxBoxSizer (wxVERTICAL);
	wxBoxSizer* sizer = new wxBoxSizer (wxVERTICAL);

	wxFont title_font (*wxNORMAL_FONT);
	title_font.SetPointSize (title_font.GetPointSize() + 12);
	title_font.SetWeight (wxFONTWEIGHT_BOLD);

	wxFont subtitle_font (*wxNORMAL_FONT);
	subtitle_font.SetPointSize (subtitle_font.GetPointSize() + 2);

	wxFont version_font (*wxNORMAL_FONT);
	version_font.SetWeight (wxFONTWEIGHT_BOLD);

	wxStaticText* t = new wxStaticText (this, wxID_ANY, _("DCP-o-matic"));
	t->SetFont (title_font);
	sizer->Add (t, wxSizerFlags().Centre().Border(wxALL, 16));

	wxString s;
	if (strcmp (dcpomatic_git_commit, "release") == 0) {
		t = new wxStaticText (this, wxID_ANY, std_to_wx (String::compose ("Version %1", dcpomatic_version)));
	} else {
		t = new wxStaticText (this, wxID_ANY, std_to_wx (String::compose ("Version %1 git %2", dcpomatic_version, dcpomatic_git_commit)));
	}
	t->SetFont (version_font);
	sizer->Add (t, wxSizerFlags().Centre().Border(wxALL, 2));
	sizer->AddSpacer (12);

	t = new wxStaticText (
		this, wxID_ANY,
		_("Free, open-source DCP creation from almost anything."),
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER
		);
	t->SetFont (subtitle_font);

	sizer->Add (t, wxSizerFlags().Centre().Border(wxALL, 8));

	wxHyperlinkCtrl* h = new wxHyperlinkCtrl (
		this, wxID_ANY,
		wxT ("dcpomatic.com"),
		wxT ("http://dcpomatic.com")
		);

	sizer->Add (h, wxSizerFlags().Centre().Border(wxALL, 8));

	t = new wxStaticText (
		this, wxID_ANY,
		_("(C) 2012-2017 Carl Hetherington, Terrence Meiczinger\n Ole Laursen, Brecht Sanders"),
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER
		);

	sizer->Add (t, wxSizerFlags().Centre().Border(wxLEFT | wxRIGHT, 16));

	_notebook = new wxNotebook (this, wxID_ANY);

	wxArrayString written_by;
	written_by.Add (wxT ("Carl Hetherington"));
	written_by.Add (wxT ("Terrence Meiczinger"));
	written_by.Add (wxT ("Ole Laursen"));
	written_by.Add (wxT ("Brecht Sanders"));
	written_by.Add (wxT ("Jianguo Huang"));
	add_section (_("Written by"), written_by);

	wxArrayString translated_by;
	translated_by.Add (wxT ("Manuel AC"));
	translated_by.Add (wxT ("Max Aeschlimann"));
	translated_by.Add (wxT ("Thiago Andre"));
	translated_by.Add (wxT ("Grégoire Ausina"));
	translated_by.Add (wxT ("Tomáš Begeni"));
	translated_by.Add (wxT ("Fabio \"Zak\" Belli"));
	translated_by.Add (wxT ("Cherif Ben Brahim"));
	translated_by.Add (wxT ("Massimiliano Broggi"));
	translated_by.Add (wxT ("Davide Dall'AraCiao"));
	translated_by.Add (wxT ("William Fanelli"));
	translated_by.Add (wxT ("Max M. Fuhlendorf"));
	translated_by.Add (wxT ("Tomáš Hlaváč"));
	translated_by.Add (wxT ("Thierry Journet"));
	translated_by.Add (wxT ("Adam Klotblixt"));
	translated_by.Add (wxT ("Theo Kooijmans"));
	translated_by.Add (wxT ("Carsten Kurz"));
	translated_by.Add (wxT ("Lilian Lefranc"));
	translated_by.Add (wxT ("Gérald Maruccia"));
	translated_by.Add (wxT ("Mike Mazur"));
	translated_by.Add (wxT ("Rob van Nieuwkerk"));
	translated_by.Add (wxT ("Anders Uhl Pedersen"));
	translated_by.Add (wxT ("Olivier Perriere"));
	translated_by.Add (wxT ("Markus Raab"));
	translated_by.Add (wxT ("Tiago Casal Ribeiro"));
	translated_by.Add (wxT ("Davide Sanvito"));
	translated_by.Add (wxT ("Marek Skrzelowski"));
	translated_by.Add (wxT ("Igor Voytovich"));
	translated_by.Add (wxT ("Rov (若文)"));
	translated_by.Add (wxT ("刘汉源"));
 	add_section (_("Translated by"), translated_by);

	wxArrayString with_help_from;
	with_help_from.Add (wxT ("David Vignoni"));
	with_help_from.Add (wxT ("Dennis Couzin"));
	with_help_from.Add (wxT ("Gérald Maruccia"));
	with_help_from.Add (wxT ("Julian van Mil"));
	add_section (_("With help from"), with_help_from);

	wxArrayString supported_by;
	supported_by.Add (wxT ("Manuel AC"));
	supported_by.Add (wxT ("Kambiz Afshar"));
	supported_by.Add (wxT ("Louis Albini"));
	supported_by.Add (wxT ("Esteban Arrangoiz"));
	supported_by.Add (wxT ("Alex Asp"));
	supported_by.Add (wxT ("Georges Asmar"));
	supported_by.Add (wxT ("Eric Audurier"));
	supported_by.Add (wxT ("Kenneth Beals"));
	supported_by.Add (wxT ("Jean-Pierre Beauviala"));
	supported_by.Add (wxT ("Louis Belloisy"));
	supported_by.Add (wxT ("Frans van den Berg"));
	supported_by.Add (wxT ("Mike Blakesley"));
	supported_by.Add (wxT ("Silvio Bonomi"));
	supported_by.Add (wxT ("Fabio Bozzoli"));
	supported_by.Add (wxT ("Pierre-Alexis Bontemps"));
	supported_by.Add (wxT ("Jeff Boot"));
	supported_by.Add (wxT ("Alexander Buchholz"));
	supported_by.Add (wxT ("Paul Carey"));
	supported_by.Add (wxT ("Nathan Carpenter"));
	supported_by.Add (wxT ("Kieran Carroll"));
	supported_by.Add (wxT ("Matt Carter"));
	supported_by.Add (wxT ("Frank Cianciolo"));
	supported_by.Add (wxT ("Central Cinema"));
	supported_by.Add (wxT ("Cinema Clarici"));
	supported_by.Add (wxT ("Dan Cohen"));
	supported_by.Add (wxT ("Adam Colt"));
	supported_by.Add (wxT ("Adam Cousins"));
	supported_by.Add (wxT ("Miller Creative Limited"));
	supported_by.Add (wxT ("Matthias Damm"));
	supported_by.Add (wxT ("JP Davidson"));
	supported_by.Add (wxT ("Yohann Dedy"));
	supported_by.Add (wxT ("Alexey Derevyanko"));
	supported_by.Add (wxT ("Thomas Dickens"));
	supported_by.Add (wxT ("e22.digital"));
	supported_by.Add (wxT ("Unwana Essien"));
	supported_by.Add (wxT ("Maxime Estoppey"));
	supported_by.Add (wxT ("Peter Ericson"));
	supported_by.Add (wxT ("Darius Family"));
	supported_by.Add (wxT ("Rasitha Ferdinando"));
	supported_by.Add (wxT ("Jose Angel Velasco Fernandez"));
	supported_by.Add (wxT ("Traverso City Film Festival"));
	supported_by.Add (wxT ("Andres Fink"));
	supported_by.Add (wxT ("Dave Fleegel"));
	supported_by.Add (wxT ("Evan Freeze"));
	supported_by.Add (wxT ("Brett Friedman"));
	supported_by.Add (wxT ("Stephen Furley"));
	supported_by.Add (wxT ("filmtechniek.nl"));
	supported_by.Add (wxT ("Jose Delio Ares Garcia"));
	supported_by.Add (wxT ("Gartenbaukino"));
	supported_by.Add (wxT ("Silvio Giuliano"));
	supported_by.Add (wxT ("Alan Gouger"));
	supported_by.Add (wxT ("Christopher Gray"));
	supported_by.Add (wxT ("Vincent Grenier"));
	supported_by.Add (wxT ("Gerhard Gruber"));
	supported_by.Add (wxT ("Flor Guillaume"));
	supported_by.Add (wxT ("Patrick Haderer"));
	supported_by.Add (wxT ("刘汉源"));
	supported_by.Add (wxT ("Jeff Hemingway"));
	supported_by.Add (wxT ("Antonio Ruiz Hernandez"));
	supported_by.Add (wxT ("Joseph Huether"));
	supported_by.Add (wxT ("Film.Ca Inc."));
	supported_by.Add (wxT ("Denis Ivashvkevych"));
	supported_by.Add (wxT ("Karl Jacob"));
	supported_by.Add (wxT ("Henrik Jäger"));
	supported_by.Add (wxT ("Tomasz Jagi"));
	supported_by.Add (wxT ("Jonathan Jensen"));
	supported_by.Add (wxT ("Mathew Johns"));
	supported_by.Add (wxT ("Thierry Journet"));
	supported_by.Add (wxT ("Mario Kalogjera"));
	supported_by.Add (wxT ("Zbigniew Kamiński"));
	supported_by.Add (wxT ("Klaus Karger"));
	supported_by.Add (wxT ("Ikram Karimov"));
	supported_by.Add (wxT ("Chris Kay"));
	supported_by.Add (wxT ("Lars Kelto"));
	supported_by.Add (wxT ("Erwan Kerzanet"));
	supported_by.Add (wxT ("Kert Kiima"));
	supported_by.Add (wxT ("Alabama Kino"));
	supported_by.Add (wxT ("James Kirst"));
	supported_by.Add (wxT ("Adam Klotblixt"));
	supported_by.Add (wxT ("Filip Kovcin"));
	supported_by.Add (wxT ("Timm Kröger"));
	supported_by.Add (wxT ("Cihan Kulaber"));
	supported_by.Add (wxT ("Carsten Kurz"));
	supported_by.Add (wxT ("David Armati Lechner"));
	supported_by.Add (wxT ("Lilian Lefranc"));
	supported_by.Add (wxT ("Robert Legato"));
	supported_by.Add (wxT ("Sean Leigh"));
	supported_by.Add (wxT ("Olivier Lemaire"));
	supported_by.Add (wxT ("Eric Lesachet"));
	supported_by.Add (wxT ("Gavin Lewarne"));
	supported_by.Add (wxT ("Theo Lipfert"));
	supported_by.Add (wxT ("Paweł Lipiński"));
	supported_by.Add (wxT ("Marco Löber"));
	supported_by.Add (wxT ("Juan Marin Lorenzo"));
	supported_by.Add (wxT ("Cinéma Le Louxor"));
	supported_by.Add (wxT ("Michael Luce"));
	supported_by.Add (wxT ("Mandorla"));
	supported_by.Add (wxT ("Andrea Maguolo"));
	supported_by.Add (wxT ("Gerard Manshanden"));
	supported_by.Add (wxT ("Daniel Martinez Lara"));
	supported_by.Add (wxT ("Stefan Massopust"));
	supported_by.Add (wxT ("Richard Malmberg"));
	supported_by.Add (wxT ("Adrian Manolescu"));
	supported_by.Add (wxT ("Mattias Mattsson"));
	supported_by.Add (wxT ("George Mazarakis"));
	supported_by.Add (wxT ("Mike Mazur"));
	supported_by.Add (wxT ("Gordon McLeod"));
	supported_by.Add (wxT ("P. Meijer"));
	supported_by.Add (wxT ("Brian Mendelssohn"));
	supported_by.Add (wxT ("Olaf Merker"));
	supported_by.Add (wxT ("Kjarten Michaelsen"));
	supported_by.Add (wxT ("Aldo Midali"));
	supported_by.Add (wxT ("Sylvain Mielle"));
	supported_by.Add (wxT ("Jill Mobley"));
	supported_by.Add (wxT ("Michal Moc"));
	supported_by.Add (wxT ("Bobby Moloney"));
	supported_by.Add (wxT ("Howard Molton"));
	supported_by.Add (wxT ("Lindsay Morris"));
	supported_by.Add (wxT ("Moviemento Kino"));
	supported_by.Add (wxT ("NT Next"));
	supported_by.Add (wxT ("David Nedrow"));
	supported_by.Add (wxT ("Niclas Nornemark"));
	supported_by.Add (wxT ("Casablanca Nuernberg"));
	supported_by.Add (wxT ("Pascal Nussbaum"));
	supported_by.Add (wxT ("Tim O'Brien"));
	supported_by.Add (wxT ("Jerome Cohen Olivar"));
	supported_by.Add (wxT ("Tito Oliveira"));
	supported_by.Add (wxT ("Anders Olsson"));
	supported_by.Add (wxT ("Kevin Orman"));
	supported_by.Add (wxT ("Olov Östlund"));
	supported_by.Add (wxT ("Vasiliy Popov"));
	supported_by.Add (wxT ("Rui Pereira"));
	supported_by.Add (wxT ("Armin Peterhans"));
	supported_by.Add (wxT ("Andrea Pieri"));
	supported_by.Add (wxT ("Denis Postle"));
	supported_by.Add (wxT ("Aditya Pratama"));
	supported_by.Add (wxT ("Ceridwen Productions"));
	supported_by.Add (wxT ("Ivan Pullman"));
	supported_by.Add (wxT ("Arts Quest"));
	supported_by.Add (wxT ("Desiderio Garcia Ramirez"));
	supported_by.Add (wxT ("Steve Reverand"));
	supported_by.Add (wxT ("Mark Rolfe"));
	supported_by.Add (wxT ("Leigh Rosin"));
	supported_by.Add (wxT ("David Rozenthal"));
	supported_by.Add (wxT ("Lasse Salling"));
	supported_by.Add (wxT ("Jussi Siponen"));
	supported_by.Add (wxT ("Gregg Smith"));
	supported_by.Add (wxT ("Marco Spiaggi"));
	supported_by.Add (wxT ("Peter Sprenger"));
	supported_by.Add (wxT ("Randy Stankey"));
	supported_by.Add (wxT ("Andrä Steiner"));
	supported_by.Add (wxT ("Mike Stiebing"));
	supported_by.Add (wxT ("Rapid Stream Media"));
	supported_by.Add (wxT ("Tomislav Stojanović"));
	supported_by.Add (wxT ("Christian Suhren"));
	supported_by.Add (wxT ("Bruce Taylor"));
	supported_by.Add (wxT ("OpsCenter Technologies, Inc."));
	supported_by.Add (wxT ("Texas Theatre"));
	supported_by.Add (wxT ("Lawrence Towers"));
	supported_by.Add (wxT ("Petter Trønsdal"));
	supported_by.Add (wxT ("Richard Turner"));
	supported_by.Add (wxT ("Tilman Vatteroth"));
	supported_by.Add (wxT ("Alexey Vinokurov"));
	supported_by.Add (wxT ("Stéphane Wagneur"));
	supported_by.Add (wxT ("Andrew Walls"));
	supported_by.Add (wxT ("Raoul Walzer"));
	supported_by.Add (wxT ("Wolfram Weber"));
	supported_by.Add (wxT ("Frank Wenz"));
	supported_by.Add (wxT ("Anja Wenz"));
	supported_by.Add (wxT ("Johannes Wilbrand"));
	supported_by.Add (wxT ("Roland Wirtz"));
	supported_by.Add (wxT ("Wolfgang Woehl"));
	supported_by.Add (wxT ("Frank de Wulf"));
	supported_by.Add (wxT ("Pavel Zhdanko"));
	supported_by.Add (wxT ("Daniel Židek"));
	supported_by.Add (wxT ("Гуляев Михаил"));
	supported_by.Add (wxT ("Cinéma de la Neuveville"));
	supported_by.Add (wxT ("Frechen-Film e.V."));
	supported_by.Add (wxT ("Mobiles Kino e.V."));
	supported_by.Add (wxT ("Kino Völkerfreundschaft"));
	supported_by.Add (wxT ("Ernst Zimmerman"));
	add_section (_("Supported by"), supported_by);

	wxArrayString tested_by;
	tested_by.Add (wxT ("Manuel AC"));
	tested_by.Add (wxT ("Trever Anderson"));
	tested_by.Add (wxT ("Mike Blakesley"));
	tested_by.Add (wxT ("David Booty"));
	tested_by.Add (wxT ("Roop Chand"));
	tested_by.Add (wxT ("Daniel Chauvet"));
	tested_by.Add (wxT ("Adam Colt"));
	tested_by.Add (wxT ("John Convertino"));
	tested_by.Add (wxT ("Andreas Eli"));
	tested_by.Add (wxT ("Leo Enticknap"));
	tested_by.Add (wxT ("Jose Angel Velasco Fernandez"));
	tested_by.Add (wxT ("Maurizio Giampà"));
	tested_by.Add (wxT ("Luke Granger-Brown"));
	tested_by.Add (wxT ("Sumit Guha"));
	tested_by.Add (wxT ("Steve Guttag"));
	tested_by.Add (wxT ("Patrick Haderer"));
	tested_by.Add (wxT ("Bill Hamell"));
	tested_by.Add (wxT ("Jonathan Jensen"));
	tested_by.Add (wxT ("Thierry Journet"));
	tested_by.Add (wxT ("Markus Kalb"));
	tested_by.Add (wxT ("Ada de Kamper"));
	tested_by.Add (wxT ("Stefan Karner"));
	tested_by.Add (wxT ("Adam Keay"));
	tested_by.Add (wxT ("Simon Kesselman"));
	tested_by.Add (wxT ("Pepijn Klijs"));
	tested_by.Add (wxT ("Denzil Kriekenbeek"));
	tested_by.Add (wxT ("Carsten Kurz"));
	tested_by.Add (wxT ("Bill Lam"));
	tested_by.Add (wxT ("Lilian Lefranc"));
	tested_by.Add (wxT ("Olivier Lemaire"));
	tested_by.Add (wxT ("Gavin Lewarne"));
	tested_by.Add (wxT ("Gérald Maruccia"));
	tested_by.Add (wxT ("George Mazarakis"));
	tested_by.Add (wxT ("Mattias Mattsson"));
	tested_by.Add (wxT ("Will Meadows"));
	tested_by.Add (wxT ("Brad Miller"));
	tested_by.Add (wxT ("Ash Mitchell"));
	tested_by.Add (wxT ("Rob van Nieuwkerk"));
	tested_by.Add (wxT ("Anders Nordentoft-Madsen"));
	tested_by.Add (wxT ("Mauro Ottonello"));
	tested_by.Add (wxT ("Peter Puchner"));
	tested_by.Add (wxT ("Markus Raab"));
	tested_by.Add (wxT ("Michael Reckert"));
	tested_by.Add (wxT ("Greg Rooke"));
	tested_by.Add (wxT ("Elad Saad"));
	tested_by.Add (wxT ("Karim Senoucci"));
	tested_by.Add (wxT ("Hordur Valgardsson"));
	tested_by.Add (wxT ("Xenophon the Vampire"));
	tested_by.Add (wxT ("Simon Vannarath"));
	tested_by.Add (wxT ("Igor Voytovich"));
	tested_by.Add (wxT ("Andrew Walls"));
	tested_by.Add (wxT ("Andreas Weiss"));
	tested_by.Add (wxT ("Paul Willmott"));
	tested_by.Add (wxT ("Wolfgang Woehl"));
	add_section (_("Tested by"), tested_by);

	sizer->Add (_notebook, wxSizerFlags().Centre().Border(wxALL, 16).Expand());

	overall_sizer->Add (sizer);

	wxSizer* buttons = CreateButtonSizer (wxOK);
	if (buttons) {
		overall_sizer->Add (buttons, 1, wxEXPAND | wxALL, 4);
	}

	SetSizerAndFit (overall_sizer);
}

/** Add a section of credits.
 *  @param name Name of section.
 *  @param credits List of names.
 */
void
AboutDialog::add_section (wxString name, wxArrayString credits)
{
	static bool first = true;
	int const N = 4;

	wxScrolledWindow* panel = new wxScrolledWindow (_notebook);
	panel->SetMaxSize (wxSize (-1, 380));
	panel->EnableScrolling (false, true);
	panel->SetScrollRate (0, 32);
	wxSizer* overall_sizer = new wxBoxSizer (wxHORIZONTAL);

	vector<wxSizer*> sizers;

	for (int i = 0; i < N; ++i) {
		sizers.push_back (new wxBoxSizer (wxVERTICAL));
		overall_sizer->Add (sizers.back (), 1, wxEXPAND | wxALL, 6);
	}

	int c = 0;
	for (size_t i = 0; i < credits.Count(); ++i) {
		add_label_to_sizer (sizers[c], panel, credits[i], false);
		++c;
		if (c == N) {
			c = 0;
		}
	}

	panel->SetSizerAndFit (overall_sizer);
	_notebook->AddPage (panel, name, first);
	first = false;
}
