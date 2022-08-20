#include "ds18b20_task.h"
#include "cpu_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file onewire.h
 *
 *  Routines to access devices using the Dallas Semiconductor 1-Wire(tm)
 *  protocol.
 */
 




MessageBufferHandle_t ds18b20degC;   //换算2831 = 28.31

extern MessageBufferHandle_t tcp_send_data;
extern char * tcprx_buffer;
extern uint32_t sse_data[sse_len];


/** Type used to hold all 1-Wire device ROM addresses (64-bit) */
typedef uint64_t onewire_addr_t;
 


/** @file ds18b20.h
 *
 *  Communicate with the DS18B20 family of one-wire temperature sensor ICs.
 *
 */
 
typedef onewire_addr_t ds18b20_addr_t;
 
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
 
/** An address value which can be used to indicate "any device on the bus" */
#define DS18B20_ANY ONEWIRE_NONE
 
gpio_config_t io_config;


typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;
 
 /*
//set 80m hz /20 ,4m hz tick 
void ds18b20_timer_init(void)
{
    int group = 0;
    int timer = 1;
    // Select and initialize basic parameters of the timer 
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = TIMER_AUTORELOAD_DIS,
    }; // default clock source is APB
    timer_init(group, timer, &config);
 
    // Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm 
    timer_set_counter_value(group, timer, 0);
 
    timer_start(group, timer);
}
 
void sdk_os_delay_us(uint16_t us) {
    uint64_t timer_counter_value = 0;
    uint64_t timer_counter_update = 0;
    //uint32_t delay_ccount = 200 * us;
    
    timer_get_counter_value(0,1,&timer_counter_value);
    timer_counter_update = timer_counter_value + (us << 2);
    do {
        timer_get_counter_value(0,1,&timer_counter_value);
    } while (timer_counter_value < timer_counter_update);
}
*/

// Waits up to `max_wait` microseconds for the specified pin to go high.
// Returns true if successful, false if the bus never comes high (likely
// shorted).
static inline bool _onewire_wait_for_bus(int pin, int max_wait) {
    bool state;
    for (int i = 0; i < ((max_wait + 4) / 5); i++) {
        if (gpio_get_level(pin)) break;
        cpu_timer0_delay_us(5);
    }
    state = gpio_get_level(pin);
    // Wait an extra 1us to make sure the devices have an adequate recovery
    // time before we drive things low again.
    cpu_timer0_delay_us(1);
    return state;
}
 
// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return false;
//
// Returns true if a device asserted a presence pulse, false otherwise.
//
bool onewire_reset(int pin) {
    bool r;
    gpio_set_direction(pin,GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(pin, 1);
    // wait until the wire is high... just in case
    if (!_onewire_wait_for_bus(pin, 250)) return false;
 
    gpio_set_level(pin, 0);
    cpu_timer0_delay_us(480);
 
    taskENTER_CRITICAL(&mux);
    gpio_set_level(pin, 1); // allow it to float
    cpu_timer0_delay_us(70);
    r = !gpio_get_level(pin);
    taskEXIT_CRITICAL(&mux);
 
    // Wait for all devices to finish pulling the bus low before returning
    if (!_onewire_wait_for_bus(pin, 410)) return false;
 
    return r;
}
 
static bool _onewire_write_bit(int pin, bool v) {
    if (!_onewire_wait_for_bus(pin, 10)) return false;
    if (v) {
        taskENTER_CRITICAL(&mux);
        gpio_set_level(pin, 0);  // drive output low
        cpu_timer0_delay_us(10);
        gpio_set_level(pin, 1);  // allow output high
        taskEXIT_CRITICAL(&mux);
        cpu_timer0_delay_us(55);
    } else {
        taskENTER_CRITICAL(&mux);
        gpio_set_level(pin, 0);  // drive output low
        cpu_timer0_delay_us(65);
        gpio_set_level(pin, 1); // allow output high
        taskEXIT_CRITICAL(&mux);
    }
    cpu_timer0_delay_us(1);
 
    return true;
}
 
static int _onewire_read_bit(int pin) {
    int r;
 
    if (!_onewire_wait_for_bus(pin, 10))
        return -1;
    taskENTER_CRITICAL(&mux);
    gpio_set_level(pin, 0);
    cpu_timer0_delay_us(2);
    gpio_set_level(pin, 1);  // let pin float, pull up will raise
    cpu_timer0_delay_us(11);
    r = gpio_get_level(pin);  // Must sample within 15us of start
    taskEXIT_CRITICAL(&mux);
    cpu_timer0_delay_us(48);
 
    return r;
}
 
// Write a byte. The writing code uses open-drain mode and expects the pullup
// resistor to pull the line high when not driven low.  If you need strong
// power after the write (e.g. DS18B20 in parasite power mode) then call
// onewire_power() after this is complete to actively drive the line high.
//
bool onewire_write(int pin, uint8_t v) {
    uint8_t bitMask;
 
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (!_onewire_write_bit(pin, (bitMask & v))) {
            return false;
        }
    }
    return true;
}
 
bool onewire_write_bytes(int pin, const uint8_t *buf, size_t count) {
    size_t i;
 
    for (i = 0; i < count; i++) {
        if (!onewire_write(pin, buf[i])) {
            return false;
        }
    }
    return true;
}
 
// Read a byte
//
int onewire_read(int pin) {
    uint8_t bitMask;
    int r = 0;
    int bit;
 
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        bit = _onewire_read_bit(pin);
        if (bit < 0) {
            return -1;
        } else if (bit) {
            r |= bitMask;
        }
    }
    return r;
}
 
bool onewire_read_bytes(int pin, uint8_t *buf, size_t count) {
    size_t i;
    int b;
 
    for (i = 0; i < count; i++) {
        b = onewire_read(pin);
        if (b < 0) return false;
        buf[i] = b;
    }
    return true;
}
 
bool onewire_select(int pin, onewire_addr_t addr) {
    uint8_t i;
 
    if (!onewire_write(pin, ONEWIRE_SELECT_ROM)) {
        return false;
    }
 
    for (i = 0; i < 8; i++) {
        if (!onewire_write(pin, addr & 0xff)) {
            return false;
        }
        addr >>= 8;
    }
 
    return true;
}
 
bool onewire_skip_rom(int pin) {
    return onewire_write(pin, ONEWIRE_SKIP_ROM);
}
 
bool onewire_power(int pin) {
    // Make sure the bus is not being held low before driving it high, or we
    // may end up shorting ourselves out.
    if (!_onewire_wait_for_bus(pin, 10)) return false;
 
    gpio_set_direction(pin,GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 1);
 
    return true;
}
 
void onewire_depower(int pin) {
    gpio_set_direction(pin,GPIO_MODE_INPUT_OUTPUT_OD);
}
 
void onewire_search_start(onewire_search_t *search) {
    // reset the search state
    memset(search, 0, sizeof(*search));
}
 
void onewire_search_prefix(onewire_search_t *search, uint8_t family_code) {
    uint8_t i;
 
    search->rom_no[0] = family_code;
    for (i = 1; i < 8; i++) {
        search->rom_no[i] = 0;
    }
    search->last_discrepancy = 64;
    search->last_device_found = false;
}
 
// Perform a search. If the next device has been successfully enumerated, its
// ROM address will be returned.  If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then ONEWIRE_NONE is returned.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return 1 : device found, ROM number in ROM_NO buffer
//        0 : device not found, end of search
//
onewire_addr_t onewire_search_next(onewire_search_t *search, int pin) {
    //TODO: add more checking for read/write errors
    uint8_t id_bit_number;
    uint8_t last_zero, search_result;
    int rom_byte_number;
    int8_t id_bit, cmp_id_bit;
    onewire_addr_t addr;
    unsigned char rom_byte_mask;
    bool search_direction;
 
    // initialize for search
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = 0;
   
    // if the last call was not the last one
    if (!search->last_device_found) {
        // 1-Wire reset
        if (!onewire_reset(pin)) {
            // reset the search
            search->last_discrepancy = 0;
            search->last_device_found = false;
            return ONEWIRE_NONE;
        }
 
        // issue the search command
        onewire_write(pin, ONEWIRE_SEARCH);
 
        // loop to do the search
        do {
            // read a bit and its complement
            id_bit = _onewire_read_bit(pin);
            cmp_id_bit = _onewire_read_bit(pin);
 
            // check for no devices on 1-wire
            if ((id_bit < 0) || (cmp_id_bit < 0)) {
                // Read error
                break;
            } else if ((id_bit == 1) && (cmp_id_bit == 1)) {
                break;
            } else {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;  // bit write value for search
                } else {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (id_bit_number < search->last_discrepancy) {
                        search_direction = ((search->rom_no[rom_byte_number] & rom_byte_mask) > 0);
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (id_bit_number == search->last_discrepancy);
                    }
 
                    // if 0 was picked then record its position in LastZero
                    if (!search_direction) {
                        last_zero = id_bit_number;
                    }
                }
 
                // set or clear the bit in the ROM byte rom_byte_number
                // with mask rom_byte_mask
                if (search_direction) {
                    search->rom_no[rom_byte_number] |= rom_byte_mask;
                } else {
                    search->rom_no[rom_byte_number] &= ~rom_byte_mask;
                }
 
                // serial number search direction write bit
                _onewire_write_bit(pin, search_direction);
 
                // increment the byte counter id_bit_number
                // and shift the mask rom_byte_mask
                id_bit_number++;
                rom_byte_mask <<= 1;
 
                // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
                if (rom_byte_mask == 0) {
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        } while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7
 
        // if the search was successful then
        if (!(id_bit_number < 65)) {
            // search successful so set last_discrepancy,last_device_found,search_result
            search->last_discrepancy = last_zero;
 
            // check for last device
            if (search->last_discrepancy == 0) {
                search->last_device_found = true;
            }
 
            search_result = 1;
        }
    }
 
    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !search->rom_no[0]) {
        search->last_discrepancy = 0;
        search->last_device_found = false;
        return ONEWIRE_NONE;
    } else {
        addr = 0;
        for (rom_byte_number = 7; rom_byte_number >= 0; rom_byte_number--) {
            addr = (addr << 8) | search->rom_no[rom_byte_number];
        }
        //printf("Ok I found something at %08x%08x...\n", (uint32_t)(addr >> 32), (uint32_t)addr);
    }
    return addr;
}
 
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//
 
#if ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};
 
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif
 
//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t onewire_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
 
    while (len--) {
        crc = pgm_read_byte(dscrc_table + (crc ^ *data++));
    }
    return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t onewire_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    
    while (len--) {
        uint8_t inbyte = *data++;
        for (int i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}
#endif
 
// Compute the 1-Wire CRC16 and compare it against the received CRC.
// Example usage (reading a DS2408):
//    // Put everything in a buffer so we can compute the CRC easily.
//    uint8_t buf[13];
//    buf[0] = 0xF0;    // Read PIO Registers
//    buf[1] = 0x88;    // LSB address
//    buf[2] = 0x00;    // MSB address
//    WriteBytes(net, buf, 3);    // Write 3 cmd bytes
//    ReadBytes(net, buf+3, 10);  // Read 6 data bytes, 2 0xFF, 2 CRC16
//    if (!CheckCRC16(buf, 11, &buf[11])) {
//        // Handle error.
//    }     
//          
// @param input - Array of bytes to checksum.
// @param len - How many bytes to use.
// @param inverted_crc - The two CRC16 bytes in the received data.
//                       This should just point into the received data,
//                       *not* at a 16-bit integer.
// @param crc - The crc starting value (optional)
// @return 1, iff the CRC matches.
bool onewire_check_crc16(const uint8_t* input, size_t len, const uint8_t* inverted_crc, uint16_t crc_iv) {
    uint16_t crc = ~onewire_crc16(input, len, crc_iv);
    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}
 
// Compute a Dallas Semiconductor 16 bit CRC.  This is required to check
// the integrity of data received from many 1-Wire devices.  Note that the
// CRC computed here is *not* what you'll get from the 1-Wire network,
// for two reasons:
//   1) The CRC is transmitted bitwise inverted.
//   2) Depending on the endian-ness of your processor, the binary
//      representation of the two-byte return value may have a different
//      byte order than the two bytes you get from 1-Wire.
// @param input - Array of bytes to checksum.
// @param len - How many bytes to use.
// @param crc - The crc starting value (optional)
// @return The CRC16, as defined by Dallas Semiconductor.
uint16_t onewire_crc16(const uint8_t* input, size_t len, uint16_t crc_iv) {
    uint16_t crc = crc_iv;
    static const uint8_t oddparity[16] =
        { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };
 
    uint16_t i;
    for (i = 0; i < len; i++) {
      // Even though we're just copying a byte from the input,
      // we'll be doing 16-bit computation with it.
      uint16_t cdata = input[i];
      cdata = (cdata ^ crc) & 0xff;
      crc >>= 8;
 
      if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
          crc ^= 0xC001;
 
      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
    return crc;
}
 
/** Find the addresses of all DS18B20 devices on the bus.
 *
 *  Scans the bus for all devices and places their addresses in the supplied
 *  array.  If there are more than `addr_count` devices on the bus, only the
 *  first `addr_count` are recorded.
 *
 *  @param pin         The GPIO pin connected to the DS18B20 bus
 *  @param addr_list   A pointer to an array of ds18b20_addr_t values.  This
 *                     will be populated with the addresses of the found
 *                     devices.
 *  @param addr_count  Number of slots in the `addr_list` array.  At most this
 *                     many addresses will be returned.
 *
 *  @returns The number of devices found.  Note that this may be less than,
 *  equal to, or more than `addr_count`, depending on how many DS18B20 devices
 *  are attached to the bus.
 */
int ds18b20_scan_devices(int pin, ds18b20_addr_t *addr_list, int addr_count);
 
/** Tell one or more sensors to perform a temperature measurement and
 *  conversion (CONVERT_T) operation.  This operation can take up to 750ms to
 *  complete.
 *
 *  If `wait=true`, this routine will automatically drive the pin high for the
 *  necessary 750ms after issuing the command to ensure parasitically-powered
 *  devices have enough power to perform the conversion operation (for
 *  non-parasitically-powered devices, this is not necessary but does not
 *  hurt).  If `wait=false`, this routine will drive the pin high, but will
 *  then return immediately.  It is up to the caller to wait the requisite time
 *  and then depower the bus using onewire_depower() or by issuing another
 *  command once conversion is done.
 *
 *  @param pin   The GPIO pin connected to the DS18B20 device
 *  @param addr  The 64-bit address of the device on the bus.  This can be set
 *               to ::DS18B20_ANY to send the command to all devices on the bus
 *               at the same time.
 *  @param wait  Whether to wait for the necessary 750ms for the DS18B20 to
 *               finish performing the conversion before returning to the
 *               caller (You will normally want to do this).
 *
 *  @returns `true` if the command was successfully issued, or `false` on error.
 */
bool ds18b20_measure(int pin, ds18b20_addr_t addr, bool wait);
 
/** Read the value from the last CONVERT_T operation.
 *
 *  This should be called after ds18b20_measure() to fetch the result of the
 *  temperature measurement.
 *
 *  @param pin     The GPIO pin connected to the DS18B20 device
 *  @param addr    The 64-bit address of the device to read.  This can be set
 *                 to ::DS18B20_ANY to read any device on the bus (but note
 *                 that this will only work if there is exactly one device
 *                 connected, or they will corrupt each others' transmissions)
 *
 *  @returns The temperature in degrees Celsius, or NaN if there was an error.
 */
float ds18b20_read_temperature(int pin, ds18b20_addr_t addr);
 
/** Read the value from the last CONVERT_T operation for multiple devices.
 *
 *  This should be called after ds18b20_measure() to fetch the result of the
 *  temperature measurement.
 *
 *  @param pin         The GPIO pin connected to the DS18B20 bus
 *  @param addr_list   A list of addresses for devices to read.
 *  @param addr_count  The number of entries in `addr_list`.
 *  @param result_list An array of floats to hold the returned temperature
 *                     values.  It should have at least `addr_count` entries.
 *
 *  @returns `true` if all temperatures were fetched successfully, or `false`
 *  if one or more had errors (the temperature for erroring devices will be
 *  returned as NaN).
 */
bool ds18b20_read_temp_multi(int pin, ds18b20_addr_t *addr_list, int addr_count, float *result_list);
 
/** Perform a ds18b20_measure() followed by ds18b20_read_temperature()
 *
 *  @param pin     The GPIO pin connected to the DS18B20 device
 *  @param addr    The 64-bit address of the device to read.  This can be set
 *                 to ::DS18B20_ANY to read any device on the bus (but note
 *                 that this will only work if there is exactly one device
 *                 connected, or they will corrupt each others' transmissions)
 *
 *  @returns The temperature in degrees Celsius, or NaN if there was an error.
 */
float ds18b20_measure_and_read(int pin, ds18b20_addr_t addr);
 
/** Perform a ds18b20_measure() followed by ds18b20_read_temp_multi()
 *
 *  @param pin         The GPIO pin connected to the DS18B20 bus
 *  @param addr_list   A list of addresses for devices to read.
 *  @param addr_count  The number of entries in `addr_list`.
 *  @param result_list An array of floats to hold the returned temperature
 *                     values.  It should have at least `addr_count` entries.
 *
 *  @returns `true` if all temperatures were fetched successfully, or `false`
 *  if one or more had errors (the temperature for erroring devices will be
 *  returned as NaN).
 */
bool ds18b20_measure_and_read_multi(int pin, ds18b20_addr_t *addr_list, int addr_count, float *result_list);
 
/** Read the scratchpad data for a particular DS18B20 device.
 *
 *  This is not generally necessary to do directly.  It is done automatically
 *  as part of ds18b20_read_temperature().
 *
 *  @param pin     The GPIO pin connected to the DS18B20 device
 *  @param addr    The 64-bit address of the device to read.  This can be set
 *                 to ::DS18B20_ANY to read any device on the bus (but note
 *                 that this will only work if there is exactly one device
 *                 connected, or they will corrupt each others' transmissions)
 *  @param buffer  An 8-byte buffer to hold the read data.
 *
 *  @returns `true` if the data was read successfully, or `false` on error.
 */
bool ds18b20_read_scratchpad(int pin, ds18b20_addr_t addr, uint8_t *buffer);
 
// The following are obsolete/deprecated APIs
 
typedef struct {
    uint8_t id;
    float value;
} ds_sensor_t;
 
// This method is just to demonstrate how to read 
// temperature from single dallas chip.
void ds18b20_read_single1(uint8_t pin){
 
    gpio_pad_select_gpio(pin);
    gpio_set_direction(pin,GPIO_MODE_INPUT_OUTPUT_OD);
    
    while(1){
        //taskENTER_CRITICAL(&mux);    
        gpio_set_level(pin, 1);
        //sdk_os_delay_us(1000);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(pin, 0);
        //sdk_os_delay_us(1000);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        //taskEXIT_CRITICAL(&mux);
    }
    return;
}
float ds18b20_read_single(uint8_t pin) {
 
    onewire_reset(pin);
    onewire_skip_rom(pin);
    onewire_write(pin, DS18B20_CONVERT_T);
 
    onewire_power(pin);
    vTaskDelay(750 / portTICK_PERIOD_MS);
 
    onewire_reset(pin);
    onewire_skip_rom(pin);
    onewire_write(pin, DS18B20_READ_SCRATCHPAD);
 
    uint8_t get[10];
 
    for (int k=0;k<9;k++){
        get[k]=onewire_read(pin);
        //printf("%02x ",get[k]);
    }
    //printf("\r\n");
 
    //debug("\n ScratchPAD DATA = %X %X %X %X %X %X %X %X %X\n",get[8],get[7],get[6],get[5],get[4],get[3],get[2],get[1],get[0]);
    uint8_t crc = onewire_crc8(get, 8);
 
    if (crc != get[8]){
        debug("CRC check failed: %02X %02X", get[8], crc);
        return 0;
    }
 
    uint16_t temp = get[1] << 8 | get[0];
 
    float temperature;
 
    temperature = (temp * 625.0)/10000;
    return temperature;
}
 
 
// Scan all ds18b20 sensors on bus and return its amount.
// Result are saved in array of ds_sensor_t structure.
uint8_t ds18b20_read_all(uint8_t pin, ds_sensor_t *result);
 
uint8_t ds18b20_read_all(uint8_t pin, ds_sensor_t *result) {
    onewire_addr_t addr;
    onewire_search_t search;
    uint8_t sensor_id = 0;
 
    onewire_search_start(&search);
 
    while ((addr = onewire_search_next(&search, pin)) != ONEWIRE_NONE) {
        uint8_t crc = onewire_crc8((uint8_t *)&addr, 7);
        if (crc != (addr >> 56)){
            debug("CRC check failed: %02X %02X\n", (unsigned)(addr >> 56), crc);
            return 0;
        }
 
        onewire_reset(pin);
        onewire_select(pin, addr);
        onewire_write(pin, DS18B20_CONVERT_T);
 
        onewire_power(pin);
        vTaskDelay(750 / portTICK_PERIOD_MS);
 
        onewire_reset(pin);
        onewire_select(pin, addr);
        onewire_write(pin, DS18B20_READ_SCRATCHPAD);
 
        uint8_t get[10];
 
        for (int k=0;k<9;k++){
            get[k]=onewire_read(pin);
        }
 
        //debug("\n ScratchPAD DATA = %X %X %X %X %X %X %X %X %X\n",get[8],get[7],get[6],get[5],get[4],get[3],get[2],get[1],get[0]);
        crc = onewire_crc8(get, 8);
 
        if (crc != get[8]){
            debug("CRC check failed: %02X %02X\n", get[8], crc);
            return 0;
        }
 
        uint8_t temp_msb = get[1]; // Sign byte + lsbit
        uint8_t temp_lsb = get[0]; // Temp data plus lsb
        uint16_t temp = temp_msb << 8 | temp_lsb;
 
        float temperature;
 
        temperature = (temp * 625.0)/10000;
        //debug("Got a DS18B20 Reading: %d.%02d\n", (int)temperature, (int)(temperature - (int)temperature) * 100);
        result[sensor_id].id = sensor_id;
        result[sensor_id].value = temperature;
        sensor_id++;
    }
    return sensor_id;
}
 
bool ds18b20_measure(int pin, ds18b20_addr_t addr, bool wait) {
    if (!onewire_reset(pin)) {
        return false;
    }
    if (addr == DS18B20_ANY) {
        onewire_skip_rom(pin);
    } else {
        onewire_select(pin, addr);
    }
    taskENTER_CRITICAL(&mux);
    onewire_write(pin, DS18B20_CONVERT_T);
    // For parasitic devices, power must be applied within 10us after issuing
    // the convert command.
    onewire_power(pin);
    taskEXIT_CRITICAL(&mux);
 
    if (wait) {
        os_sleep_ms(750);
        onewire_depower(pin);
    }
 
    return true;
}
 
bool ds18b20_read_scratchpad(int pin, ds18b20_addr_t addr, uint8_t *buffer) {
    uint8_t crc;
    uint8_t expected_crc;
 
    if (!onewire_reset(pin)) {
        return false;
    }
    if (addr == DS18B20_ANY) {
        onewire_skip_rom(pin);
    } else {
        onewire_select(pin, addr);
    }
    onewire_write(pin, DS18B20_READ_SCRATCHPAD);
 
    for (int i = 0; i < 8; i++) {
        buffer[i] = onewire_read(pin);
    }
    crc = onewire_read(pin);
 
    expected_crc = onewire_crc8(buffer, 8);
    if (crc != expected_crc) {
        debug("CRC check failed reading scratchpad: %02x %02x %02x %02x %02x %02x %02x %02x : %02x (expected %02x)\n", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], crc, expected_crc);
        return false;
    }
 
    return true;
}
 
float ds18b20_read_temperature(int pin, ds18b20_addr_t addr) {
    uint8_t scratchpad[8];
    int16_t temp;
 
    if (!ds18b20_read_scratchpad(pin, addr, scratchpad)) {
        return NAN;
    }
 
    temp = scratchpad[1] << 8 | scratchpad[0];
 
    float res;
    if ((uint8_t)addr == DS18B20_FAMILY_ID) {
        res = ((float)temp * 625.0)/10000;
    }
    else {
        temp = ((temp & 0xfffe) << 3) + (16 - scratchpad[6]) - 4;
        res = ((float)temp * 625.0)/10000 - 0.25;
    }
    return res;
}
 
float ds18b20_measure_and_read(int pin, ds18b20_addr_t addr) {
    if (!ds18b20_measure(pin, addr, true)) {
        return NAN;
    }
    return ds18b20_read_temperature(pin, addr);
}
 
bool ds18b20_measure_and_read_multi(int pin, ds18b20_addr_t *addr_list, int addr_count, float *result_list) {
    if (!ds18b20_measure(pin, DS18B20_ANY, true)) {
        for (int i=0; i < addr_count; i++) {
            result_list[i] = NAN;
        }
        return false;
    }
    return ds18b20_read_temp_multi(pin, addr_list, addr_count, result_list);
}
 
int ds18b20_scan_devices(int pin, ds18b20_addr_t *addr_list, int addr_count) {
    onewire_search_t search;
    onewire_addr_t addr;
    int found = 0;
 
    onewire_search_start(&search);
    while ((addr = onewire_search_next(&search, pin)) != ONEWIRE_NONE) {
        uint8_t family_id = (uint8_t)addr;
        if (family_id == DS18B20_FAMILY_ID || family_id == DS18S20_FAMILY_ID) {
            if (found < addr_count) {
                addr_list[found] = addr;
            }
            found++;
        }
    }
    return found;
}
 
bool ds18b20_read_temp_multi(int pin, ds18b20_addr_t *addr_list, int addr_count, float *result_list) {
    bool result = true;
 
    for (int i = 0; i < addr_count; i++) {
        result_list[i] = ds18b20_read_temperature(pin, addr_list[i]);
        if (isnan(result_list[i])) {
            result = false;
        }
    }
    return result;
}
 
 

void ds18x20_task(void *arg)
{
    int degC = 0;
    cpu_timer0_init();
    //printf("This is from ds18b20\r\n");
    float t = 0,temp_c = 0.0;
    while(1)
    {
        for(int a = 0; a < 18;a++)    
        {
            t = ds18b20_read_single(CONFIG_DS18B20_SENSOR_GPIO);//pin 8 pull up 10k
            //printf("got temprsure2 %.2f\r\n",t);           
            temp_c = t * 0.986;
            degC = degC + ((int)((temp_c + 0.005) * 100));
            ESP_LOGI("ds18b20", "DS18B20 Sensor reports++ %d deg C\n",degC);
            EventBits_t uxBits = xEventGroupGetBits(APP_event_group);
            if((uxBits & APP_event_30min_timer_BIT) != APP_event_30min_timer_BIT)
            {   
                int deg = ((int)((temp_c + 0.005) * 100));
                printf("DS18B20 Sensor %d deg C\n",deg);
                temp_c = 0;
                if(deg == 0){
                    xEventGroupClearBits(APP_event_group,APP_event_ds18b20_CONNECTED_flags_BIT);
                    sse_data[3] = 0;
                }
                else{
                    xMessageBufferSend(ds18b20degC,&deg,4,portMAX_DELAY);
                    xEventGroupSetBits(APP_event_group,APP_event_ds18b20_CONNECTED_flags_BIT);
                    sse_data[3] = deg | 0x80000000;
                    deg = 0;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(120000));
        }
        degC = degC/18;
        if(degC == 0){
                xEventGroupClearBits(APP_event_group,APP_event_ds18b20_CONNECTED_flags_BIT);
                sse_data[3] = 0;
            }
            else{
                xEventGroupSetBits(APP_event_group,APP_event_ds18b20_CONNECTED_flags_BIT);
                //printf("DS18B20 Sensor reportsAVG %d deg C\n",degC);
                xMessageBufferSend(ds18b20degC,&degC,4,portMAX_DELAY);
                sse_data[3] = degC | 0x80000000;
                degC = 0;
            }
    }
}
