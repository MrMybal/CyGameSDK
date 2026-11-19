"""Read-only legacy SQLite import. No legacy runtime package is required."""
import argparse,hashlib,json,re,sqlite3
from contextlib import closing
from pathlib import Path
from job_contract import CyToolError
def records(source,kind):
    database=Path(source)/"state.sqlite3"
    if not database.is_file():return []
    with closing(sqlite3.connect(database.resolve().as_uri()+"?mode=ro",uri=True)) as connection:
        if not connection.execute("select 1 from sqlite_master where type='table' and name='records'").fetchone():raise CyToolError("UnsupportedLegacyStore","Unknown legacy database layout")
        return [json.loads(value) for value, in connection.execute("select value from records where kind=?",(kind,))]
def migrate(source,destination,owner=None):
    source=Path(source).resolve();destination=Path(destination).resolve();count=0
    for record in records(source,"jobs"):
        identity=record.get("jobId","")
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,128}",identity):raise CyToolError("InvalidLegacyStore","Invalid legacy job identity")
        target=destination/identity/"job.json"
        if target.exists():continue
        result=dict(record);result["owner"]=owner or record["clientId"];result["legacySource"]=str(source/"state.sqlite3")
        if result.get("state") not in ("Completed","Failed","Cancelled","Interrupted"):
            result["state"]="Interrupted";result["error"]={"code":"Interrupted","message":"Legacy work was interrupted. Inspect preserved files; it is never replayed."}
        target.parent.mkdir(parents=True,exist_ok=True)
        with target.open("x",encoding="utf8") as stream:json.dump(result,stream,ensure_ascii=False,indent=2)
        count+=1
    return {"importedJobs":count,"sourceUnchanged":True,"outputs":"Original durable files remain at their recorded paths; no result or credential is deleted."}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--source",required=True);p.add_argument("--destination",required=True);p.add_argument("--owner",help="Explicitly assign imported jobs to one host identity, e.g. local. Otherwise preserve original client ownership.")
    a=p.parse_args();print(json.dumps(migrate(a.source,a.destination,a.owner)));return 0
if __name__=="__main__":raise SystemExit(main())
