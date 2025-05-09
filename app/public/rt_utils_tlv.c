/*******************************************************************************
 * Copyright (c) redtea mobile.
 * File name   : ber_tlv.c
 * Date        : 2021.01.21
 * Note        :
 * Description :
 * Contributors: RT - create the file
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Sublime text 2
 *******************************************************************************/

#include "rt_utils_tlv.h"


#define rt_utils_make_u32_by_u16(word1, word2)     ((uint32_t)(((uint32_t)(word1) << 16) | (uint16_t)(word2)))

 /**
  * @brief                 get an u16 type from big-endian u08 array
  * @param[in] bytes       The pointer of bytes
  * @return u16       The u16 number
  * @note
 */
uint16_t rt_utils_u08s_to_u16(uint8_t* bytes)
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

/**
 * @brief                 Convert an u16 type to big-endian u08 array
 * @param[in] word        The u16 number
 * @param[out] bytes      The pointer of bytes
 * @return void
 * @note
 */
void rt_utils_u16_to_u08s(uint16_t word, uint8_t* bytes)
{
    bytes[0] = (uint8_t)(word >> 8);
    bytes[1] = (uint8_t)word;
}

/**
 * @brief                 get an u32 type from big-endian u08 array
 * @param[in] bytes       The pointer of bytes
 * @return u32       The u32 number
 * @note
 */
uint32_t rt_utils_u08s_to_u32(uint8_t* bytes)
{
    return (((uint32_t)bytes[0] << 24)
        | ((uint32_t)bytes[1] << 16)
        | ((uint32_t)bytes[2] << 8)
        | bytes[3]);
}

/**
 * @brief                 Convert an u32 type to big-endian u08 array
 * @param[in] dword       The u32 number
 * @param[out] bytes      The pointer of bytes
 * @return void
 * @note
 */
void rt_utils_u32_to_u08s(uint32_t dword, uint8_t* bytes)
{
    bytes[0] = (uint8_t)(dword >> 24);
    bytes[1] = (uint8_t)(dword >> 16);
    bytes[2] = (uint8_t)(dword >> 8);
    bytes[3] = (uint8_t)dword;
}

/**
 * @brief                 Get size of tag
 * @param[in] tag         The pointer to the tag
 * @return uint8_t        The size of tag
 * @note                  
 */
uint8_t rt_bertlv_get_tag_size(const uint8_t* tag)
{
    uint8_t size = 1;

    if ((tag[0] & 0x1F) != 0x1F) {
        return size;
    }

    while ((tag[size] & 0x80) == 0x80) {
        size++;
    }

    return (uint8_t)(size + 1);
}

/**
 * @brief                 Get tag value
 * @param[in] tag         The pointer to the tag
 * @param[out] tag_size   The pointer to the size of tag
 * @return uint8_t        The size of tag
 * @note                  Only support two-bytes tag
 */
uint16_t rt_bertlv_get_tag(uint8_t* tag, uint8_t* tag_size)
{
    uint8_t size;
    uint16_t tag_value;

    size = rt_bertlv_get_tag_size((const uint8_t*)tag);
    if (size == 1) {
        tag_value = tag[0];
    } else {
        tag_value = rt_utils_u08s_to_u16(tag);
    }

    if (tag_size != NULL) {
        *tag_size = size;
    }

    return tag_value;
}

/**
 * @brief                 Get size of length and size of value
 * @param[in] len         The pointer to the length
 * @param[out] len_size   The size of length itself
 * @return uint32_t       The length value(size of value)
 * @note                  
 */
uint32_t rt_bertlv_get_length(uint8_t* len, uint8_t* len_size)
{
    if (len[0] < 0x80) {
        *len_size = 1;
        return len[0];
    }
    
    switch (len[0]) {
        case 0x81:
            *len_size = 2;
            return len[1];
        case 0x82:
            *len_size = 3;
            return rt_utils_u08s_to_u16(&len[1]);
        case 0x83:
            *len_size = 4;
            return rt_utils_make_u32_by_u16(len[1], rt_utils_u08s_to_u16(&len[2]));
        case 0x84:
            *len_size = 5;
            return rt_utils_u08s_to_u32(&len[1]);
        default:
            len_size = 0;
            return 0;
    }
}

/**
 * @brief                 Calculate the size of special length value
 * @param[in] len         The length value
 * @return uint8_t        The size of length
 * @note                  
 */
uint8_t rt_bertlv_calc_length_size(uint32_t len)
{
    if (len < 0x80) {
        return 1;
    } else if (len < 0x100) {
        return 2;
    } else if (len < 0x10000) {
        return 3;
    } else if (len < 0x1000000) {
        return 4;
    } else {
        return 5;
    }
}

/**
 * @brief                 Set a length value to buffer according BER-TLV encode
 * @param[in] len         The length value
 * @param[out] buf        The buffer to set length
 * @return uint8_t        The size of length
 * @note
 */
uint8_t rt_bertlv_set_length(uint32_t len, uint8_t* buf)
{
    if (len < 0x80) {
        buf[0] = (uint8_t)len;
        return 1;
    }
    else if (len < 0x100) {
        buf[0] = 0x81;
        buf[1] = (uint8_t)len;
        return 2;
    }
    else if (len < 0x10000) {
        buf[0] = 0x82;
        rt_utils_u16_to_u08s((uint16_t)len, &buf[1]);
        return 3;
    }
    else if (len < 0x1000000) {
        buf[0] = 0x83;
        buf[1] = (uint8_t)(len >> 0x16);
        rt_utils_u16_to_u08s((uint16_t)len, &buf[2]);
        return 4;
    }
    else {
        buf[0] = 0x84;
        rt_utils_u32_to_u08s(len, &buf[1]);
        return 5;
    }
}

/**
 * @brief                 Find the special tag in a BER-TLV list
 * @param[in] tlv         The pointer of BER-TLV list
 * @param[in] tlv_len     Length of BER-TLV list
 * @param[in] tag         The tag of the TLV element to search
 * @param[in] occurrence  The occurrence number of the TLV element (1 for the first, 2 for the second...)
 * @return uint32_t       The offset of the tag was found
 * @note                  
 */
uint32_t rt_bertlv_find_tag(uint8_t* tlv, uint32_t tlv_len, uint16_t tag, uint8_t occurrence)
{
    uint32_t offset;

    if (occurrence == 0) {
        return BER_TLV_INVALID_OFFSET;
    }

    offset = 0;

    while (offset < tlv_len) {
        if (rt_bertlv_get_tag(tlv + offset, NULL) == tag) {
            occurrence--;
            if (occurrence == 0) {
                return offset;
            }
        }
        offset += rt_bertlv_get_tlv_length(tlv + offset);
    }

    return BER_TLV_INVALID_OFFSET;
}

/**
 * @brief                 Calculate the size of T(ag) and L(ength)
 * @param[in] tlv         The pointer of TLV buffer
 * @param[out] value_len  The size of V(alue)
 * @return uint16_t       The size of T(ag) and L(ength)
 * @note                  
 */
uint32_t rt_bertlv_get_tl_length(uint8_t* tlv, uint32_t* value_len)
{
    uint8_t tag_size;
    uint8_t length_size;
    uint32_t length;

    tag_size = rt_bertlv_get_tag_size(tlv);
    length = rt_bertlv_get_length(tlv + tag_size, &length_size);

    if (value_len != NULL) {
        *value_len = length;
    }

    return (uint16_t)(tag_size + length_size);
}

/**
 * @brief                 Calculate the size of TLV
 * @param[in] tlv         The pointer of TLV buffer
 * @return uint32_t       The size of TLV
 * @note
 */
uint32_t rt_bertlv_get_tlv_length(uint8_t* tlv)
{
    uint16_t tl_size;
    uint32_t value_length;

    tl_size = rt_bertlv_get_tl_length(tlv, &value_length);

    return (uint32_t)(tl_size + value_length);
}

/**
 * @brief                 Get integer value from a integer tlv. Maximum support 4 byte value
 * @param[in] tlv         Pointer of integer TLV
 * @return uint32_t       The integer value
 * @note                  If you do not confirm that the length is less than or equal to 4, please do not use this method
 */
uint32_t rt_bertlv_get_integer(uint8_t* tlv, uint32_t* tlv_size)
{
    uint32_t value;
    uint32_t value_length;
    uint16_t offset;

    offset = rt_bertlv_get_tl_length(tlv, &value_length);
    // find by sequence number
    switch (value_length) {
    case 0x00:
        value = 0x00;
        break;
    case 0x01:
        value = tlv[offset];
        break;
    case 0x02:
        value = rt_utils_u08s_to_u16(tlv + offset);
        break;
    case 0x03:
        value = rt_utils_u08s_to_u16(tlv + offset);
        value <<= 0x08;
        value |= tlv[offset + 2];
        break;
    default:
        value = rt_utils_u08s_to_u32(tlv + offset);
        break;
    }

    if (tlv_size != NULL) {
        *tlv_size = offset + value_length;
    }

    return value;
}

/**
 * @brief                 Get the next TLV from current TLV
 * @param[in] tlv         Pointer of Current TLV
 * @param[out] next       Pointer of next TLV
 * @return uint8_t <br>
 *   <em> 0 </em>    ----success    <br>
 *   <em> -1 </em>   ----failed.
 * @note
 */
uint8_t rt_bertlv_move_to_next(uint8_t* tlv, uint8_t** next)
{
    uint32_t tlv_size;

    tlv_size = rt_bertlv_get_tlv_length(tlv);
    // Tag and length field must present
    if (tlv_size < 2) {
        return (uint8_t)-1;
    }
    *next = (uint8_t*)(tlv + tlv_size);

    return 0;
}

/**
 * @brief                 Build a BER-TLV to special buffer
 * @param[in] tag         The tag
 * @param[in] len         The length
 * @param[in] value       The pointer of value buffer
 * @param[out] dst        The buffer to store TLV data
 * @return uint32_t       The size of TLV
 * @note                  Support for overlapping value and dst arrays
 */
uint32_t rt_bertlv_build_tlv(uint16_t tag, uint32_t len, void* value, void* dst)
{
    uint8_t tag_size;
    uint8_t length_size;
    uint8_t tag_buffer[2];

    if (tag < 0x100) {
        tag_buffer[0] = (uint8_t)tag;
        tag_size = 1;
    }
    else {
        rt_utils_u16_to_u08s(tag, tag_buffer);
        tag_size = 2;
    }

    length_size = rt_bertlv_calc_length_size(len);

    if (value != NULL) {
        // Copy value
        memcpy((uint8_t*)dst + tag_size + length_size, value, (uint16_t)len);
    }
    // Set tag
    memcpy(dst, tag_buffer, tag_size);
    // Set length
    rt_bertlv_set_length(len, (uint8_t*)dst + tag_size);

    return (uint32_t)(tag_size + length_size + len);
}

/**
 * @brief                 Build a BER-TLV to special buffer, with integer value
 * @param[in] tag         The tag
 * @param[in] integer     The integer value
 * @param[out] dst        The buffer to store TLV data
 * @return uint32_t       The size of TLV
 * @note
 */
uint32_t rt_bertlv_build_integer_tlv(uint16_t tag, uint32_t integer, uint8_t* dst)
{
    uint8_t i;
    uint8_t length;
    uint8_t value[0x05];

    if (integer == 0x00) {
        value[0x00] = 0x00;
        return rt_bertlv_build_tlv(tag, 0x01, value, dst);
    }

    rt_utils_u32_to_u08s(integer, value + 1);

    for (i = 0x01; i < 0x05; i++) {
        if (value[i] != 0x00) {
            break;
        }
    }

    length = (uint8_t)(5 - i);
    if (value[i] >= 0x80) {
        value[--i] = 0x00;
        length++;
    }

    return rt_bertlv_build_tlv(tag, length, value + i, dst);
}
