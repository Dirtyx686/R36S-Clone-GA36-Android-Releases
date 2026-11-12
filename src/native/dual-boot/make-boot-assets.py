from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import io,zipfile
B=Path(__file__).resolve().parent
font='/usr/share/fonts/truetype/dejavu/DejaVuSans'
for n in [7,11]:
 frames=[]
 for selected in range(3):
  im=Image.new('RGB',(640,480),'black');d=ImageDraw.Draw(im);accent='#62e5ba' if n==7 else '#b398ff'
  d.rounded_rectangle((247,71,393,174),radius=30,fill='#14252e',outline=accent,width=3)
  d.line((274,119,305,119),fill='#eef4ff',width=6);d.line((290,103,290,135),fill='#eef4ff',width=6)
  d.ellipse((347,105,357,115),fill=accent);d.ellipse((364,125,374,135),fill=accent)
  d.text((320,231),'GA36',anchor='mm',font=ImageFont.truetype(font+'-Bold.ttf',50),fill='#eef4ff')
  d.text((320,282),f'ANDROID {n}',anchor='mm',font=ImageFont.truetype(font+'-Bold.ttf',26),fill=accent)
  d.text((320,328),'YOUR CONSOLE. YOUR GAMES.',anchor='mm',font=ImageFont.truetype(font+'.ttf',13),fill='#9ba9bf')
  for i in range(3):d.ellipse((302+i*14,383,308+i*14,389),fill=accent if i==selected else '#304057')
  data=io.BytesIO();im.save(data,format='PNG');frames.append(data.getvalue())
  if selected==0:(B/f'boot-{n}.rgb').write_bytes(im.tobytes());im.save(B/f'boot-{n}.png')
 with zipfile.ZipFile(B/f'bootanimation-{n}.zip','w',compression=zipfile.ZIP_STORED) as z:
  z.writestr('desc.txt','640 480 3\np 0 0 part0\n')
  for i,data in enumerate(frames):z.writestr(f'part0/{i:05}.png',data)
print('English GA7 and GA11 boot assets ready')
