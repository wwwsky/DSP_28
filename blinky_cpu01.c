
1 //#########################################################################
2 // FILE: blinky_cpu01.c
3 // TITLE: LED Blink Example for F2837xD.
4 //
5 //! \addtogroup cpu01_example_list
6 //! <h1> Blinky </h1>
7 //!
8 //! This example blinks LED X
9 //
10 //#########################################################################
11 // $TI Release: F2837xD Support Library v180 $
12 // $Release Date: Fri Nov 6 16:19:46 CST 2015 $
13 // $Copyright: Copyright (C) 2013-2015 Texas Instruments Incorporated -
14 // http://www.ti.com/ ALL RIGHTS RESERVED $
15 //#########################################################################
16
17 #include "F28x_Project.h" // Device Headerfile and Examples IncludeFile
18 #include <string.h>
19
20 Uint16 test = 0;
21
22 //void setup_emif1_pinmux_async_16bit(Uint16 cpu_sel);
23 //***************************************************************
24 //usart initial block
25 //***************************************************************
26 void UsartInit(void)
27 {
28 GPIO_SetupPinMux(28, GPIO_MUX_CPU1, 1);
29 GPIO_SetupPinOptions(28, GPIO_INPUT, GPIO_PUSHPULL);
30 GPIO_SetupPinMux(29, GPIO_MUX_CPU1, 1);
31 GPIO_SetupPinOptions(29, GPIO_OUTPUT, GPIO_ASYNC);
32
33 //---- USART configuration ----------
34 SciaRegs.SCIFFTX.all=0xE040;
35 SciaRegs.SCIFFRX.all=0x2044;
36 SciaRegs.SCIFFCT.all=0x0;
37
38 SciaRegs.SCICCR.all =0x0007; // 1 stop bit, No loopback
39 // No parity,8 char bits,
40 // async mode, idle-line protocol
41 SciaRegs.SCICTL1.all =0x0003; // enable TX, RX, internal SCICLK,
42 // Disable RX ERR, SLEEP, TXWAKE
43 SciaRegs.SCICTL2.all =0x0003;
44 SciaRegs.SCICTL2.bit.TXINTENA =1;
45 SciaRegs.SCICTL2.bit.RXBKINTENA =1;
46 SciaRegs.SCIHBAUD.all =0x0002;
47 SciaRegs.SCILBAUD.all =0x008B;
48 // SciaRegs.SCICCR.bit.LOOPBKENA =1; // Enable loop back
49 SciaRegs.SCICTL1.all =0x0023; // Relinquish SCI from Reset
50
51 //-DMA configuration
52 }
53
54 //***************************************************************
55 //usart UsartSendData block
56 //just fit for 8 bit data to send
57 //***************************************************************
58 void UsartSendData(uint16_t data_in)
59 {
60 SciaRegs.SCITXBUF.all = data_in;
61 while(SciaRegs.SCIFFTX.bit.TXFFST !=0); // wait for RRDY/RXFFST =1 fordata available in FIFO
62 }
63
64
65 void main(void)
66 {
67 InitSysCtrl();
68
69 InitGpio();
70
71 DINT;
72
73 InitPieCtrl();
74
75 // Disable CPU interrupts and clear all CPU interrupt flags:
76 IER = 0x0000;
77 IFR = 0x0000;
78
79 InitPieVectTable();
80
81 EINT; // Enable Global interrupt INTM
82
83 UsartInit();
84
85
86 EALLOW;
87 GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;
88 GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;
89 EDIS;
90
91 GpioDataRegs.GPADAT.bit.GPIO10 = 0;
92
93 while(1){
94
95 GpioDataRegs.GPADAT.bit.GPIO10 = 0;
96
97 DELAY_US(50000);
98
99 GpioDataRegs.GPADAT.bit.GPIO10 = 1;
100
101 DELAY_US(50000);
102
103 if(test == 255) test = 0;
104
105 test = test + 1;
106
107 UsartSendData(test);
108
109 DELAY_US(1000);
110 }
123 }
124