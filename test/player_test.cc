/*
    Copyright (C) 2014 Carl Hetherington <cth@carlh.net>

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

/** @file  test/player_test.cc
 *  @brief Test Player class.
 *  @ingroup selfcontained
 */

#include "lib/film.h"
#include "lib/ffmpeg_content.h"
#include "lib/dcp_content_type.h"
#include "lib/ratio.h"
#include "lib/audio_buffers.h"
#include "lib/player.h"
#include "lib/video_content.h"
#include "lib/image_content.h"
#include "lib/text_subtitle_content.h"
#include "lib/content_factory.h"
#include "test.h"
#include <boost/test/unit_test.hpp>
#include <iostream>

using std::cout;
using std::list;
using boost::shared_ptr;
using boost::bind;

static shared_ptr<AudioBuffers> accumulated;

static void
accumulate (shared_ptr<AudioBuffers> audio, DCPTime)
{
	BOOST_REQUIRE (accumulated);
	accumulated->append (audio);
}

/** Check that the Player correctly generates silence when used with a silent FFmpegContent */
BOOST_AUTO_TEST_CASE (player_silence_padding_test)
{
	shared_ptr<Film> film = new_test_film ("player_silence_padding_test");
	film->set_name ("player_silence_padding_test");
	shared_ptr<FFmpegContent> c (new FFmpegContent (film, "test/data/test.mp4"));
	film->set_container (Ratio::from_id ("185"));
	film->set_audio_channels (6);

	film->examine_and_add_content (c);
	wait_for_jobs ();

	accumulated.reset (new AudioBuffers (film->audio_channels(), 0));

	shared_ptr<Player> player (new Player (film, film->playlist ()));
	player->Audio.connect (bind (&accumulate, _1, _2));
	while (!player->pass ()) {}
	BOOST_REQUIRE (accumulated->frames() >= 48000);
	BOOST_CHECK_EQUAL (accumulated->channels(), film->audio_channels ());

	for (int i = 0; i < 48000; ++i) {
		for (int c = 0; c < accumulated->channels(); ++c) {
			BOOST_CHECK_EQUAL (accumulated->data()[c][i], 0);
		}
	}
}

/* Test insertion of black frames between separate bits of video content */
BOOST_AUTO_TEST_CASE (player_black_fill_test)
{
	shared_ptr<Film> film = new_test_film ("black_fill_test");
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("FTR"));
	film->set_name ("black_fill_test");
	film->set_container (Ratio::from_id ("185"));
	film->set_sequence (false);
	shared_ptr<ImageContent> contentA (new ImageContent (film, "test/data/simple_testcard_640x480.png"));
	shared_ptr<ImageContent> contentB (new ImageContent (film, "test/data/simple_testcard_640x480.png"));

	film->examine_and_add_content (contentA);
	film->examine_and_add_content (contentB);
	wait_for_jobs ();

	contentA->video->set_scale (VideoContentScale (Ratio::from_id ("185")));
	contentA->video->set_length (3);
	contentA->set_position (DCPTime::from_frames (2, film->video_frame_rate ()));
	contentB->video->set_scale (VideoContentScale (Ratio::from_id ("185")));
	contentB->video->set_length (1);
	contentB->set_position (DCPTime::from_frames (7, film->video_frame_rate ()));

	film->make_dcp ();

	wait_for_jobs ();

	boost::filesystem::path ref;
	ref = "test";
	ref /= "data";
	ref /= "black_fill_test";

	boost::filesystem::path check;
	check = "build";
	check /= "test";
	check /= "black_fill_test";
	check /= film->dcp_name();

	check_dcp (ref.string(), check.string());
}

/** Check behaviour with an awkward playlist whose data does not end on a video frame start */
BOOST_AUTO_TEST_CASE (player_subframe_test)
{
	shared_ptr<Film> film = new_test_film ("reels_test7");
	film->set_name ("reels_test7");
	film->set_container (Ratio::from_id ("185"));
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("TST"));
	shared_ptr<Content> A = content_factory(film, "test/data/flat_red.png").front();
	film->examine_and_add_content (A);
	BOOST_REQUIRE (!wait_for_jobs ());
	shared_ptr<Content> B = content_factory(film, "test/data/awkward_length.wav").front();
	film->examine_and_add_content (B);
	BOOST_REQUIRE (!wait_for_jobs ());
	film->set_video_frame_rate (24);
	A->video->set_length (3 * 24);

	BOOST_CHECK (A->full_length() == DCPTime::from_frames(3 * 24, 24));
	BOOST_CHECK (B->full_length() == DCPTime(289920));
	/* Length should be rounded up from B's length to the next video frame */
	BOOST_CHECK (film->length() == DCPTime::from_frames(3 * 24 + 1, 24));

	shared_ptr<Player> player (new Player (film, film->playlist ()));
	player->setup_pieces ();
	BOOST_REQUIRE_EQUAL (player->_black._periods.size(), 1);
	BOOST_CHECK (player->_black._periods.front() == DCPTimePeriod(DCPTime::from_frames(3 * 24, 24), DCPTime::from_frames(3 * 24 + 1, 24)));
	BOOST_REQUIRE_EQUAL (player->_silent._periods.size(), 1);
	BOOST_CHECK (player->_silent._periods.front() == DCPTimePeriod(DCPTime(289920), DCPTime::from_frames(3 * 24 + 1, 24)));
}

static Frame video_frames;
static Frame audio_frames;

static void
video (shared_ptr<PlayerVideo>, DCPTime)
{
	++video_frames;
}

static void
audio (shared_ptr<AudioBuffers> audio, DCPTime)
{
	audio_frames += audio->frames();
}

/** Check with a video-only file that the video and audio emissions happen more-or-less together */
BOOST_AUTO_TEST_CASE (player_interleave_test)
{
	shared_ptr<Film> film = new_test_film ("ffmpeg_transcoder_basic_test_subs");
	film->set_name ("ffmpeg_transcoder_basic_test");
	film->set_container (Ratio::from_id ("185"));
	film->set_audio_channels (6);

	shared_ptr<FFmpegContent> c (new FFmpegContent (film, "test/data/test.mp4"));
	film->examine_and_add_content (c);
	BOOST_REQUIRE (!wait_for_jobs ());

	shared_ptr<TextSubtitleContent> s (new TextSubtitleContent (film, "test/data/subrip.srt"));
	film->examine_and_add_content (s);
	BOOST_REQUIRE (!wait_for_jobs ());

	shared_ptr<Player> player (new Player(film, film->playlist()));
	player->Video.connect (bind (&video, _1, _2));
	player->Audio.connect (bind (&audio, _1, _2));
	video_frames = audio_frames = 0;
	while (!player->pass ()) {
		BOOST_CHECK (abs(video_frames - (audio_frames / 2000)) < 8);
	}
}
