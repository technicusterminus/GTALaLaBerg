import base64, json, sys, urllib.request, subprocess
from concurrent.futures import ThreadPoolExecutor
SAETZE = [
 ("alloy","Hast du schon gehört, der Bäcker am Hauptplatz macht morgen zu?"),
 ("coral","Komm, wir setzen uns noch kurz in die Sonne, ich lad dich auf einen Kaffee ein."),
 ("ash","Naa, des glaub i ned, der war doch gestern noch beim Lechwehr unten."),
 ("nova","Schau mal, die Tauben am Brunnen, die sind ja richtig frech heute."),
 ("echo","Um sieben am Schmalzturm? Gut, dann bis später."),
 ("shimmer","Zwei Brezen und ein Stück Zwetschgendatschi, bitte."),
 ("verse","Mei, is des a schöner Abend, gell."),
 ("sage","Ich hab die Jacke im Laden gesehen, die war aber viel zu teuer."),
 ("ballad","Und dann hat er gesagt, er fährt jetzt einfach nach München."),
 ("onyx","Ja, servus, wie geht's dir denn? Lang nicht gesehen."),
 ("fable","Wo ist denn das Rathaus? Ach da vorne, das rosa Haus."),
 ("coral","Mama, darf ich noch ein Eis?"),
]
def eins(a):
    i,(stimme,text)=a
    body={"model":"openai/gpt-audio-mini","modalities":["text","audio"],"stream":True,"max_tokens":700,"audio":{"voice":stimme,"format":"pcm16"},
      "messages":[{"role":"user","content":"Sprich diesen Satz ganz natuerlich und beilaeufig, wie im Gespraech auf der Strasse, auf Deutsch, nichts hinzufuegen: "+text}]}
    r=urllib.request.urlopen(urllib.request.Request("https://openrouter.ai/api/v1/chat/completions",data=json.dumps(body).encode(),headers={"Content-Type":"application/json","Authorization":"Bearer proxy-injected"}),timeout=300)
    d=b""
    for z in r:
        z=z.decode().strip()
        if not z.startswith("data:") or z.endswith("[DONE]"): continue
        for c in json.loads(z[5:]).get("choices",[]):
            au=c.get("delta",{}).get("audio")
            if isinstance(au,dict) and au.get("data"): d+=base64.b64decode(au["data"])
    open(sys.argv[1]+"/s%02d.pcm"%i,"wb").write(d); return len(d)
with ThreadPoolExecutor(4) as p: print(list(p.map(eins,enumerate(SAETZE))))
