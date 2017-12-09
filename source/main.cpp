#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstdarg>

#include <nds.h>
#include "device.h"
#include "blowfish_keys.h"

/*
	enum log_priority {
    LOG_DEBUG = 0, // Will probably spam logs, only use when debugging.
    LOG_INFO, // Reccomended default logfile level
    LOG_NOTICE, // Reccomended default display level, if implemented.
    LOG_WARN,
    LOG_ERR,
    LOG_PRIORITY_MAX
	};
*/
int global_loglevel =  1;

PrintConsole topScreen;
PrintConsole bottomScreen;

namespace flashcart_core {
	namespace platform {
		void showProgress(std::uint32_t current, std::uint32_t total, const char *status) {
			//do nothing
		}

		int logMessage(log_priority priority, const char *fmt, ...)
		{
			if (priority < global_loglevel) { return 0; }

			va_list args;
			va_start(args, fmt);

			const char *priority_str;
			//I use a bunch of if statements here because the array that has strings over at ntrboot_flasher's `platform.cpp` is not available here
			if (priority == 0) { priority_str = "DEBUG"; }
			if (priority == 1) { priority_str = "INFO"; }
			if (priority == 2) { priority_str = "NOTICE"; }
			if (priority == 3) { priority_str = "WARN"; }
			if (priority == 4) { priority_str = "ERROR"; }
			if (priority >= 5) { priority_str = "?!#$"; }

			char string_to_write[100]; //just do 100, should be enough for any kind of log message we get...
			sprintf(string_to_write, "[%s]: %s\n", priority_str, fmt);

			consoleSelect(&bottomScreen);
			int result = iprintf(string_to_write, args, "\n");
			consoleSelect(&topScreen);
			va_end(args);

			return result;
		}

		auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048]
		{
			switch (key) {
				default:
				case BlowfishKey::NTR:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_ntr_bin));
				case BlowfishKey::B9Retail:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
				case BlowfishKey::B9Dev:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_dev_bin));
			}
		}
	}
}

void WaitKey(u32 wait_key) {
	while (true) { scanKeys(); if (keysDown() & wait_key) { break; } }
}

int main() {
	//all this is screen init stuff
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
    consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
	consoleSelect(&topScreen);
	//give arm9 access to cart
	sysSetBusOwners(true, true);
	//setup the cart variable here, change the value in `at()` to select your flashcart (requires recompiling, i know)
	//0: ak2i, 1: dstt, 2: r4 sdhc dual core, 3: r4isdhc, 4: r4igold
	flashcart_core::Flashcart *cart = flashcart_core::flashcart_list->at(4); 
	//set up ncgc's card object which holds the cart protocol
	ncgc::NTRCard card(nullptr);
	//set the state with key2
	card.state(ncgc::NTRState::Key2);
	if (!cart->initialize(&card)) {
		iprintf("flashcart setup failed\npress <A> to shutdown");
		WaitKey(KEY_A);
		exit(0);
	}
	iprintf("flashcart setup success, now you'd proceed with the program here");
	WaitKey(KEY_A);
	exit(0);
}