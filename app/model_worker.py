import json,sys
from pathlib import Path
from scene3d import export
from portable_assets.glb import strict_json

def run(p):
    if p.get('protocolVersion')!='1.0':raise ValueError('Unsupported worker protocol')
    root=Path(p['workspace']).resolve()
    def owned(name):
        value=Path(p['parameters'][name]).resolve()
        if value==root or not value.is_relative_to(root):raise ValueError('Output must belong to this job')
        return value
    source=owned('snapshot');target=owned('output')
    if source.stat().st_size>2*1024*1024:raise ValueError('3D snapshot exceeds 2 MiB')
    model=strict_json(source.read_bytes())
    from model_authoring import select_lod
    model=select_lod(model,p['parameters'].get('lod',0));target.parent.mkdir(parents=True,exist_ok=True)
    return export(model,target)
if __name__=='__main__':
    try:print(json.dumps(run(json.load(sys.stdin))))
    except Exception as exc:print(str(exc),file=sys.stderr);raise SystemExit(1)
