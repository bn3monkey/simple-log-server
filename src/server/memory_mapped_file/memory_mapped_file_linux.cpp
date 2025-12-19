#if defined(__linux__)

#include "memory_mapped_file.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace Bn3Monkey;

MemoryMappedFile::MemoryMappedFile(const char* path, Access access, size_t size)
{
	open(path, access, size);
}
MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other)
{
	close();

	_code = other._code;
	other._code = Code::CLOSED;

	_size = other._size;
	other._size = 0;

	_data = other._data;
	other._data = nullptr;


	_handle = other._handle;
	other._handle = -1;
}

MemoryMappedFile& Bn3Monkey::MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept
{
	close();

	_code = other._code;
	other._code = Code::CLOSED;

	_size = other._size;
	other._size = 0;

	_data = other._data;
	other._data = nullptr;

	_handle = other._handle;
	other._handle = -1;

	return *this;
}

MemoryMappedFile::~MemoryMappedFile()
{
	close();
}

void MemoryMappedFile::commitAll()
{
	msync(_data, _size, MS_SYNC);
}
void MemoryMappedFile::commit(size_t offset, size_t size)
{
	msync(_data + offset, size, MS_SYNC);
}

void MemoryMappedFile::open(const char* path, Access access, size_t size)
{

	do {
		_code = Code::SUCCESS;
		
		int flags = access != Access::READONLY ? O_RDWR : O_RDONLY;
		if (access == Access::READWRITE_WITH_CREATE)
			flags != O_CREAT;

		_handle = ::open(path.c_str(), flags, 0644);
		if (_handle < 0)
			_code = Code::CANNOT_OPEN_FILE;
			break;
		}

		struct stat st {};
		fstat(_handle, &st);


		if (access != Access::READONLY) {
			if (size > 0) {
				ftruncate(_fd, size);
				st.st_size = size;
			}
			else {
				_code = Code::CREATED_FILE_NEED_NON_ZERO_SIZE;
				break;
			}
		}

		if (__size.QuadPart == 0)
		{
			_code = Code::FILE_SIZE_IS_ZERO;
			break;
		}

		int prot = access != Access::READONLY;
		_data = mmap(nullptr, st.st_size, prot, MAP_SHARED, _handle, 0);
		if (_data == MAP_FAILED) {
			_code = Code::CANNOT_MAP_FILE;
		}

		_size = static_cast<size_t>(st.st_size);

	} while (false);

	if (_code != Code::SUCCESS) {
		close();
	}
}
void MemoryMappedFile::close()
{
	if (_data) {
		munmap(_data, _size);
		_data = nullptr;
	}
	if (_handle >= 0) {
		close(_handle);
		_handle = -1;
	}
	_code = Code::CLOSED;
	_size = 0;
}

#endif // __linux__