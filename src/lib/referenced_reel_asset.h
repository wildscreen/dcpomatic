/*
    Copyright (C) 2015 Carl Hetherington <cth@carlh.net>

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

#ifndef DCPOMATIC_REFERENCED_REEL_ASSET_H
#define DCPOMATIC_REFERENCED_REEL_ASSET_H

#include <dcp/reel_asset.h>

class ReferencedReelAsset
{
public:
	ReferencedReelAsset (boost::shared_ptr<dcp::ReelAsset> asset_, DCPTimePeriod period_)
		: asset (asset_)
		, period (period_)
	{}

	/** The asset */
	boost::shared_ptr<dcp::ReelAsset> asset;
	/** Period that this asset covers in the DCP */
	DCPTimePeriod period;
};

#endif