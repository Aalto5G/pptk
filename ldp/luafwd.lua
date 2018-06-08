Ldp = require 'lualdp'

intf1 = Ldp.interface_open("vale0:1", 1, 1)
if not intf1 then
  error("Can't open intf1")
end
intf2 = Ldp.interface_open("vale1:1", 1, 1)
if not intf2 then
  error("Can't open intf2")
end

inq = Ldp.get_inq(intf1, 1)
outq = Ldp.get_outq(intf2, 1)

while not Ldp.inq_eof(inq) do
  Ldp.inq_poll(inq, 10000)
  pkts = Ldp.inq_nextpkts(inq, 512)
  Ldp.outq_inject(outq, pkts)
end

Ldp.interface_close(intf1)
Ldp.interface_close(intf2)
