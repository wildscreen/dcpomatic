/*
    Copyright (C) 2012-2017 Carl Hetherington <cth@carlh.net>

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

/** @file  test/test.cc
 *  @brief Overall test stuff and useful methods for tests.
 */

#include "lib/config.h"
#include "lib/util.h"
#include "lib/signal_manager.h"
#include "lib/film.h"
#include "lib/job_manager.h"
#include "lib/job.h"
#include "lib/cross.h"
#include "lib/encode_server_finder.h"
#include "lib/image.h"
#include "lib/ratio.h"
#include "lib/dcp_content_type.h"
#include "lib/log_entry.h"
#include <dcp/dcp.h>
#include <dcp/cpl.h>
#include <dcp/reel.h>
#include <dcp/reel_picture_asset.h>
#include <dcp/mono_picture_frame.h>
#include <dcp/mono_picture_asset.h>
#include <dcp/openjpeg_image.h>
#include <asdcp/AS_DCP.h>
#include <sndfile.h>
#include <libxml++/libxml++.h>
#include <Magick++.h>
extern "C" {
#include <libavformat/avformat.h>
}
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dcpomatic_test
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include <list>
#include <vector>
#include <iostream>

using std::string;
using std::vector;
using std::min;
using std::cout;
using std::cerr;
using std::list;
using std::abs;
using boost::shared_ptr;
using boost::scoped_array;
using boost::dynamic_pointer_cast;

boost::filesystem::path private_data = boost::filesystem::path ("..") / boost::filesystem::path ("dcpomatic-test-private");

class TestSignalManager : public SignalManager
{
public:
	/* No wakes in tests: we call ui_idle ourselves */
	void wake_ui ()
	{

	}
};

struct TestConfig
{
	TestConfig ()
	{
		dcpomatic_setup ();

		Config::instance()->set_master_encoding_threads (1);
		Config::instance()->set_server_encoding_threads (1);
		Config::instance()->set_server_port_base (61921);
		Config::instance()->set_default_isdcf_metadata (ISDCFMetadata ());
		Config::instance()->set_default_container (Ratio::from_id ("185"));
		Config::instance()->set_default_dcp_content_type (static_cast<DCPContentType*> (0));
		Config::instance()->set_default_audio_delay (0);
		Config::instance()->set_default_j2k_bandwidth (100000000);
		Config::instance()->set_default_interop (false);
		Config::instance()->set_default_still_length (10);
		Config::instance()->set_log_types (LogEntry::TYPE_GENERAL | LogEntry::TYPE_WARNING | LogEntry::TYPE_ERROR);
		Config::instance()->set_automatic_audio_analysis (false);

		EncodeServerFinder::instance()->stop ();

		signal_manager = new TestSignalManager ();

		char* env_private = getenv("DCPOMATIC_TEST_PRIVATE");
		if (env_private) {
			private_data = env_private;
		}
	}

	~TestConfig ()
	{
		JobManager::drop ();
	}
};

BOOST_GLOBAL_FIXTURE (TestConfig);

boost::filesystem::path
test_film_dir (string name)
{
	boost::filesystem::path p;
	p /= "build";
	p /= "test";
	p /= name;
	return p;
}

shared_ptr<Film>
new_test_film (string name)
{
	boost::filesystem::path p = test_film_dir (name);
	if (boost::filesystem::exists (p)) {
		boost::filesystem::remove_all (p);
	}

	shared_ptr<Film> film = shared_ptr<Film> (new Film (p));
	film->write_metadata ();
	return film;
}

shared_ptr<Film>
new_test_film2 (string name)
{
	boost::filesystem::path p = test_film_dir (name);
	if (boost::filesystem::exists (p)) {
		boost::filesystem::remove_all (p);
	}

	shared_ptr<Film> film = shared_ptr<Film> (new Film (p));
	film->set_dcp_content_type (DCPContentType::from_isdcf_name ("TST"));
	film->set_container (Ratio::from_id ("185"));
	film->write_metadata ();
	return film;
}

void
check_wav_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	SF_INFO ref_info;
	ref_info.format = 0;
	SNDFILE* ref_file = sf_open (ref.string().c_str(), SFM_READ, &ref_info);
	BOOST_CHECK (ref_file);

	SF_INFO check_info;
	check_info.format = 0;
	SNDFILE* check_file = sf_open (check.string().c_str(), SFM_READ, &check_info);
	BOOST_CHECK (check_file);

	BOOST_CHECK_EQUAL (ref_info.frames, check_info.frames);
	BOOST_CHECK_EQUAL (ref_info.samplerate, check_info.samplerate);
	BOOST_CHECK_EQUAL (ref_info.channels, check_info.channels);
	BOOST_CHECK_EQUAL (ref_info.format, check_info.format);

	/* buffer_size is in frames */
	sf_count_t const buffer_size = 65536 * ref_info.channels;
	scoped_array<int32_t> ref_buffer (new int32_t[buffer_size]);
	scoped_array<int32_t> check_buffer (new int32_t[buffer_size]);

	sf_count_t N = ref_info.frames;
	while (N) {
		sf_count_t this_time = min (buffer_size, N);
		sf_count_t r = sf_readf_int (ref_file, ref_buffer.get(), this_time);
		BOOST_CHECK_EQUAL (r, this_time);
		r = sf_readf_int (check_file, check_buffer.get(), this_time);
		BOOST_CHECK_EQUAL (r, this_time);

		for (sf_count_t i = 0; i < this_time; ++i) {
			BOOST_REQUIRE_MESSAGE (
				abs (ref_buffer[i] - check_buffer[i]) <= 65536,
				ref << " differs from " << check << " at " << (ref_info.frames - N + i) << " of " << ref_info.frames
				<< "(" << ref_buffer[i] << " vs " << check_buffer[i] << ")"
				);
		}

		N -= this_time;
	}
}

void
check_mxf_audio_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	ASDCP::PCM::MXFReader ref_reader;
	BOOST_REQUIRE (!ASDCP_FAILURE (ref_reader.OpenRead (ref.string().c_str())));

	ASDCP::PCM::AudioDescriptor ref_desc;
	BOOST_REQUIRE (!ASDCP_FAILURE (ref_reader.FillAudioDescriptor (ref_desc)));

	ASDCP::PCM::MXFReader check_reader;
	BOOST_REQUIRE (!ASDCP_FAILURE (check_reader.OpenRead (check.string().c_str())));

	ASDCP::PCM::AudioDescriptor check_desc;
	BOOST_REQUIRE (!ASDCP_FAILURE (check_reader.FillAudioDescriptor (check_desc)));

	BOOST_REQUIRE_EQUAL (ref_desc.ContainerDuration, check_desc.ContainerDuration);

	ASDCP::PCM::FrameBuffer ref_buffer (Kumu::Megabyte);
	ASDCP::PCM::FrameBuffer check_buffer (Kumu::Megabyte);
	for (size_t i = 0; i < ref_desc.ContainerDuration; ++i) {
		ref_reader.ReadFrame (i, ref_buffer, 0);
		check_reader.ReadFrame (i, check_buffer, 0);
		BOOST_REQUIRE (memcmp(ref_buffer.RoData(), check_buffer.RoData(), ref_buffer.Size()) == 0);
	}
}

void
check_image (boost::filesystem::path ref, boost::filesystem::path check)
{
#ifdef DCPOMATIC_IMAGE_MAGICK
	using namespace MagickCore;
#else
	using namespace MagickLib;
#endif

	Magick::Image ref_image;
	ref_image.read (ref.string ());
	Magick::Image check_image;
	check_image.read (check.string ());
	BOOST_CHECK_MESSAGE (ref_image.compare (check_image), ref << " differs from " << check);
}

void
check_file (boost::filesystem::path ref, boost::filesystem::path check)
{
	uintmax_t N = boost::filesystem::file_size (ref);
	BOOST_CHECK_EQUAL (N, boost::filesystem::file_size (check));
	FILE* ref_file = fopen_boost (ref, "rb");
	BOOST_CHECK (ref_file);
	FILE* check_file = fopen_boost (check, "rb");
	BOOST_CHECK (check_file);

	int const buffer_size = 65536;
	uint8_t* ref_buffer = new uint8_t[buffer_size];
	uint8_t* check_buffer = new uint8_t[buffer_size];

	string error = "File " + check.string() + " differs from reference " + ref.string();

	while (N) {
		uintmax_t this_time = min (uintmax_t (buffer_size), N);
		size_t r = fread (ref_buffer, 1, this_time, ref_file);
		BOOST_CHECK_EQUAL (r, this_time);
		r = fread (check_buffer, 1, this_time, check_file);
		BOOST_CHECK_EQUAL (r, this_time);

		BOOST_CHECK_MESSAGE (memcmp (ref_buffer, check_buffer, this_time) == 0, error);
		if (memcmp (ref_buffer, check_buffer, this_time)) {
			break;
		}

		N -= this_time;
	}

	delete[] ref_buffer;
	delete[] check_buffer;

	fclose (ref_file);
	fclose (check_file);
}

static void
note (dcp::NoteType t, string n)
{
	if (t == dcp::DCP_ERROR) {
		cerr << n << "\n";
	}
}

void
check_dcp (boost::filesystem::path ref, boost::filesystem::path check)
{
	dcp::DCP ref_dcp (ref);
	ref_dcp.read ();
	dcp::DCP check_dcp (check);
	check_dcp.read ();

	dcp::EqualityOptions options;
	options.max_mean_pixel_error = 5;
	options.max_std_dev_pixel_error = 5;
	options.max_audio_sample_error = 255;
	options.cpl_annotation_texts_can_differ = true;
	options.reel_annotation_texts_can_differ = true;
	options.reel_hashes_can_differ = true;
	options.issue_dates_can_differ = true;

	BOOST_CHECK (ref_dcp.equals (check_dcp, options, boost::bind (note, _1, _2)));
}

void
check_xml (xmlpp::Element* ref, xmlpp::Element* test, list<string> ignore)
{
	BOOST_CHECK_EQUAL (ref->get_name (), test->get_name ());
	BOOST_CHECK_EQUAL (ref->get_namespace_prefix (), test->get_namespace_prefix ());

	if (find (ignore.begin(), ignore.end(), ref->get_name()) != ignore.end ()) {
		return;
	}

	xmlpp::Element::NodeList ref_children = ref->get_children ();
	xmlpp::Element::NodeList test_children = test->get_children ();
	BOOST_REQUIRE_MESSAGE (
		ref_children.size() == test_children.size(),
		ref->get_name() << " has " << ref_children.size() << " or " << test_children.size() << " children"
		);

	xmlpp::Element::NodeList::iterator k = ref_children.begin ();
	xmlpp::Element::NodeList::iterator l = test_children.begin ();
	while (k != ref_children.end ()) {

		/* XXX: should be doing xmlpp::EntityReference, xmlpp::XIncludeEnd, xmlpp::XIncludeStart */

		xmlpp::Element* ref_el = dynamic_cast<xmlpp::Element*> (*k);
		xmlpp::Element* test_el = dynamic_cast<xmlpp::Element*> (*l);
		BOOST_CHECK ((ref_el && test_el) || (!ref_el && !test_el));
		if (ref_el && test_el) {
			check_xml (ref_el, test_el, ignore);
		}

		xmlpp::ContentNode* ref_cn = dynamic_cast<xmlpp::ContentNode*> (*k);
		xmlpp::ContentNode* test_cn = dynamic_cast<xmlpp::ContentNode*> (*l);
		BOOST_CHECK ((ref_cn && test_cn) || (!ref_cn && !test_cn));
		if (ref_cn && test_cn) {
			BOOST_CHECK_EQUAL (ref_cn->get_content(), test_cn->get_content ());
		}

		++k;
		++l;
	}

	xmlpp::Element::AttributeList ref_attributes = ref->get_attributes ();
	xmlpp::Element::AttributeList test_attributes = test->get_attributes ();
	BOOST_CHECK_EQUAL (ref_attributes.size(), test_attributes.size ());

	xmlpp::Element::AttributeList::const_iterator m = ref_attributes.begin();
	xmlpp::Element::AttributeList::const_iterator n = test_attributes.begin();
	while (m != ref_attributes.end ()) {
		BOOST_CHECK_EQUAL ((*m)->get_name(), (*n)->get_name());
		BOOST_CHECK_EQUAL ((*m)->get_value(), (*n)->get_value());

		++m;
		++n;
	}
}

void
check_xml (boost::filesystem::path ref, boost::filesystem::path test, list<string> ignore)
{
	xmlpp::DomParser* ref_parser = new xmlpp::DomParser (ref.string ());
	xmlpp::Element* ref_root = ref_parser->get_document()->get_root_node ();
	xmlpp::DomParser* test_parser = new xmlpp::DomParser (test.string ());
	xmlpp::Element* test_root = test_parser->get_document()->get_root_node ();

	check_xml (ref_root, test_root, ignore);
}

bool
wait_for_jobs ()
{
	JobManager* jm = JobManager::instance ();
	while (jm->work_to_do ()) {
		while (signal_manager->ui_idle ()) {}
		dcpomatic_sleep (1);
	}

	if (jm->errors ()) {
		int N = 0;
		for (list<shared_ptr<Job> >::iterator i = jm->_jobs.begin(); i != jm->_jobs.end(); ++i) {
			if ((*i)->finished_in_error ()) {
				++N;
			}
		}
		cerr << N << " errors.\n";

		for (list<shared_ptr<Job> >::iterator i = jm->_jobs.begin(); i != jm->_jobs.end(); ++i) {
			if ((*i)->finished_in_error ()) {
				cerr << (*i)->name() << ":\n"
				     << "\tsummary: " << (*i)->error_summary () << "\n"
				     << "\tdetails: " << (*i)->error_details () << "\n";
			}
		}
	}

	while (signal_manager->ui_idle ()) {}

	if (jm->errors ()) {
		JobManager::drop ();
		return true;
	}

	return false;
}

void
write_image (shared_ptr<const Image> image, boost::filesystem::path file, string format)
{
#ifdef DCPOMATIC_IMAGE_MAGICK
		using namespace MagickCore;
#else
		using namespace MagickLib;
#endif

	Magick::Image m (image->size().width, image->size().height, format.c_str(), CharPixel, (void *) image->data()[0]);
	m.write (file.string ());
}

static
void
check_ffmpeg_stream (boost::filesystem::path ref, boost::filesystem::path check, string stream)
{
	FILE* ref_file = popen(string("ffmpeg -loglevel error -i " + ref.string() + " -map 0:" + stream + " -f md5 -").c_str(), "r");
	BOOST_REQUIRE (ref_file);
	char ref_md5[64];
	fscanf (ref_file, "%63s", ref_md5);
	pclose (ref_file);

	FILE* check_file = popen(string("ffmpeg -loglevel error -i " + check.string() + " -map 0:" + stream + " -f md5 -").c_str(), "r");
	BOOST_REQUIRE (check_file);
	char check_md5[64];
	fscanf (check_file, "%63s", check_md5);
	pclose (check_file);

	BOOST_REQUIRE_EQUAL (strcmp(ref_md5, check_md5), 0);
}

void
check_ffmpeg (boost::filesystem::path ref, boost::filesystem::path check)
{
	int const r = system (string("ffcmp " + ref.string() + " " + check.string()).c_str());
	BOOST_REQUIRE_EQUAL (WEXITSTATUS(r), 0);
}

void
check_one_frame (boost::filesystem::path dcp_dir, int64_t index, boost::filesystem::path ref)
{
	dcp::DCP dcp (dcp_dir);
	dcp.read ();
	shared_ptr<dcp::MonoPictureAsset> asset = dynamic_pointer_cast<dcp::MonoPictureAsset> (dcp.cpls().front()->reels().front()->main_picture()->asset());
	BOOST_REQUIRE (asset);
	shared_ptr<const dcp::MonoPictureFrame> frame = asset->start_read()->get_frame(index);

	boost::uintmax_t const ref_size = boost::filesystem::file_size(ref);
	BOOST_CHECK_EQUAL (frame->j2k_size(), ref_size);

	FILE* ref_file = fopen_boost(ref, "rb");
	BOOST_REQUIRE (ref_file);

	uint8_t* ref_data = new uint8_t[ref_size];
	fread (ref_data, ref_size, 1, ref_file);
	fclose (ref_file);

	BOOST_CHECK (memcmp(ref_data, frame->j2k_data(), ref_size) == 0);
	delete[] ref_data;
}

boost::filesystem::path
dcp_file (shared_ptr<const Film> film, string prefix)
{
	boost::filesystem::directory_iterator i = boost::filesystem::directory_iterator (film->dir(film->dcp_name()));
	while (i != boost::filesystem::directory_iterator() && !boost::algorithm::starts_with (i->path().leaf().string(), prefix)) {
		++i;
	}

	BOOST_REQUIRE (i != boost::filesystem::directory_iterator());
	return i->path();
}
