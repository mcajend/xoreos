/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file graphics/video/bink.cpp
 *  Decoding RAD Game Tools' Bink videos.
 */

#include <cmath>

#include "common/util.h"
#include "common/error.h"
#include "common/stream.h"
#include "common/bitstream.h"
#include "common/huffman.h"

#include "graphics/video/bink.h"
#include "graphics/video/binkdata.h"

#include "events/events.h"

static const uint32 kBIKfID = MKID_BE('BIKf');
static const uint32 kBIKgID = MKID_BE('BIKg');
static const uint32 kBIKhID = MKID_BE('BIKh');
static const uint32 kBIKiID = MKID_BE('BIKi');

static const uint32 kVideoFlagAlpha = 0x00100000;

namespace Graphics {

Bink::Bink(Common::SeekableReadStream *bink) : _bink(bink), _curFrame(0) {
	assert(_bink);

	for (int i = 0; i < 16; i++)
		_huffman[i] = 0;

	for (int i = 0; i < kSourceMAX; i++) {
		_bundles[i].huffman = 0;
		_bundles[i].data    = 0;
		_bundles[i].dataEnd = 0;
		_bundles[i].curDec  = 0;
		_bundles[i].curPtr  = 0;
	}

	load();
}

Bink::~Bink() {
	deinitBundles();

	for (int i = 0; i < 16; i++)
		delete _huffman[i];

	delete _bink;
}

bool Bink::gotTime() const {
	return true;
}

void Bink::processData() {
	uint32 curTime = EventMan.getTimestamp();

	if (!_started) {
		_startTime     = curTime;
		_lastFrameTime = curTime;
		_started       = true;
	}

	uint32 frameTime = ((uint64) (_curFrame * 1000 * ((uint64) _fpsDen))) / _fpsNum;
	if ((curTime - _startTime) < frameTime)
		return;

	if (_curFrame >= _frames.size()) {
		_finished = true;
		return;
	}

	VideoFrame &frame = _frames[_curFrame];

	if (!_bink->seek(frame.offset))
		throw Common::Exception(Common::kSeekError);

	uint32 frameSize = frame.size;

	for (std::vector<AudioTrack>::iterator audio = _audioTracks.begin(); audio != _audioTracks.end(); ++audio) {
		uint32 audioPacketLength = _bink->readUint32LE();

		if (frameSize < (audioPacketLength + 4))
			throw Common::Exception("Audio packet too big for the frame");

		if (audioPacketLength >= 4) {
			audio->sampleCount = _bink->readUint32LE();

			audio->bits = new Common::BitStream32LE(*_bink, (audioPacketLength - 4) * 8);

			audioPacket(*audio);

			delete audio->bits;
			audio->bits = 0;

			frameSize -= audioPacketLength + 4;
		}
	}

	frame.bits = new Common::BitStream32LE(*_bink, frameSize * 8);

	videoPacket(frame);

	delete frame.bits;
	frame.bits = 0;

	_needCopy = true;

	warning("Frame %d / %d", _curFrame, (int) _frames.size());

	_curFrame++;
}

void Bink::audioPacket(AudioTrack &audio) {
}

void Bink::videoPacket(VideoFrame &video) {
	assert(video.bits);

	if (_hasAlpha) {
		if (_id == kBIKiID)
			video.bits->skip(32);

		decodePlane(video, 3, false);
	}

	if (_id == kBIKiID)
		video.bits->skip(32);

	for (int i = 0; i < 3; i++) {
		int planeIdx = ((i == 0) || !_swapPlanes) ? i : (i ^ 3);

		decodePlane(video, planeIdx, i == 0);

		if (video.bits->pos() >= video.bits->size())
			break;
	}

}

void Bink::decodePlane(VideoFrame &video, int planeIdx, bool isChroma) {
	for (int i = 0; i < kSourceMAX; i++)
		readBundle(video, i);
}

void Bink::readBundle(VideoFrame &video, int bundle) {
	if (bundle == kSourceColors) {
		for (int i = 0; i < 16; i++)
			readHuffman(video, _colHighHuffman[i]);

		_colLastVal = 0;
	}

	if ((bundle != kSourceIntraDC) && (bundle != kSourceInterDC))
		readHuffman(video, _bundles[bundle].huffman);

	_bundles[bundle].curDec = _bundles[bundle].data;
	_bundles[bundle].curPtr = _bundles[bundle].data;
}

void Bink::readHuffman(VideoFrame &video, int &huffman) {
	uint32 symbols[16];
	byte   hasSymbol[16];

	huffman = video.bits->getBits(4);

	if (huffman == 0)
		// The first tree never changes symbols
		return;

	if (video.bits->getBits()) {
		// Symbol selection

		memset(hasSymbol, 0, 16);

		uint8 length = video.bits->getBits(3);
		for (int i = 0; i <= length; i++) {
			symbols[i] = video.bits->getBits(4);
			hasSymbol[symbols[i]] = 1;
		}

		for (int i = 0; i < 16; i++)
			if (hasSymbol[i] == 0)
				symbols[++length] = i;

		_huffman[huffman]->setSymbols(symbols);
		return;
	}

	// Symbol shuffling

	uint32 tmp1[16], tmp2[16];
	uint32 *in = tmp1, *out = tmp2;

	uint8 depth = video.bits->getBits(2);

	for (int i = 0; i < 16; i++)
		in[i] = i;

	for (int i = 0; i <= depth; i++) {
		int size = 1 << i;

		for (int j = 0; j < 16; j += (size << 1))
			mergeHuffmanSymbols(video, out + j, in + j, size);

		SWAP(in, out);
	}

	_huffman[huffman]->setSymbols(in);
}

void Bink::mergeHuffmanSymbols(VideoFrame &video, uint32 *dst, uint32 *src, int size) {
	uint32 *src2  = src + size;
	int     size2 = size;

	do {
		if (video.bits->getBits()) {
			*dst++ = *src++;
			size--;
		} else {
			*dst++ = *src2++;
			size2--;
		}

	} while (size && size2);

	while (size--)
		*dst++ = *src++;
	while (size2--)
		*dst++ = *src2++;
}

void Bink::load() {
	_id = _bink->readUint32BE();
	if ((_id != kBIKfID) && (_id != kBIKgID) && (_id != kBIKhID) && (_id != kBIKiID))
		throw Common::Exception("Unknown Bink FourCC %04X", _id);

	uint32 fileSize         = _bink->readUint32LE() + 8;
	uint32 frameCount       = _bink->readUint32LE();
	uint32 largestFrameSize = _bink->readUint32LE();

	if (largestFrameSize > fileSize)
		throw Common::Exception("Largest frame size greater than file size");

	_bink->skip(4);

	uint32 width  = _bink->readUint32LE();
	uint32 height = _bink->readUint32LE();

	createData(width, height);

	_fpsNum = _bink->readUint32LE();
	_fpsDen = _bink->readUint32LE();

	if ((_fpsNum == 0) || (_fpsDen == 0))
		throw Common::Exception("Invalid FPS (%d/%d)", _fpsNum, _fpsDen);

	_videoFlags = _bink->readUint32LE();

	uint32 audioTrackCount = _bink->readUint32LE();
	if (audioTrackCount > 0) {
		_audioTracks.resize(audioTrackCount);

		_bink->skip(4 * audioTrackCount);

		for (std::vector<AudioTrack>::iterator it = _audioTracks.begin(); it != _audioTracks.end(); ++it) {
			it->sampleRate  = _bink->readUint16LE();
			it->flags       = _bink->readUint16LE();
			it->sampleCount = 0;
			it->bits        = 0;
		}

		_bink->skip(4 * audioTrackCount);
	}

	_frames.resize(frameCount);
	for (uint32 i = 0; i < frameCount; i++) {
		_frames[i].offset   = _bink->readUint32LE();
		_frames[i].keyFrame = _frames[i].offset & 1;

		_frames[i].offset &= ~1;

		if (i != 0)
			_frames[i - 1].size = _frames[i].offset - _frames[i - 1].offset;

		_frames[i].bits = 0;
	}

	_frames[frameCount - 1].size = _bink->size() - _frames[frameCount - 1].offset;

	_hasAlpha   = _videoFlags & kVideoFlagAlpha;
	_swapPlanes = (_id == kBIKhID) || (_id == kBIKiID);

	initBundles();
	initHuffman();
}

void Bink::initBundles() {
	uint32 bw     = (_width  + 7) >> 3;
	uint32 bh     = (_height + 7) >> 3;
	uint32 blocks = bw * bh;

	for (int i = 0; i < kSourceMAX; i++) {
		_bundles[i].data    = new byte[blocks * 64];
		_bundles[i].dataEnd = _bundles[i].data + blocks * 64;
	}
}

void Bink::deinitBundles() {
	for (int i = 0; i < kSourceMAX; i++)
		delete[] _bundles[i].data;
}

void Bink::initHuffman() {
	for (int i = 0; i < 16; i++)
		_huffman[i] = new Common::Huffman(binkHuffmanLengths[i][15], 16, binkHuffmanCodes[i], binkHuffmanLengths[i]);
}

} // End of namespace Graphics