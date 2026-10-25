"""Desktop SDK client: mirrors one official Core job into a local UI status file."""
import argparse,json,os,time
from pathlib import Path

def write(path,value):
    temporary=path.with_suffix('.tmp');temporary.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
    for attempt in range(20):
        try:os.replace(temporary,path);return
        except PermissionError:
            if attempt==19:raise
            time.sleep(.01)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--run',required=True);args=parser.parse_args()
    run=Path(args.run).resolve();request=json.loads((run/'request.json').read_text('utf-8-sig'))
    root=Path(request['editorRoot']).resolve()
    if not run.is_relative_to((root/'data/build-runs').resolve()):raise ValueError('Invalid build UI directory')
    os.environ['CYGAME_EDITOR_ROOT']=str(root)
    import tool
    tool.ROOT=root;state=run/'state.json';cancelled=False
    try:
        with tool.create_runtime(run/'sdk') as runtime:
            actor=runtime.authenticate(runtime.create_client('Desktop build')['token'])
            session=runtime.create_session(actor,temporary=False)
            operation=request.get('operation','build_execute')
            if operation not in ('build_execute','material_compile','asset_import','asset_reimport','model_export','model_process','game_codec_list','game_codec_export'):raise ValueError('Unsupported desktop job operation')
            job=runtime.submit(actor,session['sessionId'],operation,request['parameters'],execution_timeout=120)
            while True:
                if (run/'cancel.request').exists() and not cancelled:runtime.cancel(actor,job['jobId']);cancelled=True
                job=runtime.get_job(actor,job['jobId']);snapshot={'runId':run.name,'job':job,'state':job['state']}
                if job['state'] in ('Completed','Failed','Cancelled','Interrupted'):break
                write(state,snapshot)
                time.sleep(.1)
        # Publish terminal readiness only after Core closes its workers/database handles.
        write(state,{**snapshot,'clientClosed':True})
    except Exception as exc:write(state,{'runId':run.name,'state':'Failed','error':str(exc),'clientClosed':True})
if __name__=='__main__':main()
