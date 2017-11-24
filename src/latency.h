/* latency.h -- latency monitor API header file
 * See latency.c for more information.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2014, Salvatore Sanfilippo <antirez at gmail dot com>
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

#ifndef __LATENCY_H
#define __LATENCY_H

#define LATENCY_TS_LEN 160 /* History length for every monitored event. 每个监控时间的历史长度 */

/* Representation of a latency sample: the sampling time and the latency
 * observed in milliseconds.
 * 延迟样本的表示：采样时间和在毫秒内观察到的潜伏期。 */
struct latencySample {
    int32_t time; /* We don't use time_t to force 4 bytes usage everywhere.  我们不使用time_t迫使4字节的使用无处不在。*/
    uint32_t latency; /* Latency in milliseconds.  毫秒的延迟。*/
};

/* The latency time series for a given event. 给定事件的等待时间序列。*/
struct latencyTimeSeries {
    int idx; /* Index of the next sample to store. 下一个存储的索引。*/
    uint32_t max; /* Max latency observed for this event. */
    struct latencySample samples[LATENCY_TS_LEN]; /* Latest history. */
};

/* Latency statistics structure.  延迟统计结构。*/
struct latencyStats {
    uint32_t all_time_high; /* Absolute max observed since latest reset. 绝对最大值观察从最近的复位 */
    uint32_t avg;           /* Average of current samples. 样本平均值。 */
    uint32_t min;           /* Min of current samples.  样本最小值。*/
    uint32_t max;           /* Max of current samples.  样本最大值。*/
    uint32_t mad;           /* Mean absolute deviation.  平均绝对偏差。*/
    uint32_t samples;       /* Number of non-zero samples.  非零样本数*/
    time_t period;          /* Number of seconds since first event and now.  自第一事件和现在的秒数。*/
};

void latencyMonitorInit(void);
void latencyAddSample(char *event, mstime_t latency);
int THPIsEnabled(void);

/* Latency monitoring macros. 延迟监视宏。*/

/* Start monitoring an event. We just set the current time.
 * 开始监视事件。我们只是设置当前时间。 */
#define latencyStartMonitor(var) if (server.latency_monitor_threshold) { \
    var = mstime(); \
} else { \
    var = 0; \
}

/* End monitoring an event, compute the difference with the current time
 * to check the amount of time elapsed.
 * 结束监视事件，计算当前时间的差异，检查所经过的时间。 */
#define latencyEndMonitor(var) if (server.latency_monitor_threshold) { \
    var = mstime() - var; \
}

/* Add the sample only if the elapsed time is >= to the configured threshold.
 * 仅当所经过的时间为已配置的阈值时，才添加该示例。 */
#define latencyAddSampleIfNeeded(event,var) \
    if (server.latency_monitor_threshold && \
        (var) >= server.latency_monitor_threshold) \
          latencyAddSample((event),(var));

/* Remove time from a nested event.  从嵌套事件中删除时间。*/
#define latencyRemoveNestedEvent(event_var,nested_var) \
    event_var += nested_var;

#endif /* __LATENCY_H */
