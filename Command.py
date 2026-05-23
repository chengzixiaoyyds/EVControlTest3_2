"""
循环缓冲区模块 —— 按固定帧格式从字节流中提取完整数据帧。

支持的帧格式（均为固定 6 字节）：
  发送帧： 0xFA 0xAF [CMD] [DATA] 0xFB 0xBF
  响应帧： 0xFC 0xCF [CMD] [DATA] 0xFD 0xDF
"""

_FRAME_LEN = 6


class CommandBuffer:
    """循环缓冲区，从串口字节流中解析完整数据帧"""

    BUFFER_SIZE = 256

    def __init__(self):
        self.buffer = bytearray(self.BUFFER_SIZE)
        self.read_index = 0
        self.write_index = 0

    # ---------- 指针操作 ----------
    def _read(self, index: int) -> int:
        """读取缓冲区第 index 位（自动循环）"""
        return self.buffer[index % self.BUFFER_SIZE]

    def _add_read_index(self, length: int) -> None:
        """读指针前进 length 字节"""
        self.read_index = (self.read_index + length) % self.BUFFER_SIZE

    def get_length(self) -> int:
        """未处理数据长度"""
        return (self.write_index - self.read_index + self.BUFFER_SIZE) % self.BUFFER_SIZE

    def get_remain(self) -> int:
        """缓冲区剩余空间"""
        return self.BUFFER_SIZE - self.get_length()

    def write(self, data: bytes) -> int:
        """写入数据，返回实际写入的字节数（空间不足返回0）"""
        length = len(data)
        if self.get_remain() < length:
            return 0

        first_part = self.BUFFER_SIZE - self.write_index
        if length <= first_part:
            self.buffer[self.write_index : self.write_index + length] = data
            self.write_index += length
        else:
            self.buffer[self.write_index :] = data[:first_part]
            second_part = length - first_part
            self.buffer[:second_part] = data[first_part:]
            self.write_index = second_part
        return length

    # ---------- 帧提取 ----------
    def get_command(self) -> bytes | None:
        """
        从缓冲区提取一条完整帧，返回 6 字节的 bytes；数据不足或未找到有效帧时返回 None。
        """
        while True:
            if self.get_length() < _FRAME_LEN:
                return None

            b0 = self._read(self.read_index)
            b1 = self._read(self.read_index + 1)

            # 识别帧头
            if b0 == 0xFA and b1 == 0xAF:
                tail_1, tail_2 = 0xFB, 0xBF
            elif b0 == 0xFC and b1 == 0xCF:
                tail_1, tail_2 = 0xFD, 0xDF
            else:
                self._add_read_index(1)
                continue

            # 校验帧尾
            if self._read(self.read_index + 4) != tail_1 or self._read(self.read_index + 5) != tail_2:
                self._add_read_index(1)
                continue

            # 提取帧
            frame = bytes(self._read(self.read_index + i) for i in range(_FRAME_LEN))
            self._add_read_index(_FRAME_LEN)
            return frame