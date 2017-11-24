/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __RDB_H
#define __RDB_H

#include <stdio.h>
#include "rio.h"

/* TBD: include only necessary headers. */
#include "server.h"

/* The current RDB version. When the format changes in a way that is no longer
 * backward compatible this number gets incremented.
 * 当前数据库版本。当格式不再向后兼容时，该数字将递增。*/
#define RDB_VERSION 7

/* Defines related to the dump file format. To store 32 bits lengths for short
 * keys requires a lot of space, so we check the most significant 2 bits of
 * the first byte to interpreter the length:
 *
 * 00|000000 => if the two MSB are 00 the len is the 6 bits of this byte
 * 01|000000 00000000 =>  01, the len is 14 byes, 6 bits + 8 bits of next byte
 * 10|000000 [32 bit integer] => if it's 01, a full 32 bit len will follow
 * 11|000000 this means: specially encoded object will follow. The six bits
 *           number specify the kind of object that follows.
 *           See the RDB_ENC_* defines.
 *
 * Lengths up to 63 are stored using a single byte, most DB keys, and may
 * values, will fit inside.
 * 定义与转储文件格式相关的内容。为了存储短key的32位长度需要大量的空间，所以我们检查第一字节中最重要的2位来解释长度：
 * 00|000000 =>如果两个MSB 00长度是6位的字节
 * 01|000000 00000000 => 01，长度为14字节，6位+ 8位一个字节
 * 10|000000 [32 bit integer] =>如果是10，一个完整的32位长度将跟随
 * 11|000000 特殊编码对象将跟随。六位数字指定后面的对象类型。*/
#define RDB_6BITLEN 0
#define RDB_14BITLEN 1
#define RDB_32BITLEN 2
#define RDB_ENCVAL 3
#define RDB_LENERR UINT_MAX

/* When a length of a string object stored on disk has the first two bits
 * set, the remaining two bits specify a special encoding for the object
 * accordingly to the following defines:
 * 当存储在磁盘上的字符串对象的长度有前两位设置时，剩下的两位为相应的对象指定一个特殊的编码，如下所述*/
#define RDB_ENC_INT8 0        /* 8 bit signed integer  8位有符号整数*/
#define RDB_ENC_INT16 1       /* 16 bit signed integer  16位有符号整数*/
#define RDB_ENC_INT32 2       /* 32 bit signed integer  32位有符号整数*/
#define RDB_ENC_LZF 3         /* string compressed with FASTLZ   字符串通过FASTLZ压缩*/

/* Dup object types to RDB object types. Only reason is readability (are we
 * dealing with RDB types or with in-memory object types?).
 * DUP的对象类型的数据库对象类型。唯一的原因是可读性（我们处理关系数据库类型或内存中的对象类型？）。 */
#define RDB_TYPE_STRING 0
#define RDB_TYPE_LIST   1
#define RDB_TYPE_SET    2
#define RDB_TYPE_ZSET   3
#define RDB_TYPE_HASH   4
/* NOTE: WHEN ADDING NEW RDB TYPE, UPDATE rdbIsObjectType() BELOW
 * 添加新的数据类型时，更新rdbIsObjectType()下面*/

/* Object types for encoded objects.  编码对象的对象类型。*/
#define RDB_TYPE_HASH_ZIPMAP    9
#define RDB_TYPE_LIST_ZIPLIST  10
#define RDB_TYPE_SET_INTSET    11
#define RDB_TYPE_ZSET_ZIPLIST  12
#define RDB_TYPE_HASH_ZIPLIST  13
#define RDB_TYPE_LIST_QUICKLIST 14
/* NOTE: WHEN ADDING NEW RDB TYPE, UPDATE rdbIsObjectType() BELOW 添加新的数据类型时，更新rdbIsObjectType()下面*/

/* Test if a type is an object type.  测试类型是否为一个对象*/
#define rdbIsObjectType(t) ((t >= 0 && t <= 4) || (t >= 9 && t <= 14))

/* Special RDB opcodes (saved/loaded with rdbSaveType/rdbLoadType).
 * 特殊的数据库操作(saved/loaded with rdbSaveType/rdbLoadType) */
#define RDB_OPCODE_AUX        250
#define RDB_OPCODE_RESIZEDB   251
#define RDB_OPCODE_EXPIRETIME_MS 252
#define RDB_OPCODE_EXPIRETIME 253
#define RDB_OPCODE_SELECTDB   254
#define RDB_OPCODE_EOF        255

int rdbSaveType(rio *rdb, unsigned char type);
int rdbLoadType(rio *rdb);
int rdbSaveTime(rio *rdb, time_t t);
time_t rdbLoadTime(rio *rdb);
int rdbSaveLen(rio *rdb, uint32_t len);
uint32_t rdbLoadLen(rio *rdb, int *isencoded);
int rdbSaveObjectType(rio *rdb, robj *o);
int rdbLoadObjectType(rio *rdb);
int rdbLoad(char *filename);
int rdbSaveBackground(char *filename);
int rdbSaveToSlavesSockets(void);
void rdbRemoveTempFile(pid_t childpid);
int rdbSave(char *filename);
ssize_t rdbSaveObject(rio *rdb, robj *o);
size_t rdbSavedObjectLen(robj *o);
robj *rdbLoadObject(int type, rio *rdb);
void backgroundSaveDoneHandler(int exitcode, int bysignal);
int rdbSaveKeyValuePair(rio *rdb, robj *key, robj *val, long long expiretime, long long now);
robj *rdbLoadStringObject(rio *rdb);

#endif
