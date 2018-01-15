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

## for XNYS
xnys = top["XNYS"]
ob = {}
for x in xnys:
    if x[0] in ["OBUA", "OBUB", "IMBA", "IMBB"]:
        if x[0] not in ob:
            ob[x[0]] = []
        address = '{}:{}'.format(x[2],x[3])
        ob[x[0]].append(address)

units = ['AA','BB','CC','DD','EE','FF','GG','HH','IJ','KK','LL','MM','NN','OO','PQ','RR','SS','TT','UV','WZ']
#print('conf = {\nmd= {\n decoder= {\npdp = {\nsymbol_mapping="data/NYSESymbolMapping.xml",\nfeeds={\nXNYS={')
print('XNYS={')
for i in range(0,len(ob["OBUA"])):
    sep=','
    # if i == len(ob["OBUA"])-1:
    #     sep =''
    print('{} = {{\nA = {{ address = "{}" }},\nB= {{ address = "{}" }}\n }}{}'.format("OB_"+units[i],ob["OBUA"][i], ob["OBUB"][i],sep))
    #print(ob["OBUA"][i], ob["OBUB"][i])

for i in range(0,len(ob["IMBA"])):
    sep=','
    if i == len(ob["IMBA"])-1:
        sep =''
    print('{} = {{\nA = {{ address = "{}" }},\nB= {{ address = "{}" }}\n }}{}'.format("IMB",ob["IMBA"][i], ob["IMBB"][i],sep))

print('}\nreturn XNYS')

f.close()
