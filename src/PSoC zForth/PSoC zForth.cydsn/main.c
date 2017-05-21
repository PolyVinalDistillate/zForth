/*
	This main file is intended to operate with a millisecond interrupt and a USBUART component on a PSoC 5. The original file was derived from the 
	Cypress USBUART example project. The main aim here was to get zForth running word-at-a-time rather than getting stuck in its execution loop while
	executing Forth loops. This way, an update function can be called repeatedly by the host without losing the ability to do other stuff
	in between executing Forth code. Ultimately, this may be interrupt driven in the target system.
	
	I make no claims to good programming here - none whatsoever! Much of the work done here took place between 1 AM and 12 PM. Any suggestions
	for improvements of my modifications to the zForth source code will certainly be welcomed! (I still don't entirely understand the zForth
	source, so it's a miracle I've got it working as well as I have). Next step is to actually *learn* Forth!
*/

#include <project.h>
#include <stdarg.h>
#include <stdio.h>
#include "..\..\zforth\zforth.h"
#define USBFS_DEVICE    (0u)
#define USBUART_BUFFER_SIZE (64u)

void USB_StatePoll()
{
    if (0u != USBUART_IsConfigurationChanged())
        if (0u != USBUART_GetConfiguration())
            USBUART_CDC_Init();
}

int USB_GetBytes(unsigned char* buffer)
{
    int count = 0;
    if (0u != USBUART_GetConfiguration())
        if (0u != USBUART_DataIsReady())
            count = USBUART_GetAll(buffer);
    return count;
}

unsigned char InBuf[64];
unsigned char nBufPos = 0;
unsigned char nBufLen = 0;
unsigned char USB_GetChar()
{
    if(nBufPos==nBufLen)
    {
        nBufPos = 0;
        nBufLen = USB_GetBytes(InBuf);
    }
    if(nBufPos != nBufLen)
    {
        unsigned char c = InBuf[nBufPos];
        nBufPos++;
        return c;
    }
    return 0;
}

void USB_PutBytes(unsigned char* buffer, int count)
{
    if (0u == USBUART_GetConfiguration()) return;
    while(count > 0)
    {
        while (0u == USBUART_CDCIsReady());
        int cn = (count > (int)USBUART_BUFFER_SIZE) ? USBUART_BUFFER_SIZE:count;
        USBUART_PutData(buffer, cn);
        
        if(cn == USBUART_BUFFER_SIZE)
        {
            while (0u == USBUART_CDCIsReady());
            USBUART_PutData(NULL, 0u);        
        }
        count -= USBUART_BUFFER_SIZE;
        buffer += USBUART_BUFFER_SIZE;
    }
    
}

CY_ISR(MSI)
{
    USB_StatePoll();
}

void USB_Print(char* str)
{
    USB_PutBytes(str, strlen(str));
}

void uprintf(const char* pStr, va_list va)
{
    
    char tmp[1024];
    sprintf(tmp, pStr, va);
    USB_Print(tmp);
}

int main()
{
    CyGlobalIntEnable;
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);    
    MS_StartEx(MSI);

	zf_init(0);					//Standard init
	zf_bootstrap();				//Standard bootstrap
	zf_eval(ZF_CORE_STR, 0);	//Load up core definitions from const string in "zForth.c" with the contents of "core.zf"
    
    int nUSB_Bytes = 0;
    unsigned short nBytes = nUSB_Bytes;
    unsigned char pUSB_Buf[64];
    unsigned char* pBufPos = pUSB_Buf;
	for(;;) 
    {        
        
        if(nUSB_Bytes <= 0)
        {
            nUSB_Bytes = USB_GetBytes(pUSB_Buf);
            pBufPos = pUSB_Buf;
        }
        
        nBytes = nUSB_Bytes;
        zf_result r = zf_Main_Update_Fxn(pBufPos, &nBytes);	//This function wraps the zf_eval() and my questionable word-at-a-time code
        nUSB_Bytes -= nBytes;
        pBufPos += nBytes;
        switch(r)
        {
        	case ZF_OK:
            {
                break;
            }
        	case ZF_ABORT_INTERNAL_ERROR:
            {
                USB_Print("\r\nINTERNAL ERROR\r\n");
                break;
            }
        	case ZF_ABORT_OUTSIDE_MEM:
            {
                USB_Print("\r\nOUTSIDE MEMORY\r\n");
                break;
            }
        	case ZF_ABORT_DSTACK_UNDERRUN:
            {
                USB_Print("\r\nDATA STACK UNDERRUN\r\n");
                break;
            }
        	case ZF_ABORT_DSTACK_OVERRUN:
            {
                USB_Print("\r\nDATA STACK OVERRUN\r\n");
                break;
            }
        	case ZF_ABORT_RSTACK_UNDERRUN:
            {
                USB_Print("\r\nRETURN STACK UNDERRUN\r\n");
                break;
            }
        	case ZF_ABORT_RSTACK_OVERRUN:
            {
                USB_Print("\r\nRETURN STACK OVERRUN\r\n");
                break;
            }
        	case ZF_ABORT_NOT_A_WORD:
            {
                USB_Print("\r\nNOT A WORD\r\n");
                break;
            }
        	case ZF_ABORT_COMPILE_ONLY_WORD:
            {
                USB_Print("\r\nCOMPILE ONLY WORD\r\n");
                break;
            }
        	case ZF_ABORT_INVALID_SIZE:
            {
                USB_Print("\r\nINVALID SIZE\r\n");
                break;
            }            
        }
		/*
		Other system code can go in here
		*/
        
	}
}


zf_input_state zf_host_sys(zf_syscall_id id, const char *input)
{
	switch((int)id) 
    {
		case ZF_SYSCALL_EMIT:			//Send byte to the USBUART
        {
            char tmp = (char)zf_pop();
			USB_PutBytes(&tmp, 1);
			break;
        }
		case ZF_SYSCALL_PRINT:			//An outrageously convoluted "printf("%f", fVal)" to avoid use of heap
        {                               //NOTE: It has some issues with *big* numbers!
            char buf[100];
            int nIndex = 0;
            int nDP = 5;
            double i = zf_pop();
            if(i == 0)
            {
                buf[0] = '0';
                buf[1] = 0;
            }
            else
            {
                unsigned char bNeg = 0;
                if(i < 0)
                {
                    i = -i;
                    bNeg = 1;
                }
                
                double v_dp = i - (long)i;
                long v_wn = i-v_dp;
                
                while((v_dp != 0) || (v_wn != 0))
                {
                    if(v_wn != 0)
                    {
                        long tmp = v_wn % 10;
                        v_wn /= 10;
                        buf[nIndex] = tmp+'0';
                        if(v_wn == 0)
                        {
                            if(bNeg)
                            {
                                nIndex++;
                                buf[nIndex] = '-';
                            }
                            for(tmp = 0; tmp < (nIndex+1)/2; tmp++)
                            {
                                char tmp2 = buf[tmp];
                                buf[tmp] = buf[nIndex-tmp];
                                buf[nIndex-tmp] = tmp2;
                            }
                            if(v_dp)
                            {
                                nIndex++;
                                buf[nIndex] = '.';
                            }
                        }
                        nIndex++;
                    }
                    else if (v_dp != 0)
                    {
                        v_dp *= 10;
                        unsigned char v = (unsigned char)v_dp + '0';
                        v_dp -= (unsigned char)v_dp;
                        buf[nIndex] = v;
                        nIndex++;
                        nDP--;
                        if(nDP == 0)
                            v_dp = 0;
                    }
                }
                buf[nIndex] = 0;
            }
            USB_Print(buf);
			break;
        }
		case ZF_SYSCALL_TELL: //Still not entirely sure what this does... But it's based on the example in the "linux" source folder
        {
			zf_cell len = zf_pop();
			void *buf = (uint8_t *)zf_dump(NULL) + (int)zf_pop();
			USB_PutBytes(buf, len);
			break;
        }        
	}

	return 0;
}




/* [] END OF FILE */
