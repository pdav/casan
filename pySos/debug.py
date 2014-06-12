from enum import Enum
dbg_levels = Enum('dbg_levels', {'MESSAGE':0x1,
                                 'OPTION':0x2,
                                 'STATE':0x4,
                                 'CACHE':0x8,
                                 'CONF':0x10,
                                 'HTTP':0x20,
                                 'ALL' : 0x8000})

