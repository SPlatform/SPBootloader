
/********************************* INCLUDES ***********************************/
#include "Drv_UART.h"

#include "TestData.h"

/***************************** MACRO DEFINITIONS ******************************/
#define EXTERNAL_TEST_DATA          1
#define VALID_DATA                  0

#if EXTERNAL_TEST_DATA
#define LENGTH_OF_REGULAR		sizeof(testImage) / (sizeof(char*))

#else
#define LENGTH_OF_REGULAR		sizeof(regularIntelHex) / (sizeof(char*))
#endif
/***************************** TYPE DEFINITIONS *******************************/

/**************************** FUNCTION PROTOTYPES *****************************/

/******************************** VARIABLES ***********************************/


#if !EXTERNAL_TEST_DATA

#if VALID_DATA
PRIVATE char* regularIntelHex[] =
{
	":020000040000FA",
	":0402FC00FFFFFFFF02",
	":020000040000FA",
	":10800000600400104D810000558100005781000080",
	":10801000598100005B8100005D81000000000000CC",
	":108020000000000000000000000000005F81000070",
	":108030006181000000000000638100006581000094",
	":108040006781000067810000678100006781000090",
	":108050006781000067810000678100006781000080",
	":1080D00000F02CF80AA090E8000C82448344AAF136",
	":1080E0000107DA4501D100F021F8AFF2090EBAE834",
	":10847000C8FEFFF7ECFF0000888400000000001039",
	":08848000600400000881000007",
	":04000005000080CDAA",
	":00000001FF"
};
#else
PRIVATE char* regularIntelHex[] =
{
	":020000040000FA",
	":0402FC00FFFFFFFF02",
	":0402FC00FFFFFFFF01", // ":0402FC00FFFFFFFF02",
	":020000040000FA",
	":10800000:10800000600400104D810000558100005781000080",
	"1000598100005B8100005D81000000000000CC",     /* Missing line */
	":10801000598100005B8100005D81000000000000CC",
	":108020000000000000000000000000005F81000070",
	":108030006181000000000000",
	"638100006581000094",
	":108040006781000067810000678100006781000090",
	":108050006781000067810000678100006781000080:1080D00000F02CF80AA090E8000C82448344AAF136",
	":1080E0000107DA4501D100F021F8AFF2090EBAE834",
	":10847000C8FEFFF7ECFF00008884",
	"00000000001039:08848000600400000881000007",
	":04000005000080CDAA",
	":00000001FF",
	""
};
#endif

#endif

UARTDataReceivedEventHandler evHandler;
PRIVATE uint32_t index = 0;

/**************************** PRIVATE FUNCTIONS ******************************/

/***************************** PUBLIC FUNCTIONS *******************************/
void Drv_UART_Init(void)
{

}

UartHandle Drv_UART_Get(uint32_t uartNo, uint32_t baudRate, UARTDataReceivedEventHandler dataReceivedEventHandler)
{
	evHandler = dataReceivedEventHandler;

	evHandler();

	return uartNo;
}

void Drv_UART_Release(UartHandle uart)
{
}

int32_t Drv_UART_Receive(UartHandle uart, uint8_t* receiveBuffer, uint32_t receiveLength)
{
	int32_t msgLeng;
	const char* hexLine;
#if EXTERNAL_TEST_DATA
	hexLine = testImage[index];
#else
	hexLine = regularIntelHex[index];
#endif

	msgLeng = strlen(hexLine);

	memcpy(receiveBuffer, hexLine, msgLeng);

	if (index++ < LENGTH_OF_REGULAR)
		evHandler();
	else
		return -1;

	return msgLeng;
}
