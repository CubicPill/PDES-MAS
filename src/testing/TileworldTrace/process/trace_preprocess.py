import re

msg_ptn = re.compile(r'message: (<.+>)\n')
ssv_ptn = re.compile(
    r'sim.preload_variable\((?:agent )?(\d+), Point\((\d+), (\d+)\), 0\)')
agent_fp = dict()


def process(msg):
    msg_param = msg.replace('>', '').replace('<', '').split(':')
    msg_text = ''
    agent = ''
    if msg_param[0] == '10':  # rq
        t, ori, dest, ts, mc, noh, id, agent, _, range1x, range1y, range2x, range2y, nth, map = msg_param
        msg_text = ','.join([t, ts, range1x, range1y, range2x, range2y])
    elif msg_param[0] == '14':  # read
        # print(msg_param)
        t, ori, dest, ts, mc, noh, id, agent, _, ssvid = msg_param
        msg_text = ','.join([t, ts, ssvid])
    elif msg_param[0] == '18':  # write
        # print(msg_param)
        t, ori, dest, ts, mc, noh, id, agent, _, ssvid, v_type, px, py = msg_param
        msg_text = ','.join([t, ts, ssvid, px, py])
    else:
        return '', ''
    return agent, msg_text


for fn in ['alp1.txt', 'alp2.txt']:
    with open(fn) as f:
        for line in f:
            group = msg_ptn.search(line)
            if group:
                agent, processed = process(group.group(1))

                if processed:
                    if agent not in agent_fp:
                        agent_fp[agent] = open('../trace/{}.txt'.format(agent), 'w')
                    agent_fp[agent].write(processed + '\n')

ssv_f = open('../trace/ssv.txt', 'w')
with open('preload_sim.txt') as f:
    for line in f:
        groups = ssv_ptn.search(line)
        if groups:
            ssv_id, px, py = groups.group(1), groups.group(2), groups.group(3)
            ssv_f.write('0,{},{},{}\n'.format(ssv_id, px, py))
ssv_f.close()
