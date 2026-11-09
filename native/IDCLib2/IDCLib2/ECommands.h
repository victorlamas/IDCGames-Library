#pragma once

enum Command : unsigned int {
	Error = 0xfbb65363,
	InitLib = 0x89d4951f,
	EndLib = 0x87a3628f,
	PurchaseInit = 0xffeecce6,
	PurchaseEnd = 0xa9bbb04b,
	UserData = 0x3275c4a, //DTINF
	BadUserInitData = 0xaba83a28, //BDINT
	PurchaseNotification = 0x733f2850, //PPNOT
	NewPurchaseCode = 0xfa4967c3, //NWPRC
	PurchaseOk = 0x8218d08c, //OKPRC
	PurchaseError = 0xbb485d88, //BDPRC
	BadPurchaseCode = 0x6a6ac964, // BDCPR
	PurchaseClosed = 0x6c1ccf80, // PRCHC
	VideoAdResult = 0xb1de0a7d // VADRS
	/*

	37e5121c
	834e0e5e
	ca01d6b2
	fb090158
	c6c45313
	8421e045
	c0a53a66
	c13492e6
	4008e557
	52cfcbc1
	6af6f4c4
	96c4729f
	685a4808
	aa80c175
	e0e156c3
	a9ace67b
	b931c6b8
	98210d80
	*/
};
