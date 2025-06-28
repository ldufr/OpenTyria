from dataclasses import dataclass
# from linux_process import *
from process import *

@dataclass
class Array:
    buf: int
    cap: int
    len: int
    param: int

class pygw(object):
    def __init__(self, proc):
        self.proc = proc
        self.scanner = ProcessScanner(proc)

        tmp = self.scanner.find(b'\x50\x6A\x0F\x6A\x00\xFF\x35', +7)
        self.ctx, = proc.read(tmp)

        tmp = self.scanner.find(b'\x8b\x0c\x90\x85\xc9\x74\x19', -0x4)
        self.agent_ptr, = proc.read(tmp)

        tmp = self.scanner.find(b'\xD9\x45\x08\xFF\x75\x10\xFF\x75\x0C\x51\xB9', +0xB)
        self.camera_ptr, = proc.read(tmp)

        tmp = self.scanner.find(b'\x6B\xC6\x7C\x5E\x05', +0x5)
        self.areas_info_ptr, = proc.read(tmp)

    def get_game_ctx(self):
        tmp, = self.proc.read(self.ctx)
        return self.proc.read(tmp + 24)[0]

    def get_map_ctx(self):
        return self.proc.read(self.get_game_ctx() + 0x14)[0]

    def get_char_ctx(self):
        return self.proc.read(self.get_game_ctx() + 0x44)[0]

    def get_world_ctx(self):
        return self.proc.read(self.get_game_ctx() + 0x2C)[0]

    def get_player_array(self):
        tmp = self.get_world_ctx()
        return Array(*self.proc.read(tmp + 0x80C, 'IIII'))

    def get_agent_array(self):
        return Array(*self.proc.read(self.agent_ptr, 'IIII'))

    def get_player_id(self):
        ctx = self.get_char_ctx()
        return self.proc.read(ctx + 0x2A4)[0]

    def get_map_id(self):
        ctx = self.get_char_ctx()
        return self.proc.read(ctx + 0x198, 'I')[0]

    def get_player_by_id(self, player_id):
        players = self.get_player_array()
        if players.len <= player_id:
            raise RuntimeError(f'Invalid player id {player_id}')
        return players.buf + (player_id * 0x4C)

    def get_agent_id_by_player_id(self):
        player = self.get_player_by_id(self.get_player_id())
        return self.proc.read(player)[0]

    def get_agent_by_id(self, agent_id):
        agents = self.get_agent_array()
        if agents.len <= agent_id:
            raise RuntimeError(f'Invalid agent id {agent_id}')
        return self.proc.read(agents.buf + (agent_id * 4))[0]

    def get_agent_pos(self, agent_id):
        agent = self.get_agent_by_id(agent_id)
        return self.proc.read(agent + 0x74, 'ffI')

    def get_pathing_maps(self):
        tmp = self.get_map_ctx()
        tmp, = self.proc.read(tmp + 0x74)
        tmp, = self.proc.read(tmp)
        return Array(*self.proc.read(tmp + 0x18, 'IIII'))

    def get_camera_pos(self):
        return self.proc.read(self.camera_ptr + 0x78, 'ff')

    def get_player_array(self):
        return Array(*self.proc.read(self.get_world_ctx() + 0x80C, 'IIII'))

    def get_map_file_id_by_map_id(self, map_id):
        return self.proc.read(self.areas_info_ptr + (0x7C * map_id) + 0x68)[0]
