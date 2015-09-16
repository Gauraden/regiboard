
// Last Ver. 0.0.2.28
// Prev Ver. 0.0.2.28


// Номера регистров конфигурации

#define   c_ModbusParams  110 //78//36 //26
#define   c_ModbusConfig  67 //54 //45//172 //140 //105 //73
#define   c_ModbusCoil    16
#define   c_ModbusLogic   2

//для протокола Modbus RTU
#define start_adr_izm_reg16   	0	    //адрес начала блока измеряемых параметров (в 16 битных регистрах)
#define   end_adr_izm_reg16   	(c_ModbusParams)*2	//адрес конца блока измеряемых параметров  (в 16 битных регистрах) должно быть кратно 2-м
#define start_adr_conf_reg16   	0	//адрес начала блока параметров конф(в 16 битных регистрах)
#define   end_adr_conf_reg16   	c_ModbusConfig	//адрес конца блока параметров конф (в 16 битных регистрах)
#define start_adr_coil_reg16  	0	//адрес начала блока параметров конф(в 16 битных регистрах)
#define   end_adr_coil_reg16 	  c_ModbusCoil	//адрес конца блока параметров конф (в 16 битных регистрах)
#define start_adr_logic_reg16   0	//адрес начала блока параметров конф(в 16 битных регистрах)
#define   end_adr_logic_reg16   c_ModbusLogic	//адрес конца блока параметров конф (в 16 битных регистрах)



// === Coil Reg [0x01/0x05]===

// For Rele Pcb
#define  r_ReleRaw       end_adr_coil_reg16  - 1

// For Logic Input/output Pcb
#define  r_LogicInRaw    end_adr_coil_reg16  - 1
#define  r_LogicOutRaw   end_adr_coil_reg16  - 2


// Помощник-макрос получения числа без знака
#define MAKE_ABS(a)      (a < 0 ? -(a) : a)

// === Measure Reg [0x04] === Плата ADC ===

#define  r_ADCh1         c_ModbusParams  - 1
#define  r_ADCh2         c_ModbusParams  - 2
#define  r_ADCh3         c_ModbusParams  - 3
#define  r_ADCh4         c_ModbusParams  - 4
#define  r_ADCh5         c_ModbusParams  - 5
#define  r_ADCh6         c_ModbusParams  - 6
#define  r_ADCh7         c_ModbusParams  - 7
#define  r_ADCh8         c_ModbusParams  - 8


// Заведем специальные макросы по которым будут адресоваться регистры.
// Таким образом избавимся от привязки к конкретным адресам регистров.
// GET_ADCh(n)    n = 1..8
// descr: получиь значение аналогового входа
// return: float число
#define GET_ADCh(n)                                               ( -2 * ((r_ADCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_LastReadTimeCh12   c_ModbusParams  - 9
#define  r_LastReadTimeCh34   c_ModbusParams  - 10
#define  r_LastReadTimeCh56   c_ModbusParams  - 11
#define  r_LastReadTimeCh78   c_ModbusParams  - 12
// GET_LASTREADTIMECH(n)   n - номер пары, n = 1..4
// descr: получить время последнего обновления регистра в программе прошивки (для синхронизации может быть пригодится?)
// return: unsigned long, в котором лежит пара коротких значений, которые надо распарсить будет
#define GET_LASTREADTIMECH(n)                                     ( -2 * ((r_LastReadTimeCh12) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_TempHS		     c_ModbusParams  - 13
// GET_COLDJUNCTION_TEMP(n)    n - номер датчика, n = 1..1 (пока на плате он только один, так что только n = 1)
// descr: получить температуру холодного спая в градусах
// return: float
#define GET_COLDJUNCTION_TEMP(n)                                  ( -2 * ((r_TempHS) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_ADTempCh1     c_ModbusParams  - 14
#define  r_ADTempCh2     c_ModbusParams  - 15
#define  r_ADTempCh3     c_ModbusParams  - 16
#define  r_ADTempCh4     c_ModbusParams  - 17
#define  r_ADTempCh5     c_ModbusParams  - 18
#define  r_ADTempCh6     c_ModbusParams  - 19
#define  r_ADTempCh7     c_ModbusParams  - 20
#define  r_ADTempCh8     c_ModbusParams  - 21
// GET_ADTEMPCH(n)  n - номер канала. n = 1..8
// descr: получить температуру канала
// return: float
#define GET_ADTEMPCH(n)                                           ( -2 * ((r_ADTempCh1) - c_ModbusParams + 1) - (2 * (n - 1)) )

#define  r_ADUsuppCh1     c_ModbusParams  - 22
#define  r_ADUsuppCh2     c_ModbusParams  - 23
#define  r_ADUsuppCh3     c_ModbusParams  - 24
#define  r_ADUsuppCh4     c_ModbusParams  - 25
#define  r_ADUsuppCh5     c_ModbusParams  - 26
#define  r_ADUsuppCh6     c_ModbusParams  - 27
#define  r_ADUsuppCh7     c_ModbusParams  - 28
#define  r_ADUsuppCh8     c_ModbusParams  - 29
// GET_ADVOLTAGESUPP(n)  n - номер канала. n = 1..8
// descr: получить напряжение на канале
// return: float
#define GET_ADVOLTAGESUPP(n)                                      ( -2 * ((r_ADUsuppCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )


#define  r_CommCurrentSupply1		c_ModbusParams  - 30
#define  r_CommCurrentSupply2		c_ModbusParams  - 31
#define  r_CommShortcutChQty		c_ModbusParams  - 32   //  0x0000000xx - 1я четверка каналов, 0x00000xx00 - 2я четверка каналов
#define  r_CommVoltSupply1			c_ModbusParams  - 33
#define  r_CommVoltSupply2			c_ModbusParams  - 34


#define  r_AllCurSupply		c_ModbusParams  - 35
// GET_ALLCURSUPPLY(n)   n - 1..1
// descr: получить общее потребление тока
// return: float
#define GET_ALLCURSUPPLY(n)                                       ( -2 * ((r_AllCurSupply) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_ADCType_10VDet	c_ModbusParams  - 36  //  0x00XX   1 - AD7795 (16-bit), 			0 - AD7794 (24-bit)
																								//  0xXX00   1 - Делитель отсутствует, 	0 - Делитель присутствует
// GET_ADCTYPE     нет параметров
// descr: получить тип ADC
// return: Long
#define GET_ADCTYPE                                               MAKE_ABS( 2 * ( (r_ADCType_10VDet) - c_ModbusParams + 1 ) )

#define  r_ADStatus		    c_ModbusParams  - 37  //  0x00XX   1 - Event, 0 - No event         Addr 0x3C - 0x3D
																									 //  0 bit - AD_BreakLine
																									 //  1 bit - AD_DownDiap_UpDiap_DownNom_UpNom
																									 //  2 bit - AD_Overload
																									 //  3 bit - AD_Connect
																									 //  4 bit r_AD_BadReInit
																									 //  5 bit - AD_Commmon_ChCrash
																									 //  6 bit - AD_TempOMMCom_TempOvlCh_TempMaxCh_TempMinCh
																									 //  7 bit - AD_SuppOMMCom_SuppOvlCh_SuppMaxCh_SuppMinCh
																									 //  8 bit - AD_ExternalSensorInfo
																									 //  9 bit - AD_InputElementCrash1
																									 // 10 bit - AD_InputElementCrash2
																									 // 11 bit - AD_InputElementCrash3
																									 // 12 bit - AD_InputElementCrash4
																									 // 13 bit - PCB_SystemError
																									 
// GET_ADSTATUS   нет параметров
// descr: получить статус системы (определение обрывов, сбоев; всё тут)
// return: Long
#define GET_ADSTATUS                                              MAKE_ABS( 2 * ( (r_ADStatus) - c_ModbusParams + 1 ) )

#define  r_AD_BreakLine   		 c_ModbusParams  - 38                           				//        Addr 0x3E - 0x3F
//r_ADBreakLine		//  0x0000000XX   1 - Input Line is break,				 0 - Input Line is OK
// GET_AD_BREAKLINE    нет параметров
// descr: получить карту входов, на которых случился обрыв
// return: Long
#define GET_AD_BREAKLINE                                          MAKE_ABS( 2 * ( (r_AD_BreakLine) - c_ModbusParams + 1 ) )

#define  r_AD_DownDiap_UpDiap_DownNom_UpNom   c_ModbusParams  - 39                           				//        Addr 0x40 - 0x41
//UpNom			//  0x0000000XX   1 - Input Line is below min, 0 - Input Line is OK
//DownNom		//  0x00000XX00   1 - Input Line is over max,  0 - Input Line is OK
//UpDiap		//  0x000XX0000   1 - Input Line is overload,  0 - Input Line is OK
//DownDiap	//  0x0XX000000   1 - Input Line is over max,  0 - Input Line is OK

#define  r_AD_Overload   c_ModbusParams  - 40                           				//        Addr 0x40 - 0x41
//r_ADOvl					//  0x0000000XX   1 - Input Line is below min, 0 - Input Line is OK
//r_ADScaleOvl		//  0x00000XX00   1 - Input Line is below min, 0 - Input Line is OK

#define  r_AD_Connect  c_ModbusParams  - 41														//        Addr 0x42 - 0x43
//r_ADConnect1Error //  0x0000000XX   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect2Error //  0x00000XX00   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect3Error //  0x000XX0000   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect4Error //  0x0XX000000   1 - ADC Connect Error, 0 - ADC connection is OK

#define  r_AD_BadReInit  c_ModbusParams  - 42
//r_AD_BadReInit1 //  0x0000000XX   1 - ADC ReInit, 0 - ADC not ReInit
//r_AD_BadReInit2 //  0x00000XX00   1 - ADC Bad ReInit, 0 - ADC ReInit is OK


#define  r_AD_Commmon_ChCrash  c_ModbusParams  - 43														//        Addr 0x42 - 0x43
//r_ADChCrash			 //  0x0000000XX   1 - ADC Crash, 				0 - ADC is OK
//r_ADCommonError	 //  0x00000XX00   1 - ADC error, 				0 - ADC is OK

#define  r_AD_TempOMMCom_TempOvlCh_TempMaxCh_TempMinCh	c_ModbusParams  - 44					//        Addr 0x44 - 0x45
//r_ADTempMaximumChannel    //  0x0000000XX   1 - Temp error, 	  0 - Temp is OK
//r_ADTempOverloadChannel   //  0x00000XX00   1 - Temp error, 	  0 - Temp is OK
//r_ADTempMinOvlErrCommon   //  0x000XX0000   1 - Temp error, 	  0 - Temp is OK

#define  r_AD_SuppOMMCom_SuppOvlCh_SuppMaxCh_SuppMinCh	c_ModbusParams  - 45					//        Addr 0x46 - 0x47
//r_ADSupplyMinimalChannel  //  0x0000000XX   1 - Supply error, 	0 - supply is OK
//r_ADSupplyMaximumChannel  //  0x00000XX00   1 - Supply error, 	0 - supply is OK
//r_ADSupplyOverloadChannel //  0x000XX0000   1 - Supply error, 	0 - supply is OK
//r_ADSupplyMinMaxOvlCommon //  0x0XX000000   1 - Supply error, 	0 - supply is OK

#define  r_AD_ExternalSensorInfo                        c_ModbusParams  - 46					//        Addr 0x48 - 0x49

#define  r_AD_InputElementCrash1                        c_ModbusParams  - 47					//        Addr 0x50 - 0x51
//r_AD_InputElementCrash_1  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_2 	//  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_3	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_4	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash2                        c_ModbusParams  - 48					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_5  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_6 	//  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_7	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_8	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash3                        c_ModbusParams  - 49					//        Addr 0x50 - 0x51
//r_AD_InputElementCrash_9  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_10 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_11	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_12	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash4                        c_ModbusParams  - 50					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_13 //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_14 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_15	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_16	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_BadCalib						                      c_ModbusParams  - 51					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_13 //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_14 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK

// === Measure Reg [0x04] === Плата DAC ===

// === Measure Reg [0x04] === Плата Реле ===

// === Measure Reg [0x04] === Плата дискр вх./вых. === 
#define  r_DInput      c_ModbusParams  - 1
// GET_DINPUT    без параметров
// descr: получить маску битов выставленных лог.входов в 1
// return: unsigned long
#define GET_DINPUT                                                ( -2 * ((r_DInput) - c_ModbusParams + 1) )

#define  r_DIFreqCh1   c_ModbusParams  - 2
#define  r_DIFreqCh2   c_ModbusParams  - 3
#define  r_DIFreqCh3   c_ModbusParams  - 4
#define  r_DIFreqCh4   c_ModbusParams  - 5
#define  r_DIFreqCh5   c_ModbusParams  - 6
#define  r_DIFreqCh6   c_ModbusParams  - 7
#define  r_DIFreqCh7   c_ModbusParams  - 8
#define  r_DIFreqCh8   c_ModbusParams  - 9
#define  r_DIFreqCh9   c_ModbusParams  - 10
#define  r_DIFreqCh10  c_ModbusParams  - 11
#define  r_DIFreqCh11  c_ModbusParams  - 12
#define  r_DIFreqCh12  c_ModbusParams  - 13
// GET_DIFREQ(n)    n - номер канала. n = 1..4
// descr: получить частоту по каналу
// return: unsigned long
#define GET_DIFREQ(n)                                              ( -2 * ((r_DIFreqCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DIPerDurCh1   c_ModbusParams  - 14
#define  r_DIPerDurCh2   c_ModbusParams  - 15
#define  r_DIPerDurCh3   c_ModbusParams  - 16
#define  r_DIPerDurCh4   c_ModbusParams  - 17
#define  r_DIPerDurCh5   c_ModbusParams  - 18
#define  r_DIPerDurCh6   c_ModbusParams  - 19
#define  r_DIPerDurCh7   c_ModbusParams  - 20
#define  r_DIPerDurCh8   c_ModbusParams  - 21
#define  r_DIPerDurCh9   c_ModbusParams  - 22
#define  r_DIPerDurCh10  c_ModbusParams  - 23
#define  r_DIPerDurCh11  c_ModbusParams  - 24
#define  r_DIPerDurCh12  c_ModbusParams  - 25

// GET_DIPERDUR(n)  n - number of a channel. n = 1..4
// descr: get a duration period of a channel
// returning: unsigned long
#define GET_DIPERDUR(n)                                            ( -2 * ((r_DIPerDurCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DIPosDurCh1   c_ModbusParams  - 26
#define  r_DIPosDurCh2   c_ModbusParams  - 27
#define  r_DIPosDurCh3   c_ModbusParams  - 28
#define  r_DIPosDurCh4   c_ModbusParams  - 29
#define  r_DIPosDurCh5   c_ModbusParams  - 30
#define  r_DIPosDurCh6   c_ModbusParams  - 31
#define  r_DIPosDurCh7   c_ModbusParams  - 32
#define  r_DIPosDurCh8   c_ModbusParams  - 33
#define  r_DIPosDurCh9   c_ModbusParams  - 34
#define  r_DIPosDurCh10  c_ModbusParams  - 35
#define  r_DIPosDurCh11  c_ModbusParams  - 36
#define  r_DIPosDurCh12  c_ModbusParams  - 37

// GET_DIPOSDUR(n)  n - number of a channel. n = 1..4
// descr: get a positive duration of a channel
// returning: unsigned long
#define GET_DIPOSDUR(n)                                            ( -2 * ((r_DIPosDurCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DINegDurCh1   c_ModbusParams  - 38
#define  r_DINegDurCh2   c_ModbusParams  - 39
#define  r_DINegDurCh3   c_ModbusParams  - 40
#define  r_DINegDurCh4   c_ModbusParams  - 41
#define  r_DINegDurCh5   c_ModbusParams  - 42
#define  r_DINegDurCh6   c_ModbusParams  - 43
#define  r_DINegDurCh7   c_ModbusParams  - 44
#define  r_DINegDurCh8   c_ModbusParams  - 45
#define  r_DINegDurCh9   c_ModbusParams  - 46
#define  r_DINegDurCh10  c_ModbusParams  - 47
#define  r_DINegDurCh11  c_ModbusParams  - 48
#define  r_DINegDurCh12  c_ModbusParams  - 49

// GET_DINEGDUR(n)  n - number of a channel. n = 1..4
// descr: get a negative period of a channel
// returning: unsigned long
#define GET_DINEGDUR(n)                                            ( -2 * ((r_DINegDurCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DIPosDurPPCh1   c_ModbusParams  - 50
#define  r_DIPosDurPPCh2   c_ModbusParams  - 51
#define  r_DIPosDurPPCh3   c_ModbusParams  - 52
#define  r_DIPosDurPPCh4   c_ModbusParams  - 53
#define  r_DIPosDurPPCh5   c_ModbusParams  - 54
#define  r_DIPosDurPPCh6   c_ModbusParams  - 55
#define  r_DIPosDurPPCh7   c_ModbusParams  - 56
#define  r_DIPosDurPPCh8   c_ModbusParams  - 57
#define  r_DIPosDurPPCh9   c_ModbusParams  - 58
#define  r_DIPosDurPPCh10  c_ModbusParams  - 59
#define  r_DIPosDurPPCh11  c_ModbusParams  - 60
#define  r_DIPosDurPPCh12  c_ModbusParams  - 61

// GET_DIPOSDURPP(n)  n - number of a channel. n = 1..4
// descr: get a positive period PP of a channel
// returning: unsigned long
#define GET_DIPOSDURPP(n)                                          ( -2 * ((r_DIPosDurPPCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DINegDurPPCh1   c_ModbusParams  - 62
#define  r_DINegDurPPCh2   c_ModbusParams  - 63
#define  r_DINegDurPPCh3   c_ModbusParams  - 64
#define  r_DINegDurPPCh4   c_ModbusParams  - 65
#define  r_DINegDurPPCh5   c_ModbusParams  - 66
#define  r_DINegDurPPCh6   c_ModbusParams  - 67
#define  r_DINegDurPPCh7   c_ModbusParams  - 68
#define  r_DINegDurPPCh8   c_ModbusParams  - 69
#define  r_DINegDurPPCh9   c_ModbusParams  - 70
#define  r_DINegDurPPCh10  c_ModbusParams  - 71
#define  r_DINegDurPPCh11  c_ModbusParams  - 72
#define  r_DINegDurPPCh12  c_ModbusParams  - 73
// GET_DINEGDURPP(n)  n - number of a channel. n = 1..4
// descr: get a negative duration period PP of a channel
// returning: unsigned long
#define GET_DINEGDURPP(n)                                          ( -2 * ((r_DINegDurPPCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DIDutyCh1     c_ModbusParams  - 74
#define  r_DIDutyCh2     c_ModbusParams  - 75
#define  r_DIDutyCh3     c_ModbusParams  - 76
#define  r_DIDutyCh4     c_ModbusParams  - 77
#define  r_DIDutyCh5     c_ModbusParams  - 78
#define  r_DIDutyCh6     c_ModbusParams  - 79
#define  r_DIDutyCh7     c_ModbusParams  - 80
#define  r_DIDutyCh8     c_ModbusParams  - 81
#define  r_DIDutyCh9     c_ModbusParams  - 82
#define  r_DIDutyCh10    c_ModbusParams  - 83
#define  r_DIDutyCh11    c_ModbusParams  - 84
#define  r_DIDutyCh12    c_ModbusParams  - 85

// GET_DIPERDUR(n)  n - number of a channel. n = 1..4
// descr: get a duration period of a channel
// returning: unsigned long
#define GET_DIDUTY(n)                                              ( -2 * ((r_DIDutyCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

// GET_DIPERDUR(n)  n - number of a channel. n = 1..12
// descr: get an impulse count
// returning: unsigned long
//#define GET_DIIMPQTY(n)                                            ( -2 * ((r_DIImpQtyCh1) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_DIImpQtyCh1H   c_ModbusParams  - 86
#define  r_DIImpQtyCh1L   c_ModbusParams  - 87
#define  r_DIImpQtyCh2H   c_ModbusParams  - 88
#define  r_DIImpQtyCh2L   c_ModbusParams  - 89
#define  r_DIImpQtyCh3H   c_ModbusParams  - 90
#define  r_DIImpQtyCh3L   c_ModbusParams  - 91
#define  r_DIImpQtyCh4H   c_ModbusParams  - 92
#define  r_DIImpQtyCh4L   c_ModbusParams  - 93
#define  r_DIImpQtyCh5H   c_ModbusParams  - 94
#define  r_DIImpQtyCh5L   c_ModbusParams  - 95
#define  r_DIImpQtyCh6H   c_ModbusParams  - 96
#define  r_DIImpQtyCh6L   c_ModbusParams  - 97
#define  r_DIImpQtyCh7H   c_ModbusParams  - 98
#define  r_DIImpQtyCh7L   c_ModbusParams  - 99
#define  r_DIImpQtyCh8H   c_ModbusParams  - 100
#define  r_DIImpQtyCh8L   c_ModbusParams  - 101
#define  r_DIImpQtyCh9H   c_ModbusParams  - 102
#define  r_DIImpQtyCh9L   c_ModbusParams  - 103
#define  r_DIImpQtyCh10H  c_ModbusParams  - 104
#define  r_DIImpQtyCh10L  c_ModbusParams  - 105
#define  r_DIImpQtyCh11H  c_ModbusParams  - 106
#define  r_DIImpQtyCh11L  c_ModbusParams  - 107
#define  r_DIImpQtyCh12H  c_ModbusParams  - 108
#define  r_DIImpQtyCh12L  c_ModbusParams  - 109
// GET_DIPERDUR(n)  n - номер части канала. (потому что 2 регистра на одно значение)  n = 1..24
// descr: получить кол-во импульсов
// return: unsigned long
#define GET_DIIMPTQTY64(n)                                         ( -2 * ((r_DIImpQtyCh1H) - c_ModbusParams + 1) + (2 * (n - 1)) )

// ====================================================



//  === Config Reg [0x03/0x10]===

// --- Общая для всех плат часть ---
// all registers are 16-bit ones!
#define  r_FEId          end_adr_conf_reg16 - 1	// Модель прибора
#define GET_FEID                                                   MAKE_ABS( (r_FEId) - end_adr_conf_reg16 + 1 )

#define  r_Modif         end_adr_conf_reg16 - 2  // Модификация прибора, № точки исполнения
//  1772 - X1 - X2 - X3 - X4
//
//	X1:  [0] - Плата АЦП -----------------|
//																				X2: [1] -  4 входа
//																				X2: [2] -  8 входов
//
//			 [1] - Плата ЦАП -----------------|
//																				X2: [1] -  4 входа   0-20мА
//																				X2: [2] -  8 входов  0-20мА
//																				X2: [3] -  4 входа   0-10В
//																				X2: [4] -  8 входов  0-10В
//																				X2: [5] -  4+4 входа 0-20мА + 0-10В
//
//			 [2] - Плата Реле ----------------|
//																				X2: [1] -  8 реле контактные
//																				X2: [2] - 16 реле контактные 
//																				X2: [1] -  8 реле твердотельные
//																				X2: [2] - 16 реле твердотельные 
//																				X2: [2] - 24 реле твердотельные 
//
//			 [3] - Плата Дискр Вх./Вых. ------|
//																				X2: [1] -  4 входа
//																				X2: [2] -  8 входов
#define GET_MODIF                                                  MAKE_ABS( (r_Modif) - end_adr_conf_reg16 + 1 )

#define  r_SerialNumber  end_adr_conf_reg16 - 3	// Серийный номер в цикле одного года
#define GET_SN                                                     MAKE_ABS( (r_SerialNumber) - end_adr_conf_reg16 + 1 )

#define  r_ManufDate     end_adr_conf_reg16 - 4	// Дата выпуска прибора
#define GET_MANUFDATE                                              MAKE_ABS( (r_ManufDate) - end_adr_conf_reg16 + 1 )

#define  r_CheckDate     end_adr_conf_reg16 - 5  // Дата следующей поверки
// (Day << 11) + (Month << 7) + Year
#define GET_CHECKDATE                                              MAKE_ABS( (r_CheckDate) - end_adr_conf_reg16 + 1 )

#define  r_FEStatus      end_adr_conf_reg16 - 6
#define GET_FESTATUS                                               MAKE_ABS( (r_FEStatus) - end_adr_conf_reg16 + 1 )
			                        
#define  r_Address	     end_adr_conf_reg16 - 7  // Адрес прибора
#define GET_ADDRESS                                                MAKE_ABS( (r_Address) - end_adr_conf_reg16 + 1 )

#define  r_UARTparam     end_adr_conf_reg16 - 8  // Параметры UART
#define GET_UARTPARAM                                              MAKE_ABS( (r_UARTparam) - end_adr_conf_reg16 + 1 )

#define  r_Reserv1	     end_adr_conf_reg16 - 9	
#define  r_HSScale       end_adr_conf_reg16 - 10
#define GET_CJScale                                                MAKE_ABS( (r_HSScale) - end_adr_conf_reg16 + 1 )

#define  r_HardwareVer   end_adr_conf_reg16 - 11
#define GET_HARDWAREVER                                            MAKE_ABS( (r_HardwareVer) - end_adr_conf_reg16 + 1 )

#define  r_FirmwareVer   end_adr_conf_reg16 - 12	// Версия прошивки
#define GET_FIRMWAREVER                                            MAKE_ABS( (r_FirmwareVer) - end_adr_conf_reg16 + 1 )

#define  r_OutputParam   end_adr_conf_reg16 - 13
#define  r_TestCmd       end_adr_conf_reg16 - 14

#define  r_Calib         end_adr_conf_reg16 - 15  // Регистр для команд калибровок
#define GET_CALIBR                                                 MAKE_ABS( (r_Calib) - end_adr_conf_reg16 + 1 )

#define  r_Restore       end_adr_conf_reg16 - 16	// Возращение заводских установок
#define GET_RESTORER                                               MAKE_ABS( (r_Restore) - end_adr_conf_reg16 + 1 )

#define  r_Password      end_adr_conf_reg16 - 17
#define GET_PASSWORDR                                              MAKE_ABS( (r_Password) - end_adr_conf_reg16 + 1 )

#define  r_ReservPass    end_adr_conf_reg16 - 18

#define  r_Cmd       		 end_adr_conf_reg16 - 19
#define GET_SYSTCMDR                                               MAKE_ABS( (r_Cmd) - end_adr_conf_reg16 + 1 )

// --------------------------------------------------

// === [0x03/0x10] === Плата ADC ===
#define  r_BreakLinePeriod end_adr_conf_reg16 - 20
#define  r_ADChipType      end_adr_conf_reg16 - 21  // For ADC 0x00XX  1 - AD7794 (24bit), 0 - AD7795 (16bit)


// Режимы и диапазоны аналоговых каналов
#define  r_ADSingleConvEn  end_adr_conf_reg16 - 22 // Write 1 - Enable single conversion and disable continus conversion, If write 0 - Disable single conversion and enable continus conversion,
#define  r_ADSingleConv    end_adr_conf_reg16 - 23 // Write 1 - Start single conversion, If Read 1 - conversion is process, if read 0 - conversion complete  0x00XX, XX - bits of 8 channel

//  >> r_ADxMode <<
// === Work modes of measure channels ===
//
//                >>>  0 x  1  2  3  4  <<<
//                         /  /    \  \.
//     	           ___[]__/  /      \  \__[Range]___
//                          /        \.
//     ___[Measure Mode]___/          \___[Input Type]___

//  >> r_ADxSettings <<
// === Settings of measure channels ===
//
//                >>>  0 x  1  2  3  4  <<<
//                         /  /    \  \.
//     	           ___[]__/  /      \  \__[Measure Speed]___
//                          /        \.
//___[Break Line Detect]___/          \___[FIFO Buffer]___
//
//
// Measure Speed:
//    0 - None,  1 -   2мс,  2 - 	 4мс,  3 -   8мс,  4 -  16мс, 5 - 20мс, 6 - 24мс, 7 - 30мс, 8 - 50мс, 9 - 60мс (80дБ) DEFAULT FILTER, 10 - 60мс (65дБ),
//   11 - 80мс, 12 - 100мс, 13 - 120мс, 14 - 160мс, 15 - 240мс
//
//
// when you work with ADMode and ADSettings u r supposed to use these definitions (when calibrating don't forget to increase range as +1. Yes, I'm not a freak)
// channel ranges:
#define  CHANNEL_MODE_VOLTAGE_30mV         0
#define  CHANNEL_MODE_VOLTAGE_100mV        1
#define  CHANNEL_MODE_VOLTAGE_1V           2
#define  CHANNEL_MODE_VOLTAGE_10V          3
#define  CHANNEL_MODE_CURRENT_5mA          4
#define  CHANNEL_MODE_CURRENT_20mA         5
#define  CHANNEL_MODE_RESISTANCE_100Om4w   6
#define  CHANNEL_MODE_RESISTANCE_400Om4w   7
#define  CHANNEL_MODE_RESISTANCE_4000Om4w  8
#define  CHANNEL_MODE_RESISTANCE_100Om3w   9
#define  CHANNEL_MODE_RESISTANCE_400Om3w   10
#define  CHANNEL_MODE_RESISTANCE_4000Om3w  11
#define  CHANNEL_MODE_RESISTANCE_100Om2w   12
#define  CHANNEL_MODE_RESISTANCE_400Om2w   13
#define  CHANNEL_MODE_RESISTANCE_4000Om2w  14


#define  r_AD1Mode       end_adr_conf_reg16 - 24  // 0x1234  [1] -   [2] - Measure Mode  [3][4] - Input Type & Range
#define  r_AD1Settings   end_adr_conf_reg16 - 25  // 0x1234  [1][2] - Break Line Detect  [3] - FIFO Buffer [4] - Measure Speed
#define  r_AD2Mode       end_adr_conf_reg16 - 26
#define  r_AD2Settings   end_adr_conf_reg16 - 27
#define  r_AD3Mode       end_adr_conf_reg16 - 28
#define  r_AD3Settings   end_adr_conf_reg16 - 29
#define  r_AD4Mode       end_adr_conf_reg16 - 30
#define  r_AD4Settings   end_adr_conf_reg16 - 31
#define  r_AD5Mode       end_adr_conf_reg16 - 32
#define  r_AD5Settings   end_adr_conf_reg16 - 33
#define  r_AD6Mode       end_adr_conf_reg16 - 34
#define  r_AD6Settings   end_adr_conf_reg16 - 35
#define  r_AD7Mode       end_adr_conf_reg16 - 36
#define  r_AD7Settings   end_adr_conf_reg16 - 37
#define  r_AD8Mode       end_adr_conf_reg16 - 38
#define  r_AD8Settings   end_adr_conf_reg16 - 39
// GET_ADModeR
// descr: прочитать или выставить режим канала
// return: unsigned long
// FIXME: формула неверная (адресует верно только при n = 1)
#define GET_ADModeR(n)                                               ( -1 * ((r_AD1Mode) - end_adr_conf_reg16 + 1) + (2 * (n - 1)) )

// GET_ADSettingsR
// descr: прочитать или выставить настройка канала
// return: unsigned long
// FIXME: формула неверная (адресует верно только при n = 1)
#define GET_ADSettingsR(n)                                           ( -1 * ((r_AD1Settings) - end_adr_conf_reg16 + 1) + (2 * (n - 1)) )

// Калибровки аналоговых каналов
// Команда записи регистров платы в регистры АЦП выбранного канала
#define  cm_AD1ParamSave  0x0E901  
// ..........................
#define  cm_AD8ParamSave  0x0E908
// Команда чтения регистров платы в регистры АЦП выбранного канала
#define  cm_AD1ParamRead  0x0E911  
// ..........................
#define  cm_AD8ParamRead  0x0E918



//
#define  r_ADNum         end_adr_conf_reg16 - 40
#define  r_ADDiap        end_adr_conf_reg16 - 41
#define  r_ADZeroH       end_adr_conf_reg16 - 42
#define  r_ADZeroL       end_adr_conf_reg16 - 43
#define  r_ADScaleH      end_adr_conf_reg16 - 44
#define  r_ADScaleL      end_adr_conf_reg16 - 45

#define  r_ADRegConfig   end_adr_conf_reg16 - 46
#define  r_ADRegMode     end_adr_conf_reg16 - 47
#define  r_ADRegIDIO     end_adr_conf_reg16 - 48
#define  r_ADRegStatus   end_adr_conf_reg16 - 49
#define  r_ADRegDataH    end_adr_conf_reg16 - 50
#define  r_ADRegDataL    end_adr_conf_reg16 - 51
#define  r_ADRegOffsetH  end_adr_conf_reg16 - 52
#define  r_ADRegOffsetL  end_adr_conf_reg16 - 53
#define  r_ADRegScaleH   end_adr_conf_reg16 - 54
#define  r_ADRegScaleL   end_adr_conf_reg16 - 55


// Резервные регистры. Под нужды 1772_data_emulator
#define  r_Debug1	         end_adr_conf_reg16 - 56
#define  r_Debug2	         end_adr_conf_reg16 - 57
#define  r_Debug3	         end_adr_conf_reg16 - 58
#define  r_Debug4	         end_adr_conf_reg16 - 59
#define  r_Debug5	         end_adr_conf_reg16 - 60
#define  r_Debug6	         end_adr_conf_reg16 - 61
#define  r_Debug7	         end_adr_conf_reg16 - 62
#define  r_Debug8	         end_adr_conf_reg16 - 63
#define GET_DEBUG_ERROR_CODES(n)                                               ( -2 * ((r_Debug1) - end_adr_conf_reg16 + 1) + (2 * (n - 1)) )
#define  r_Debug9	         end_adr_conf_reg16 - 64
#define  r_Debug10         end_adr_conf_reg16 - 65




// === [0x03/0x10] === Плата DAC ===
#define  r_DACh1H        end_adr_conf_reg16 - 24
#define  r_DACh1L        end_adr_conf_reg16 - 25
#define  r_DACh2H        end_adr_conf_reg16 - 26
#define  r_DACh2L        end_adr_conf_reg16 - 27
#define  r_DACh3H        end_adr_conf_reg16 - 28
#define  r_DACh3L        end_adr_conf_reg16 - 29
#define  r_DACh4H        end_adr_conf_reg16 - 30
#define  r_DACh4L        end_adr_conf_reg16 - 31
#define  r_DACh5H        end_adr_conf_reg16 - 32
#define  r_DACh5L        end_adr_conf_reg16 - 33
#define  r_DACh6H        end_adr_conf_reg16 - 34
#define  r_DACh6L        end_adr_conf_reg16 - 35
#define  r_DACh7H        end_adr_conf_reg16 - 36
#define  r_DACh7L        end_adr_conf_reg16 - 37
#define  r_DACh8H        end_adr_conf_reg16 - 38
#define  r_DACh8L        end_adr_conf_reg16 - 39
// GET_DACHR
// descr: получить значение части канала аналогового выхода
// return: unsigned short (16-bit) (из двух регистров значение)
// FIXME: расчет неверный регистра если n != 1
#define GET_DACHR(n)                                             MAKE_ABS((r_DACh1H) - end_adr_conf_reg16 + 1 - (n - 1) )

#define  r_DANum         end_adr_conf_reg16 - 50
#define  r_DAZero        end_adr_conf_reg16 - 51
#define  r_DAScale       end_adr_conf_reg16 - 52

// Адреса с 55 по 64 зарезервированы
//#define  r_Debug1	         end_adr_conf_reg16 - 56
//#define  r_Debug2	         end_adr_conf_reg16 - 57
//#define  r_Debug3	         end_adr_conf_reg16 - 58
//#define  r_Debug4	         end_adr_conf_reg16 - 59
//#define  r_Debug5	         end_adr_conf_reg16 - 60
//#define  r_Debug6	         end_adr_conf_reg16 - 61
//#define  r_Debug7	         end_adr_conf_reg16 - 62
//#define  r_Debug8	         end_adr_conf_reg16 - 63
//#define  r_Debug9	         end_adr_conf_reg16 - 64
//#define  r_Debug10         end_adr_conf_reg16 - 65

// === [0x03/0x10] === Плата Реле ===
#define  r_Rele		       end_adr_conf_reg16 - 24
// GET_RELAYR
// descr: выставление значения реле (битовая маска)
// return: unsigned long (реле бывает: 8, 16, 24 штук на плате)
#define GET_RELAYR                                               MAKE_ABS((r_Rele) - end_adr_conf_reg16 + 1 )

// Адреса с 55 по 64 зарезервированы
//#define  r_Debug1	         end_adr_conf_reg16 - 56
//#define  r_Debug2	         end_adr_conf_reg16 - 57
//#define  r_Debug3	         end_adr_conf_reg16 - 58
//#define  r_Debug4	         end_adr_conf_reg16 - 59
//#define  r_Debug5	         end_adr_conf_reg16 - 60
//#define  r_Debug6	         end_adr_conf_reg16 - 61
//#define  r_Debug7	         end_adr_conf_reg16 - 62
//#define  r_Debug8	         end_adr_conf_reg16 - 63
//#define  r_Debug9	         end_adr_conf_reg16 - 64
//#define  r_Debug10         end_adr_conf_reg16 - 65



// === [0x03/0x10] === Плата дискр вх./вых. === 
#define  r_DInputTypeH   			end_adr_conf_reg16  - 20
#define  r_DInputTypeL   			end_adr_conf_reg16  - 21
#define  r_DIFreq	  	   			end_adr_conf_reg16  - 22
#define  r_DIDuty		 	   			end_adr_conf_reg16  - 23

// Режимы. (2 бита на каждый канал)
// 00 - выключен
// 01 - фронт
// 10 - спад
// 11 - фронт и спад
#define  r_DIImpTrigType1			end_adr_conf_reg16  - 24    // b0000FFEEDDCCBBAA  AA = Ch1Trig; BB = Ch2Trig; CC = Ch3Trig; DD = Ch4Trig;  EE = Ch5Trig;  FF = Ch6Trig
// GET_DI_PULSE_SIGNAL_MODE_1_6
// descr: получить/установить режим работы дискретного входа. работа по фронту, по спаду или по обоим частям сигнала
//        в этом регистре по 2 бита на каждый канал. Настраиваются только с 1 по 6
// return: uint16_t
#define GET_DI_PULSE_SIGNAL_MODE_1_6                                ( -1 * ((r_DIImpTrigType1) - end_adr_conf_reg16 + 1) )

#define  r_DIImpTrigType2			end_adr_conf_reg16  - 25    // b0000LLKKJJIIHHGG  GG = Ch7Trig; HH = Ch8Trig; II = Ch9Trig; JJ = Ch10Trig; KK = Ch11Trig; LL = Ch12Trig
// GET_DI_PULSE_SIGNAL_MODE_7_12
// descr: получить/установить режим работы дискретного входа. работа по фронту, по спаду или по обоим частям сигнала
//        в этом регистре по 2 бита на каждый канал. Настраиваются только с 7 по 12
// return: uint16_t
#define GET_DI_PULSE_SIGNAL_MODE_7_12                               ( -1 * ((r_DIImpTrigType2) - end_adr_conf_reg16 + 1) )

#define  r_DIImpFilterType		end_adr_conf_reg16  - 26    // b0000CBA987654321  b0000xxxxxxxxxxxx   if x = 0  {FilterM=FilterP= XXX ms};   if x = 1  {FilterP= XXX ms  FilterM= YYY ms}
#define GET_DI_FILTER_TYPE                                          ( -1 * ((r_DIImpFilterType) - end_adr_conf_reg16 + 1) )

#define  r_DIImpFilterPCh1		end_adr_conf_reg16  - 27    // Фильтр положительного импульса 10 ... 60000 ms
#define  r_DIImpFilterPCh2		end_adr_conf_reg16  - 28
#define  r_DIImpFilterPCh3		end_adr_conf_reg16  - 29
#define  r_DIImpFilterPCh4		end_adr_conf_reg16  - 30
#define  r_DIImpFilterPCh5		end_adr_conf_reg16  - 31
#define  r_DIImpFilterPCh6		end_adr_conf_reg16  - 32
#define  r_DIImpFilterPCh7		end_adr_conf_reg16  - 33
#define  r_DIImpFilterPCh8		end_adr_conf_reg16  - 34
#define  r_DIImpFilterPCh9		end_adr_conf_reg16  - 35
#define  r_DIImpFilterPCh10	 	end_adr_conf_reg16  - 36
#define  r_DIImpFilterPCh11	 	end_adr_conf_reg16  - 37
#define  r_DIImpFilterPCh12	 	end_adr_conf_reg16  - 38
// GET_DI_FILTER_POSITIVE(n)     n - номер 1..12
// descr: получить/установить значение фильтра в мс (фронт)
// return: uint16_t
#define GET_DI_FILTER_POSITIVE(n)                                   ( -1 * ((r_DIImpFilterPCh1) - end_adr_conf_reg16 + 1) + (1 * (n - 1)) )

#define  r_DIImpFilterMCh1		end_adr_conf_reg16  - 39    // Фильтр отрицательного импульса 10 ... 60000 ms
#define  r_DIImpFilterMCh2		end_adr_conf_reg16  - 40
#define  r_DIImpFilterMCh3		end_adr_conf_reg16  - 41
#define  r_DIImpFilterMCh4		end_adr_conf_reg16  - 42
#define  r_DIImpFilterMCh5		end_adr_conf_reg16  - 43
#define  r_DIImpFilterMCh6		end_adr_conf_reg16  - 44
#define  r_DIImpFilterMCh7		end_adr_conf_reg16  - 45
#define  r_DIImpFilterMCh8		end_adr_conf_reg16  - 46
#define  r_DIImpFilterMCh9		end_adr_conf_reg16  - 47
#define  r_DIImpFilterMCh10	 	end_adr_conf_reg16  - 48
#define  r_DIImpFilterMCh11	 	end_adr_conf_reg16  - 49
#define  r_DIImpFilterMCh12	 	end_adr_conf_reg16  - 50
// GET_DI_FILTER_NEGATIVE(n)     n - номер 1..12
// descr: получить/установить значение фильтра в мс (спад)
// return: uint16_t
#define GET_DI_FILTER_NEGATIVE(n)                                   ( -1 * ((r_DIImpFilterMCh1) - end_adr_conf_reg16 + 1) + (1 * (n - 1)) )


#define  r_DIImpEnable			 		end_adr_conf_reg16  - 51
#define  r_DIImpEdgeLevel		 		end_adr_conf_reg16  - 52
#define  r_DIImpFrontRear			 	end_adr_conf_reg16  - 53
#define  r_DIImpBothFrontRear	 	end_adr_conf_reg16  - 54

// Адреса с 55 по 64 зарезервированы
//#define  r_Debug1	         end_adr_conf_reg16 - 56
//#define  r_Debug2	         end_adr_conf_reg16 - 57
//#define  r_Debug3	         end_adr_conf_reg16 - 58
//#define  r_Debug4	         end_adr_conf_reg16 - 59
//#define  r_Debug5	         end_adr_conf_reg16 - 60
//#define  r_Debug6	         end_adr_conf_reg16 - 61
//#define  r_Debug7	         end_adr_conf_reg16 - 62
//#define  r_Debug8	         end_adr_conf_reg16 - 63
//#define  r_Debug9	         end_adr_conf_reg16 - 64
//#define  r_Debug10         end_adr_conf_reg16 - 65
// ====================================================




#define  r_SystCommands  254  // Регистр скрытых системных комманд
						   

// === Команды калибровки измерительных трактов ===
//
//                     0 x  1  2  3  4
//                         /  /    \  \.
//   ___[Process Status]__/  /      \  \__[Ch number]___
//                          /        \.
//     ___[Command type]___/          \___[Diapozone]___
//

// *** Маски команд *****************************
#define  cm_TestInput_Mask  							0x0E000
#define  cm_TestInputProcess_Mask					0x0D000
#define  cm_ZSReset_Mask			 					 	0x0E100
#define  cm_ZSCopy_Mask			   					 	0x0E200
#define  cm_CalibZero_Mask						  	0x0E300
#define  cm_CalibScale_Mask								0x0E400
#define  cm_CalibHalfScale_Mask						0x0E500
#define  cm_CalibDAC_Mask 						 		0x0E500
#define  cm_Correct3w2w_Mask							0x0E700
//#define  cm_CalibTestRefZero_Mask					0x0E800
//#define  cm_CalibTestRefZeroProcess_Mask	0x0D800
#define  cm_CalibHS_Mask									0x0EA00
#define  cm_Correct10V_Mask								0x0EB00
// **********************************************



// *** Reset Zero & Scale ****************
#define  cm_ZSReset_D01V_ChAll  0x0E110
#define  cm_ZSReset_D003V_Ch1   0x0E111
#define  cm_ZSReset_D01V_Ch1    0x0E121
// .........
#define  cm_ZSReset_D4k_Ch8     0x0E1F8
// ***************************************

// *** Copy Zero & Scale to all other channel ****
#define  cm_ZSCopy_D01V_ChAll  0x0E210
#define  cm_ZSCopy_D01V_Ch1    0x0E211
#define  cm_ZSCopy_D01V_Ch1    0x0E211
// .........
#define  cm_ZSCopy_D4k_Ch8     0x0E2F8
// ************************************************


// *** Calibrate Zero ********************
#define  cm_CalibZero_D01V_ChAll  0x0E310
#define  cm_CalibZero_D003V_Ch1   0x0E311
#define  cm_CalibZero_D01V_Ch1    0x0E321
// .........
#define  cm_CalibZero_D4k_Ch8     0x0E3F8
// ***************************************

// *** Calibrate Scale *******************
#define  cm_CalibScale_D01V_ChAll 0x0E410
#define  cm_CalibScale_D003V_Ch1  0x0E411
#define  cm_CalibScale_D01V_Ch1   0x0E421
// .........
#define  cm_CalibScale_D4k_Ch8    0x0E4F8
// ***************************************

// *** Correction Line Zero ********************
// 3-x проводная
#define  cm_Calib100ohm3w_Ch1	   0x0E7C1
#define  cm_Calib400ohm3w_Ch1    0x0E7C1
#define  cm_Calib4000ohm3w_Ch1   0x0E7C1
//  ........
//  ........
#define  cm_Calib100ohm3w_Ch8	   0x0E7C8
#define  cm_Calib400ohm3w_Ch8	   0x0E7C8
#define  cm_Calib4000ohm3w_Ch8   0x0E7C8

#define  cm_Calib100ohm2w_Ch1	   0x0E7D1
#define  cm_Calib400ohm2w_Ch1    0x0E7D1
#define  cm_Calib4000ohm2w_Ch1   0x0E7D1
//  ........
//  ........
#define  cm_Calib100ohm2w_Ch8	   0x0E7D8
#define  cm_Calib400ohm2w_Ch8	   0x0E7D8
#define  cm_Calib4000ohm2w_Ch8   0x0E7D8
// ***************************************

// *** Correction Line Zero ********************

#define  cm_CalibTestRefZero_ChAll	 0x0E800
#define  cm_CalibTestRefZero_Ch1 		 0x0E801
// ......
// ......
#define  cm_CalibTestRefZero_Ch8 		 0x0E808
// ***************************************

// *** Correction Line Zero ********************
#define  cm_CalibHS							 		 0x0EA00
// ***************************************


// ===============================================
//  Формат команды  0 x 0  E  0  x  x
//										     ----  -  -
//                          /     \  \.
//                         /       \  \___Номер канала__
//                        /         \.
//         ___Команда ___/           \___Тип теста_____
//

#define  cm_TestInputAll_ChAll  0x0E000 // Тестирование входных элементов 1234 канал 12345678
#define  cm_TestInput1_ChAll	  0x0E010 // Тестирование входных элементов 1    канал 12345678
#define  cm_TestInput2_ChAll  	0x0E020 // Тестирование входных элементов 2    канал 12345678
#define  cm_TestInput3_ChAll 	 	0x0E030 // Тестирование входных элементов 3		 канал 12345678
#define  cm_TestInput4_ChAll 		0x0E040 // Тестирование входных элементов 4    канал 12345678

#define  cm_TestInputAll_Ch1  	0x0E001 // Тестирование входных элементов 1234 канал 1
#define  cm_TestInput1_Ch1	  	0x0E011 // Тестирование входных элементов 1    канал 1
#define  cm_TestInput2_Ch1	  	0x0E021 // Тестирование входных элементов 2    канал 1
#define  cm_TestInput3_Ch1	  	0x0E031 // Тестирование входных элементов 3    канал 1
#define  cm_TestInput4_Ch1	  	0x0E041 // Тестирование входных элементов 4    канал 1
//				......
//				......
//				......
#define  cm_TestInputAll_Ch8  	0x0E008 // Тестирование входных элементов 1234 канал 8
#define  cm_TestInput1_Ch8	  	0x0E018 // Тестирование входных элементов 1    канал 8
#define  cm_TestInput2_Ch8	  	0x0E028 // Тестирование входных элементов 2    канал 8
#define  cm_TestInput3_Ch8	  	0x0E038 // Тестирование входных элементов 3    канал 8
#define  cm_TestInput4_Ch8	  	0x0E048 // Тестирование входных элементов 4    канал 8


// Коды ошибок при проведении калибровки плат АЦП
#define  err_ADCCalibOk   		 0x01000   // Команда + код  (0x0E310 + 0x1000 = 0x0F310) калибровка нуля АЦП произведена корректно
#define  err_ADCInProcess  		 0x01000   // Команда - код  (0x0E310 - 0x1000 = 0x0D310) калибровка в процессе
#define  err_ADCNoCh      		 0x02000   // Команда - код  (0x0E310 - 0x2000 = 0x0С310) калибровка нуля АЦП не произведена, нет такого канала
#define  err_ADCNoDiap    		 0x03000   // Команда - код  (0x0E310 - 0x3000 = 0x0B310) калибровка нуля АЦП не произведена, нет такого диапазона
#define  err_ADCDiapNotEqu 		 0x04000   // Команда - код  (0x0E310 - 0x4000 = 0x0A310) калибровка нуля АЦП не произведена, нет такого диапазона
#define  err_ADCCalibOverNorm  0x05000   // Команда - код  (0x0E310 - 0x5000 = 0x09310) калибровка нуля АЦП не произведена, некорректный входной сигнал
#define  err_ADCZeroCalibNotOk 0x06000   // Команда - код  (0x0E310 - 0x6000 = 0x08310) калибровка нуля АЦП не произведена, нет такого диапазона
#define  err_ADCNoHalfDiap 		 0x07000   // Команда - код  (0x0E310 - 0x7000 = 0x07310) калибровка нуля АЦП не произведена, нет такого диапазона

// =======================================================================================


// === Команды калибровки токовых интерфейсов ===
//
//                     0 x  1  2  3  4
//                          -  -  -  -
//                         /  /    \  \.
//             ___Статус__/  /      \  \__Номер канала___
//                          /        \.
//            ___Команда___/          \___Подкоманда___
//

#define  cm_CalibDACZero_Ch1 		 0x0E501
#define  cm_CalibDACScale_Ch1    0x0E511
#define  cm_CalibDACEnd_Ch1			 0x0E521
#define  cm_CalibDACPlus_Ch1 		 0x0E531
#define  cm_CalibDACMinus_Ch1		 0x0E541
#define  cm_CalibDACReset_Ch1		 0x0E551
#define  cm_CalibDACCancel_Ch1	 0x0E561
// ..................
// ...............               0x0E50x
// ............										 ...
// .........										 0x0E55x
// ......
// ...
#define  cm_CalibDACZero_Ch8 		 0x0E508
#define  cm_CalibDACScale_Ch8    0x0E518
#define  cm_CalibDACEnd_Ch8			 0x0E528
#define  cm_CalibDACPlus_Ch8 		 0x0E538
#define  cm_CalibDACMinus_Ch8		 0x0E548
#define  cm_CalibDACReset_Ch8		 0x0E558
#define  cm_CalibDACCancel_Ch8	 0x0E568


#define  c_DACZeroDelta       50
#define  c_DACScaleDelta      50


#define  cm_rAllDefault         0x0E630 // 0. Загрузка всех установок по-умолчанию в рабочие
#define  cm_rAllManufToW        0x0E631 // 1. Загрузка всех заводских установок в рабочие
							    
#define  cm_rZSCalibDefault     0x0E632 // 2. Загрузка калибровок нулей и шкал по-умолчанию в рабочие
#define  cm_rZSCalibManufToW    0x0E633 // 3. Загрузка заводских калибровок нулей и шкал в рабочие
	  
#define  cm_rDACCalibDefault    0x0E634 // 4. Загрузка калибровок нулей и шкал токовых выходов по-умолчанию в рабочие
#define  cm_rDACCalibManufToW   0x0E635 // 5. Загрузка заводских калибровок нулей и шкал токовых выходов в рабочие

#define  cm_rLastCalibZToW      0x0E636 // 6. Вернуть последние калибровки нулей в рабочие
#define  cm_rLastCalibSToW      0x0E637 // 7. Вернуть последние калибровки шкал в рабочие
#define  cm_rLastCalibDAC1      0x0E638 // 8. Вернуть последние калибровки нуля и шкалы 1-го токового выхода в рабочие
#define  cm_rLastCalibDAC2      0x0E639 // 9. Вернуть последние калибровки нуля и шкалы 2-го токового выхода в рабочие

#define  cm_rRegMapDefault      0x0E63A //10. Загрузка карты переадрессации регистров по-умолчанию в рабочую	
#define  cm_rRegMapManufToW     0x0E63B //11. Загрузка заводской карты переадрессации регистров в рабочую

#define  cm_rAllMWDefault       0x0E63C //12. Загрузка всех установок по-умолчанию в заводские и рабочие
#define  cm_rZSCalibMWDefault   0x0E63D //13. Загрузка калибровок нулей и шкал в заводские и рабочие
#define  cm_rDACCalibMWDefault  0x0E63E //14. Загрузка калибровок нулей и шкал токовых выходов в заводские и рабочие
#define  cm_rRegMapMWDefault    0x0E63F //15. Загрузка карты переадрессации регистров по-умолчанию в заводские и рабочие


//===== r_Cmd ====================
#define  cm_DIResetChAll			  0x0E10F // 
#define  cm_DIResetCh1         	0x0E101 //
#define  cm_DIResetCh2         	0x0E102 //
#define  cm_DIResetCh3         	0x0E103 //
#define  cm_DIResetCh4         	0x0E104 //
#define  cm_DIResetCh5         	0x0E105 //
#define  cm_DIResetCh6         	0x0E106 //
#define  cm_DIResetCh7         	0x0E107 //
#define  cm_DIResetCh8         	0x0E108 //
#define  cm_DIResetCh9         	0x0E109 //
#define  cm_DIResetCh10         0x0E10A //
#define  cm_DIResetCh11         0x0E10B //
#define  cm_DIResetCh12         0x0E10C //

#define  cm_DIResetCh_Mask		 	0x0E100

