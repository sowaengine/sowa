#ifndef SW_ERROR_HXX
#define SW_ERROR_HXX
#pragma once

enum Error {
	OK = 0,
	ERR_FAILED,
	ERR_NOT_INITIALIZED,
	ERR_FILE_NOT_FOUND,
	MAX_ERROR
};

#endif // SW_ERROR_HXX