# iperf3_odroid
iperf3 for odroid jig

### iperf3_odroid usage

* iperf3 server demon mode 실행시 다중 client 호출의 경우 iperf3 server가 응답하지 않는 문제가 발생함
* socket통신을 구현하여 접속되어진 순서대로 iperf3를 실행함.
* server의 경우 socket을 통하여 "iperf3" 요청되면 iperf3 server를 실행후 "iperf3_run"으로 응답함. iperf3가 실행중에는 대기열에 대기하도록 함.
* client의 경우 소켓을 통하여 "iperf3" 값을 보내고 "iperf3_run" 응답이 올때까지 일정시간 대기하며 응답을 받은경우 iperf3 client를 실행하여 속도를 구함.
* make install을 통하여 binary파일을 755 mode로 /usr/bin에 복사하여 어디서든 호출될 수 있도록 함.
* server모드의 경우 자동으로 실행되는 service를 설치하도록 함.

```
Usage: ./iperf3_odroid [-s:server | -c:client {server ip}] [-p:port] [-r:retry time] [-d:delay] [-R:reverse]

  -s --server     Server mode
  -c --client     Client mode (req : server ip sddr)
  -p --port       TCP/IP message control port
  -r --retry      Connect retry count
  -d --delay(ms)  Retry wait delay
  -R --reverse    Client mode only(server sendsm client receives)

  - IPERF3 Server mode.
      iperf3_odroid -s -p 1234
  - IPERF3 client mode.
      iperf3_odroid -c 192.168.0.2 -p 1234 -r 10 -d 1000

```
