
// Last Ver. 0.0.1.96
// Prev Ver. 0.0.1.96


// ������ ��������� ������������

#define   c_ModbusParams  65//78//36 //26
#define   c_ModbusConfig  54 //45//172 //140 //105 //73
#define   c_ModbusCoil    16
#define   c_ModbusLogic   2

//��� ��������� Modbus RTU
#define start_adr_izm_reg16   	0	    //����� ������ ����� ���������� ���������� (� 16 ������ ���������)
#define   end_adr_izm_reg16   	(c_ModbusParams)*2	//����� ����� ����� ���������� ����������  (� 16 ������ ���������) ������ ���� ������ 2-�
#define start_adr_conf_reg16   	0	//����� ������ ����� ���������� ����(� 16 ������ ���������)
#define   end_adr_conf_reg16   	c_ModbusConfig	//����� ����� ����� ���������� ���� (� 16 ������ ���������)
#define start_adr_coil_reg16  	0	//����� ������ ����� ���������� ����(� 16 ������ ���������)
#define   end_adr_coil_reg16 	  c_ModbusCoil	//����� ����� ����� ���������� ���� (� 16 ������ ���������)
#define start_adr_logic_reg16   0	//����� ������ ����� ���������� ����(� 16 ������ ���������)
#define   end_adr_logic_reg16   c_ModbusLogic	//����� ����� ����� ���������� ���� (� 16 ������ ���������)



// === Coil Reg [0x01/0x05]===

// For Rele Pcb
#define  r_ReleRaw       end_adr_coil_reg16  - 1

// For Logic Input/output Pcb
#define  r_LogicInRaw    end_adr_coil_reg16  - 1
#define  r_LogicOutRaw   end_adr_coil_reg16  - 2


#define MAKE_ABS(a)      (a < 0 ? -(a) : a)

// === Measure Reg [0x04] === ����� ADC ===

#define  r_ADCh1         c_ModbusParams  - 1
#define  r_ADCh2         c_ModbusParams  - 2
#define  r_ADCh3         c_ModbusParams  - 3
#define  r_ADCh4         c_ModbusParams  - 4
#define  r_ADCh5         c_ModbusParams  - 5
#define  r_ADCh6         c_ModbusParams  - 6
#define  r_ADCh7         c_ModbusParams  - 7
#define  r_ADCh8         c_ModbusParams  - 8
// this is a definition of a special macros that helps to address registers properly. This approach you will see through the whole file. Done.
// GET_ADCh(n)    n = 1..8
// descr: get a float value of an analog input channel
// returning: float
#define GET_ADCh(n)                                               MAKE_ABS( (r_ADCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_LastReadTimeCh12   c_ModbusParams  - 9
#define  r_LastReadTimeCh34   c_ModbusParams  - 10
#define  r_LastReadTimeCh56   c_ModbusParams  - 11
#define  r_LastReadTimeCh78   c_ModbusParams  - 12
// GET_LASTREADTIMECH(n)   n - number of pair, n = 1..4
// descr: get the last read time of a channel. (It might be useful for synchronization dealing and etc)
// returning: unsigned long (that you have to parse as a pair of short values)
#define GET_LASTREADTIMECH(n)                                     ( -2 * ((r_LastReadTimeCh12) - c_ModbusParams + 1) + (2 * (n - 1)) )

#define  r_TempHS		     c_ModbusParams  - 13
// GET_COLDJUNCTION_TEMP(n)    n - number of junction, this time it must be equal to one only. n = 1
// descr: get a cold junction temperature
// returning: float
#define GET_COLDJUNCTION_TEMP(n)                                  ( -2 * ((r_TempHS) - c_ModbusParams + 1) - (2 * (n - 1)) )

#define  r_ADTempCh1     c_ModbusParams  - 14
#define  r_ADTempCh2     c_ModbusParams  - 15
#define  r_ADTempCh3     c_ModbusParams  - 16
#define  r_ADTempCh4     c_ModbusParams  - 17
#define  r_ADTempCh5     c_ModbusParams  - 18
#define  r_ADTempCh6     c_ModbusParams  - 19
#define  r_ADTempCh7     c_ModbusParams  - 20
#define  r_ADTempCh8     c_ModbusParams  - 21
// GET_ADTEMPCH(n)  n - number of a channel. n = 1..8
// descr: get a temperature of a channel
// returning: float
#define GET_ADTEMPCH(n)                                           MAKE_ABS( (r_ADTempCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_ADUsuppCh1     c_ModbusParams  - 22
#define  r_ADUsuppCh2     c_ModbusParams  - 23
#define  r_ADUsuppCh3     c_ModbusParams  - 24
#define  r_ADUsuppCh4     c_ModbusParams  - 25
#define  r_ADUsuppCh5     c_ModbusParams  - 26
#define  r_ADUsuppCh6     c_ModbusParams  - 27
#define  r_ADUsuppCh7     c_ModbusParams  - 28
#define  r_ADUsuppCh8     c_ModbusParams  - 29
// GET_ADVOLTAGESUPP(n)  n - number of a channel. n = 1..8
// descr: get a voltage supply of a channel
// returning: float?
#define GET_ADVOLTAGESUPP(n)                                      MAKE_ABS( (r_ADUsuppCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_AllCurSupply		c_ModbusParams  - 30
// GET_ALLCURSUPPLY(n)   n - number that must be equal to one only at this time. n = 1
// descr: get the whole current supply
// returning: float?
#define GET_ALLCURSUPPLY(n)                                       MAKE_ABS( (r_AllCurSupply) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_ADCType			  c_ModbusParams  - 31  //  0x00XX   1 - AD7795 (16-bit), 0 - AD7794 (24-bit)
// GET_ADCTYPE     no params
// descr: get a type of ADC
// returning: float?
#define GET_ADCTYPE                                               MAKE_ABS( (r_ADCType) - c_ModbusParams + 1 )

#define  r_ADStatus		    c_ModbusParams  - 32  //  0x00XX   1 - Event, 0 - No event         Addr 0x3C - 0x3D
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
																									 
// GET_ADSTATUS   no params
// descr: get a status of a system
// returning: float?
#define GET_ADSTATUS                                              MAKE_ABS( (r_ADStatus) - c_ModbusParams + 1 )

#define  r_AD_BreakLine   		 c_ModbusParams  - 33                           				//        Addr 0x3E - 0x3F
//r_ADBreakLine		//  0x0000000XX   1 - Input Line is break,				 0 - Input Line is OK

#define  r_AD_DownDiap_UpDiap_DownNom_UpNom   c_ModbusParams  - 34                           				//        Addr 0x40 - 0x41
//UpNom			//  0x0000000XX   1 - Input Line is below min, 0 - Input Line is OK
//DownNom		//  0x00000XX00   1 - Input Line is over max,  0 - Input Line is OK
//UpDiap		//  0x000XX0000   1 - Input Line is overload,  0 - Input Line is OK
//DownDiap	//  0x0XX000000   1 - Input Line is over max,  0 - Input Line is OK

#define  r_AD_Overload   c_ModbusParams  - 35                           				//        Addr 0x40 - 0x41
//r_ADOvl					//  0x0000000XX   1 - Input Line is below min, 0 - Input Line is OK
//r_ADScaleOvl		//  0x00000XX00   1 - Input Line is below min, 0 - Input Line is OK

#define  r_AD_Connect  c_ModbusParams  - 36														//        Addr 0x42 - 0x43
//r_ADConnect1Error //  0x0000000XX   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect2Error //  0x00000XX00   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect3Error //  0x000XX0000   1 - ADC Connect Error, 0 - ADC connection is OK
//r_ADConnect4Error //  0x0XX000000   1 - ADC Connect Error, 0 - ADC connection is OK

#define  r_AD_BadReInit  c_ModbusParams  - 37
//r_AD_BadReInit1 //  0x0000000XX   1 - ADC ReInit, 0 - ADC not ReInit
//r_AD_BadReInit2 //  0x00000XX00   1 - ADC Bad ReInit, 0 - ADC ReInit is OK


#define  r_AD_Commmon_ChCrash  c_ModbusParams  - 38														//        Addr 0x42 - 0x43
//r_ADChCrash			 //  0x0000000XX   1 - ADC Crash, 				0 - ADC is OK
//r_ADCommonError	 //  0x00000XX00   1 - ADC error, 				0 - ADC is OK

#define  r_AD_TempOMMCom_TempOvlCh_TempMaxCh_TempMinCh	c_ModbusParams  - 39					//        Addr 0x44 - 0x45
//r_ADTempMaximumChannel    //  0x0000000XX   1 - Temp error, 	  0 - Temp is OK
//r_ADTempOverloadChannel   //  0x00000XX00   1 - Temp error, 	  0 - Temp is OK
//r_ADTempMinOvlErrCommon   //  0x000XX0000   1 - Temp error, 	  0 - Temp is OK

#define  r_AD_SuppOMMCom_SuppOvlCh_SuppMaxCh_SuppMinCh	c_ModbusParams  - 40					//        Addr 0x46 - 0x47
//r_ADSupplyMinimalChannel  //  0x0000000XX   1 - Supply error, 	0 - supply is OK
//r_ADSupplyMaximumChannel  //  0x00000XX00   1 - Supply error, 	0 - supply is OK
//r_ADSupplyOverloadChannel //  0x000XX0000   1 - Supply error, 	0 - supply is OK
//r_ADSupplyMinMaxOvlCommon //  0x0XX000000   1 - Supply error, 	0 - supply is OK

#define  r_AD_ExternalSensorInfo                        c_ModbusParams  - 41					//        Addr 0x48 - 0x49

#define  r_AD_InputElementCrash1                        c_ModbusParams  - 42					//        Addr 0x50 - 0x51
//r_AD_InputElementCrash_1  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_2 	//  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_3	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_4	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash2                        c_ModbusParams  - 43					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_5  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_6 	//  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_7	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_8	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash3                        c_ModbusParams  - 44					//        Addr 0x50 - 0x51
//r_AD_InputElementCrash_9  //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_10 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_11	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_12	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_InputElementCrash4                        c_ModbusParams  - 45					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_13 //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_14 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_15	//  0x000XX0000   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_16	//  0x0XX000000   1 - ElementCrash, 	0 - Element is OK
#define  r_AD_BadCalib						                      c_ModbusParams  - 46					//        Addr 0x52 - 0x53
//r_AD_InputElementCrash_13 //  0x0000000XX   1 - ElementCrash, 	0 - Element is OK
//r_AD_InputElementCrash_14 //  0x00000XX00   1 - ElementCrash, 	0 - Element is OK

// === Measure Reg [0x04] === ����� DAC ===

// === Measure Reg [0x04] === ����� ���� ===

// === Measure Reg [0x04] === ����� ����� ��./���. === 
#define  r_DInput      c_ModbusParams  - 1
// GET_DINPUT
// descr: get logic inputs
// returning: unsigned long (a bit mask!)
#define GET_DINPUT                                                MAKE_ABS( (r_DInput) - c_ModbusParams + 1 )

#define  r_DIFreqCh1   c_ModbusParams  - 2
#define  r_DIFreqCh2   c_ModbusParams  - 3
#define  r_DIFreqCh3   c_ModbusParams  - 4
#define  r_DIFreqCh4   c_ModbusParams  - 5
// GET_DIFREQ(n)    n - number of channel. n = 1..4
// descr: get a frequence of a channel
// returning: unsigned long
#define GET_DIFREQ(n)                                              MAKE_ABS( (r_DIFreqCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIPerDurCh1   c_ModbusParams  - 6
#define  r_DIPerDurCh2   c_ModbusParams  - 7
#define  r_DIPerDurCh3   c_ModbusParams  - 8
#define  r_DIPerDurCh4   c_ModbusParams  - 9
// GET_DIPERDUR(n)  n - number of a channel. n = 1..4
// descr: get a duration period of a channel
// returning: unsigned long
#define GET_DIPERDUR(n)                                            MAKE_ABS( (r_DIPerDurCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIPosDurCh1   c_ModbusParams  - 10
#define  r_DIPosDurCh2   c_ModbusParams  - 11
#define  r_DIPosDurCh3   c_ModbusParams  - 12
#define  r_DIPosDurCh4   c_ModbusParams  - 13
// GET_DIPOSDUR(n)  n - number of a channel. n = 1..4
// descr: get a positive duration of a channel
// returning: unsigned long
#define GET_DIPOSDUR(n)                                            MAKE_ABS( (r_DIPosDurCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DINegDurCh1   c_ModbusParams  - 14
#define  r_DINegDurCh2   c_ModbusParams  - 15
#define  r_DINegDurCh3   c_ModbusParams  - 16
#define  r_DINegDurCh4   c_ModbusParams  - 17
// GET_DINEGDUR(n)  n - number of a channel. n = 1..4
// descr: get a negative period of a channel
// returning: unsigned long
#define GET_DINEGDUR(n)                                            MAKE_ABS( (r_DINegDurCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIPosDurPPCh1   c_ModbusParams  - 18
#define  r_DIPosDurPPCh2   c_ModbusParams  - 19
#define  r_DIPosDurPPCh3   c_ModbusParams  - 20
#define  r_DIPosDurPPCh4   c_ModbusParams  - 21
// GET_DIPOSDURPP(n)  n - number of a channel. n = 1..4
// descr: get a positive period PP of a channel
// returning: unsigned long
#define GET_DIPOSDURPP(n)                                          MAKE_ABS( (r_DIPosDurPPCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DINegDurPPCh1   c_ModbusParams  - 22
#define  r_DINegDurPPCh2   c_ModbusParams  - 23
#define  r_DINegDurPPCh3   c_ModbusParams  - 24
#define  r_DINegDurPPCh4   c_ModbusParams  - 25
// GET_DINEGDURPP(n)  n - number of a channel. n = 1..4
// descr: get a negative duration period PP of a channel
// returning: unsigned long
#define GET_DINEGDURPP(n)                                          MAKE_ABS( (r_DINegDurPPCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIDutyCh1     c_ModbusParams  - 26
#define  r_DIDutyCh2     c_ModbusParams  - 27
#define  r_DIDutyCh3     c_ModbusParams  - 28
#define  r_DIDutyCh4     c_ModbusParams  - 29
// GET_DIPERDUR(n)  n - number of a channel. n = 1..4
// descr: get a duration period of a channel
// returning: unsigned long
#define GET_DIDUTY(n)                                              MAKE_ABS( (r_DIDutyCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIImpQtyCh1   c_ModbusParams  - 30
#define  r_DIImpQtyCh2   c_ModbusParams  - 31
#define  r_DIImpQtyCh3   c_ModbusParams  - 32
#define  r_DIImpQtyCh4   c_ModbusParams  - 33
#define  r_DIImpQtyCh5   c_ModbusParams  - 34
#define  r_DIImpQtyCh6   c_ModbusParams  - 35
#define  r_DIImpQtyCh7   c_ModbusParams  - 36
#define  r_DIImpQtyCh8   c_ModbusParams  - 37
#define  r_DIImpQtyCh9   c_ModbusParams  - 38
#define  r_DIImpQtyCh10	 c_ModbusParams  - 39
#define  r_DIImpQtyCh11	 c_ModbusParams  - 40
#define  r_DIImpQtyCh12  c_ModbusParams  - 41
// GET_DIPERDUR(n)  n - number of a channel. n = 1..12
// descr: get an impulse count
// returning: unsigned long
#define GET_DIIMPQTY(n)                                            MAKE_ABS( (r_DIImpQtyCh1) - c_ModbusParams + 1 - (2 * (n - 1)) )

#define  r_DIImpQtyCh1H   c_ModbusParams  - 42
#define  r_DIImpQtyCh1L   c_ModbusParams  - 43
#define  r_DIImpQtyCh2H   c_ModbusParams  - 44
#define  r_DIImpQtyCh2L   c_ModbusParams  - 45
#define  r_DIImpQtyCh3H   c_ModbusParams  - 46
#define  r_DIImpQtyCh3L   c_ModbusParams  - 47
#define  r_DIImpQtyCh4H   c_ModbusParams  - 48
#define  r_DIImpQtyCh4L   c_ModbusParams  - 49
#define  r_DIImpQtyCh5H   c_ModbusParams  - 50
#define  r_DIImpQtyCh5L   c_ModbusParams  - 51
#define  r_DIImpQtyCh6H   c_ModbusParams  - 52
#define  r_DIImpQtyCh6L   c_ModbusParams  - 53
#define  r_DIImpQtyCh7H   c_ModbusParams  - 54
#define  r_DIImpQtyCh7L   c_ModbusParams  - 55
#define  r_DIImpQtyCh8H   c_ModbusParams  - 56
#define  r_DIImpQtyCh8L   c_ModbusParams  - 57
#define  r_DIImpQtyCh9H   c_ModbusParams  - 58
#define  r_DIImpQtyCh9L   c_ModbusParams  - 59
#define  r_DIImpQtyCh10H  c_ModbusParams  - 60
#define  r_DIImpQtyCh10L  c_ModbusParams  - 61
#define  r_DIImpQtyCh11H  c_ModbusParams  - 62
#define  r_DIImpQtyCh11L  c_ModbusParams  - 63
#define  r_DIImpQtyCh12H  c_ModbusParams  - 64
#define  r_DIImpQtyCh12L  c_ModbusParams  - 65
// GET_DIPERDUR(n)  n - number of a part of channel. (two registers per a value)  n = 1..24
// descr: get an impulse count
// returning: unsigned long
#define GET_DIIMPTQTY64(n)                                         MAKE_ABS( (r_DIImpQtyCh1H) - c_ModbusParams + 1 - (2 * (n - 1)) )


// ====================================================



//  === Config Reg [0x03/0x10]===

// --- ����� ��� ���� ���� ����� ---
// all registers are 16-bit ones!
#define  r_FEId          end_adr_conf_reg16 - 1	// ������ �������
#define GET_FEID                                                   MAKE_ABS( (r_FEId) - end_adr_conf_reg16 + 1 )

#define  r_Modif         end_adr_conf_reg16 - 2  // ����������� �������, � ����� ����������
//  1772 - X1 - X2 - X3 - X4
//
//	X1:  [0] - ����� ��� -----------------|
//																				X2: [1] -  4 �����
//																				X2: [2] -  8 ������
//
//			 [1] - ����� ��� -----------------|
//																				X2: [1] -  4 �����   0-20��
//																				X2: [2] -  8 ������  0-20��
//																				X2: [3] -  4 �����   0-10�
//																				X2: [4] -  8 ������  0-10�
//																				X2: [5] -  4+4 ����� 0-20�� + 0-10�
//
//			 [2] - ����� ���� ----------------|
//																				X2: [1] -  8 ���� ����������
//																				X2: [2] - 16 ���� ���������� 
//																				X2: [1] -  8 ���� �������������
//																				X2: [2] - 16 ���� ������������� 
//																				X2: [2] - 24 ���� ������������� 
//
//			 [3] - ����� ����� ��./���. ------|
//																				X2: [1] -  4 �����
//																				X2: [2] -  8 ������
#define GET_MODIF                                                  MAKE_ABS( (r_Modif) - end_adr_conf_reg16 + 1 )

#define  r_SerialNumber  end_adr_conf_reg16 - 3	// �������� ����� � ����� ������ ����
#define GET_SN                                                     MAKE_ABS( (r_SerialNumber) - end_adr_conf_reg16 + 1 )

#define  r_ManufDate     end_adr_conf_reg16 - 4	// ���� ������� �������
#define GET_MANUFDATE                                              MAKE_ABS( (r_ManufDate) - end_adr_conf_reg16 + 1 )

#define  r_CheckDate     end_adr_conf_reg16 - 5  // ���� ��������� �������
// (Day << 11) + (Month << 7) + Year
#define GET_CHECKDATE                                              MAKE_ABS( (r_CheckDate) - end_adr_conf_reg16 + 1 )

#define  r_FEStatus      end_adr_conf_reg16 - 6
#define GET_FESTATUS                                               MAKE_ABS( (r_FEStatus) - end_adr_conf_reg16 + 1 )
			                        
#define  r_Address	     end_adr_conf_reg16 - 7  // ����� �������
#define GET_ADDRESS                                                MAKE_ABS( (r_Address) - end_adr_conf_reg16 + 1 )

#define  r_UARTparam     end_adr_conf_reg16 - 8  // ��������� UART
#define GET_UARTPARAM                                              MAKE_ABS( (r_UARTparam) - end_adr_conf_reg16 + 1 )

#define  r_Reserv1	     end_adr_conf_reg16 - 9	
#define  r_Reserv2       end_adr_conf_reg16 - 10

#define  r_HardwareVer   end_adr_conf_reg16 - 11
#define GET_HARDWAREVER                                            MAKE_ABS( (r_HardwareVer) - end_adr_conf_reg16 + 1 )

#define  r_FirmwareVer   end_adr_conf_reg16 - 12	// ������ ��������
#define GET_FIRMWAREVER                                            MAKE_ABS( (r_FirmwareVer) - end_adr_conf_reg16 + 1 )

#define  r_OutputParam   end_adr_conf_reg16 - 13
#define  r_TestCmd       end_adr_conf_reg16 - 14

#define  r_Calib         end_adr_conf_reg16 - 15  // ������� ��� ������ ����������
#define GET_CALIBR                                                 MAKE_ABS( (r_Calib) - end_adr_conf_reg16 + 1 )

#define  r_Restore       end_adr_conf_reg16 - 16	// ���������� ��������� ���������
#define GET_RESTORER                                               MAKE_ABS( (r_Restore) - end_adr_conf_reg16 + 1 )

#define  r_Password      end_adr_conf_reg16 - 17
#define GET_PASSWORDR                                              MAKE_ABS( (r_Password) - end_adr_conf_reg16 + 1 )

#define  r_ReservPass    end_adr_conf_reg16 - 18

#define  r_SystCmd       end_adr_conf_reg16 - 19
#define GET_SYSTCMDR                                               MAKE_ABS( (r_SystCmd) - end_adr_conf_reg16 + 1 )

// --------------------------------------------------

// === [0x03/0x10] === ����� ADC ===
#define  r_BreakLinePeriod  end_adr_conf_reg16 - 20
#define  r_ADChipType      end_adr_conf_reg16 - 21  // For ADC 0x00XX  1 - AD7794 (24bit), 0 - AD7795 (16bit)


// ������ � ��������� ���������� �������
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
//      ___[FIFO Buffer]___/          \___[Break Line Detect]___

// when you work with ADMode and ADSettings u r supposed to use these definitions (when calibrating don't forget to increase range as +1. Yes, I'm not a freak)
// channel ranges:
#define  CHANNEL_MODE_VOLTAGE_100mV        0
#define  CHANNEL_MODE_VOLTAGE_1V           1
#define  CHANNEL_MODE_CURRENT_5mA          2
#define  CHANNEL_MODE_CURRENT_20mA         3
#define  CHANNEL_MODE_RESISTANCE_100Om4w   4
#define  CHANNEL_MODE_RESISTANCE_400Om4w   5
#define  CHANNEL_MODE_RESISTANCE_4000Om4w  6
#define  CHANNEL_MODE_RESISTANCE_100Om3w   7
#define  CHANNEL_MODE_RESISTANCE_400Om3w   8
#define  CHANNEL_MODE_RESISTANCE_4000Om3w  9
#define  CHANNEL_MODE_RESISTANCE_100Om2w  10
#define  CHANNEL_MODE_RESISTANCE_400Om2w  11
#define  CHANNEL_MODE_RESISTANCE_4000Om2w 12

//#define  CHANNEL_MODE_RESISTANCE_100Om     4
//#define  CHANNEL_MODE_RESISTANCE_400Om     5
//#define  CHANNEL_MODE_RESISTANCE_4000Om    6


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
// descr: read mode or set one of a channel
// returning: unsigned long
#define GET_ADModeR(n)                                             MAKE_ABS( (r_AD1Mode) - end_adr_conf_reg16 + 1 - (2 * (n - 1)) )
                                                                   // 2 * (n - 1) you see, but remember all regs are 16-bit here

// GET_ADSettingsR
// descr: read settings or set ones of a channel
// returning: unsigned long
#define GET_ADSettingsR(n)                                         MAKE_ABS( (r_AD1Settings) - end_adr_conf_reg16 + 1 - (2 * (n - 1)) )
                                                                   //2 * (n - 1) you see, but remember all regs are 16-bit here

// ���������� ���������� �������
// ������� ������ ��������� ����� � �������� ��� ���������� ������
#define  cm_AD1ParamSave  0x0E901  
// ..........................
#define  cm_AD8ParamSave  0x0E908
// ������� ������ ��������� ����� � �������� ��� ���������� ������
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


#define  r_Cr1	         end_adr_conf_reg16 - 56
#define  r_Cr2	         end_adr_conf_reg16 - 57
#define  r_Cr3	         end_adr_conf_reg16 - 58
#define  r_Cr4	         end_adr_conf_reg16 - 59
#define  r_Cr5	         end_adr_conf_reg16 - 60
#define  r_Cr6	         end_adr_conf_reg16 - 61
#define  r_Cr7	         end_adr_conf_reg16 - 62
#define  r_Cr8	         end_adr_conf_reg16 - 63




// === [0x03/0x10] === ����� DAC ===
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
// descr: get a part of an analog output register
// returning: unsigned short (16-bit)
#define GET_DACHR(n)                                             MAKE_ABS((r_DACh1H) - end_adr_conf_reg16 + 1 - (n - 1) )

#define  r_DANum         end_adr_conf_reg16 - 50
#define  r_DAZero        end_adr_conf_reg16 - 51
#define  r_DAScale       end_adr_conf_reg16 - 52


// === [0x03/0x10] === ����� ���� ===
#define  r_Rele		       end_adr_conf_reg16 - 24
// GET_RELAYR
// descr: get a relay register
// returning: unsigned long (why not short?)
#define GET_RELAYR                                               MAKE_ABS((r_Rele) - end_adr_conf_reg16 + 1 )

// === [0x03/0x10] === ����� ����� ��./���. === 
#define  r_DInputTypeH   			 end_adr_conf_reg16  - 20
#define  r_DInputTypeL   			 end_adr_conf_reg16  - 21
#define  r_DIFreq	  	   			 end_adr_conf_reg16  - 22
#define  r_DIPerDur 	   			 end_adr_conf_reg16  - 23
#define  r_DIPosDur 	   			 end_adr_conf_reg16  - 24
#define  r_DINegDur 	   			 end_adr_conf_reg16  - 25
#define  r_DIDuty		 	   			 end_adr_conf_reg16  - 26
#define  r_DIImpQtyPeriodCh1 	 end_adr_conf_reg16  - 27
#define  r_DIImpQtyPeriodCh2	 end_adr_conf_reg16  - 28
#define  r_DIImpQtyPeriodCh3	 end_adr_conf_reg16  - 29
#define  r_DIImpQtyPeriodCh4	 end_adr_conf_reg16  - 30
#define  r_DIImpQtyPeriodCh5	 end_adr_conf_reg16  - 31
#define  r_DIImpQtyPeriodCh6	 end_adr_conf_reg16  - 32
#define  r_DIImpQtyPeriodCh7	 end_adr_conf_reg16  - 33
#define  r_DIImpQtyPeriodCh8	 end_adr_conf_reg16  - 34
#define  r_DIImpQtyPeriodCh9	 end_adr_conf_reg16  - 35
#define  r_DIImpQtyPeriodCh10	 end_adr_conf_reg16  - 36
#define  r_DIImpQtyPeriodCh11	 end_adr_conf_reg16  - 37
#define  r_DIImpQtyPeriodCh12	 end_adr_conf_reg16  - 38



// ====================================================




#define  r_SystCommands  254  // ������� ������� ��������� �������
						   

// === ������� ���������� ������������� ������� ===
//
//                     0 x  1  2  3  4
//                         /  /    \  \.
//   ___[Process Status]__/  /      \  \__[Ch number]___
//                          /        \.
//     ___[Command type]___/          \___[Diapozone]___
//

// *** ����� ������ *****************************
#define  cm_TestInput_Mask  							0x0E000
#define  cm_TestInputProcess_Mask					0x0D000
#define  cm_ZSReset_Mask			 					 	0x0E100
#define  cm_ZSCopy_Mask			   					 	0x0E200
#define  cm_CalibZero_Mask						  	0x0E300
#define  cm_CalibScale_Mask								0x0E400
#define  cm_CalibDAC_Mask 						 		0x0E500
#define  cm_Correct3w2w_Mask							0x0E700
//#define  cm_CalibTestRefZero_Mask					0x0E800
//#define  cm_CalibTestRefZeroProcess_Mask	0x0D800
// **********************************************



// *** Reset Zero & Scale ****************
#define  cm_ZSReset_D01V_ChAll  0x0E110
#define  cm_ZSReset_D01V_Ch1    0x0E111
// .........
#define  cm_ZSReset_D4k_Ch8     0x0E178
// ***************************************

// *** Copy Zero & Scale to all other channel ****
#define  cm_ZSCopy_D01V_ChAll  0x0E210
#define  cm_ZSCopy_D01V_Ch1    0x0E211
// .........
#define  cm_ZSCopy_D4k_Ch8     0x0E278
// ************************************************


// *** Calibrate Zero ********************
#define  cm_CalibZero_D01V_ChAll  0x0E310
#define  cm_CalibZero_D01V_Ch1    0x0E311
// .........
#define  cm_CalibZero_D4k_Ch8     0x0E378
// ***************************************

// *** Calibrate Scale *******************
#define  cm_CalibScale_D01V_ChAll 0x0E410
#define  cm_CalibScale_D01V_Ch1   0x0E411
// .........
#define  cm_CalibScale_D4k_Ch8    0x0E478
// ***************************************

// *** Correction Line Zero ********************
// 3-x ���������
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

// ===============================================
//  ������ �������  0 x 0  E  0  x  x
//										     ----  -  -
//                          /     \  \.
//                         /       \  \___����� ������__
//                        /         \.
//         ___������� ___/           \___��� �����_____
//

#define  cm_TestInputAll_ChAll  0x0E000 // ������������ ������� ��������� 1234 ����� 12345678
#define  cm_TestInput1_ChAll	  0x0E010 // ������������ ������� ��������� 1    ����� 12345678
#define  cm_TestInput2_ChAll  	0x0E020 // ������������ ������� ��������� 2    ����� 12345678
#define  cm_TestInput3_ChAll 	 	0x0E030 // ������������ ������� ��������� 3		 ����� 12345678
#define  cm_TestInput4_ChAll 		0x0E040 // ������������ ������� ��������� 4    ����� 12345678

#define  cm_TestInputAll_Ch1  	0x0E001 // ������������ ������� ��������� 1234 ����� 1
#define  cm_TestInput1_Ch1	  	0x0E011 // ������������ ������� ��������� 1    ����� 1
#define  cm_TestInput2_Ch1	  	0x0E021 // ������������ ������� ��������� 2    ����� 1
#define  cm_TestInput3_Ch1	  	0x0E031 // ������������ ������� ��������� 3    ����� 1
#define  cm_TestInput4_Ch1	  	0x0E041 // ������������ ������� ��������� 4    ����� 1
//				......
//				......
//				......
#define  cm_TestInputAll_Ch8  	0x0E008 // ������������ ������� ��������� 1234 ����� 8
#define  cm_TestInput1_Ch8	  	0x0E018 // ������������ ������� ��������� 1    ����� 8
#define  cm_TestInput2_Ch8	  	0x0E028 // ������������ ������� ��������� 2    ����� 8
#define  cm_TestInput3_Ch8	  	0x0E038 // ������������ ������� ��������� 3    ����� 8
#define  cm_TestInput4_Ch8	  	0x0E048 // ������������ ������� ��������� 4    ����� 8


// ���� ������ ��� ���������� ���������� ���� ���
#define  err_ADCCalibOk   		 0x01000   // ������� + ���  (0x0E310 + 0x1000 = 0x0F310) ���������� ���� ��� ����������� ���������
#define  err_ADCInProcess  		 0x01000   // ������� - ���  (0x0E310 - 0x1000 = 0x0D310) ���������� � ��������
#define  err_ADCNoCh      		 0x02000   // ������� - ���  (0x0E310 - 0x2000 = 0x0�310) ���������� ���� ��� �� �����������, ��� ������ ������
#define  err_ADCNoDiap    		 0x03000   // ������� - ���  (0x0E310 - 0x3000 = 0x0B310) ���������� ���� ��� �� �����������, ��� ������ ���������
#define  err_ADCDiapNotEqu 		 0x04000   // ������� - ���  (0x0E310 - 0x4000 = 0x0A310) ���������� ���� ��� �� �����������, ��� ������ ���������
#define  err_ADCCalibOverNorm  0x05000   // ������� - ���  (0x0E310 - 0x5000 = 0x09310) ���������� ���� ��� �� �����������, ������������ ������� ������
#define  err_ADCZeroCalibNotOk 0x06000   // ������� - ���  (0x0E310 - 0x6000 = 0x08310) ���������� ���� ��� �� �����������, ��� ������ ���������

// =======================================================================================


// === ������� ���������� ������� ����������� ===
//
//                     0 x  1  2  3  4
//                          -  -  -  -
//                         /  /    \  \.
//             ___������__/  /      \  \__����� ������___
//                          /        \.
//            ___�������___/          \___����������___
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


#define  cm_rAllDefault         0x0E630 // 0. �������� ���� ��������� ��-��������� � �������
#define  cm_rAllManufToW        0x0E631 // 1. �������� ���� ��������� ��������� � �������
							    
#define  cm_rZSCalibDefault     0x0E632 // 2. �������� ���������� ����� � ���� ��-��������� � �������
#define  cm_rZSCalibManufToW    0x0E633 // 3. �������� ��������� ���������� ����� � ���� � �������
	  
#define  cm_rDACCalibDefault    0x0E634 // 4. �������� ���������� ����� � ���� ������� ������� ��-��������� � �������
#define  cm_rDACCalibManufToW   0x0E635 // 5. �������� ��������� ���������� ����� � ���� ������� ������� � �������

#define  cm_rLastCalibZToW      0x0E636 // 6. ������� ��������� ���������� ����� � �������
#define  cm_rLastCalibSToW      0x0E637 // 7. ������� ��������� ���������� ���� � �������
#define  cm_rLastCalibDAC1      0x0E638 // 8. ������� ��������� ���������� ���� � ����� 1-�� �������� ������ � �������
#define  cm_rLastCalibDAC2      0x0E639 // 9. ������� ��������� ���������� ���� � ����� 2-�� �������� ������ � �������

#define  cm_rRegMapDefault      0x0E63A //10. �������� ����� �������������� ��������� ��-��������� � �������
#define  cm_rRegMapManufToW     0x0E63B //11. �������� ��������� ����� �������������� ��������� � �������

#define  cm_rAllMWDefault       0x0E63C //12. �������� ���� ��������� ��-��������� � ��������� � �������
#define  cm_rZSCalibMWDefault   0x0E63D //13. �������� ���������� ����� � ���� � ��������� � �������
#define  cm_rDACCalibMWDefault  0x0E63E //14. �������� ���������� ����� � ���� ������� ������� � ��������� � �������
#define  cm_rRegMapMWDefault    0x0E63F //15. �������� ����� �������������� ��������� ��-��������� � ��������� � �������


