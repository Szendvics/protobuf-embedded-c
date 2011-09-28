/*--------------------------------------------------------------------------+
|                                                                          |
| Copyright 2008-2011 Technische Universitaet Muenchen                     |
|                                                                          |
| Licensed under the Apache License, Version 2.0 (the "License");          |
| you may not use this file except in compliance with the License.         |
| You may obtain a copy of the License at                                  |
|                                                                          |
|    http://www.apache.org/licenses/LICENSE-2.0                            |
|                                                                          |
| Unless required by applicable law or agreed to in writing, software      |
| distributed under the License is distributed on an "AS IS" BASIS,        |
| WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. |
| See the License for the specific language governing permissions and      |
| limitations under the License.                                           |
+--------------------------------------------------------------------------*/

#include "Test.h"

/*******************************************************************
 * General functions
 *******************************************************************/
 
void _memset(void *msg_ptr, char init_val, unsigned int size) {
    int i;
    for(i = 0; i < size; ++ i)
        *((char*)msg_ptr + i) = init_val;
}

int varint_packed_size(unsigned long value) {
    if ((value & (0xffffffff <<  7)) == 0) return 1;
    if ((value & (0xffffffff << 14)) == 0) return 2;
    if ((value & (0xffffffff << 21)) == 0) return 3;
    if ((value & (0xffffffff << 28)) == 0) return 4;
    return 5;  
}

int write_raw_byte(char value, void *_buffer, int offset) {
    *((char *)_buffer + offset) = value;
    return ++offset;
}

/** Write a little-endian 32-bit integer. */
int write_raw_little_endian32(unsigned long value, void *_buffer, int offset) {
    offset = write_raw_byte((char)((value      ) & 0xFF), _buffer, offset);
    offset = write_raw_byte((char)((value >>  8) & 0xFF), _buffer, offset);
    offset = write_raw_byte((char)((value >> 16) & 0xFF), _buffer, offset);
    offset = write_raw_byte((char)((value >> 24) & 0xFF), _buffer, offset);
    
    return offset;
}

int write_raw_varint32(unsigned long value, void *_buffer, int offset) {
    unsigned long sign = 1 & (value >> 31);
    while (1) {
        if ((value & ~0x7F) == 0) {
             if (sign) {
              // Must sign extend to 64 bit.
              offset = write_raw_byte(0xF0 | value, _buffer, offset);
              offset = write_raw_byte(255, _buffer, offset);
              offset = write_raw_byte(255, _buffer, offset);
              offset = write_raw_byte(255, _buffer, offset);
              offset = write_raw_byte(255, _buffer, offset);
              offset = write_raw_byte(1, _buffer, offset);
            } else {
              offset = write_raw_byte((char)value, _buffer, offset);
            }
            return offset;
        } else {
            offset = write_raw_byte((char)((value & 0x7F) | 0x80), _buffer, offset);
            value = value >> 7;
        }
    }
    return offset;
}

int write_raw_bytes(char *bytes, int bytes_size, void *_buffer, int offset) {
    int i; 
    for(i = 0; i < bytes_size; ++ i) {
        offset = write_raw_byte((char)*(bytes + i), _buffer, offset);
    }
    
    return offset;   
}

int read_raw_byte(char *tag, void *_buffer, int offset) {
    *tag = *((char *) _buffer + offset);
    
    return ++offset;
}

/** Read a 32-bit little-endian integer from the stream. */
int read_raw_little_endian32(unsigned long *tag, void *_buffer, int offset) {
    offset = read_raw_byte((char *)tag, _buffer, offset);
    char b1 = (char) *tag;
    offset = read_raw_byte((char *)tag, _buffer, offset);
    char b2 = (char) *tag;
    offset = read_raw_byte((char *)tag, _buffer, offset);
    char b3 = (char) *tag;
    offset = read_raw_byte((char *)tag, _buffer, offset);
    char b4 = (char) *tag;
    
    *tag = (((unsigned long)b1 & 0xff)      ) |
           (((unsigned long)b2 & 0xff) <<  8) |
           (((unsigned long)b3 & 0xff) << 16) |
           (((unsigned long)b4 & 0xff) << 24);
           
    return offset;
}

int read_raw_varint32(unsigned long *tag, void *_buffer, int offset) {
    char result;
    
    offset = read_raw_byte(&result, _buffer, offset);
    if (result >= 0) {
        *tag = result;
        return offset;
    }
    *tag = result & 0x7f;
    offset = read_raw_byte(&result, _buffer, offset);
    if (result >= 0) {
        *tag |= result << 7;
    } else {
        *tag |= (result & 0x7f) << 7;
        offset = read_raw_byte(&result, _buffer, offset);
        if (result >= 0) {
            *tag |= result << 14;
        } else {
            *tag |= (result & 0x7f) << 14;
            offset = read_raw_byte(&result, _buffer, offset);
            if (result >= 0) {
                *tag |= result << 21;
            } else {
                *tag |= (result & 0x7f) << 21;
                offset = read_raw_byte(&result, _buffer, offset);
                *tag |= result << 28;
                if (result < 0) {
                    // Discard upper 32 bits.
                    int i;
                    for (i = 0; i < 5; ++ i) {
                        offset = read_raw_byte(&result, _buffer, offset);
                        if (result >= 0) {
                            return offset;
                        }
                    }
                    //invalid state
                }
            }
        }
    }
    return offset;
}

/*******************************************************************
 * Enum: Test.proto, line 10
 *******************************************************************/
int PhoneType_write_with_tag(enum PhoneType *_PhoneType, void *_buffer, int offset, int tag) {
    /* Write tag.*/
    offset = write_raw_varint32((tag<<3)+0, _buffer, offset);
    /* Write content.*/
    offset = write_raw_varint32(*_PhoneType, _buffer, offset);
    
    return offset;
}

/*******************************************************************
 * Message: Test.proto, line 16
 *******************************************************************/
int PhoneNumber_write(struct PhoneNumber *_PhoneNumber, void *_buffer, int offset) {
    /* Write content of each message element.*/
    offset = write_raw_varint32((1<<3)+5, _buffer, offset);
    unsigned long *number_ptr = (unsigned long *)&_PhoneNumber->_number;
    offset = write_raw_little_endian32(*number_ptr, _buffer, offset);

    offset = PhoneType_write_with_tag(&_PhoneNumber->_type, _buffer, offset, 2);
    
    return offset;
}

int PhoneNumber_write_with_tag(struct PhoneNumber *_PhoneNumber, void *_buffer, int offset, int tag) {
    /* Write tag.*/
    offset = write_raw_varint32((tag<<3)+2, _buffer, offset);
    /* Write content.*/
    offset = PhoneNumber_write(_PhoneNumber, _buffer, offset);
    
    return offset;
}

int PhoneNumber_write_delimited_to(struct PhoneNumber *_PhoneNumber, void *_buffer, int offset) {
    int i, shift, new_offset, size;
    
    new_offset = PhoneNumber_write(_PhoneNumber, _buffer, offset);
    size = new_offset - offset;
    shift = (size > 127) ? 2 : 1;
    for (i = new_offset - 1; i >= offset; -- i)
        *((char *)_buffer + i + shift) = *((char *)_buffer + i);
    
    write_raw_varint32((unsigned long) size, _buffer, offset);         
        
    return new_offset + shift;
}

void PhoneNumber_clear(struct PhoneNumber *_PhoneNumber) {
    _memset(_PhoneNumber, 0, sizeof(struct PhoneNumber));
}

int PhoneNumber_read(void *_buffer, struct PhoneNumber *_PhoneNumber, int offset, int limit) {
    int i = 0;
    unsigned long tag = i;

/* Reset all attributes to 0 in advance. */
    PhoneNumber_clear(_PhoneNumber);

    /* Read/interpret all attributes from buffer offset until upper limit is reached. */
    while(offset < limit) {
        offset = read_raw_varint32(&tag, _buffer, offset);
        tag = tag>>3;
        switch(tag){
            //tag of: _PhoneNumber._number 
            case 1 :
                offset = read_raw_little_endian32(&tag, _buffer, offset);
                float *number = (float *)(&tag);
                _PhoneNumber->_number = *number;
                break;
            //tag of: _PhoneNumber._type 
            case 2 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _PhoneNumber->_type = tag;
                break;
        }
    }
    
    return offset;
}

int PhoneNumber_read_delimited_from(void *_buffer, struct PhoneNumber *_PhoneNumber, int offset) {
    unsigned long size;
    int shift;
    
    offset = read_raw_varint32(&size, _buffer, offset);
    shift = (size > 127) ? 2 : 1;
    PhoneNumber_read(_buffer, _PhoneNumber, offset, size + offset);
    
    return offset + size;
}


/*******************************************************************
 * Message: Test.proto, line 21
 *******************************************************************/
int Person_write(struct Person *_Person, void *_buffer, int offset) {
    /* Write content of each message element.*/
    offset = write_raw_varint32((1<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name1_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name1, _Person->_name1_len, _buffer, offset);

    offset = write_raw_varint32((2<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name2_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name2, _Person->_name2_len, _buffer, offset);

    offset = write_raw_varint32((3<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name3_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name3, _Person->_name3_len, _buffer, offset);

    offset = write_raw_varint32((4<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name4_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name4, _Person->_name4_len, _buffer, offset);

    offset = write_raw_varint32((5<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name5_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name5, _Person->_name5_len, _buffer, offset);

    offset = write_raw_varint32((6<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_Person->_name6_len, _buffer, offset);
    offset = write_raw_bytes(_Person->_name6, _Person->_name6_len, _buffer, offset);

    offset = write_raw_varint32((7<<3)+0, _buffer, offset);
    offset = write_raw_varint32(_Person->_id, _buffer, offset);

    offset = write_raw_varint32((8<<3)+5, _buffer, offset);
    unsigned long *iq_ptr = (unsigned long *)&_Person->_iq;
    offset = write_raw_little_endian32(*iq_ptr, _buffer, offset);

    offset = write_raw_varint32((9<<3)+0, _buffer, offset);
    offset = write_raw_byte(_Person->_email, _buffer, offset);

    offset = PhoneType_write_with_tag(&_Person->_phone, _buffer, offset, 10);
    int strAttr_cnt;
    for (strAttr_cnt = 0; strAttr_cnt < _Person->_strAttr_repeated_len; ++ strAttr_cnt) {
        offset = write_raw_varint32((11<<3)+2, _buffer, offset);
        offset = write_raw_varint32(_Person->_strAttr_len[strAttr_cnt], _buffer, offset);
        offset = write_raw_bytes(_Person->_strAttr[strAttr_cnt], _Person->_strAttr_len[strAttr_cnt], _buffer, offset);
    }

    int intAttr_cnt;
    for (intAttr_cnt = 0; intAttr_cnt < _Person->_intAttr_repeated_len; ++ intAttr_cnt) {
        offset = write_raw_varint32((12<<3)+0, _buffer, offset);
        offset = write_raw_varint32(_Person->_intAttr[intAttr_cnt], _buffer, offset);
    }

    int boolAttr_cnt;
    for (boolAttr_cnt = 0; boolAttr_cnt < _Person->_boolAttr_repeated_len; ++ boolAttr_cnt) {
        offset = write_raw_varint32((13<<3)+0, _buffer, offset);
        offset = write_raw_byte(_Person->_boolAttr[boolAttr_cnt], _buffer, offset);
    }

    int floatAttr_cnt;
    for (floatAttr_cnt = 0; floatAttr_cnt < _Person->_floatAttr_repeated_len; ++ floatAttr_cnt) {
        offset = write_raw_varint32((14<<3)+5, _buffer, offset);
        unsigned long *floatAttr_ptr = (unsigned long *)&_Person->_floatAttr[floatAttr_cnt];
        offset = write_raw_little_endian32(*floatAttr_ptr, _buffer, offset);
    }

    int enumAttr_cnt;
    for (enumAttr_cnt = 0; enumAttr_cnt < _Person->_enumAttr_repeated_len; ++ enumAttr_cnt) {
        offset = PhoneType_write_with_tag(&_Person->_enumAttr[enumAttr_cnt], _buffer, offset, 15);
    }
    
    return offset;
}

int Person_write_with_tag(struct Person *_Person, void *_buffer, int offset, int tag) {
    /* Write tag.*/
    offset = write_raw_varint32((tag<<3)+2, _buffer, offset);
    /* Write content.*/
    offset = Person_write(_Person, _buffer, offset);
    
    return offset;
}

int Person_write_delimited_to(struct Person *_Person, void *_buffer, int offset) {
    int i, shift, new_offset, size;
    
    new_offset = Person_write(_Person, _buffer, offset);
    size = new_offset - offset;
    shift = (size > 127) ? 2 : 1;
    for (i = new_offset - 1; i >= offset; -- i)
        *((char *)_buffer + i + shift) = *((char *)_buffer + i);
    
    write_raw_varint32((unsigned long) size, _buffer, offset);         
        
    return new_offset + shift;
}

void Person_clear(struct Person *_Person) {
    _memset(_Person, 0, sizeof(struct Person));
}

int Person_read(void *_buffer, struct Person *_Person, int offset, int limit) {
    int i = 0;
    unsigned long tag = i;

/* Reset all attributes to 0 in advance. */
    Person_clear(_Person);

    /* Read/interpret all attributes from buffer offset until upper limit is reached. */
    while(offset < limit) {
        offset = read_raw_varint32(&tag, _buffer, offset);
        tag = tag>>3;
        switch(tag){
            //tag of: _Person._name1 
            case 1 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name1_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name1 + i), _buffer, offset);
                break;
            //tag of: _Person._name2 
            case 2 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name2_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name2 + i), _buffer, offset);
                break;
            //tag of: _Person._name3 
            case 3 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name3_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name3 + i), _buffer, offset);
                break;
            //tag of: _Person._name4 
            case 4 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name4_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name4 + i), _buffer, offset);
                break;
            //tag of: _Person._name5 
            case 5 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name5_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name5 + i), _buffer, offset);
                break;
            //tag of: _Person._name6 
            case 6 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_name6_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_Person->_name6 + i), _buffer, offset);
                break;
            //tag of: _Person._id 
            case 7 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_id = (signed long)tag;
                break;
            //tag of: _Person._iq 
            case 8 :
                offset = read_raw_little_endian32(&tag, _buffer, offset);
                float *iq = (float *)(&tag);
                _Person->_iq = *iq;
                break;
            //tag of: _Person._email 
            case 9 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_email = tag & 1;
                break;
            //tag of: _Person._phone 
            case 10 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_phone = tag;
                break;
            //tag of: _Person._strAttr 
            case 11 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                /* Set length of current string. */
                _Person->_strAttr_len[(int)_Person->_strAttr_repeated_len] = tag;
                /* Copy raw bytes of current string. */
                for(i = 0; i < tag; ++ i) {
                    offset = read_raw_byte(&_Person->_strAttr[(int)_Person->_strAttr_repeated_len][i], _buffer, offset);
                }
                /* Advance to next string. */
                _Person->_strAttr_repeated_len++;    
                break;
            //tag of: _Person._intAttr 
            case 12 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_intAttr[(int)_Person->_intAttr_repeated_len++] = (signed long)tag;
                break;
            //tag of: _Person._boolAttr 
            case 13 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_boolAttr[(int)_Person->_boolAttr_repeated_len++] = tag & 1;
                break;
            //tag of: _Person._floatAttr 
            case 14 :
                offset = read_raw_little_endian32(&tag, _buffer, offset);
                float *floatAttr = (float *)(&tag);
                _Person->_floatAttr[(int)_Person->_floatAttr_repeated_len++] = *floatAttr;
                break;
            //tag of: _Person._enumAttr 
            case 15 :
                offset = read_raw_varint32(&tag, _buffer, offset);
                _Person->_enumAttr[(int)_Person->_enumAttr_repeated_len++] = tag;
                break;
        }
    }
    
    return offset;
}

int Person_read_delimited_from(void *_buffer, struct Person *_Person, int offset) {
    unsigned long size;
    int shift;
    
    offset = read_raw_varint32(&size, _buffer, offset);
    shift = (size > 127) ? 2 : 1;
    Person_read(_buffer, _Person, offset, size + offset);
    
    return offset + size;
}


/*******************************************************************
 * Message: Test.proto, line 39
 *******************************************************************/
int AddressBook_write(struct AddressBook *_AddressBook, void *_buffer, int offset) {
    /* Write content of each message element.*/
    offset = write_raw_varint32((1<<3)+2, _buffer, offset);
    offset = write_raw_varint32(_AddressBook->_address_len, _buffer, offset);
    offset = write_raw_bytes(_AddressBook->_address, _AddressBook->_address_len, _buffer, offset);

    
    return offset;
}

int AddressBook_write_with_tag(struct AddressBook *_AddressBook, void *_buffer, int offset, int tag) {
    /* Write tag.*/
    offset = write_raw_varint32((tag<<3)+2, _buffer, offset);
    /* Write content.*/
    offset = AddressBook_write(_AddressBook, _buffer, offset);
    
    return offset;
}

int AddressBook_write_delimited_to(struct AddressBook *_AddressBook, void *_buffer, int offset) {
    int i, shift, new_offset, size;
    
    new_offset = AddressBook_write(_AddressBook, _buffer, offset);
    size = new_offset - offset;
    shift = (size > 127) ? 2 : 1;
    for (i = new_offset - 1; i >= offset; -- i)
        *((char *)_buffer + i + shift) = *((char *)_buffer + i);
    
    write_raw_varint32((unsigned long) size, _buffer, offset);         
        
    return new_offset + shift;
}

void AddressBook_clear(struct AddressBook *_AddressBook) {
    _memset(_AddressBook, 0, sizeof(struct AddressBook));
}

int AddressBook_read(void *_buffer, struct AddressBook *_AddressBook, int offset, int limit) {
    int i = 0;
    unsigned long tag = i;

/* Reset all attributes to 0 in advance. */
    AddressBook_clear(_AddressBook);

    /* Read/interpret all attributes from buffer offset until upper limit is reached. */
    while(offset < limit) {
        offset = read_raw_varint32(&tag, _buffer, offset);
        tag = tag>>3;
        switch(tag){
            //tag of: _AddressBook._address 
            case 1 :
                 /* Re-use 'tag' to store string length. */
                offset = read_raw_varint32(&tag, _buffer, offset);
                _AddressBook->_address_len = tag;
                for(i = 0; i < tag; ++ i) 
                    offset = read_raw_byte((_AddressBook->_address + i), _buffer, offset);
                break;
        }
    }
    
    return offset;
}

int AddressBook_read_delimited_from(void *_buffer, struct AddressBook *_AddressBook, int offset) {
    unsigned long size;
    int shift;
    
    offset = read_raw_varint32(&size, _buffer, offset);
    shift = (size > 127) ? 2 : 1;
    AddressBook_read(_buffer, _AddressBook, offset, size + offset);
    
    return offset + size;
}


/*******************************************************************
 * Message: Test.proto, line 43
 *******************************************************************/
int Foo_write(void *_buffer, int offset) {
    /* Write content of each message element.*/
    
    return offset;
}

int Foo_write_with_tag(void *_buffer, int offset, int tag) {
    /* Write tag.*/
    offset = write_raw_varint32((tag<<3)+2, _buffer, offset);
    /* Write content.*/
    offset = Foo_write(_buffer, offset);
    
    return offset;
}

int Foo_write_delimited_to(void *_buffer, int offset) {
    int i, shift, new_offset, size;
    
    new_offset = Foo_write(_buffer, offset);
    size = new_offset - offset;
    shift = (size > 127) ? 2 : 1;
    for (i = new_offset - 1; i >= offset; -- i)
        *((char *)_buffer + i + shift) = *((char *)_buffer + i);
    
    write_raw_varint32((unsigned long) size, _buffer, offset);         
        
    return new_offset + shift;
}

void Foo_clear(struct Foo *_Foo) {
    _memset(_Foo, 0, sizeof(struct Foo));
}

int Foo_read(void *_buffer, int offset, int limit) {
    int i = 0;
    unsigned long tag = i;


    /* Read/interpret all attributes from buffer offset until upper limit is reached. */
    while(offset < limit) {
        offset = read_raw_varint32(&tag, _buffer, offset);
        tag = tag>>3;
        switch(tag){
        }
    }
    
    return offset;
}

int Foo_read_delimited_from(void *_buffer, int offset) {
    unsigned long size;
    int shift;
    
    offset = read_raw_varint32(&size, _buffer, offset);
    shift = (size > 127) ? 2 : 1;
    Foo_read(_buffer, offset, size + offset);
    
    return offset + size;
}
