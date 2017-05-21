DISCLAIMER: There is some potentially silly code in here - mo refactoring has been done!
Any suggestions for improvement will be welcomed when I see them :)

The PSoC Creator project in this folder is intended to operate on the CY8CKIT-059.
It sets the CPU to 64 MHz and utilises the USBUART to provide an interactive interface.

The project was originally derived from the Cypress USBUART example, however many changes
have been made from the original project:

    USB UART Print functions
    USB UART buffer handling
    Moving USB UART state handling to a millisecond interrupt
    Integrating zForth


Nick (PolyVinalDistillate).