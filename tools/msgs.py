import utils

__all__ = [
    'auth_cmsg_names',
    'auth_smsg_names',
    'game_cmsg_names',
    'game_smsg_names',
]

auth_cmsg_names = {}
auth_smsg_names = {}
game_cmsg_names = {}
game_smsg_names = {}

opcodes_path = utils.get_path('code', 'OpenTyria', 'opcodes.h')
lines = open(opcodes_path, 'r').read().splitlines()

for line in lines:
    if len(line) == 0 or line.startswith('//'):
        continue

    if not line.startswith('#define'):
        continue

    words = line.split()
    if len(words) != 7:
        continue

    msg_id = int(words[4][3:-1], 16)
    msg_name = words[1]

    if msg_name.startswith('AUTH_CMSG_'):
        auth_cmsg_names[msg_id] = msg_name
    elif msg_name.startswith('AUTH_SMSG_'):
        auth_smsg_names[msg_id] = msg_name
    elif msg_name.startswith('GAME_CMSG_'):
        game_cmsg_names[msg_id] = msg_name
    elif msg_name.startswith('GAME_SMSG_'):
        game_smsg_names[msg_id] = msg_name

