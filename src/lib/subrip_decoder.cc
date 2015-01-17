/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <dcp/subtitle_string.h>
#include "subrip_decoder.h"
#include "subrip_content.h"

using std::list;
using std::vector;
using std::string;
using std::cout;
using boost::shared_ptr;
using boost::optional;

SubRipDecoder::SubRipDecoder (shared_ptr<const SubRipContent> content)
	: SubtitleDecoder (content)
	, SubRip (content)
	, _next (0)
{

}

void
SubRipDecoder::seek (ContentTime time, bool accurate)
{
	SubtitleDecoder::seek (time, accurate);
	
	_next = 0;
	while (_next < _subtitles.size() && ContentTime::from_seconds (_subtitles[_next].from.all_as_seconds ()) < time) {
		++_next;
	}
}

bool
SubRipDecoder::pass ()
{
	if (_next >= _subtitles.size ()) {
		return true;
	}

	/* XXX: we are ignoring positioning specified in the file */
	
	list<dcp::SubtitleString> out;
	for (list<sub::Line>::const_iterator i = _subtitles[_next].lines.begin(); i != _subtitles[_next].lines.end(); ++i) {
		for (list<sub::Block>::const_iterator j = i->blocks.begin(); j != i->blocks.end(); ++j) {
			out.push_back (
				dcp::SubtitleString (
					SubRipContent::font_id,
					j->italic,
					dcp::Colour (255, 255, 255),
					j->font_size.points (72 * 11),
					dcp::Time (rint (_subtitles[_next].from.all_as_seconds())),
					dcp::Time (rint (_subtitles[_next].to.all_as_seconds())),
					i->vertical_position.line.get() * (1.5 / 22) + 0.8,
					dcp::TOP,
					j->text,
					dcp::NONE,
					dcp::Colour (255, 255, 255),
					0,
					0
					)
				);
		}
	}

	text_subtitle (out);
	++_next;
	return false;
}

list<ContentTimePeriod>
SubRipDecoder::subtitles_during (ContentTimePeriod p, bool starting) const
{
	/* XXX: inefficient */

	list<ContentTimePeriod> d;

	for (vector<sub::Subtitle>::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {

		ContentTimePeriod t (
			ContentTime::from_seconds (i->from.all_as_seconds()),
			ContentTime::from_seconds (i->to.all_as_seconds())
			);

		if ((starting && p.contains (t.from)) || (!starting && p.overlaps (t))) {
			d.push_back (t);
		}
	}

	return d;
}
