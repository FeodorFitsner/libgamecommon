/**
 * @file   iff.cpp
 * @brief  RIFF/IFF/RIFX reader/writer.
 *
 * Copyright (C) 2010-2014 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <camoto/iostream_helpers.hpp>
#include <camoto/iff.hpp>

namespace camoto {

IFFReader::IFFReader(stream::input_sptr iff, Filetype filetype)
	:	iff(iff),
		filetype(filetype)
{
	this->root();
}

void IFFReader::root()
{
	this->iff->seekg(0, stream::start);
	this->loadChunks(this->iff->size());
	return;
}

std::vector<IFFReader::fourcc> IFFReader::list()
{
	std::vector<fourcc> names;
	for (std::vector<Chunk>::const_iterator
		i = this->chunks.begin(); i != this->chunks.end(); i++
	) {
		names.push_back(i->name);
	}
	return names;
}

stream::len IFFReader::seek(const fourcc& name)
{
	for (std::vector<Chunk>::const_iterator
		i = this->chunks.begin(); i != this->chunks.end(); i++
	) {
		if (name.compare(i->name) == 0) {
			this->iff->seekg(i->start, stream::start);
			return i->len;
		}
	}
	return 0;
}

stream::len IFFReader::seek(unsigned int index)
{
	Chunk& chunk = this->chunks[index];
	this->iff->seekg(chunk.start, stream::start);
	return chunk.len;
}

stream::len IFFReader::open(const fourcc& name, fourcc *type)
{
	stream::len len = this->seek(name);
	this->iff >> fixedLength(*type, 4);
	this->loadChunks(len - 4);
	return len;
}

stream::len IFFReader::open(unsigned int index, fourcc *type)
{
	stream::len len = this->seek(index);
	this->iff >> fixedLength(*type, 4);
	this->loadChunks(len - 4);
	return len;
}

void IFFReader::loadChunks(stream::len lenChunk)
{
	this->chunks.clear();
	while (lenChunk) {
		lenChunk -= 8; // ID and chunk size fields
		Chunk c;
		c.start = this->iff->tellg() + 8;
		this->iff >> fixedLength(c.name, 4);
		switch (this->filetype) {
			case Filetype_RIFF:
				this->iff >> u32le(c.len);
				break;
			case Filetype_IFF:
				this->iff >> u32be(c.len);
				break;
		}
		if (lenChunk < c.len) c.len = lenChunk; // truncated
		this->chunks.push_back(c);
		lenChunk -= c.len;
		this->iff->seekg(c.len, stream::cur);
	}
	return;
}


IFFWriter::IFFWriter(stream::output_sptr iff, Filetype filetype)
	:	iff(iff),
		filetype(filetype)
{
}

void IFFWriter::begin(const fourcc& name)
{
	this->chunk.push_back(this->iff->tellp());
	this->iff
		<< nullPadded(name, 4)
		<< nullPadded("", 4)
	;
	return;
}

void IFFWriter::begin(const fourcc& name, const fourcc& type)
{
	this->chunk.push_back(this->iff->tellp());
	this->iff
		<< nullPadded(name, 4)
		<< nullPadded("", 4)
		<< nullPadded(type, 4)
	;
	return;
}

void IFFWriter::end()
{
	stream::pos orig = this->iff->tellp();
	stream::pos start = this->chunk.back();
	this->chunk.pop_back();
	stream::len lenChunk = orig - (start + 8);

	this->iff->seekp(start + 4, stream::start);
	switch (this->filetype) {
		case Filetype_RIFF:
			this->iff << u32le(lenChunk);
			break;
		case Filetype_IFF:
			this->iff << u32be(lenChunk);
			break;
	}
	this->iff->seekp(orig, stream::start);
	return;
}

} // namespace camoto