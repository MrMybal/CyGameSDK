"""Cancellation fixture; the official ProcessWorker owns this subprocess."""
import json,sys,time
from pathlib import Path
payload=json.load(sys.stdin)
(Path(payload['workspace'])/'worker-started.txt').write_text('started',encoding='utf-8')
time.sleep(30)
print(json.dumps({'unexpected':'Cancellation did not stop the worker'}))
