#pragma once

#define FAKE_ANSI_

#ifdef FAKE_ANSI
	#define ANSI(code) "{" << code
#else
	#define ANSI(code) "\x1b[" << code
#endif

#define LOG(data) std::cout << data
#define LOGn(data) std::cout << data << std::endl

#define CH_MTXLCK_PRINT(mtx, data) \
	mtx.lock(); \
	LOG(data); \
	mtx.unlock();



//If your windows console doesn't support ANSI escape sequences by default
void enableANSI();

void removeANSIEscapeSequences(char* data, size_t size);
