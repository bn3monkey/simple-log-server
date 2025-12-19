#if defined(_WIN32)

#include "memory_mapped_file.hpp"
#include <windows.h>


using namespace Bn3Monkey;

MemoryMappedFile::MemoryMappedFile(const char* path, Access access, size_t size) noexcept
{
	open(path, access, size);
}
MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
{
	close();

	_code = other._code;
	other._code = Code::CLOSED;

	_size = other._size;
	other._size = 0;

	_data = other._data;
	other._data = nullptr;


	_handle = other._handle;
	other._handle = nullptr;

	_mapping = other._mapping;
	other._mapping = nullptr;
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
	other._handle = nullptr;

	_mapping = other._mapping;
	other._mapping = nullptr;

	return *this;
}

MemoryMappedFile::~MemoryMappedFile()
{
	close();
}

void MemoryMappedFile::commitAll()
{
	FlushViewOfFile(_data, _size);
	FlushFileBuffers((HANDLE)_handle);
}
void MemoryMappedFile::commit(size_t offset, size_t size)
{
	FlushViewOfFile(_data + offset, size);
	FlushFileBuffers((HANDLE)_handle);
}

void MemoryMappedFile::open(const char* path, Access access, size_t size)
{

	do {
		_code = Code::SUCCESS;
		DWORD _access = access == Access::READONLY ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
		DWORD _create = access == Access::READWRITE_WITH_CREATE ? OPEN_ALWAYS : OPEN_EXISTING;

		_handle = CreateFileA(path, _access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, _create, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (_handle == INVALID_HANDLE_VALUE)
		{
			_code = Code::CANNOT_OPEN_FILE;
			break;
		}

		LARGE_INTEGER __size{};
		GetFileSizeEx(_handle, &__size);


		if (access != Access::READONLY) {
			if (size > 0) {
				__size.QuadPart = size;
				SetFilePointerEx(_handle, __size, nullptr, FILE_BEGIN);
				SetEndOfFile(_handle);
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

		_mapping = CreateFileMappingA(_handle, nullptr, access != Access::READONLY ? PAGE_READWRITE : PAGE_READONLY, __size.HighPart, __size.LowPart, nullptr);
		if (!_mapping) {
			_code = Code::CANNOT_MAP_FILE;
			break;
		}

		_data = reinterpret_cast<char*>(MapViewOfFile(_mapping, access != Access::READONLY ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, __size.QuadPart));
		if (!_data) {
			_code = Code::CANNOT_MAP_FILE;
			break;
		}
		_size = __size.QuadPart;

	} while (false);

	if (_code != Code::SUCCESS) {
		close();
	}
}
void MemoryMappedFile::close()
{
	if (_data != nullptr) {
		UnmapViewOfFile(_data);
		_data = nullptr;
	}
	if (_mapping != nullptr) {
		CloseHandle(_mapping);
		_mapping = nullptr;
	}
	if (_handle != nullptr) {
		CloseHandle(_handle);
		_handle = nullptr;
	}
	_code = Code::CLOSED;
	_size = 0;
}

#endif //_WIN32