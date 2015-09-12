/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DIRECTORY_H
#define	DIRECTORY_H

#include <usb/Exception.h>
#include <mtp/ByteArray.h>
#include <vector>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

namespace mtp { namespace usb
{
	class File : Noncopyable
	{
		FILE *_f;

	public:
		File(const std::string &path) : _f(fopen(path.c_str(), "rb"))
		{
			if (!_f)
				throw Exception("open " + path);
		}

		~File()
		{ fclose(_f); }

		FILE *GetHandle()
		{ return _f; }

		std::string ReadLine(size_t bufSize = 1024)
		{
			std::vector<char> buf(bufSize);
			if (!fgets(buf.data(), buf.size(), _f))
				throw Exception("fgets");
			return buf.data();
		}

		ByteArray ReadAll()
		{
			static const size_t step = 4096;

			fseek(_f, 0, SEEK_SET);
			ByteArray buf;
			size_t r;
			do
			{
				size_t offset = buf.size();
				buf.resize(offset + step);
				r = fread(buf.data() + offset, 1, step, _f);
			}
			while(r == step);
			buf.resize(buf.size() - step + r);
			return buf;
		}

		int ReadInt(int base)
		{
			int r;
			switch(base)
			{
				case 16: if (fscanf(_f, "%x", &r) != 1) throw std::runtime_error("cannot read number"); break;
				case 10: if (fscanf(_f, "%d", &r) != 1) throw std::runtime_error("cannot read number"); break;
				default: throw std::runtime_error("invalid base");
			}
			return r;
		}
	};

	class Directory : Noncopyable
	{
	private:
		DIR *				_dir;
		std::vector<u8>		_buffer;

	public:
		Directory(const std::string &path): _dir(opendir(path.c_str()))
		{
			if (!_dir)
				throw Exception("opendir");

			long name_max = pathconf(path.c_str(), _PC_NAME_MAX);
			if (name_max == -1)         /* Limit not defined, or error */
				name_max = 255;         /* Take a guess */

			_buffer.resize(offsetof(struct dirent, d_name) + name_max + 1);
		}

		std::string Read()
		{
			dirent *buffer = static_cast<dirent *>(static_cast<void *>(_buffer.data()));
			dirent *entry = 0;
			int r = readdir_r(_dir, buffer, &entry);
			if (r)
				throw Exception("readdir_r", r);
			return entry? entry->d_name: "";
		}

		~Directory()
		{ closedir(_dir); }

		static int ReadInt(const std::string &path, int base = 16)
		{
			File f(path);
			return f.ReadInt(base);
		}

		static std::string ReadString(const std::string &path)
		{
			File f(path);
			std::string str = f.ReadLine();
			size_t end = str.find_last_not_of(" \t\r\n\f");
			return end != str.npos? str.substr(0, end + 1): str;
		}

		static ByteArray ReadAll(const std::string &path)
		{
			File f(path);
			return f.ReadAll();
		}
	};
}}

#endif	/* DIRECTORY_H */
