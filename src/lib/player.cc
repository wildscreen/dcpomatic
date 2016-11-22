/*
    Copyright (C) 2013-2016 Carl Hetherington <cth@carlh.net>

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

#include "player.h"
#include "film.h"
#include "audio_buffers.h"
#include "content_audio.h"
#include "dcp_content.h"
#include "job.h"
#include "image.h"
#include "raw_image_proxy.h"
#include "ratio.h"
#include "log.h"
#include "render_subtitles.h"
#include "config.h"
#include "content_video.h"
#include "player_video.h"
#include "frame_rate_change.h"
#include "audio_processor.h"
#include "playlist.h"
#include "referenced_reel_asset.h"
#include "decoder_factory.h"
#include "decoder.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "subtitle_content.h"
#include "subtitle_decoder.h"
#include "ffmpeg_content.h"
#include "audio_content.h"
#include "content_subtitle.h"
#include "dcp_decoder.h"
#include "image_decoder.h"
#include <dcp/reel.h>
#include <dcp/reel_sound_asset.h>
#include <dcp/reel_subtitle_asset.h>
#include <dcp/reel_picture_asset.h>
#include <boost/foreach.hpp>
#include <stdint.h>
#include <algorithm>
#include <iostream>

#include "i18n.h"

#define LOG_GENERAL(...) _film->log()->log (String::compose (__VA_ARGS__), LogEntry::TYPE_GENERAL);

using std::list;
using std::cout;
using std::min;
using std::max;
using std::min;
using std::vector;
using std::pair;
using std::map;
using std::make_pair;
using std::copy;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;
using boost::optional;
using boost::scoped_ptr;

static bool
has_video (Content* c)
{
	return static_cast<bool>(c->video);
}

static bool
has_audio (Content* c)
{
	return static_cast<bool>(c->audio);
}

static bool
has_subtitle (Content* c)
{
	return static_cast<bool>(c->subtitle);
}

Player::Player (shared_ptr<const Film> film, shared_ptr<const Playlist> playlist)
	: _film (film)
	, _playlist (playlist)
	, _have_valid_pieces (false)
	, _ignore_video (false)
	, _ignore_audio (false)
	, _always_burn_subtitles (false)
	, _fast (false)
	, _play_referenced (false)
{
	_film_changed_connection = _film->Changed.connect (bind (&Player::film_changed, this, _1));
	_playlist_changed_connection = _playlist->Changed.connect (bind (&Player::playlist_changed, this));
	_playlist_content_changed_connection = _playlist->ContentChanged.connect (bind (&Player::playlist_content_changed, this, _1, _2, _3));
	set_video_container_size (_film->frame_size ());

	film_changed (Film::AUDIO_PROCESSOR);
}

void
Player::setup_pieces ()
{
	_pieces.clear ();

	BOOST_FOREACH (shared_ptr<Content> i, _playlist->content ()) {

		if (!i->paths_valid ()) {
			continue;
		}

		shared_ptr<Decoder> decoder = decoder_factory (i, _film->log());
		FrameRateChange frc (i->active_video_frame_rate(), _film->video_frame_rate());

		if (!decoder) {
			/* Not something that we can decode; e.g. Atmos content */
			continue;
		}

		if (decoder->video && _ignore_video) {
			decoder->video->set_ignore ();
		}

		if (decoder->audio && _ignore_audio) {
			decoder->audio->set_ignore ();
		}

		if (decoder->audio && _fast) {
			decoder->audio->set_fast ();
		}

		shared_ptr<DCPDecoder> dcp = dynamic_pointer_cast<DCPDecoder> (decoder);
		if (dcp && _play_referenced) {
			dcp->set_decode_referenced ();
		}

		shared_ptr<Piece> piece (new Piece (i, decoder, frc));
		_pieces.push_back (piece);

		if (decoder->video) {
			decoder->video->Data.connect (bind (&Player::video, this, weak_ptr<Piece> (piece), _1));
		}

		if (decoder->audio) {
			decoder->audio->Data.connect (bind (&Player::audio, this, weak_ptr<Piece> (piece), _1));
		}

		if (decoder->subtitle) {
			decoder->subtitle->ImageData.connect (bind (&Player::image_subtitle, this, weak_ptr<Piece> (piece), _1));
			decoder->subtitle->TextData.connect (bind (&Player::text_subtitle, this, weak_ptr<Piece> (piece), _1));
		}
	}

	_have_valid_pieces = true;
}

void
Player::playlist_content_changed (weak_ptr<Content> w, int property, bool frequent)
{
	shared_ptr<Content> c = w.lock ();
	if (!c) {
		return;
	}

	if (
		property == ContentProperty::POSITION ||
		property == ContentProperty::LENGTH ||
		property == ContentProperty::TRIM_START ||
		property == ContentProperty::TRIM_END ||
		property == ContentProperty::PATH ||
		property == VideoContentProperty::FRAME_TYPE ||
		property == DCPContentProperty::NEEDS_ASSETS ||
		property == DCPContentProperty::NEEDS_KDM ||
		property == SubtitleContentProperty::COLOUR ||
		property == SubtitleContentProperty::OUTLINE ||
		property == SubtitleContentProperty::SHADOW ||
		property == SubtitleContentProperty::EFFECT_COLOUR ||
		property == FFmpegContentProperty::SUBTITLE_STREAM ||
		property == VideoContentProperty::COLOUR_CONVERSION
		) {

		_have_valid_pieces = false;
		Changed (frequent);

	} else if (
		property == SubtitleContentProperty::LINE_SPACING ||
		property == SubtitleContentProperty::OUTLINE_WIDTH ||
		property == SubtitleContentProperty::Y_SCALE ||
		property == SubtitleContentProperty::FADE_IN ||
		property == SubtitleContentProperty::FADE_OUT ||
		property == ContentProperty::VIDEO_FRAME_RATE ||
		property == SubtitleContentProperty::USE ||
		property == SubtitleContentProperty::X_OFFSET ||
		property == SubtitleContentProperty::Y_OFFSET ||
		property == SubtitleContentProperty::X_SCALE ||
		property == SubtitleContentProperty::FONTS ||
		property == VideoContentProperty::CROP ||
		property == VideoContentProperty::SCALE ||
		property == VideoContentProperty::FADE_IN ||
		property == VideoContentProperty::FADE_OUT
		) {

		Changed (frequent);
	}
}

void
Player::set_video_container_size (dcp::Size s)
{
	_video_container_size = s;

	_black_image.reset (new Image (AV_PIX_FMT_RGB24, _video_container_size, true));
	_black_image->make_black ();
}

void
Player::playlist_changed ()
{
	_have_valid_pieces = false;
	Changed (false);
}

void
Player::film_changed (Film::Property p)
{
	/* Here we should notice Film properties that affect our output, and
	   alert listeners that our output now would be different to how it was
	   last time we were run.
	*/

	if (p == Film::CONTAINER) {
		Changed (false);
	} else if (p == Film::VIDEO_FRAME_RATE) {
		/* Pieces contain a FrameRateChange which contains the DCP frame rate,
		   so we need new pieces here.
		*/
		_have_valid_pieces = false;
		Changed (false);
	} else if (p == Film::AUDIO_PROCESSOR) {
		if (_film->audio_processor ()) {
			_audio_processor = _film->audio_processor()->clone (_film->audio_frame_rate ());
		}
	}
}

list<PositionImage>
Player::transform_image_subtitles (list<ImageSubtitle> subs) const
{
	list<PositionImage> all;

	for (list<ImageSubtitle>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
		if (!i->image) {
			continue;
		}

		/* We will scale the subtitle up to fit _video_container_size */
		dcp::Size scaled_size (i->rectangle.width * _video_container_size.width, i->rectangle.height * _video_container_size.height);

		/* Then we need a corrective translation, consisting of two parts:
		 *
		 * 1.  that which is the result of the scaling of the subtitle by _video_container_size; this will be
		 *     rect.x * _video_container_size.width and rect.y * _video_container_size.height.
		 *
		 * 2.  that to shift the origin of the scale by subtitle_scale to the centre of the subtitle; this will be
		 *     (width_before_subtitle_scale * (1 - subtitle_x_scale) / 2) and
		 *     (height_before_subtitle_scale * (1 - subtitle_y_scale) / 2).
		 *
		 * Combining these two translations gives these expressions.
		 */

		all.push_back (
			PositionImage (
				i->image->scale (
					scaled_size,
					dcp::YUV_TO_RGB_REC601,
					i->image->pixel_format (),
					true,
					_fast
					),
				Position<int> (
					lrint (_video_container_size.width * i->rectangle.x),
					lrint (_video_container_size.height * i->rectangle.y)
					)
				)
			);
	}

	return all;
}

shared_ptr<PlayerVideo>
Player::black_player_video_frame (DCPTime time) const
{
	return shared_ptr<PlayerVideo> (
		new PlayerVideo (
			shared_ptr<const ImageProxy> (new RawImageProxy (_black_image)),
			time,
			Crop (),
			optional<double> (),
			_video_container_size,
			_video_container_size,
			EYES_BOTH,
			PART_WHOLE,
			PresetColourConversion::all().front().conversion
		)
	);
}

Frame
Player::dcp_to_content_video (shared_ptr<const Piece> piece, DCPTime t) const
{
	DCPTime s = t - piece->content->position ();
	s = min (piece->content->length_after_trim(), s);
	s = max (DCPTime(), s + DCPTime (piece->content->trim_start(), piece->frc));

	/* It might seem more logical here to convert s to a ContentTime (using the FrameRateChange)
	   then convert that ContentTime to frames at the content's rate.  However this fails for
	   situations like content at 29.9978733fps, DCP at 30fps.  The accuracy of the Time type is not
	   enough to distinguish between the two with low values of time (e.g. 3200 in Time units).

	   Instead we convert the DCPTime using the DCP video rate then account for any skip/repeat.
	*/
	return s.frames_floor (piece->frc.dcp) / piece->frc.factor ();
}

DCPTime
Player::content_video_to_dcp (shared_ptr<const Piece> piece, Frame f) const
{
	/* See comment in dcp_to_content_video */
	DCPTime const d = DCPTime::from_frames (f * piece->frc.factor(), piece->frc.dcp) - DCPTime (piece->content->trim_start (), piece->frc);
	return max (DCPTime (), d + piece->content->position ());
}

Frame
Player::dcp_to_resampled_audio (shared_ptr<const Piece> piece, DCPTime t) const
{
	DCPTime s = t - piece->content->position ();
	s = min (piece->content->length_after_trim(), s);
	/* See notes in dcp_to_content_video */
	return max (DCPTime (), DCPTime (piece->content->trim_start (), piece->frc) + s).frames_floor (_film->audio_frame_rate ());
}

ContentTime
Player::dcp_to_content_subtitle (shared_ptr<const Piece> piece, DCPTime t) const
{
	DCPTime s = t - piece->content->position ();
	s = min (piece->content->length_after_trim(), s);
	return max (ContentTime (), ContentTime (s, piece->frc) + piece->content->trim_start());
}

DCPTime
Player::content_subtitle_to_dcp (shared_ptr<const Piece> piece, ContentTime t) const
{
	return max (DCPTime (), DCPTime (t - piece->content->trim_start(), piece->frc) + piece->content->position());
}

list<shared_ptr<Font> >
Player::get_subtitle_fonts ()
{
	if (!_have_valid_pieces) {
		setup_pieces ();
	}

	list<shared_ptr<Font> > fonts;
	BOOST_FOREACH (shared_ptr<Piece>& p, _pieces) {
		if (p->content->subtitle) {
			/* XXX: things may go wrong if there are duplicate font IDs
			   with different font files.
			*/
			list<shared_ptr<Font> > f = p->content->subtitle->fonts ();
			copy (f.begin(), f.end(), back_inserter (fonts));
		}
	}

	return fonts;
}

/** Set this player never to produce any video data */
void
Player::set_ignore_video ()
{
	_ignore_video = true;
}

/** Set this player never to produce any audio data */
void
Player::set_ignore_audio ()
{
	_ignore_audio = true;
}

/** Set whether or not this player should always burn text subtitles into the image,
 *  regardless of the content settings.
 *  @param burn true to always burn subtitles, false to obey content settings.
 */
void
Player::set_always_burn_subtitles (bool burn)
{
	_always_burn_subtitles = burn;
}

void
Player::set_fast ()
{
	_fast = true;
	_have_valid_pieces = false;
}

void
Player::set_play_referenced ()
{
	_play_referenced = true;
	_have_valid_pieces = false;
}

list<ReferencedReelAsset>
Player::get_reel_assets ()
{
	list<ReferencedReelAsset> a;

	BOOST_FOREACH (shared_ptr<Content> i, _playlist->content ()) {
		shared_ptr<DCPContent> j = dynamic_pointer_cast<DCPContent> (i);
		if (!j) {
			continue;
		}

		scoped_ptr<DCPDecoder> decoder;
		try {
			decoder.reset (new DCPDecoder (j, _film->log()));
		} catch (...) {
			return a;
		}

		int64_t offset = 0;
		BOOST_FOREACH (shared_ptr<dcp::Reel> k, decoder->reels()) {

			DCPOMATIC_ASSERT (j->video_frame_rate ());
			double const cfr = j->video_frame_rate().get();
			Frame const trim_start = j->trim_start().frames_round (cfr);
			Frame const trim_end = j->trim_end().frames_round (cfr);
			int const ffr = _film->video_frame_rate ();

			DCPTime const from = i->position() + DCPTime::from_frames (offset, _film->video_frame_rate());
			if (j->reference_video ()) {
				shared_ptr<dcp::ReelAsset> ra = k->main_picture ();
				DCPOMATIC_ASSERT (ra);
				ra->set_entry_point (ra->entry_point() + trim_start);
				ra->set_duration (ra->duration() - trim_start - trim_end);
				a.push_back (
					ReferencedReelAsset (ra, DCPTimePeriod (from, from + DCPTime::from_frames (ra->duration(), ffr)))
					);
			}

			if (j->reference_audio ()) {
				shared_ptr<dcp::ReelAsset> ra = k->main_sound ();
				DCPOMATIC_ASSERT (ra);
				ra->set_entry_point (ra->entry_point() + trim_start);
				ra->set_duration (ra->duration() - trim_start - trim_end);
				a.push_back (
					ReferencedReelAsset (ra, DCPTimePeriod (from, from + DCPTime::from_frames (ra->duration(), ffr)))
					);
			}

			if (j->reference_subtitle ()) {
				shared_ptr<dcp::ReelAsset> ra = k->main_subtitle ();
				DCPOMATIC_ASSERT (ra);
				ra->set_entry_point (ra->entry_point() + trim_start);
				ra->set_duration (ra->duration() - trim_start - trim_end);
				a.push_back (
					ReferencedReelAsset (ra, DCPTimePeriod (from, from + DCPTime::from_frames (ra->duration(), ffr)))
					);
			}

			/* Assume that main picture duration is the length of the reel */
			offset += k->main_picture()->duration ();
		}
	}

	return a;
}

list<shared_ptr<Piece> >
Player::overlaps (DCPTime from, DCPTime to, boost::function<bool (Content *)> valid)
{
	if (!_have_valid_pieces) {
		setup_pieces ();
	}

	list<shared_ptr<Piece> > overlaps;
	BOOST_FOREACH (shared_ptr<Piece> i, _pieces) {
		if (valid (i->content.get ()) && i->content->position() < to && i->content->end() > from) {
			overlaps.push_back (i);
		}
	}

	return overlaps;
}

bool
Player::pass ()
{
	if (!_have_valid_pieces) {
		setup_pieces ();
	}

	shared_ptr<Piece> earliest;
	DCPTime earliest_position;
	BOOST_FOREACH (shared_ptr<Piece> i, _pieces) {
		/* Convert i->decoder->position() to DCPTime and work out the earliest */
	}

	earliest->decoder->pass ();

	/* XXX: collect audio and maybe emit some */
}

void
Player::video (weak_ptr<Piece> wp, ContentVideo video)
{
	shared_ptr<Piece> piece = wp.lock ();
	if (!piece) {
		return;
	}

	/* Get subs to burn in and burn them in */


	/* Fill gaps */

	DCPTime time = content_video_to_dcp (piece, video.frame.index());

	dcp::Size image_size = piece->content->video->scale().size (
		piece->content->video, _video_container_size, _film->frame_size ()
		);

	Video (
		shared_ptr<PlayerVideo> (
			new PlayerVideo (
				video.image,
				time,
				piece->content->video->crop (),
				piece->content->video->fade (video.frame.index()),
				image_size,
				_video_container_size,
				video.frame.eyes(),
				video.part,
				piece->content->video->colour_conversion ()
				)
			)
		);

}

void
Player::audio (weak_ptr<Piece> piece, ContentAudio video)
{
	/* gain, remap, processor */
	/* Put into merge buffer */
}

void
Player::image_subtitle (weak_ptr<Piece> piece, ContentImageSubtitle subtitle)
{
	/* Store for video to see */
}

void
Player::text_subtitle (weak_ptr<Piece> piece, ContentTextSubtitle subtitle)
{
	/* Store for video to see, or emit */
}

void
Player::seek (DCPTime time, bool accurate)
{
	if (accurate) {
		_last_video = time - DCPTime::from_frames (1, _film->video_frame_rate ());
	}
}
