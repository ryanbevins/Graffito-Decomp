# usage: python tools/agent/move_nerves.py src/Enemy/foo.cpp TNerveA TNerveB ...
# Moves each contiguous DEFINE_NERVE block (up to the next top-level item) to the end
# of the file in the given order. For -inline deferred TUs pass the retail
# numbering order printed by tools/agent/nerve_order.py.
import re
import sys

path, order = sys.argv[1], sys.argv[2:]
lines = open(path, encoding="utf-8").read().split("\n")
blocks = {}
keep = []
cur = None
for line in lines:
    m = re.match(r"DEFINE_NERVE\((\w+)\s*,", line)
    if m:
        cur = m.group(1)
        blocks[cur] = [line]
        continue
    if cur and line and not line[0].isspace() and line[0] not in "{}":
        cur = None
    (blocks[cur] if cur else keep).append(line)
if sorted(order) != sorted(blocks):
    sys.exit("order/blocks mismatch: %s" % sorted(set(order) ^ set(blocks)))
while keep and keep[-1] == "":
    keep.pop()
for name in order:
    body = blocks[name]
    while body and body[-1] == "":
        body.pop()
    keep += [""] + body
open(path, "w", encoding="utf-8").write("\n".join(keep) + "\n")
