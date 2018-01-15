f = open("conf/capture.conf")

top= {}
for line in f:
    words = line.split()
    if len(words) == 3:
        vals = words[2].split(":")
        if len(vals) == 5:
            if vals[0] not in top:
                top[vals[0]] = []

            top[vals[0]].append(vals[1:])

## for EDGE
exchange = "EDGX"
bats = top[exchange]
ob = {}
port_nums = {}
for x in bats:
    if x[0] in ["A", "B"]:
        port = int(x[3])
        if x[0] not in port_nums:
            port_nums[x[0]] = []
        port_nums[x[0]].append(port)
        unit_name = 'unit{}'.format(port - min(port_nums[x[0]])+1)
        address = '{}:{}'.format(x[2],x[3])
        if unit_name not in ob:
            ob[unit_name] = {}
        ob[unit_name][x[0]]= address
#print('conf = {\nmd= {\n decoder= {\npdp = {\nsymbol_mapping="data/NYSESymbolMapping.xml",\nfeeds={\nXNYS={')
print(exchange, '={\n\tfeeds = {\n\t\tPITCH = {\n\t\t\tunits = {')
for i in range(1,len(ob)+1):
    sep=','
    # if i == len(ob["OBUA"])-1:
    #     sep =''
    unit_name = "unit{}".format(i)
    addresses = ob[unit_name]
    print('\t\t\t\t["{}"] = {{\n\t\t\t\t\tA = {{ address = "{}" }},\n\t\t\t\t\tB = {{ address = "{}" }}\n\t\t\t\t}}{}'.format(i,addresses["A"],addresses["B"],sep))
    #print(ob["OBUA"][i], ob["OBUB"][i])


print('}}}}\nreturn ',exchange)

f.close()
