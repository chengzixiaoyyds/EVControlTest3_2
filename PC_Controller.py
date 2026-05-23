import serial
import logging
import sys
import threading
import time
import Command

# ---------- 配置 ----------
SERIAL_PORT = 'COM8'  # 根据实际情况修改
BAUDRATE = 115200
RECV_POLL_INTERVAL = 0.01    # 接收轮询间隔

stop_event = threading.Event()
command_buffer = Command.CommandBuffer()

def connect_com():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
        print(f"✅ 成功连接到串口 {SERIAL_PORT}，波特率 {BAUDRATE}")
        logging.info(f"成功连接到串口 {SERIAL_PORT}")
        return ser
    except serial.SerialException as e:
        print(f"❌ 无法打开串口 {SERIAL_PORT}: {e}")
        logging.error(f"无法打开串口: {e}")
        return None
    
# ---------- 数据包构建 ----------
def build_packet(command: int, value: int) -> bytes:
    HEADER = bytes([0xFA, 0xAF])
    TAIL = bytes([0xFB, 0xBF])
    # 构建数据部分：包头 + 命令 + 值
    payload = HEADER + bytes([command, value]) + TAIL
    return payload

# ---------- 接收线程 ----------
def receiver(ser: serial.Serial, buf: Command.CommandBuffer):
    """持续读取串口数据并写入循环缓冲区"""
    while not stop_event.is_set():
        try:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                buf.write(data)
                pkt = buf.get_command()
                if pkt:
                    decode_and_show(pkt)
        except serial.SerialException:
            break
        time.sleep(RECV_POLL_INTERVAL)

# ---------- 解码并显示数据包 ----------
def decode_and_show(packet: bytes):
    if packet[0] == 0xFC and packet[1] == 0xCF and packet[-2] == 0xFD and packet[-1] == 0xDF:
        data = packet[3]
        if packet[2] == 3:
            print("确认响应")
            if data == 0:
                print("执行失败")
            elif data == 1:
                print("执行成功")
    elif packet[0] == 0xFA and packet[1] == 0xAF and packet[-2] == 0xFB and packet[-1] == 0xBF:
        data = packet[3]
        if packet[2] == 1:
            print(f"接收到数据: 0x{data:02X}")

# ---------- 用户输入处理 ----------
def get_user_input(prompt: str) -> int:
    """循环直到用户输入 0~255 的整数"""
    while True:
        raw = input(prompt).strip()
        if not raw.isdigit():
            print("❌ 请输入 0~255 的整数")
            continue
        num = int(raw)
        if 0 <= num <= 255:
            return num
        print("❌ 数值超出范围，请输入 0~255")

if __name__ == "__main__":
    ser = connect_com()
    if ser is None:
        sys.exit(1)
    print("命令: 1=设置LED状态  |  2=设置闪烁次数")
    print("输入 'q' 退出\n")
    t_recv = threading.Thread(target=receiver, args=(ser, command_buffer), daemon=True)
    t_recv.start()

    try:
        while True:
            cmd_input = input("请输入指令 (1/2): ").strip()
            if cmd_input.lower() == 'q':
                break
            if not cmd_input.isdigit():
                print("❌ 请输入数字 1、2 或 'q'")
                continue
            cmd = int(cmd_input)
            if cmd not in (1, 2):
                print("❌ 指令码必须为 1 或 2")
                continue

            # 输入数据值
            if cmd == 1:
                val = get_user_input("LED状态 (0=熄灭, 1=点亮): ")
            else:
                val = get_user_input("闪烁次数 (0~255): ")

            packet = build_packet(cmd, val)
            ser.write(packet)
    
    finally:
        stop_event.set()        # 通知线程退出
        t_recv.join(timeout=1)
        ser.close()
        print("串口已关闭，程序结束。")
        sys.exit(0)