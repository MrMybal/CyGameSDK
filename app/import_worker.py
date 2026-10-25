"""Original-file importer worker. No source commands or codecs are loaded from assets."""
import hashlib,json,sys
from pathlib import Path
from datetime import datetime,timezone

def run(payload):
    if payload.get('protocolVersion')!='1.0':raise ValueError('Unsupported worker protocol')
    workspace=Path(payload['workspace']).resolve();parameters=payload['parameters']
    def owned(key):
        path=Path(parameters[key]).resolve()
        if not path.is_relative_to(workspace) or path==workspace:raise ValueError('Import intermediates must belong to the job')
        path.parent.mkdir(parents=True,exist_ok=True);return path
    plan=json.loads(owned('planPath').read_text('utf-8'));source=Path(plan['sourcePath']);staged=owned('stagedPath');report_path=owned('reportPath')
    before=source.stat();limit=plan['maxBytes'];total=0;digest=hashlib.sha256()
    if before.st_size>limit:raise ValueError('Source exceeds importer limits')
    if plan['importerId']=='scene3d.glb':
        from scene3d import decode
        model=decode(source)
        preserved=plan.get("preservedAssetId","")
        if preserved:
            if source.suffix.lower()==".cypack" and model["assetId"]!=preserved:raise ValueError("Reimported portable asset identity changed")
            model["assetId"]=preserved
        with staged.open('x',encoding='utf-8') as writer:json.dump(model,writer,ensure_ascii=False,separators=(',',':'),allow_nan=False)
        total=before.st_size
        with source.open('rb') as reader:
            while chunk:=reader.read(256*1024):digest.update(chunk)
    else:
        with source.open('rb') as reader,staged.open('xb') as writer:
            while chunk:=reader.read(256*1024):
                total+=len(chunk)
                if total>limit:raise ValueError('Source exceeds importer limits')
                digest.update(chunk);writer.write(chunk)
    after=source.stat()
    if (before.st_size,before.st_mtime_ns)!=(after.st_size,after.st_mtime_ns) or total!=before.st_size:raise ValueError('Source changed during import; retry from the current file')
    report={'format':'cygame-import-result','schemaVersion':1,'plan':plan,'sourceSha256':digest.hexdigest(),'importedSha256':hashlib.sha256(staged.read_bytes()).hexdigest(),'sourceBytes':total,'stagedPath':str(staged),'importedAt':datetime.now(timezone.utc).isoformat()}
    report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8');return {'reportPath':str(report_path),'stagedPath':str(staged),'sha256':digest.hexdigest()}
if __name__=='__main__':
    try:print(json.dumps(run(json.load(sys.stdin))))
    except Exception as exc:print(str(exc),file=sys.stderr);raise SystemExit(1)
