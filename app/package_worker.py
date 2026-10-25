"""One-shot Sandbox exporter using the official ProcessWorker JSON protocol."""
import hashlib,json,sys,zipfile
from pathlib import Path

def run(payload):
    if payload.get('protocolVersion')!='1.0':raise ValueError('Unsupported worker protocol')
    workspace=Path(payload['workspace']).resolve();p=payload['parameters']
    def owned(name):
        path=Path(p[name]).resolve()
        if not path.is_relative_to(workspace) or path==workspace:raise ValueError('Build artifact must remain in the job workspace')
        path.parent.mkdir(parents=True,exist_ok=True);return path
    source=owned('snapshotPath');archive=owned('archivePath');report=owned('reportPath')
    if source.stat().st_size>4*1024*1024:raise ValueError('Build snapshot exceeds worker limit')
    snapshot=json.loads(source.read_text('utf-8'))
    if snapshot.get('format')!='cygame-build-snapshot' or snapshot.get('adapterId')!='sandbox':raise ValueError('Sandbox worker requires a Sandbox document snapshot')
    compression=zipfile.ZIP_DEFLATED if p['options'].get('compression','Deflated')=='Deflated' else zipfile.ZIP_STORED
    entries={'document.cyproject.json':json.dumps({'format':'cygame-project','schemaVersion':1,'adapterId':'sandbox','document':snapshot['document']},ensure_ascii=False,indent=2),
        'package.json':json.dumps({'format':'cygame-document-package','schemaVersion':1,'adapterId':'sandbox','snapshotRevision':snapshot['revision'],'configuration':p['configuration'],'gameExecutable':False},indent=2)}
    if p['options'].get('includeMaterialValues',True):entries['material-values.json']=json.dumps(snapshot['materialValues'],indent=2)
    with zipfile.ZipFile(archive,'w',compression) as output:
        for name,data in entries.items():output.writestr(name,data)
    result={'adapterId':'sandbox','targetId':p['targetId'],'snapshotRevision':snapshot['revision'],'archivePath':str(archive),'reportPath':str(report),'bytes':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'entries':list(entries),'gameExecutable':False}
    report.write_text(json.dumps(result,indent=2),encoding='utf-8');return result
if __name__=='__main__':
    try:print(json.dumps(run(json.load(sys.stdin)),ensure_ascii=False))
    except Exception as exc:print(str(exc),file=sys.stderr);raise SystemExit(1)
