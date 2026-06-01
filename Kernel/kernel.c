#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include "include/videoDriver.h"
#include "include/keyboardDriver.h"
#include "include/idtLoader.h"
#include <memManager.h>
#include "include/scheduler.h"

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;

typedef int (*EntryPoint)();


void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
    return (void*)((uint64_t)&endOfKernel + PageSize); 
}

void * initializeKernelBinary()
{
	char buffer[10];

	ncPrint("[x64BareBones]");
	ncNewline();

	ncPrint("CPU Vendor:");
	ncPrint(cpuVendor(buffer));
	ncNewline();

	ncPrint("[Loading modules]");
	ncNewline();
	void * moduleAddresses[] = {
		sampleCodeModuleAddress,
		sampleDataModuleAddress
	};

	loadModules(&endOfKernelBinary, moduleAddresses);
	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	ncPrint("[Initializing kernel's binary]");
	ncNewline();

	clearBSS(&bss, &endOfKernel - &bss);

	void * heapStart = (void*)((uint64_t)&endOfKernel + PageSize * 8);
	uint64_t heapSize = (uint64_t)sampleCodeModuleAddress - (uint64_t)heapStart;
	mm_init(heapStart, heapSize);

	ncPrint("  text: 0x");
	ncPrintHex((uint64_t)&text);
	ncNewline();
	ncPrint("  rodata: 0x");
	ncPrintHex((uint64_t)&rodata);
	ncNewline();
	ncPrint("  data: 0x");
	ncPrintHex((uint64_t)&data);
	ncNewline();
	ncPrint("  bss: 0x");
	ncPrintHex((uint64_t)&bss);
	ncNewline();

	ncPrint("[Done]");
	ncNewline();
	ncNewline();
	return getStackBase();
}

int main()
{	
    scheduler_init();

    ncPrint("[Kernel Main]");
    ncNewline();
    ncPrint("  Sample code module at 0x");
    ncPrintHex((uint64_t)sampleCodeModuleAddress);
    ncNewline();
    ncPrint("  Sample data module at 0x");
    ncPrintHex((uint64_t)sampleDataModuleAddress);
    ncNewline();

    load_idt(); 

    scheduler_create(
        (void*)sampleCodeModuleAddress,
        "shell",
        3,    // priority
        1,    // foreground
        0,    // argc
        NULL  // argv
    );

    while(1) {
        _hlt();
    }

    return 0;
}
