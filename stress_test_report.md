# mymuduo 压力测试报告

## 1. 测试对象

本次测试对象为 `mymuduo` 项目中的 TCP Echo Server。

服务端逻辑如下：

```text
客户端建立 TCP 连接
客户端发送数据
服务端接收数据
服务端原样回显
服务端主动 shutdown
连接关闭
```

监听地址：

```text
127.0.0.1:8080
```

服务端线程模型：

```text
1 个 main loop 线程
3 个 sub loop 线程
共 4 个 testserver 线程
```

测试环境：

```text
VMware 虚拟机
Linux
本机回环地址 127.0.0.1
```

## 2. 测试工具

使用自定义压测客户端：

```text
example/stress_client
```

命令格式：

```bash
./stress_client [host] [port] [total] [concurrency] [message_size] [timeout_ms]
```

示例：

```bash
./stress_client 127.0.0.1 8080 100000 1000 16 3000
```

参数说明：

```text
host           服务端 IP
port           服务端端口
total          总请求数
concurrency    并发数
message_size   每次发送的数据大小，单位字节
timeout_ms     超时时间，单位毫秒
```

## 3. 测试指标

本次主要观察以下指标：

```text
成功请求数 success
失败请求数 failed
连接失败 connect_fail
发送失败 send_fail
接收失败 recv_fail
总耗时 elapsed_sec
吞吐量 qps
数据吞吐 read_throughput_MiBps
平均延迟 avg
P50 / P95 / P99 延迟
最大延迟 max
CPU 使用情况
线程负载分布
fd 是否泄漏
内存是否异常增长
```

## 4. 短连接并发测试

关闭连接 UP/DOWN 日志后，测试结果如下。

### 4.1 1000 并发测试

```text
1000 并发，100000 请求，16B 消息：
success=100000
failed=0
connect_fail=0
send_fail=0
recv_fail=0
elapsed_sec=39.6337
qps=2523.10
read_throughput_MiBps=0.0384995
latency_ms avg=393.861 p50=399.592 p95=473.381 p99=488.427 max=509.02
```

### 4.2 1500 并发测试

```text
1500 并发，100000 请求，16B 消息：
success=98486
failed=1514
connect_fail=566
send_fail=0
recv_fail=948
elapsed_sec=41.233
qps=2388.53
read_throughput_MiBps=0.036446
latency_ms avg=516.33 p50=436.73 p95=1271.8 p99=3227.46 max=9003.92
```

失败率：

```text
1514 / 100000 = 1.514%
```

### 4.3 2000 并发测试

```text
2000 并发，100000 请求，16B 消息：
success=96617
failed=3383
connect_fail=1603
send_fail=0
recv_fail=1780
elapsed_sec=39.5446
qps=2443.24
read_throughput_MiBps=0.0372809
latency_ms avg=557.761 p50=414.004 p95=1615.99 p99=4244.14 max=8643.91
```

失败率：

```text
3383 / 100000 = 3.383%
```

### 4.4 并发测试结论

```text
1000 并发以内服务端稳定，无失败。
1500 并发开始出现失败，失败率约 1.51%。
2000 并发失败率升至约 3.38%，尾延迟明显恶化。
```

因此当前环境下推荐稳定并发为：

```text
<= 1000
```

过载区间大约为：

```text
1500 ~ 2000 并发
```

## 5. 日志影响测试

关闭连接 UP/DOWN 日志前：

```text
1000 并发，100000 请求，16B 消息：
success=100000
failed=0
qps=2290.07
avg=433.882ms
p99=554.844ms
```

关闭连接 UP/DOWN 日志后：

```text
1000 并发，100000 请求，16B 消息：
success=100000
failed=0
qps=2523.10
avg=393.861ms
p99=488.427ms
```

对比结论：

```text
QPS 从约 2290 提升到约 2523，提升约 10.2%。
平均延迟从约 433.9ms 降至约 393.9ms。
P99 延迟从约 554.8ms 降至约 488.4ms。
```

说明连接级 INFO 日志对短连接高并发场景有明显影响。大量 `Connection UP` / `Connection DOWN` 日志会降低吞吐并增加尾延迟。

## 6. Buffer 收发性能测试

### 6.1 1KB 消息测试

```text
500 并发，50000 请求，1024B 消息：
success=50000
failed=0
connect_fail=0
send_fail=0
recv_fail=0
elapsed_sec=19.7707
qps=2529
read_throughput_MiBps=2.46972
latency_ms avg=196.41 p50=208.151 p95=251.516 p99=263.654 max=289.981
```

### 6.2 64KB 消息测试

```text
200 并发，10000 请求，65536B 消息：
success=10000
failed=0
connect_fail=0
send_fail=0
recv_fail=0
elapsed_sec=4.0903
qps=2444.81
read_throughput_MiBps=152.801
latency_ms avg=80.8542 p50=78.559 p95=100.714 p99=168.486 max=176.727
```

### 6.3 Buffer 测试结论

```text
1KB 和 64KB 消息均无发送失败、接收失败或响应不完整。
Buffer 在 1KB 到 64KB 消息收发场景下工作正常。
当前主要瓶颈不在 Buffer 数据收发层，而更可能来自短连接建连、断连、内核网络栈和线程调度。
```

## 7. 长时间稳定性测试

测试命令：

```bash
./stress_client 127.0.0.1 8080 500000 500 16 5000
```

测试结果：

```text
500 并发，500000 请求，16B 消息：
success=500000
failed=0
connect_fail=0
send_fail=0
recv_fail=0
elapsed_sec=204.389
qps=2446.31
read_throughput_MiBps=0.0373277
latency_ms avg=204.27 p50=210.953 p95=252.08 p99=269.124 max=333.643
```

结论：

```text
500 并发、50 万次短连接请求全部成功。
测试持续约 204 秒，吞吐稳定在约 2446 QPS。
未出现性能明显下降、请求失败或服务端异常。
```

## 8. 多线程 EventLoopThreadPool 测试

压测过程中观察服务端线程：

```text
Threads: 4 total
```

压测时多个线程均有 CPU 占用：

```text
27.7%
18.7%
18.3%
18.3%
```

说明：

```text
EventLoopThreadPool 正常工作。
连接没有集中在单一线程处理。
main loop 和 sub loop 均参与负载处理。
```

短连接场景下系统态 CPU 占比较高，说明压力主要来自：

```text
accept
read
write
close
epoll_wait
TCP 建连和断连
```

## 9. fd 和内存泄漏观察

fd 观察结果：

```text
压测结束后 fd 能够回到测试前数量。
未出现 fd 持续增长。
```

内存观察：

```text
测试开始：
VIRT 228516 KB
RES  4664 KB

测试结束：
VIRT 229100 KB
RES  5816 KB
```

变化：

```text
VIRT 增加约 584 KB
RES 增加约 1152 KB
```

线程状态：

```text
测试开始：4 个线程，全部 sleeping
测试结束：4 个线程，全部 sleeping
```

结论：

```text
进程内存仅小幅增长。
线程数量保持稳定。
压测结束后 CPU 回落到空闲状态。
fd 能够正常回落。
未观察到明显内存泄漏、fd 泄漏或线程异常。
```

## 10. 总体结论

在当前 VMware 虚拟机环境下，`mymuduo` EchoServer 在 TCP echo 短连接场景下表现稳定。

最终结论：

```text
1. 服务端在 1000 并发以内可以稳定运行。
2. 稳定吞吐约为 2400 ~ 2500 QPS。
3. 1500 并发开始出现失败，失败率约 1.51%。
4. 2000 并发失败率约 3.38%，P99 延迟超过 4 秒，系统进入明显过载状态。
5. 关闭连接 UP/DOWN 日志后，吞吐提升约 10%，说明日志对性能有明显影响。
6. Buffer 在 1KB 和 64KB 消息场景下收发正常，未发现明显瓶颈。
7. EventLoopThreadPool 能够正常分发连接和事件，多个线程均参与处理。
8. 长时间 50 万请求测试无失败，稳定性良好。
9. fd 压测后可回落，未观察到 fd 泄漏。
10. 服务端进程内存仅小幅增长，未观察到明显内存泄漏。
```

容量建议：

```text
推荐稳定并发：1000 以内
极限并发区间：1500 ~ 2000
稳定吞吐能力：约 2400 ~ 2500 QPS
主要瓶颈：短连接建连/断连、内核网络栈开销、线程调度以及日志输出
```
