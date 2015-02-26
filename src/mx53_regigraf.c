#include <linux/types.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/ads7846.h>
#include <linux/fsl_devices.h>
#include <linux/ipu.h>
#include <linux/mxcfb.h>
#include <linux/pwm_backlight.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <mach/common.h>
#include <mach/gpio.h>
#include <mach/iomux-mx53.h>
#include <mach/mmc.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/mach/flash.h>
#include "devices.h"
#include "mx53_diagnostic_drv.h"

//#define MX53_REGIBOARD_V1
#define MX53_SPI2GPIO_TOUCH

#define GPIO_ID(port, pin) ((port - 1)*32 + pin)

// ECSPI-1 pins
#undef MX53_PAD_CSI0_DAT4__ECSPI1_SCLK
#undef MX53_PAD_CSI0_DAT5__ECSPI1_MOSI
#undef MX53_PAD_CSI0_DAT6__ECSPI1_MISO
#undef MX53_PAD_CSI0_DAT7__ECSPI1_SS0

#define MX53_ESCPI1_PAD_CTRL	(PAD_CTL_HYS | PAD_CTL_DSE_HIGH)
				
#define MX53_PAD_CSI0_DAT4__ECSPI1_SCLK (_MX53_PAD_CSI0_DAT4__ECSPI1_SCLK | MUX_PAD_CTRL(MX53_ESCPI1_PAD_CTRL))
#define MX53_PAD_CSI0_DAT5__ECSPI1_MOSI (_MX53_PAD_CSI0_DAT5__ECSPI1_MOSI | MUX_PAD_CTRL(MX53_ESCPI1_PAD_CTRL))
#define MX53_PAD_CSI0_DAT6__ECSPI1_MISO (_MX53_PAD_CSI0_DAT6__ECSPI1_MISO | MUX_PAD_CTRL(MX53_ESCPI1_PAD_CTRL))
#define MX53_PAD_CSI0_DAT7__ECSPI1_SS0  (_MX53_PAD_CSI0_DAT7__ECSPI1_SS0 | MUX_PAD_CTRL(MX53_ESCPI1_PAD_CTRL))
// CSPI pins
#undef MX53_PAD_EIM_D20__CSPI_SS0
#undef MX53_PAD_EIM_D21__CSPI_SCLK
#undef MX53_PAD_EIM_D22__CSPI_MISO
#undef MX53_PAD_EIM_D28__CSPI_MOSI

#define MX53_CSPI_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_HYS | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST) 

#define MX53_PAD_EIM_D20__CSPI_SS0  (_MX53_PAD_EIM_D20__CSPI_SS0 | MUX_PAD_CTRL(MX53_CSPI_PAD_CTRL))
#define MX53_PAD_EIM_D21__CSPI_SCLK (_MX53_PAD_EIM_D21__CSPI_SCLK | MUX_PAD_CTRL(MX53_CSPI_PAD_CTRL))
#define MX53_PAD_EIM_D22__CSPI_MISO (_MX53_PAD_EIM_D22__CSPI_MISO | MUX_PAD_CTRL(MX53_CSPI_PAD_CTRL))
#define MX53_PAD_EIM_D28__CSPI_MOSI (_MX53_PAD_EIM_D28__CSPI_MOSI | MUX_PAD_CTRL(MX53_CSPI_PAD_CTRL))
// I2C pins
#undef _MX53_PAD_GPIO_5__I2C3_SCL
#undef _MX53_PAD_GPIO_6__I2C3_SDA
#undef MX53_PAD_GPIO_5__I2C3_SCL
#undef MX53_PAD_GPIO_6__I2C3_SDA

#define _MX53_PAD_GPIO_5__I2C3_SCL IOMUX_PAD(0x6C0, 0x330, 6 | IOMUX_CONFIG_SION, 0x824, 2, 0)
#define _MX53_PAD_GPIO_6__I2C3_SDA IOMUX_PAD(0x6B4, 0x324, 2 | IOMUX_CONFIG_SION, 0x828, 1, 0)
#define MX53_PAD_GPIO_5__I2C3_SCL (_MX53_PAD_GPIO_5__I2C3_SCL | MUX_PAD_CTRL(MX53_I2C_PAD_CTRL_2))
#define MX53_PAD_GPIO_6__I2C3_SDA (_MX53_PAD_GPIO_6__I2C3_SDA | MUX_PAD_CTRL(MX53_I2C_PAD_CTRL_2))
/* Global configuration of PADs ***********************************************/
/* 
 * Format for pin descriptions:
 * i.MX53 unit pin -> output pin -> external device pin
 */
static iomux_v3_cfg_t iomux_pads[] = {
	// PSU (Power Supply Unit) signals
	MX53_PAD_PATA_DATA8__GPIO2_8, // GPIO2_8 -> PATA_DATA8 -> Reset (X2)
	MX53_PAD_PATA_DATA9__GPIO2_9, // GPIO2_9 -> PATA_DATA9 -> Save  (X2)
	// PSU (thermo sensor) (lm92)
	MX53_PAD_CSI0_DAT8__I2C1_SDA, // I2C1_SDA -> CSI0_DAT8 -> SDA (7)
	MX53_PAD_CSI0_DAT9__I2C1_SCL, // I2C1_SCL -> CSI0_DAT9 -> SCL (8)
  // RTC (DS3231MZ+)
	MX53_PAD_GPIO_5__I2C3_SCL,    // I2C3_SCL -> GPIO_5  -> SCL     (8)
	MX53_PAD_GPIO_6__I2C3_SDA,    // I2C3_SDA -> GPIO_6  -> SDA     (7)
	MX53_PAD_GPIO_16__GPIO7_11,   // GPIO7_11 -> GPIO_16 -> INT/SQW (3)
	MX53_PAD_GPIO_17__GPIO7_12,   // GPIO7_12 -> GPIO_17 -> RST     (4)
	// BackplaneSec (X2)
	MX53_PAD_CSI0_DAT12__GPIO5_30, // UART4_TX_GPIO5_30 -> CSI0_DAT12 -> 32
	MX53_PAD_CSI0_DAT13__GPIO5_31, // UART4_RX_GPIO5_31 -> CSI0_DAT13 -> 34
	MX53_PAD_CSI0_DAT14__GPIO6_0,  // UART5_TX_GPIO6_0  -> CSI0_DAT14 -> 36
	MX53_PAD_CSI0_DAT15__GPIO6_1,  // UART5_RX_GPIO6_1  -> CSI0_DAT15 -> 38
	MX53_PAD_CSI0_DAT16__GPIO6_2,  // GPIO6_2           -> CSI0_DAT16 -> 42
	MX53_PAD_CSI0_DAT17__GPIO6_3,  // GPIO6_3           -> CSI0_DAT17 -> 44
	MX53_PAD_CSI0_DAT18__GPIO6_4,  // GPIO6_4           -> CSI0_DAT18 -> 46
	MX53_PAD_CSI0_DAT19__GPIO6_5,  // GPIO6_5           -> CSI0_DAT19 -> 48
	// DISP 0 (X25)
	MX53_PAD_GPIO_1__GPIO1_1,             // DISP0_CONTRAST -> GPIO_1  -> 112
#ifdef MX53_REGIBOARD_V1
	MX53_PAD_EIM_D21__IPU_DISPB0_SER_CLK, // DISP0_SER_SCLK -> EIM_D21 -> 89
	MX53_PAD_EIM_D28__IPU_DISPB0_SER_DIO, // DISP0_SER_MOSI -> EIM_D28 -> 87
#endif
	// ParalDisp (X26)
	MX53_PAD_DI0_DISP_CLK__IPU_DI0_DISP_CLK, // DISP0_DCLK  -> DI0_DISP_CLK -> 2
	MX53_PAD_DI0_PIN2__IPU_DI0_PIN2,         // DISP0_HSYNC -> DI0_PIN2     -> 3
	MX53_PAD_DI0_PIN3__IPU_DI0_PIN3,         // DISP0_VSYNC -> DI0_PIN3     -> 4
	MX53_PAD_DI0_PIN15__IPU_DI0_PIN15,       // DISP0_DRDY  -> DI0_PIN15    -> 27
	MX53_PAD_DISP0_DAT0__IPU_DISP0_DAT_0,
	MX53_PAD_DISP0_DAT1__IPU_DISP0_DAT_1,
	MX53_PAD_DISP0_DAT2__IPU_DISP0_DAT_2,
	MX53_PAD_DISP0_DAT3__IPU_DISP0_DAT_3,
	MX53_PAD_DISP0_DAT4__IPU_DISP0_DAT_4,
	MX53_PAD_DISP0_DAT5__IPU_DISP0_DAT_5,
	MX53_PAD_DISP0_DAT6__IPU_DISP0_DAT_6,
	MX53_PAD_DISP0_DAT7__IPU_DISP0_DAT_7,
	MX53_PAD_DISP0_DAT8__IPU_DISP0_DAT_8,
	MX53_PAD_DISP0_DAT9__IPU_DISP0_DAT_9,
	MX53_PAD_DISP0_DAT10__IPU_DISP0_DAT_10,
	MX53_PAD_DISP0_DAT11__IPU_DISP0_DAT_11,
	MX53_PAD_DISP0_DAT12__IPU_DISP0_DAT_12,
	MX53_PAD_DISP0_DAT13__IPU_DISP0_DAT_13,
	MX53_PAD_DISP0_DAT14__IPU_DISP0_DAT_14,
	MX53_PAD_DISP0_DAT15__IPU_DISP0_DAT_15,
	MX53_PAD_DISP0_DAT16__IPU_DISP0_DAT_16,
	MX53_PAD_DISP0_DAT17__IPU_DISP0_DAT_17,
	MX53_PAD_DISP0_DAT18__IPU_DISP0_DAT_18,
	MX53_PAD_DISP0_DAT19__IPU_DISP0_DAT_19,
	MX53_PAD_DISP0_DAT20__IPU_DISP0_DAT_20,
	MX53_PAD_DISP0_DAT21__IPU_DISP0_DAT_21,
	MX53_PAD_DISP0_DAT22__IPU_DISP0_DAT_22,
	MX53_PAD_DISP0_DAT23__IPU_DISP0_DAT_23,
	// LVDS (X27)
	MX53_PAD_LVDS0_TX0_P__LDB_LVDS0_TX0,
	MX53_PAD_LVDS0_TX1_P__LDB_LVDS0_TX1,
	MX53_PAD_LVDS0_TX2_P__LDB_LVDS0_TX2,
	MX53_PAD_LVDS0_TX3_P__LDB_LVDS0_TX3,
	MX53_PAD_LVDS0_CLK_P__LDB_LVDS0_CLK,
	MX53_PAD_GPIO_10__GPIO4_0,           // GPIO4_0 -> GPIO_10 -> LVDS0_DPS   -> 4
	MX53_PAD_GPIO_11__GPIO4_1,           // GPIO4_1 -> GPIO_11 -> LVDS0_SEL68 -> 20
	// Front USB/LED
	MX53_PAD_KEY_ROW3__GPIO4_13, // GPIO4_13 -> KEY_ROW3 -> VD3_4   -> 7 (GREEN)
	MX53_PAD_KEY_COL4__GPIO4_14, // GPIO4_14 -> KEY_ROW3 -> VD1_2   -> 8 (RED)
	MX53_PAD_KEY_ROW4__GPIO4_15, // GPIO4_15 -> KEY_ROW3 -> Dinamic -> 9
	// SD 1
	MX53_PAD_PATA_DIOW__GPIO6_17,    // GPIO6_17 -> PATA_DIOW  -> +3.3U (Card detect)
	MX53_PAD_PATA_DMACK__GPIO6_18,   // GPIO6_18 -> PATA_DMACK -> +3.3U (Write protect)
	MX53_PAD_SD1_CMD__ESDHC1_CMD,    // ESDHC1_CMD  -> SD1_CMD   -> CMD     (3)
	MX53_PAD_SD1_CLK__ESDHC1_CLK,    // ESDHC1_CLK  -> SD1_CLK   -> CLK     (5)
	MX53_PAD_SD1_DATA0__ESDHC1_DAT0, // ESDHC1_DAT0 -> SD1_DATA0 -> DAT0    (7)
	MX53_PAD_SD1_DATA1__ESDHC1_DAT1, // ESDHC1_DAT1 -> SD1_DATA1 -> DAT1    (8)
	MX53_PAD_SD1_DATA2__ESDHC1_DAT2, // ESDHC1_DAT2 -> SD1_DATA2 -> DAT2    (1)
	MX53_PAD_SD1_DATA3__ESDHC1_DAT3, // ESDHC1_DAT3 -> SD1_DATA3 -> CD/DAT3 (2)
	// SD 2
	MX53_PAD_SD2_CMD__ESDHC2_CMD,
	MX53_PAD_SD2_CLK__ESDHC2_CLK,
	MX53_PAD_SD2_DATA0__ESDHC2_DAT0,
	MX53_PAD_SD2_DATA1__ESDHC2_DAT1,
	MX53_PAD_SD2_DATA2__ESDHC2_DAT2,
	MX53_PAD_SD2_DATA3__ESDHC2_DAT3,
	// UART 1
	MX53_PAD_CSI0_DAT10__UART1_TXD_MUX,
	MX53_PAD_CSI0_DAT11__UART1_RXD_MUX,
	// FEC
	MX53_PAD_FEC_MDC__FEC_MDC,
	MX53_PAD_FEC_MDIO__FEC_MDIO,
	MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
	MX53_PAD_FEC_RX_ER__FEC_RX_ER,
	MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
	MX53_PAD_FEC_RXD1__FEC_RDATA_1,
	MX53_PAD_FEC_RXD0__FEC_RDATA_0,
	MX53_PAD_FEC_TX_EN__FEC_TX_EN,
	MX53_PAD_FEC_TXD1__FEC_TDATA_1,
	MX53_PAD_FEC_TXD0__FEC_TDATA_0,
#ifdef MX53_SPI2GPIO_TOUCH
	// SPI->GPIO touch screen
	MX53_PAD_PATA_DATA6__GPIO2_6, // GPIO2_6  -> PATA_DATA6 -> PENIRQ (11)
	MX53_PAD_CSI0_DAT4__GPIO5_22, // GPIO5_22 -> CSI0_DAT4  -> SCK    (16)
	MX53_PAD_CSI0_DAT5__GPIO5_23, // GPIO5_23 -> CSI0_DAT5  -> MOSI   (14)
	MX53_PAD_CSI0_DAT6__GPIO5_24, // GPIO5_24 -> CSI0_DAT6  -> MISO   (12)
	MX53_PAD_CSI0_DAT7__GPIO5_25, // GPIO5_25 -> CSI0_DAT7  -> CS     (15)
#else
	// ECSPI-1 Touch screen
	MX53_PAD_PATA_DATA6__GPIO2_6,
	MX53_PAD_CSI0_DAT4__ECSPI1_SCLK,
	MX53_PAD_CSI0_DAT5__ECSPI1_MOSI,
	MX53_PAD_CSI0_DAT6__ECSPI1_MISO,
	MX53_PAD_CSI0_DAT7__ECSPI1_SS0,
#endif
	// PWM Backlight
	MX53_PAD_PATA_INTRQ__GPIO7_2, // GPIO7_2   -> PATA_INTRQ (Backlight enable/disable)
	MX53_PAD_GPIO_9__PWM1_PWMO,   // PWM1_PWMO -> GPIO_9 (PWM output)
	// EIM
	MX53_PAD_EIM_RW__ECSPI2_SS0,   // SPI2_nCS  -> EIM_RW  -> 3
	MX53_PAD_EIM_CS0__ECSPI2_SCLK, // SPI2_CLK  -> EIM_CS0 -> 4
	MX53_PAD_EIM_CS1__ECSPI2_MOSI, // SPI2_MOSI -> EIM_CS1 -> 5
	MX53_PAD_EIM_OE__ECSPI2_MISO,  // SPI2_MISO -> EIM_OE  -> 6
	// NAND
	MX53_PAD_EIM_DA0__EMI_NAND_WEIM_DA_0, // IO_0
	MX53_PAD_EIM_DA1__EMI_NAND_WEIM_DA_1, // IO_1
	MX53_PAD_EIM_DA2__EMI_NAND_WEIM_DA_2, // IO_2
	MX53_PAD_EIM_DA3__EMI_NAND_WEIM_DA_3, // IO_3
	MX53_PAD_EIM_DA4__EMI_NAND_WEIM_DA_4, // IO_4
	MX53_PAD_EIM_DA5__EMI_NAND_WEIM_DA_5, // IO_5
	MX53_PAD_EIM_DA6__EMI_NAND_WEIM_DA_6, // IO_6
	MX53_PAD_EIM_DA7__EMI_NAND_WEIM_DA_7, // IO_7
	MX53_PAD_NANDF_WE_B__EMI_NANDF_WE_B,  // WE
	MX53_PAD_NANDF_RE_B__EMI_NANDF_RE_B,  // RE
	MX53_PAD_NANDF_ALE__EMI_NANDF_ALE,	  // ALE
	MX53_PAD_NANDF_CLE__EMI_NANDF_CLE,    // CLE
	MX53_PAD_NANDF_WP_B__EMI_NANDF_WP_B,  // WP
	MX53_PAD_NANDF_RB0__EMI_NANDF_RB_0,   // R/B
	MX53_PAD_NANDF_CS0__EMI_NANDF_CS_0,   // CE
	// NOR
  MX53_PAD_EIM_D20__CSPI_SS0,  // CSPI_SS0  -> EIM_D20 -> CS_B (4)
  MX53_PAD_EIM_D21__CSPI_SCLK, // CSPI_SCLK -> EIM_D21 -> SCK  (2)
  MX53_PAD_EIM_D22__CSPI_MISO, // CSPI_MISO -> EIM_D22 -> SO   (8)
  MX53_PAD_EIM_D28__CSPI_MOSI, // CSPI_MOSI -> EIM_D28 -> SI   (1)
};
/* PWM backlight **************************************************************/
#define BACKLIGHT_ENABLE GPIO_ID(7,2) // GPIO-7.2

static int mxc_pwm_backlight_init(struct device *dev) {
	printk("Regiboard PWM backlight init\n");
	if (gpio_request(BACKLIGHT_ENABLE, "lcd_backlight") == 0)
		gpio_direction_output(BACKLIGHT_ENABLE, 1);
	return 0;
}

static int mxc_pwm_backlight_notify(struct device *dev, int brightness) {
	return brightness;
}

static void mxc_pwm_backlight_exit(struct device *dev) {
	printk("Regiboard PWM backlight exit\n");
}

static struct platform_pwm_backlight_data mxc_pwm_backlight_data = {
	.pwm_id         = 0,
	.max_brightness = 255,
	.dft_brightness = 128,
	.pwm_period_ns  = 50000,
	.init           = mxc_pwm_backlight_init,
	.notify         = mxc_pwm_backlight_notify,
	.exit           = mxc_pwm_backlight_exit
};
/* SPI->GPIO touch screen *****************************************************/
#define TS_PENIRQ  GPIO_ID(2,6)  // GPIO-2.6
#define TS_BUSY    GPIO_ID(2,7)  // GPIO-2.7
#define TS_SCK     GPIO_ID(5,22) // GPIO-5.22      
#define TS_MOSI    GPIO_ID(5,23) // GPIO-5.23 
#define TS_MISO    GPIO_ID(5,24) // GPIO-5.24
#define TS_CS      GPIO_ID(5,25) // GPIO-5.25
#define TS_SCK_HZ  (125000 * 16) // max spi clock (SCK) speed in HZ
#define TS_MODEL   "ads7846"

#ifdef MX53_SPI2GPIO_TOUCH
struct spi_gpio_platform_data spi_gpio_data = {
	.sck            = TS_SCK,
	.mosi           = TS_MOSI,
	.miso           = TS_MISO,
	.num_chipselect = 1,
};

struct platform_device spi_gpio_device = {
	.name = "spi_gpio",
	.id   = 0,
	.dev  = {
		.platform_data = &spi_gpio_data,
	},
};
#endif
static struct ads7846_platform_data ads_info = {
	.model            = 7843, // ADS7843E
	.vref_mv          = 2500, // 2.5V external VREF
	.swap_xy          = false,
	.x_min            = 1155,//550,//150,
	.x_max            = 3079,//3030, //3830,
	.y_min            = 1089,//550,//190,
	.y_max            = 2886,//2900,//3830,
	.vref_delay_usecs = 10,
	.x_plate_ohms     = 450,
	.y_plate_ohms     = 250,
	.pressure_max     = 15000,
	.debounce_max     = 5,
	.debounce_rep     = 0,
	.debounce_tol     = 10,
	.gpio_pendown     = TS_PENIRQ,
};
/* Displays *******************************************************************/
/*
 pixclock    : Pixel clock, dot clock or just clock, usually in MHz. It needs to
               be entered in picoseconds.
                 pixclock in ps= 10 ^(-12) / dotclock in MHz
 left_margin : Number of pixclk pulses between HSYNC signal and the first valid 
               pixel data. (Horizontal Back Porch)
 right_margin: Number of pixclk between the last valid pixel data in the line 
               and the nect hsync pulse. (Horizontal Front Porch)
 upper_margin: Number of lines (HSYNC pulses) from a VSYNC signal to the first 
               valid line. (Vertical Back Porch)
 lower_margin: Number of lines (HSYNC pulses) between the last valid line of the 
               frame and the next VSYNC pulse. (Vertical Front Porch)
 hsync_len   : Number of pixclk pulses when a HSYNC signal is active.
 vsync_len   : Number of HSYNC pulses when a VSYNC signal is active.
 sync        : Polarity on the Data Enable
 */
// G150XGE-L04 
// - Datasheets.: G150XGE-L04.pdf, G150XGE-L04_RevC4_Datasheet.pdf
// - Resolution.: 1024x768
// - Pixel clock: 65MHz
// - PINs:
// -- REV   (4 ) <- LVDS0_DPS   <- GPIO_10 (GPIO4_0); Reverse Scan selection.
// --- Normal scan (Pin4, REV = High or NC)
// --- Reverse scan (Pin4, REV = Low)
// -- SEL68 (20) <- LVDS0_SEL68 <- GPIO_11 (GPIO4_1); Selection for 6 bits/8bits LVDS data input.
// --- ”High” for 6 bits LVDS Input
// --- “Low” or “NC” for 8 bits LVDS Input
// G150XG01
// - Resolution.: 1024x2, 768 X2 (RGBW)
// - Pixel clock: 65MHz
// - PINs:
// -- REV   (4 ) <- LVDS0_DPS   <- GPIO_10 (GPIO4_0); Reverse Scan selection.
// --- Normal scan (Pin4, REV = Low or NC)
// --- Reverse scan (Pin4, REV = High)
// -- NC    (19) <- not used
// -- SEL68 (20) <- LVDS0_SEL68 <- GPIO_11 (GPIO4_1); Selection for 6 bits/8bits LVDS data input.
// --- ”High” or “NC” for 6 bits LVDS Input
// --- “Low” for 8 bits LVDS Input
// NLB150XG01L-01
// - Resolution.: 1024x768
// - Pixel clock: ...MHz
#define LVDS0_DPS   GPIO_ID(4,0) // GPIO-4.0
#define LVDS0_SEL68 GPIO_ID(4,1) // GPIO-4.1

#define FB_SYNC (FB_SYNC_CLK_LAT_FALL)
#define FB_MODE (FB_VMODE_NONINTERLACED) //FB_VMODE_INTERLACED

static struct fb_videomode video_modes[] = {
	{ // Regigraf 15" display
		"G150XGE-L04", // name
		60,            // refresh
		1024, 768,     // x,y res
		15385,         // pixclock (10^12 / Pixel clock) picoseconds
		220,           // left_margin
		40,            // right_margin
		21,            // upper_margin
		7,             // lower_margin
		60,            // hsync_len
		10,            // vsync_len
		FB_SYNC,       // sync
		FB_MODE,       // vmode
		0,             // flag
	},
	{ // Regigraf 15" display (RGBW)
		"G150XG01",    // name
		60,            // refresh
		1024, 768,     // x,y res
		15385,         // pixclock (10^12 / Pixel clock) picoseconds
		220,           // left_margin
		40,            // right_margin
		21,            // upper_margin
		7,             // lower_margin
		60,            // hsync_len
		10,            // vsync_len
		FB_SYNC,       // sync
		FB_MODE,       // vmode
    0,             // flag	
	},
	{ // Regigraf 15" display
		"NLB150XG01L-01", // name
		60,            // refresh
		1024, 768,     // x,y res
		15385,         // pixclock (10^12 / Pixel clock) picoseconds
		320,           // left_margin
		0,             // right_margin
		38,            // upper_margin
		0,             // lower_margin
		0,             // hsync_len
		0,             // vsync_len
		FB_SYNC,       // sync
		FB_MODE,       // vmode
		0,             // flag
	},
	{ // iMX51 babbage display
		"CLAA-WVGA", 30, 800, 480, 37037, 40, 60, 10, 10, 20, 10,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
	{ // Regigraf 7" (mx1) display
		"PD104VT2", 30, 640, 480, 19200, 0x4e, 0x0e, 0x23, 0, 0x3f, 1,
		0,
		FB_VMODE_NONINTERLACED,
		0,
	},
};

struct LCDControl {
  int rsc;   // Reverse Scan Control
  int sel68; // LVDS 6/8 bit select
} static const kLCDConfiguration[] = {
  { // kG150XGE_L04
    .rsc   = 1, // normal scan
    .sel68 = 0  // 8 bit
  },
  { // kG150XG01
    .rsc   = 0, // normal scan
    .sel68 = 0  // 8 bit
  },
  { // kNLB150XG01L_01
    .rsc   = 0, // normal scan
    .sel68 = 0  // 8 bit
  },
  { // kCLAA_WVGA
    .rsc   = 0,
    .sel68 = 0
  },
  { // kPD104VT2
    .rsc   = 0, // NU
    .sel68 = 0  // NU
  },
};

enum LCDModel {
	kG150XGE_L04    = 0,
	kG150XG01       = 1,
	kNLB150XG01L_01 = 2,
	kCLAA_WVGA      = 3,
	kPD104VT2       = 4,
	kLcdAmount      = 5, // must be the last
};

static struct mxc_fb_platform_data fb_data[] = {
	{
		.interface_pix_fmt = IPU_PIX_FMT_RGB24, // IPU_PIX_FMT_RGBA32
		.mode_str          = "G150XGE-L04",
		.mode              = video_modes,
		.num_modes         = ARRAY_SIZE(video_modes),
	}, {
		.interface_pix_fmt = IPU_PIX_FMT_RGB24, // IPU_PIX_FMT_RGBA32
		.mode_str          = "G150XG01",
		.mode              = video_modes,
		.num_modes         = ARRAY_SIZE(video_modes),
  }, {
		.interface_pix_fmt = IPU_PIX_FMT_RGB24, // IPU_PIX_FMT_RGBA32
		.mode_str          = "NLB150XG01L-01",
		.mode              = video_modes,
		.num_modes         = ARRAY_SIZE(video_modes),
	}, {
		.interface_pix_fmt = IPU_PIX_FMT_RGB24, // IPU_PIX_FMT_RGBA32
		.mode_str          = "CLAA-WVGA",
		.mode              = video_modes,
		.num_modes         = ARRAY_SIZE(video_modes),
	}, {
		.interface_pix_fmt = IPU_PIX_FMT_RGB666,
		.mode_str          = "PD104VT2",
		.mode              = video_modes,
		.num_modes         = ARRAY_SIZE(video_modes),	  
	}
};

static struct resource mxcfb_resources[] = {
	{
		.flags = IORESOURCE_MEM,
		.start = 0,
		.end   = 0,
	},
};

static void mx53_regiboard_init_fb(int lcd_id, int fb_dev_id)
{
	const int kDevNum = kLcdAmount; // ARRAY_SIZE(mxc_fb_devices);
	const int kLCDNum = ARRAY_SIZE(fb_data);
  const struct LCDControl *lcd_ctl;
	if (fb_dev_id >= kDevNum)
		fb_dev_id = kDevNum - 1;
	if (lcd_id >= kLCDNum) {
		printk("Regiboard with not supported LCD!\n");
		return;
	}
	printk("Regiboard LCD setup: %s\n", fb_data[lcd_id].mode_str);
	// LVDS disp  (X27): fb_dev_id = 0
	lcd_ctl = &kLCDConfiguration[lcd_id];
	if (gpio_request(LVDS0_DPS, "lvds_dps") == 0 &&
	    gpio_direction_output(LVDS0_DPS, lcd_ctl->rsc) == 0) {
  	printk("* REV pin: %d\n", lcd_ctl->rsc);
	}
	if (gpio_request(LVDS0_SEL68, "lvds_sel68") == 0 &&
	    gpio_direction_output(LVDS0_SEL68, lcd_ctl->sel68) == 0) {
  	printk("* SEL68 pin: %d\n", lcd_ctl->sel68);
	}
	// Paral disp (X26): ...
	// ...        (X25): ...
	mxc_fb_devices[fb_dev_id].num_resources = ARRAY_SIZE(mxcfb_resources);
	mxc_fb_devices[fb_dev_id].resource      = mxcfb_resources;
	mxc_register_device(&mxc_fb_devices[fb_dev_id], &fb_data[lcd_id]);
}

static int mx53_detect_lcd_model()
{
  int id;
  printk("Regiboard trying to detect LCD:\n");
  for (id = 0; id < kLcdAmount; id++) {
    printk("* %d - %s\n", id, fb_data[id].mode_str);
    if (strstr(saved_command_line, fb_data[id].mode_str) != 0)
      return id;
  }
  printk("Regiboard LCD was not detected!\n");
  return kCLAA_WVGA;
}
/* NAND ***********************************************************************/
// Micron MT29F4G08ABAEA: 512Mb, 8-bit, 3.3V
// * Type.: SLC
// * Page.: 4096 + 224
// * Block: 64 pages
// * ECC..: 8-bit per 540 bytes of data
// * LUN..: 1
#define CLEAR_REG(addr, out, mask, offs) \
	(out(addr) & ~(mask << offs))
#define UP_REG(addr, getter, setter, mask, offs, value) \
	setter(addr, CLEAR_REG(addr, getter, mask, offs) | ((value) << offs))
// EIM	
#define EIM_CS0GCR1 0x0  // Chip Select 0. General Configuration Register 1
#define EIM_CS0GCR2 0x4  // Chip Select 0. General Configuration Register 2

#define EIM_CS0GCR1_GET(addr)      __raw_readl((u32)addr + EIM_CS0GCR1)
#define EIM_CS0GCR1_SET(addr, val) __raw_writel(val, (u32)addr + EIM_CS0GCR1)
#define EIM_CS0GCR2_GET(addr)      __raw_readl((u32)addr + EIM_CS0GCR2)
#define EIM_CS0GCR2_SET(addr, val) __raw_writel(val, (u32)addr + EIM_CS0GCR2)
	
#define EIM_CS0GCR1_DSZ(addr, val) \
	UP_REG(addr, EIM_CS0GCR1_GET, EIM_CS0GCR1_SET, 0x7, 16, val)
#define EIM_CS0GCR1_MUM(addr, val) \
	UP_REG(addr, EIM_CS0GCR1_GET, EIM_CS0GCR1_SET, 0x1, 3,  val)
#define EIM_CS0GCR1_CSEN(addr, val) \
	UP_REG(addr, EIM_CS0GCR1_GET, EIM_CS0GCR1_SET, 0x1, 0,  val)
#define EIM_CS0GCR2_MUX16_BYP_GRANT(addr, val) \
	UP_REG(addr, EIM_CS0GCR2_GET, EIM_CS0GCR2_SET, 0x1, 12, val)
// M4IF
#define M4IF_GENP 0xC

#define M4IF_GENP_GET(addr)      __raw_readl((u32)addr + M4IF_GENP)
#define M4IF_GENP_SET(addr, val) __raw_writel(val, (u32)addr + M4IF_GENP)

#define M4IF_GENP_MM(addr, val) \
	UP_REG(addr, M4IF_GENP_GET, M4IF_GENP_SET, 0x1, 0, val)
// NFC
#define NFC_CONFIG2 0x24
#define NFC_CONFIG3 0x28

#define NFC_CONFIG2_GET(addr)      __raw_readl((u32)addr + NFC_CONFIG2)
#define NFC_CONFIG2_SET(addr, val) __raw_writel(val, (u32)addr + NFC_CONFIG2)
#define NFC_CONFIG3_GET(addr)      __raw_readl((u32)addr + NFC_CONFIG3)
#define NFC_CONFIG3_SET(addr, val) __raw_writel(val, (u32)addr + NFC_CONFIG3)

#define NFC_SPAS(addr, val) \
	UP_REG(addr, NFC_CONFIG2_GET, NFC_CONFIG2_SET, 0xFF, 16, val / 2)
#define NFC_PPB(addr, val) \
	UP_REG(addr, NFC_CONFIG2_GET, NFC_CONFIG2_SET, 0x3, 8, val)
#define NFC_ECC_MODE(addr, val) \
	UP_REG(addr, NFC_CONFIG2_GET, NFC_CONFIG2_SET, 0x3, 6, val)
#define NFC_ECC_EN(addr, val) \
	UP_REG(addr, NFC_CONFIG2_GET, NFC_CONFIG2_SET, 0x1, 3, val)
	
#define NFC_NUM_OF_DEV(addr, val) \
	UP_REG(addr, NFC_CONFIG3_GET, NFC_CONFIG3_SET, 0x7, 12, val)	
#define NFC_FW(addr, val) \
	UP_REG(addr, NFC_CONFIG3_GET, NFC_CONFIG3_SET, 0x1, 3, val)
#define NFC_TOO(addr, val) \
	UP_REG(addr, NFC_CONFIG3_GET, NFC_CONFIG3_SET, 0x1, 2, val)
#define NFC_ADD_OP(addr, val) \
	UP_REG(addr, NFC_CONFIG3_GET, NFC_CONFIG3_SET, 0x3, 0, val)

static struct mtd_partition nand_flash_partitions[] = {
#ifdef MX53_REGIBOARD_V1
  /* MX53 ROM require the boot FCB/DBBT support which need
   * more space to store such info on NAND boot partition.
   * 16M should cover all kind of NAND boot support on MX53.
   */
	{
		.name   = "bootloader",
		.offset = 0,
		.size   = 0x000500000
	},
	{
		.name   = "config",
		.offset = 0x000500000,
		.size   = 0x000100000
	}, {
		.name   = "kernel",
		.offset = 0x000600000,
		.size   = 10 * 1024 * 1024
	}, {
#else
  {
		.name   = "kernel",
		.offset = 0,
		.size   = 10 * 1024 * 1024
	}, {
#endif
		.name   = "rootfs",
		.offset = MTDPART_OFS_APPEND,
		.size   = 128 * 1024 * 1024
	}, {
		.name   = "storage",
		.offset = MTDPART_OFS_APPEND,
		.size   = MTDPART_SIZ_FULL	
	},
};

static int mxc_mt29f_nand_init(void)
{
//	u32 i, reg;
	void __iomem *m4if_base;
	void __iomem *eim_base;
	void __iomem *nfc_base;
	printk("Regiboard NFC setup...\n");
	// EIM signals:
	// * WEIM_A[26:16]   | Address Bus MSB      | Output |
	// * WEIM_D[31:16]   | Data MSB             | I/O    |
	// * WEIM_DA[15:0]   | Address/Data Bus LSB | I/O    |
	// * EIM_NFC_D[15:0] | Data LSB             | I/O    |
	// ---------------------------------------------------
	// WEIM_DA   - NFC data lines can be multiplexed on these lines as well
	// EIM_NFC_D - The EIM LSB data signals can be shared with the NFC
	//             data lines through a multiplexor in the EXTMC module, so
	//             the EIM LSB data signals can be routed out through either
	//             the EIM_NFC_D[15:0] path OR the EIM_DA[15:0] path.
	// ---------------------------------------------------
	// General Purpose Register (M4IF_GPR)
	m4if_base = ioremap(MX53_BASE_ADDR(M4IF_BASE_ADDR), SZ_4K);
	// * MM=1 means that NFC_D is muxed with EIM DATA on NANDF_D (on PATA_DATA[15:0] pins).
	// * MM=0 means that NFC_D is muxed with EIM A/D on EIM_DA[15:1].
	// In this case the corresponding CSxGCR2[12] (MUX16_BYP_GRANT) must also be 
	// set to notify EIM that its address is muxed with NF data.
	// iMX53RM.pdf, page 3492
	M4IF_GENP_MM(m4if_base, 0);
	// EIM Registers
	eim_base = ioremap(MX53_BASE_ADDR(WEIM_BASE_ADDR), SZ_4K);
	// CS0GCR1
	EIM_CS0GCR1_DSZ (eim_base, 0x4); // 100 - 8 bit port resides on DATA[7:0]
	EIM_CS0GCR1_MUM (eim_base, 0x1); // Multiplexed Mode enable
	EIM_CS0GCR1_CSEN(eim_base, 0x0); // Chip select function is disabled
	// CS0GCR2
	// Muxed 16 bypass grant. This bit when asserted causes EIM to bypass the
	// grant/ack. arbitration with NFC (only for 16 bit muxed mode accesses).
	// 0 - EIM waits for grant before driving a 16 bit muxed mode access to the 
	//     memory.
	// 1 - EIM ignores the grant signal and immediately drives a 16 bit muxed
	//     mode access to the memory.
	// iMX53RM.pdf, page 1121
	EIM_CS0GCR2_MUX16_BYP_GRANT(eim_base, 0);
	// NFC_CONFIGURATION
	nfc_base = ioremap(MX53_BASE_ADDR(NFC_BASE_ADDR), SZ_4K);
	NFC_SPAS      (nfc_base, 218); // 218 Spare size
	NFC_PPB       (nfc_base, 1);   // 1 - 64 pages per block
	NFC_ECC_MODE  (nfc_base, 1);   // 1 - set 8bit ECC
	NFC_ECC_EN    (nfc_base, 1);   // 1 - enable ECC
	NFC_NUM_OF_DEV(nfc_base, 0);   // 0 - 1 NAND device
	NFC_FW        (nfc_base, 1);   // 1 - Nand Flash IO width: 8bit
	NFC_TOO       (nfc_base, 0);   // 0 - Only 1 device is connected to all CS lines.
	NFC_ADD_OP    (nfc_base, 0);   // 0 - NFC will use only address_group0 (i.e NAND_ADD0,NAND_ADD8)
	iounmap(nfc_base);
	iounmap(m4if_base);
	iounmap(eim_base);
	return 0;
}

static struct flash_platform_data mxc_nand_data = {
	.parts    = nand_flash_partitions,
	.nr_parts = ARRAY_SIZE(nand_flash_partitions),
	.width    = 1,
	.init     = mxc_mt29f_nand_init,
};
/* NOR ************************************************************************/
// [Adesto Tech] AT45DB321D-SU: 32Mb, 2.7V to 3.6V, (66 MHz)
// Bus        : CSPI
// Page size  : 512 - 528 bytes
// Pages      : 8,192
// Page erase : 512 bytes
// Block erase: 4KB
#define NOR_ALIAS  "mxc_dataflash"
#define NOR_CLK_HZ (25 * 1000 * 1000) // max spi clock (SCK) speed in HZ

static struct mtd_partition mxc_dataflash_partitions[] = {
	{
	 .name   = "bootloader",
	 .offset = 0,
// 	 .size   = (1986 * 528), // ~ 1 MByte (page 528 bytes)
	 .size   = (2048 * 512), // 1 MByte (page 512 bytes)
	},
	{
	 .name   = "config",
	 .offset = MTDPART_OFS_APPEND,
	 .size   = MTDPART_SIZ_FULL,
	},
};

static struct flash_platform_data mxc_spi_flash_data[] = {
	{
   .type     = "at45db321d",
	 .name     = NOR_ALIAS,
	 .parts    = mxc_dataflash_partitions,
	 .nr_parts = ARRAY_SIZE(mxc_dataflash_partitions),
	},
};
/* PSU (Power Supply Unit) termo-sensor ***************************************/
// Model: lm92
// Bus  : I2C-1
// Addr : 0x4b or 0x48
static struct i2c_board_info mxc_i2c1_board_info[] = {
	{
		.type = "lm92",
		.addr = 0x4b,
	},
	{
		.type = "lm92",
		.addr = 0x48,
	},
};
/* RTC ************************************************************************/
// Model: DS3231MZ+
// Bus  : I2C-3
// Addr : 0x68
#define RTC_INT GPIO_ID(7,11) // GPIO-7.11 -> _INT_RTC
#define RTC_RST GPIO_ID(7,12) // GPIO-7.12 -> _RST_RTC

static struct i2c_board_info mxc_i2c3_board_info[] = {
	{
		.type = "ds3231",
		.addr = 0x68,
		.irq  = gpio_to_irq(RTC_INT)
	},
};
/* SD *************************************************************************/
#define SD1_CD GPIO_ID(6,17) // GPIO-6.17 -> SD1_CD (MX53_PAD_PATA_DIOW__GPIO6_17)
#define SD1_WP GPIO_ID(6,18) // GPIO-6.18 -> SD1_WP

static unsigned int sdhc_get_card_det_status(struct device *dev) {
  unsigned int sd_cd;
  sd_cd = gpio_get_value(SD1_CD);
  // i.MX53 board v.2
  // Card detection pin is not connected to card slot, so we must always return
  // state of inserted card (0)
	return 0;
}

static unsigned int sdhc_get_card_wp_status(struct device *dev) {
  unsigned int sd_wp;
  sd_wp = gpio_get_value(SD1_WP);
  // i.MX53 board v.2
  // Similar to SD1_CD
	return 0;
}

static struct mxc_mmc_platform_data mmc1_data = {
	.ocr_mask            = MMC_VDD_27_28 | MMC_VDD_28_29 | 
	                       MMC_VDD_29_30 | MMC_VDD_31_32,
	.caps                = MMC_CAP_4_BIT_DATA,
	.min_clk             = 400000,
	.max_clk             = 50000000,
	.card_fixed          = 0,
	.card_inserted_state = 0,
	.status              = sdhc_get_card_det_status,
	.wp_status           = sdhc_get_card_wp_status,
	.clock_mmc           = "esdhc_clk",
	.power_mmc           = NULL,
};

static void mx53_regiboard_init_sd(void) {
	mxcsdhc1_device.resource[2].start = gpio_to_irq(SD1_CD);
	mxcsdhc1_device.resource[2].end   = gpio_to_irq(SD1_CD);
}
/* Setup of clocks ************************************************************/
void mx53_regigraf_clock_init(void) {
//	printk("Regiboard clocks init...\n");
}
/* Setup of specific pins *****************************************************/
void mx53_regigraf_io_init(void) {
	printk("Regiboard IOMUX init...\n");
  // ---------------------------------------------------------------------------
	// For more information about IOMUX look at: iMX53RM.pdf (page 221);
	// Descriptions:
	// * [X] - Alt mode number
	// * mxc_iomux_set_input(<Register address>, <Daisy chain value>)
	mxc_iomux_v3_setup_multiple_pads(iomux_pads, ARRAY_SIZE(iomux_pads));
	// RTC gpio
	if (gpio_request(RTC_RST, "rtc_reset") == 0)
		gpio_direction_input(RTC_RST);
	if (gpio_request(RTC_INT, "rtc_irq") == 0)
		gpio_direction_input(RTC_INT);
  // SD gpio
  if (gpio_request(SD1_CD, "sd1-cd") == 0)
  	gpio_direction_input(SD1_CD);
  if (gpio_request(SD1_WP, "sd1-wp") == 0)
  	gpio_direction_input(SD1_WP);
}
/* Setup SPI devices **********************************************************/
static struct mxc_spi_master mxc_ecspi_data = {
  .maxchipselect = 4,
  .spi_version   = 23, // version 23 - means ECSPI (high frequency <= 52 Mbps)
};

static struct mxc_spi_master mxc_cspi_data = {
  .maxchipselect = 4,
  .spi_version   = 7, // version 7 - means CSPI (<= 26 Mbps)
};
// ECSPI-1
static struct spi_board_info mxc_ecspi_device[] __initdata = {
	{ // Touchscreen
    // for debug we can use modalias = "spidev" -> /dev/(spidev1.0 | spidev0.0)
		.modalias        = TS_MODEL, 
		.platform_data   = &ads_info,
		.mode            = SPI_MODE_0,
		.irq             = gpio_to_irq(TS_PENIRQ),
		.max_speed_hz    = TS_SCK_HZ, // max: 52 Mbps
#ifdef MX53_SPI2GPIO_TOUCH
		.bus_num         = 0, // SPI over GPIO
#else
		.bus_num         = 1, // ECSPI-1
#endif
		.chip_select     = 0, // ECSPI-1.SS0
		.controller_data = (void *)TS_CS,
	},
};
// CSPI
static struct spi_board_info mxc_cspi_device[] __initdata = {
#ifndef MX53_REGIBOARD_V1
	{ // NOR
    .modalias      = NOR_ALIAS,
    .max_speed_hz  = NOR_CLK_HZ, // max: 26 Mbps
    .bus_num       = 3, // CSPI
    .chip_select   = 0, // CSPI.SS0
    .platform_data = &mxc_spi_flash_data[0],
	},
#endif
};

/* Setup of specific devices **************************************************/
static struct platform_device mx53_diagnostic_device = {
	.name          = MX53_DIAGNOSTIC_DRV_NAME,
	.id            = -1,
	.num_resources = 0,
	.resource      = 0,
};

void mx53_regiboard_fixup(int fb_mem, int mem_start) {
	if (!fb_mem)
		return;
	mxcfb_resources[0].start = gpu_data.enable_mmu ? mem_start :
	                                                 gpu_device.resource[5].end + 1;
	mxcfb_resources[0].end   = mxcfb_resources[0].start + fb_mem - 1;
}

void mx53_regigraf_devices(void) {
	printk("Regiboard devices init...\n");
	/* iMX53 diagnostic interface */
// 	platform_device_register(&mx53_diagnostic_device);
	mxc_register_device(&imx_ahci_device_hwmon, NULL);
	/* I2C devices */
	i2c_register_board_info(0, mxc_i2c1_board_info, ARRAY_SIZE(mxc_i2c1_board_info));
  i2c_register_board_info(2, mxc_i2c3_board_info, ARRAY_SIZE(mxc_i2c3_board_info));
	/* SPI devices */
	// The i.MX53 chip has three CSPI interfaces: one CSPI and two ECSPI
	// The sequence of functions calls is very important!
	// 1. mxc_register_device     - initialisation of SPI bus
	// 2. spi_register_board_info - registration of devices on this bus
  mxc_register_device(&mxcspi1_device, &mxc_ecspi_data); // ECSPI-1
  mxc_register_device(&mxcspi2_device, &mxc_ecspi_data); // ECSPI-2
  mxc_register_device(&mxcspi3_device, &mxc_cspi_data);  // CSPI
  // Register CSPI devices
  spi_register_board_info(mxc_cspi_device, ARRAY_SIZE(mxc_cspi_device));
  // Register ECSPI devices
  spi_register_board_info(mxc_ecspi_device, ARRAY_SIZE(mxc_ecspi_device));
#ifdef MX53_SPI2GPIO_TOUCH
  platform_device_register(&spi_gpio_device);
#endif
	/* PWM backlight */
	mxc_register_device(&mxc_pwm1_device, NULL);
	mxc_register_device(&mxc_pwm1_backlight_device, &mxc_pwm_backlight_data);
  /* SD */
 	mx53_regiboard_init_sd();
	mxc_register_device(&mxcsdhc1_device, &mmc1_data);
  /* NAND */
	mxc_register_device(&mxc_nandv2_mtd_device, &mxc_nand_data);
	/* LCD */
//	mx53_regiboard_init_fb(kG150XGE_L04, 0); // 0 - LVDS; Main 15" display
//	mx53_regiboard_init_fb(kG150XG01, 0); // 0 - LVDS; Alt_0 15" display
//	mx53_regiboard_init_fb(kNLB150XG01L_01, 0); // 0 - LVDS; Alt_1 15" display
  mx53_regiboard_init_fb(mx53_detect_lcd_model(), 0);
}
