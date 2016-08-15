/*
 * Copyright 2016 DRS.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef LIB_MNE_HELPER_H_
#define LIB_MNE_HELPER_H_

/**
 * @def MHZ_SCALE
 * @brief Scale to convert Hz to MHz.
 *
 * MHZ_SCALE is defined as 1,000,000 for converting from Hz to
 * MHz.
 */
#define MHZ_SCALE 1000000

/**
 * @def ENABLE_RX_STREAM_MNE
 * @brief Mnemonic command to enable/disable streaming.
 *
 * ENABLE_RX_STREAM_MNE prepares a formatted string for a
 * stringstream object to create. This mnemonic will
 * enable/disable Vita49 streaming to a host machine.
*/
#define ENABLE_RX_STREAM_MNE(enable) "OPM" << enable << ";"

/**
 * @def ENABLE_TX_STREAM_MNE
 * @brief Mnemonic command to enable/disable streaming.
 *
 * ENABLE_TX_STREAM_MNE prepares a formatted string for a
 * stringstream object to create. This mnemonic will
 * enable/disable data streaming from a host machine.
*/
#define ENABLE_TX_STREAM_MNE(enable) "ISM" << enable << ";"

/**
 * @def SAMPLE_RATE_CMD
 * @brief Mnemonic command to change the radio's sample rate.
 *
 * SAMPLE_RATE_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will change the sample rate of
 * the radio.  Expects Msps.
*/
#define SAMPLE_RATE_CMD(spr) "SPR" << spr << ";"

/**
 * @def SAMPLE_RATE_QRY
 * @brief Mnemonic command to query the radio's sample rate.
 *
 * SAMPLE_RATE_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return back the
 * sample rate the radio is using.  Returns Msps.
*/
#define SAMPLE_RATE_QRY "SPR?"

/**
 * @def SAMPLE_RATE_HLP
 * @brief Mnemonic command to query available sample rates.
 *
 * SAMPLE_RATE_HLP prepares a formatted string for a stringstream
 * object to create. This mnemonic will return all available
 * sample rates that the radio supports at the current bandwidth.
*/
#define SAMPLE_RATE_HLP "?SPR"

/**
 * @def POWER_CMD
 * @brief Mnemonic command to change the radio's transmit power.
 *
 * POWER_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will change the transmit
 * power for the radio. Expects dBm.
*/
#define POWER_CMD(txp) "TXP" << txp << ";"

/**
 * @def POWER_QRY
 * @brief Mnemonic command to query the radio's sample rate.
 *
 * POWER_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return back the
 * transmit power of the radio. Returns dBm.
*/
#define POWER_QRY "TXP?"

/**
 * @def ATTENUATION_CMD
 * @brief Mnemonic command to change the radio's attenuation.
 *
 * ATTENUATION_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will change the amount of
 * attenuation for the radio. Expects dB.
*/
#define ATTENUATION_CMD(att) "ATT" << att << ";"

/**
 * @def ATTENUATION_QRY
 * @brief Mnemonic command to query the radio's attenuation.
 *
 * ATTENUATION_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return back the
 * amount of attenuation for the radio. Returns dB.
*/
#define ATTENUATION_QRY "ATT?"

/**
 * @def FREQUENCY_CMD
 * @brief Mnemonic command to change the radio's frequency.
 *
 * FREQUENCY_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will tune both the tuner and
 * ddc/duc to get the selected frequency. Expects MHz.
*/
#define FREQUENCY_CMD(frq) "FRQ" << frq << ";"

/**
 * @def FREQUENCY_QRY
 * @brief Mnemonic command to query the radio's frequency.
 *
 * FREQUENCY_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will return back the
 * frequency the radio is tuned to. Returns MHz.
*/
#define FREQUENCY_QRY "FRQ?"

/**
 * @def BANDWIDTH_CMD
 * @brief Mnemonic command to change the radio's bandwidth.
 *
 * BANDWIDTH_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will change the analog
 * bandwidth of the radio. Expects MHz.
*/
#define BANDWIDTH_CMD(bwt) "BWT" << bwt << ";"

/**
 * @def BANDWIDTH_QRY
 * @brief Mnemonic command to query the radio's bandwidth.
 *
 * BANDWIDTH_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return the analog
 * bandwidth of the radio. Returns MHz.
*/
#define BANDWIDTH_QRY "BWT?"

/**
 * @def BANDWIDTH_HLP
 * @brief Mnemonic command to query available bandwidths.
 *
 * BANDWIDTH_HLP prepares a formatted string for a stringstream
 * object to create. This mnemonic will return all available
 * analog bandwidth's that the radio supports.
*/
#define BANDWIDTH_HLP "?BWT"

/**
 * @def CHANNEL_CMD
 * @brief Mnemonic command to change the board for control.
 *
 * CHANNEL_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic changes which board you are
 * addressing.  For example to address the transmitter on a
 * PicoTransciever you would use RCH 3.
*/
#define CHANNEL_CMD(chn) "RCH" << chn << ";"

/**
 * @def SIGINT_CHANNEL
 * @brief Numeric value for the SigInt channel on a Pico.
 *
 * SIGINT_CHANNEL always points to a SigInt channel on a Pico
 * radio.
 */
#define SIGINT_CHANNEL 1

/**
 * @def TX_CHANNEL
 * @brief Numeric value for the TX channel on a PicoTXR.
 *
 * TX_CHANNEL always points to the TX channel on a PicoTXR
 * radio.
 */
#define TX_CHANNEL 3

/**
 * @def NETWORKING_CMD
 * @brief Mnemonic command to change the streaming configuration.
 *
 * NETWORKING_CMD prepares a formatted string for a stringstream
 * object to create.  This mnemonic is used to setup the networking
 * for streaming data to or from the Pico.
*/
#define NETWORKING_CMD(addr,port)  "SIP" << addr << "," << port \
                                          << ";"

/**
 * @def STREAM_ID_QRY
 * @brief Mnemonic command to query the stream ids of the radio.
 *
 * STREAM_ID_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return the Vita49 stream
 * ids which can be output from the radio.
*/
#define STREAM_ID_QRY "VID?"

/**
 * @def ENDIAN_CMD
 * @brief Mnemonic command to change endianness of the Pico.
 *
 * ENDIAN_CMD prepares a formatted string for a stringstream
 * object to create. This mnemonic will change the endianness
 * of the output data stream.
*/
#define ENDIAN_CMD(end) "END" << end

/**
 * @def HELP_QRY
 * @brief Mnemonic query to list all available mnemonics.
 *
 * HELP_QRY prepares a formatted string for a stringstream
 * object to create. This mnemonic will return a list of
 * all mnemonics available on the radio.
*/
#define HELP_QRY "HLP?"

/**
 * @def SEND_STR
 * @brief Formats a string to be sent to the radio.
 *
 * SEND_STR prepares a command or query to be sent to the radio
 * by concatenating string together and shipping them out in one
 * stringstream.
 */
#define SEND_STR(s,a) {s.str("");s<<a;send_message(s.str());\
                                        s.str("");}

#endif /* LIB_MNE_HELPER_H_ */
