/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

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

#include "decoder.h"

class ImageContent;
class Log;
class ImageProxy;

class ImageDecoder : public Decoder
{
public:
	ImageDecoder (boost::shared_ptr<const ImageContent> c, boost::shared_ptr<Log> log);

	boost::shared_ptr<const ImageContent> content () {
		return _image_content;
	}

	bool pass ();
	void seek (ContentTime, bool);

private:

	boost::shared_ptr<const ImageContent> _image_content;
	boost::shared_ptr<ImageProxy> _image;
	Frame _frame_video_position;
};
