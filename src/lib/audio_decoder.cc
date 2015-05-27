/*
    Copyright (C) 2012-2015 Carl Hetherington <cth@carlh.net>

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

#include "audio_decoder.h"
#include "audio_buffers.h"
#include "audio_decoder_stream.h"
#include <boost/foreach.hpp>
#include <iostream>

#include "i18n.h"

using std::cout;
using std::map;
using boost::shared_ptr;

AudioDecoder::AudioDecoder (shared_ptr<const AudioContent> content)
	: _audio_content (content)
{
	BOOST_FOREACH (AudioStreamPtr i, content->audio_streams ()) {
		_streams[i] = shared_ptr<AudioDecoderStream> (new AudioDecoderStream (_audio_content, i, this));
	}
}

ContentAudio
AudioDecoder::get_audio (AudioStreamPtr stream, Frame frame, Frame length, bool accurate)
{
	return _streams[stream]->get (frame, length, accurate);
}

void
AudioDecoder::audio (AudioStreamPtr stream, shared_ptr<const AudioBuffers> data, ContentTime time)
{
	_streams[stream]->audio (data, time);
}

void
AudioDecoder::flush ()
{
	for (map<AudioStreamPtr, shared_ptr<AudioDecoderStream> >::const_iterator i = _streams.begin(); i != _streams.end(); ++i) {
		i->second->flush ();
	}
}

void
AudioDecoder::seek (ContentTime t, bool accurate)
{
	for (map<AudioStreamPtr, shared_ptr<AudioDecoderStream> >::const_iterator i = _streams.begin(); i != _streams.end(); ++i) {
		i->second->seek (t, accurate);
	}
}
