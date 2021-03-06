/*
    Copyright (C) 2014-2015 Carl Hetherington <cth@carlh.net>

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

/** @file  test/recover_test.cc
 *  @brief Test recovery of a DCP transcode after a crash.
 *  @ingroup specific
 */

#include "test.h"
#include "lib/film.h"
#include "lib/dcp_content_type.h"
#include "lib/image_content.h"
#include "lib/ffmpeg_content.h"
#include "lib/video_content.h"
#include "lib/ratio.h"
#include <dcp/mono_picture_asset.h>
#include <dcp/stereo_picture_asset.h>
#include <boost/test/unit_test.hpp>
#include <iostream>

using std::cout;
using std::string;
using boost::shared_ptr;

static void
note (dcp::NoteType t, string n)
{
	if (t == dcp::DCP_ERROR) {
		cout << n << "\n";
	}
}

BOOST_AUTO_TEST_CASE (recover_test_2d)
{
	shared_ptr<Film> film = new_test_film ("recover_test_2d");
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("FTR"));
	film->set_container (Ratio::from_id ("185"));
	film->set_name ("recover_test");

	shared_ptr<FFmpegContent> content (new FFmpegContent (film, "test/data/count300bd24.m2ts"));
	film->examine_and_add_content (content);
	wait_for_jobs ();

	film->make_dcp ();
	wait_for_jobs ();

	boost::filesystem::path const video = "build/test/recover_test_2d/video/185_2K_84d36460538435d5d511ee533c8528df_24_100000000_P_S_0_1200000.mxf";
	boost::filesystem::copy_file (
		video,
		"build/test/recover_test_2d/original.mxf"
		);

	boost::filesystem::resize_file (video, 2 * 1024 * 1024);

	film->make_dcp ();
	wait_for_jobs ();

	shared_ptr<dcp::MonoPictureAsset> A (new dcp::MonoPictureAsset ("build/test/recover_test_2d/original.mxf"));
	shared_ptr<dcp::MonoPictureAsset> B (new dcp::MonoPictureAsset (video));

	dcp::EqualityOptions eq;
	BOOST_CHECK (A->equals (B, eq, boost::bind (&note, _1, _2)));
}

BOOST_AUTO_TEST_CASE (recover_test_3d)
{
	shared_ptr<Film> film = new_test_film ("recover_test_3d");
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("FTR"));
	film->set_container (Ratio::from_id ("185"));
	film->set_name ("recover_test");
	film->set_three_d (true);

	shared_ptr<ImageContent> content (new ImageContent (film, "test/data/3d_test"));
	content->video->set_frame_type (VIDEO_FRAME_TYPE_3D_LEFT_RIGHT);
	film->examine_and_add_content (content);
	wait_for_jobs ();

	film->make_dcp ();
	wait_for_jobs ();

	boost::filesystem::path const video = "build/test/recover_test_3d/video/185_2K_961f053444e90c5ddbf978eb0ebfa772_24_100000000_P_S_3D_0_96000.mxf";

	boost::filesystem::copy_file (
		video,
		"build/test/recover_test_3d/original.mxf"
		);

	boost::filesystem::resize_file (video, 2 * 1024 * 1024);

	film->make_dcp ();
	wait_for_jobs ();

	shared_ptr<dcp::StereoPictureAsset> A (new dcp::StereoPictureAsset ("build/test/recover_test_3d/original.mxf"));
	shared_ptr<dcp::StereoPictureAsset> B (new dcp::StereoPictureAsset (video));

	dcp::EqualityOptions eq;
	BOOST_CHECK (A->equals (B, eq, boost::bind (&note, _1, _2)));
}


BOOST_AUTO_TEST_CASE (recover_test_2d_encrypted)
{
	shared_ptr<Film> film = new_test_film ("recover_test_2d_encrypted");
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("FTR"));
	film->set_container (Ratio::from_id ("185"));
	film->set_name ("recover_test");
	film->set_encrypted (true);

	shared_ptr<FFmpegContent> content (new FFmpegContent (film, "test/data/count300bd24.m2ts"));
	film->examine_and_add_content (content);
	wait_for_jobs ();

	film->make_dcp ();
	wait_for_jobs ();

	boost::filesystem::path const video =
		"build/test/recover_test_2d_encrypted/video/185_2K_84d36460538435d5d511ee533c8528df_24_100000000_E_S_0_1200000.mxf";

	boost::filesystem::copy_file (
		video,
		"build/test/recover_test_2d_encrypted/original.mxf"
		);

	boost::filesystem::resize_file (video, 2 * 1024 * 1024);

	film->make_dcp ();
	wait_for_jobs ();

	shared_ptr<dcp::MonoPictureAsset> A (new dcp::MonoPictureAsset ("build/test/recover_test_2d_encrypted/original.mxf"));
	A->set_key (film->key ());
	shared_ptr<dcp::MonoPictureAsset> B (new dcp::MonoPictureAsset (video));
	B->set_key (film->key ());

	dcp::EqualityOptions eq;
	BOOST_CHECK (A->equals (B, eq, boost::bind (&note, _1, _2)));
}
