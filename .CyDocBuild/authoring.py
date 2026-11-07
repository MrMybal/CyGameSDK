from pathlib import Path
import json,hashlib,re
ROOT=Path(__file__).resolve().parent

def page(pid,title_en,title_fr,en,fr,section='Editor',parent=None,order=10,description_en=None,description_fr=None):
    for lang,title,body,desc in [('en',title_en,en,description_en),('fr',title_fr,fr,description_fr)]:
        desc=desc or title.rstrip('.')
        meta=f'---\nid: {pid}\ntitle: {title}\ndescription: {desc}\nsection: {section}\norder: {order}\n'+('revision: 1\n' if lang=='en' else 'sourceRevision: 1\n')+(f'parent: {parent}\n' if parent else '')+'---\n\n'
        (ROOT/'manual'/lang/(pid+'.md')).write_text(meta+body.strip()+'\n',encoding='utf-8')

def image(name,alt,id=None):return '```image\n'+json.dumps({'id':id or name,'src':'assets/screenshots/'+name+'.png','alt':alt,'controls':'minimal'},ensure_ascii=False)+'\n```'
